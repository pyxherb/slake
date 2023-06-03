#include <slake/runtime.h>

using namespace Slake;

ValueRef<> FnValue::call(uint8_t nArgs, ValueRef<> *args) {
	std::shared_ptr<Context> context = std::make_shared<Context>();
	getRuntime()->threadCurrentContexts[std::this_thread::get_id()] = context;

	{
		auto frame = MajorFrame();
		frame.curFn = this;
		frame.curIns = UINT32_MAX - 1;
		context->majorFrames.push_back(frame);

		frame = MajorFrame();
		{
			auto minorFrame = MinorFrame();
			minorFrame.exitOff = UINT32_MAX - 1;
			frame.minorFrames.push_back(minorFrame);
		}
		frame.curFn = this;
		frame.curIns = 0;
		frame.scopeValue = _parent;
		context->majorFrames.push_back(frame);
	}

	while (context->majorFrames.back().curIns != UINT32_MAX) {
		uint32_t curIns = context->majorFrames.back().curIns;
		curIns = curIns + 1 - 1;
		if (context->majorFrames.back().curIns >= _nIns)
			throw std::runtime_error("Out of function body");
		getRuntime()->_execIns(context.get(), context->majorFrames.back().curFn->_body[context->majorFrames.back().curIns]);
	}

	getRuntime()->threadCurrentContexts.erase(std::this_thread::get_id());
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
