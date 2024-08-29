#include "typedef.h"

#include <slake/runtime.h>

using namespace slake;

Object *TypeDefObject::duplicate() const {
	return (Object *)alloc(this).get();
}

HostObjectRef<TypeDefObject> slake::TypeDefObject::alloc(Runtime *rt, const Type &type) {
	std::pmr::polymorphic_allocator<TypeDefObject> allocator(&rt->globalHeapPoolResource);

	TypeDefObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, type);

	rt->createdObjects.insert(ptr);

	return ptr;
}

HostObjectRef<TypeDefObject> slake::TypeDefObject::alloc(const TypeDefObject *other) {
	std::pmr::polymorphic_allocator<TypeDefObject> allocator(&other->_rt->globalHeapPoolResource);

	TypeDefObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, *other);

	other->_rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::TypeDefObject::dealloc() {
	std::pmr::polymorphic_allocator<TypeDefObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
