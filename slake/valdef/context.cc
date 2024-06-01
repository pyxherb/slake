#include <slake/runtime.h>

using namespace slake;

ContextValue::ContextValue(
	Runtime *rt,
	std::shared_ptr<Context> context)
	: Value(rt), _context(context) {
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(Value));
}

ContextValue::~ContextValue() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(Value));
}

ValueRef<> ContextValue::resume() {
	_rt->activeContexts[std::this_thread::get_id()] = _context;
	return _context->majorFrames.back().curFn->call(nullptr, {});
}

ValueRef<> ContextValue::getResult() {
	return _context->majorFrames.back().returnValue;
}

bool ContextValue::isDone() {
	return _context->flags & CTX_DONE;
}
