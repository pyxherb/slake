#include <slake/runtime.h>

using namespace slake;

SLAKE_API ContextObject::ContextObject(
	Runtime *rt,
	std::shared_ptr<Context> context)
	: Object(rt), _context(context) {
}

SLAKE_API ContextObject::ContextObject(const ContextObject &x) : Object(x) {
	_context = x._context;
}

SLAKE_API ContextObject::~ContextObject() {
}

SLAKE_API ObjectKind ContextObject::getKind() const { return ObjectKind::Context; }

SLAKE_API HostObjectRef<ContextObject> slake::ContextObject::alloc(Runtime *rt, std::shared_ptr<Context> context) {
	using Alloc = std::pmr::polymorphic_allocator<ContextObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<ContextObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, context);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<ContextObject> slake::ContextObject::alloc(const ContextObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<ContextObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<ContextObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::ContextObject::dealloc() {
	std::pmr::polymorphic_allocator<ContextObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API Value ContextObject::resume(HostRefHolder *hostRefHolder) {
	_rt->activeContexts[std::this_thread::get_id()] = _context;
	return _context->majorFrames.back()->curFn->call(nullptr, {}, hostRefHolder);
}

SLAKE_API Value ContextObject::getResult() {
	return _context->majorFrames.back()->returnValue;
}

SLAKE_API bool ContextObject::isDone() {
	return _context->flags & CTX_DONE;
}
