#include <slake/runtime.h>

using namespace slake;

ContextObject::ContextObject(
	Runtime *rt,
	std::shared_ptr<Context> context)
	: Object(rt), _context(context) {
}

ContextObject::~ContextObject() {
}

HostObjectRef<ContextObject> slake::ContextObject::alloc(Runtime *rt, std::shared_ptr<Context> context) {
	std::pmr::polymorphic_allocator<ContextObject> allocator(&rt->globalHeapPoolResource);

	ContextObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, context);

	return ptr;
}

void slake::ContextObject::dealloc() {
	std::pmr::polymorphic_allocator<ContextObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
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
