#include <slake/runtime.h>

using namespace slake;

ArrayObject::ArrayObject(Runtime *rt, const Type &type)
	: Object(rt), type(type) {
}

ArrayObject::~ArrayObject() {
}

Object *ArrayObject::duplicate() const {
	ArrayObject *v = new ArrayObject(_rt, type);

	*v = *this;

	return (Object *)v;
}

HostObjectRef<ArrayObject> slake::ArrayObject::alloc(Runtime *rt, const Type &type) {
	std::pmr::polymorphic_allocator<ArrayObject> allocator(&rt->globalHeapPoolResource);

	ArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, type);

	rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::ArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<ArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
