#include "typedef.h"

#include <slake/runtime.h>

using namespace slake;

Object *TypeDefObject::duplicate() const {
	return (Object *)alloc(this).get();
}

HostObjectRef<TypeDefObject> slake::TypeDefObject::alloc(Runtime *rt, const Type &type) {
	using Alloc = std::pmr::polymorphic_allocator<TypeDefObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<TypeDefObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, type);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

HostObjectRef<TypeDefObject> slake::TypeDefObject::alloc(const TypeDefObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<TypeDefObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<TypeDefObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

void slake::TypeDefObject::dealloc() {
	std::pmr::polymorphic_allocator<TypeDefObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
