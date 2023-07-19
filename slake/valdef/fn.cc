#include <slake/runtime.h>

using namespace slake;

Type BasicFnValue::getType() const { return TypeId::FN; }
Type BasicFnValue::getReturnType() const { return returnType; }

FnValue::~FnValue() {
	// Because the runtime will release all values, so we fill the body with 0
	// (because the references do not release their held object).
	if (_rt->_flags & _RT_DELETING)
		memset((void *)body, 0, sizeof(Instruction) * nIns);

	if (body)
		delete[] body;
}

ValueRef<> FnValue::call(uint8_t nArgs, ValueRef<> *args) const {
	bool isDestructing = _rt->destructingThreads.count(std::this_thread::get_id());
	std::shared_ptr<Context> context = std::make_shared<Context>(), savedContext;

	// Save previous context
	if (_rt->activeContexts.count(std::this_thread::get_id()))
		savedContext = _rt->activeContexts.at(std::this_thread::get_id());

	_rt->activeContexts[std::this_thread::get_id()] = context;

	{
		auto frame = MajorFrame();
		frame.curFn = this;	 // The garbage collector does not check if curFn is nullptr.
		frame.curIns = UINT32_MAX - 1;
		context->majorFrames.push_back(frame);

		frame = MajorFrame();
		frame.curFn = this;
		frame.curIns = 0;
		frame.scopeValue = _parent;
		context->majorFrames.push_back(frame);
	}

	while (context->majorFrames.back().curIns != UINT32_MAX) {
		if (context->majorFrames.back().curIns >= context->majorFrames.back().curFn->nIns)
			throw OutOfFnBodyError("Out of function body");

		// Pause if the runtime is in GC
		while ((getRuntime()->_flags & _RT_INGC) && !isDestructing)
			std::this_thread::yield();

		getRuntime()->_execIns(context.get(), context->majorFrames.back().curFn->body[context->majorFrames.back().curIns]);

		if ((_rt->_szMemInUse > (_rt->_szMemUsedAfterLastGc << 1)) && !isDestructing)
			_rt->gc();
	}

	// Restore previous context
	if (savedContext)
		_rt->activeContexts[std::this_thread::get_id()] = savedContext;
	else
		_rt->activeContexts.erase(std::this_thread::get_id());

	if (!isDestructing)
		_rt->gc();
	return context->majorFrames.back().returnValue;
}

ValueRef<> NativeFnValue::call(uint8_t nArgs, ValueRef<> *args) const {
	return body(getRuntime(), nArgs, args);
}

Value *FnValue::duplicate() const {
	FnValue* v = new FnValue(_rt, 0, 0, {});

	*v = *this;

	return (Value *)v;
}

Value *NativeFnValue::duplicate() const {
	NativeFnValue* v = new NativeFnValue(_rt, {}, 0, {});

	*v = *this;

	return (Value *)v;
}
