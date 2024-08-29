#include <slake/runtime.h>

using namespace slake;

ModuleObject::ModuleObject(Runtime *rt, AccessModifier access)
	: MemberObject(rt, access) {
	scope = new Scope(this);
}

ModuleObject::~ModuleObject() {
}

Object *ModuleObject::duplicate() const {
	return (Object *)alloc(this).get();
}

HostObjectRef<ModuleObject> slake::ModuleObject::alloc(Runtime *rt, AccessModifier access) {
	std::pmr::polymorphic_allocator<ModuleObject> allocator(&rt->globalHeapPoolResource);

	ModuleObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, access);

	rt->createdObjects.insert(ptr);

	return ptr;
}

HostObjectRef<ModuleObject> slake::ModuleObject::alloc(const ModuleObject *other) {
	std::pmr::polymorphic_allocator<ModuleObject> allocator(&other->_rt->globalHeapPoolResource);

	ModuleObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, *other);

	other->_rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::ModuleObject::dealloc() {
	std::pmr::polymorphic_allocator<ModuleObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
