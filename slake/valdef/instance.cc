#include "instance.h"

#include <slake/runtime.h>

using namespace slake;

Object* InstanceObject::duplicate() const {
	HostObjectRef<InstanceObject> v = InstanceObject::alloc(_rt, _class);

	*(v.get()) = *this;

	return (Object *)v.release();
}

HostObjectRef<InstanceObject> slake::InstanceObject::alloc(Runtime *rt, ClassObject *cls, InstanceObject *parent) {
	std::pmr::polymorphic_allocator<InstanceObject> allocator(&rt->globalHeapPoolResource);

	InstanceObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, cls, parent);

	return ptr;
}

void slake::InstanceObject::dealloc() {
	std::pmr::polymorphic_allocator<InstanceObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
