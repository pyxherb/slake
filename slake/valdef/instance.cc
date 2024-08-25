#include "instance.h"

#include <slake/runtime.h>

using namespace slake;

Object* InstanceObject::duplicate() const {
	return (Object *)alloc(this).get();
}

HostObjectRef<InstanceObject> slake::InstanceObject::alloc(Runtime *rt, ClassObject *cls, InstanceObject *parent) {
	std::pmr::polymorphic_allocator<InstanceObject> allocator(&rt->globalHeapPoolResource);

	InstanceObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, cls, parent);

	rt->createdObjects.insert(ptr);

	return ptr;
}

HostObjectRef<InstanceObject> slake::InstanceObject::alloc(const InstanceObject *other) {
	std::pmr::polymorphic_allocator<InstanceObject> allocator(&other->_rt->globalHeapPoolResource);

	InstanceObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, *other);

	other->_rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::InstanceObject::dealloc() {
	std::pmr::polymorphic_allocator<InstanceObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
