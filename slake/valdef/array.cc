#include <slake/runtime.h>

using namespace slake;

ArrayObject::ArrayObject(Runtime *rt, const Type &type)
	: Object(rt), type(type) {
}

ArrayObject::~ArrayObject() {
}

Object *ArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

HostObjectRef<ArrayObject> slake::ArrayObject::alloc(Runtime *rt, const Type &type) {
	std::pmr::polymorphic_allocator<ArrayObject> allocator(&rt->globalHeapPoolResource);

	ArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, type);

	rt->createdObjects.insert(ptr);

	return ptr;
}

HostObjectRef<ArrayObject> slake::ArrayObject::alloc(const ArrayObject *other) {
	std::pmr::polymorphic_allocator<ArrayObject> allocator(&other->_rt->globalHeapPoolResource);

	ArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, *other);

	other->_rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::ArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<ArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
