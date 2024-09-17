#include <slake/runtime.h>

using namespace slake;

RootObject::RootObject(Runtime *rt)
	: Object(rt) {
	scope = Scope::alloc(&rt->globalHeapPoolResource, this);
}

MemberObject* RootObject::getMember(
	const std::pmr::string& name,
	VarRefContext* varRefContextOut) const {
	return scope->getMember(name);
}

HostObjectRef<RootObject> slake::RootObject::alloc(Runtime *rt) {
	std::pmr::polymorphic_allocator<RootObject> allocator(&rt->globalHeapPoolResource);

	RootObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt);

	rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::RootObject::dealloc() {
	std::pmr::polymorphic_allocator<RootObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
