#include <slake/runtime.h>

using namespace slake;

ContextValue::ContextValue(
	Runtime *rt,
	std::shared_ptr<Context> context)
	: Value(rt), _context(context) {
}

ValueRef<> ContextValue::resume() {
	_context->flags &= ~CTX_YIELDED;
	return _context->majorFrames.back().curFn->exec(_context);
}

ValueRef<> ContextValue::getResult() {
	return _context->majorFrames.back().returnValue;
}

bool ContextValue::isDone() {
	return _context->flags & CTX_DONE;
}
