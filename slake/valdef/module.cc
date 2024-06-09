#include <slake/runtime.h>

using namespace slake;

ModuleObject::ModuleObject(Runtime *rt, AccessModifier access)
	: MemberObject(rt, access) {
	scope = new Scope(this);
}

ModuleObject::~ModuleObject() {
}

Type ModuleObject::getType() const {
	return TypeId::Module;
}

Object *ModuleObject::duplicate() const {
	HostObjectRef<ModuleObject> v = ModuleObject::alloc(_rt, getAccess());

	*(v.get()) = *this;

	return (Object *)v.release();
}

HostObjectRef<ModuleObject> slake::ModuleObject::alloc(Runtime *rt, AccessModifier access) {
	std::pmr::polymorphic_allocator<ModuleObject> allocator(&rt->globalHeapPoolResource);

	ModuleObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, access);

	rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::ModuleObject::dealloc() {
	std::pmr::polymorphic_allocator<ModuleObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
