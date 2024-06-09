#include "var.h"
#include <slake/runtime.h>

using namespace slake;

BasicVarObject::BasicVarObject(Runtime *rt, AccessModifier access, Type type) : MemberObject(rt, access), type(type) {
}

BasicVarObject::~BasicVarObject() {
}

slake::VarObject::VarObject(Runtime *rt, AccessModifier access, const Type &type)
	: BasicVarObject(rt, access, type) {
}

VarObject::~VarObject() {
}

Object *VarObject::duplicate() const {
	HostObjectRef<VarObject> v = VarObject::alloc(_rt, 0, type);

	*(v.get()) = *this;

	return (Object *)v.release();
}

void slake::VarObject::dealloc() {
	std::pmr::polymorphic_allocator<VarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

HostObjectRef<VarObject> slake::VarObject::alloc(Runtime *rt, AccessModifier access, const Type &type) {
	std::pmr::polymorphic_allocator<VarObject> allocator(&rt->globalHeapPoolResource);

	VarObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, access, type);

	rt->createdObjects.insert(ptr);

	return ptr;
}
