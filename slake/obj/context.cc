#include <slake/runtime.h>

using namespace slake;

SLAKE_API ContextObject::ContextObject(
	Runtime *rt)
	: Object(rt), _context(rt) {
}

SLAKE_API ContextObject::~ContextObject() {
}

SLAKE_API ObjectKind ContextObject::getKind() const { return ObjectKind::Context; }

SLAKE_API HostObjectRef<ContextObject> slake::ContextObject::alloc(Runtime *rt) {
	using Alloc = std::pmr::polymorphic_allocator<ContextObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<ContextObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::ContextObject::dealloc() {
	std::pmr::polymorphic_allocator<ContextObject> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API Value ContextObject::resume(HostRefHolder *hostRefHolder) {
	associatedRuntime->activeContexts[std::this_thread::get_id()] = this;
	return _context.majorFrames.back()->curFn->call(nullptr, {}, hostRefHolder);
}

SLAKE_API Value ContextObject::getResult() {
	return _context.majorFrames.back()->returnValue;
}

SLAKE_API bool ContextObject::isDone() {
	return _context.flags & CTX_DONE;
}
