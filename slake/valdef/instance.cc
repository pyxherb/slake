#include "instance.h"

#include <slake/runtime.h>

using namespace slake;

Object* InstanceObject::duplicate() const {
	return (Object *)alloc(this).get();
}

MemberObject *InstanceObject::getMember(
	const std::string& name,
	VarRefContext* varRefContextOut) const {
	for (const InstanceObject* i = this; i; i = i->_parent) {
		if (auto m = i->scope->getMember(name); m)
			return m;
	}

	return nullptr;
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
