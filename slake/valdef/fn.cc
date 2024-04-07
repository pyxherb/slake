#include <slake/runtime.h>

using namespace slake;

Type BasicFnValue::getType() const { return TypeId::Fn; }
Type BasicFnValue::getReturnType() const { return returnType; }

BasicFnValue::~BasicFnValue() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(MemberValue));
}

slake::FnValue::FnValue(Runtime *rt, uint32_t nIns, AccessModifier access, Type returnType)
	: nIns(nIns),
	  BasicFnValue(rt, access, returnType) {
	if (nIns)
		body = new Instruction[nIns];
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(BasicFnValue) + sizeof(Instruction) * nIns);
}

FnValue::~FnValue() {
	// Because the runtime will release all values, so we just fill the body with 0
	// (because the references do not release objects they held).
	if (_rt->_flags & _RT_DELETING)
		memset((void *)body, 0, sizeof(Instruction) * nIns);

	if (body) {
		delete[] body;
		body = nullptr;
	}

	reportSizeFreedToRuntime(sizeof(*this) - sizeof(BasicFnValue) + sizeof(Instruction) * nIns);
}

ValueRef<> FnValue::exec(std::shared_ptr<Context> context) const {
	if (context->flags & CTX_DONE)
		throw std::logic_error("Executing with a done context");

	// Save previous context
	std::shared_ptr<Context> savedContext;
	if (_rt->activeContexts.count(std::this_thread::get_id()))
		savedContext = _rt->activeContexts.at(std::this_thread::get_id());

	_rt->activeContexts[std::this_thread::get_id()] = context;

	bool isDestructing = _rt->destructingThreads.count(std::this_thread::get_id());

	try {
		while (context->majorFrames.back().curIns != UINT32_MAX) {
			if (context->majorFrames.back().curIns >= context->majorFrames.back().curFn->nIns)
				throw OutOfFnBodyError("Out of function body");

			if (context->flags & CTX_YIELDED)
				break;

			// Pause if the runtime is in GC
			while ((getRuntime()->_flags & _RT_INGC) && !isDestructing)
				std::this_thread::yield();

			getRuntime()->_execIns(context.get(), context->majorFrames.back().curFn->body[context->majorFrames.back().curIns]);

			if ((_rt->_szMemInUse > (_rt->_szMemUsedAfterLastGc << 1)) && !isDestructing)
				_rt->gc();
		}
	} catch (...) {
		context->flags |= CTX_DONE;
		std::rethrow_exception(std::current_exception());
	}

	// Do a GC cycle if size of memory in use is greater than double the size used after last cycle.
	if (((_rt->_szMemInUse >> 1) > _rt->_szMemUsedAfterLastGc) && !isDestructing)
		_rt->gc();

	// Restore previous context
	if (savedContext)
		_rt->activeContexts[std::this_thread::get_id()] = savedContext;
	else
		_rt->activeContexts.erase(std::this_thread::get_id());

	if (context->flags & CTX_YIELDED)
		return new ContextValue(_rt, context);

	context->flags |= CTX_DONE;
	return context->majorFrames.back().returnValue;
}

ValueRef<> FnValue::call(Value *thisObject, std::deque<Value *> args) const {
	std::shared_ptr<Context> context = std::make_shared<Context>();

	{
		auto frame = MajorFrame(_rt);
		frame.curFn = this;	 // The garbage collector does not check if curFn is nullptr.
		frame.curIns = UINT32_MAX - 1;
		context->majorFrames.push_back(frame);

		frame = MajorFrame(_rt);
		frame.curFn = this;
		frame.curIns = 0;
		frame.scopeValue = _parent;
		frame.thisObject = thisObject;
		context->majorFrames.push_back(frame);
	}

	return exec(context);
}

slake::NativeFnValue::NativeFnValue(Runtime *rt, NativeFnCallback body, AccessModifier access, Type returnType)
	: BasicFnValue(rt, access | ACCESS_NATIVE, returnType), body(body) {
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(BasicFnValue));
}

NativeFnValue::~NativeFnValue() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(BasicFnValue));
}

ValueRef<> NativeFnValue::call(Value *thisObject, std::deque<Value *> args) const {
	return body(_rt, thisObject, args, mappedGenericArgs);
}

Value *FnValue::duplicate() const {
	FnValue *v = new FnValue(_rt, 0, 0, {});

	*v = *this;

	return (Value *)v;
}

FnValue &slake::FnValue::operator=(const FnValue &x) {
	((BasicFnValue &)*this) = (BasicFnValue &)x;

	// Delete existing function body.
	if (body) {
		delete[] body;
		body = nullptr;
	}

	// Copy the function body if the source function is not abstract.
	if (x.body) {
		body = new Instruction[x.nIns];

		// Copy each instruction.
		for (size_t i = 0; i < x.nIns; ++i) {
			// Duplicate current instruction from the source function.
			auto ins = x.body[i];

			// Copy each operand.
			for (size_t j = 0; j < ins.operands.size(); ++j) {
				auto &operand = ins.operands[j];
				if (operand)
					operand = operand->duplicate();
			}

			// Move current instruction into the function body.
			body[i] = ins;
		}
	}

	if (nIns > x.nIns)
		reportSizeFreedToRuntime(sizeof(Instruction) * (nIns - x.nIns));
	else
		reportSizeAllocatedToRuntime(sizeof(Instruction) * (x.nIns - nIns));

	nIns = x.nIns;

	return *this;
}

Value *NativeFnValue::duplicate() const {
	NativeFnValue *v = new NativeFnValue(_rt, {}, 0, {});

	*v = *this;

	return (Value *)v;
}
