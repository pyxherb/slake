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
	using Alloc = std::pmr::polymorphic_allocator<ContextObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<ContextObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, context);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

HostObjectRef<ContextObject> slake::ContextObject::alloc(const ContextObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<ContextObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<ContextObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

void slake::ContextObject::dealloc() {
	std::pmr::polymorphic_allocator<ContextObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

Value ContextObject::resume(HostRefHolder *hostRefHolder) {
	_rt->activeContexts[std::this_thread::get_id()] = _context;
	return _context->majorFrames.back()->curFn->call(nullptr, {}, hostRefHolder);
}

Value ContextObject::getResult() {
	return _context->majorFrames.back()->returnValue;
}

bool ContextObject::isDone() {
	return _context->flags & CTX_DONE;
}
