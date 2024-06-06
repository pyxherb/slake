#include <slake/runtime.h>

using namespace slake;

ContextObject::ContextObject(
	Runtime *rt,
	std::shared_ptr<Context> context)
	: Object(rt), _context(context) {
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(Object));
}

ContextObject::~ContextObject() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(Object));
}

Value ContextObject::resume() {
	_rt->activeContexts[std::this_thread::get_id()] = _context;
	return _context->majorFrames.back().curFn->call(nullptr, {});
}

Value ContextObject::getResult() {
	return _context->majorFrames.back().returnValue;
}

bool ContextObject::isDone() {
	return _context->flags & CTX_DONE;
}
