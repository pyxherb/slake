#include <slake/runtime.h>

using namespace slake;

SLAKE_API RootObject::RootObject(Runtime *rt)
	: Object(rt) {
	scope = Scope::alloc(&rt->globalHeapPoolResource, this);
}

SLAKE_API RootObject::~RootObject() {
	scope->dealloc();
}

SLAKE_API ObjectKind RootObject::getKind() const { return ObjectKind::RootObject; }

SLAKE_API MemberObject *RootObject::getMember(
	const std::pmr::string& name,
	VarRefContext* varRefContextOut) const {
	return scope->getMember(name);
}

SLAKE_API HostObjectRef<RootObject> slake::RootObject::alloc(Runtime *rt) {
	using Alloc = std::pmr::polymorphic_allocator<RootObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<RootObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt);

	rt->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::RootObject::dealloc() {
	std::pmr::polymorphic_allocator<RootObject> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
