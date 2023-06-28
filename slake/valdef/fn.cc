#include <slake/runtime.h>

using namespace Slake;

ValueRef<> FnValue::call(uint8_t nArgs, ValueRef<> *args) {
	bool isDestructing = _rt->destructingThreads.count(std::this_thread::get_id());
	std::shared_ptr<Context> context = std::make_shared<Context>(), savedContext;

	// Save previous context
	if (_rt->currentContexts.count(std::this_thread::get_id()))
		savedContext = _rt->currentContexts.at(std::this_thread::get_id());

	_rt->currentContexts[std::this_thread::get_id()] = context;

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
		if (context->majorFrames.back().curIns >= context->majorFrames.back().curFn->_nIns)
			throw OutOfFnBodyError("Out of function body");

		// Pause if the runtime is in GC
		while (getRuntime()->_isInGc && !isDestructing)
			std::this_thread::yield();

		getRuntime()->_execIns(context.get(), context->majorFrames.back().curFn->_body[context->majorFrames.back().curIns]);

		if ((_rt->_szMemInUse > (_rt->_szMemUsedAfterLastGc << 1)) && !isDestructing)
			_rt->gc();
	}

	// Restore previous context
	if (savedContext)
		_rt->currentContexts[std::this_thread::get_id()] = savedContext;
	else
		_rt->currentContexts.erase(std::this_thread::get_id());

	if (!isDestructing)
		_rt->gc();
	return context->majorFrames.back().returnValue;
}

std::string FnValue::toString() const {
	std::string s = Value::toString() + ",\"instructions\":[";

	for (size_t i = 0; i < _nIns; i++) {
		s += (i ? "," : "") + std::to_string(_body[i]);
	}

	s += "]";

	return s;
}
