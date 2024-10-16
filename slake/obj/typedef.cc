#include "typedef.h"

#include <slake/runtime.h>

using namespace slake;

SLAKE_API TypeDefObject::TypeDefObject(Runtime *rt, const Type &type)
	: Object(rt), type(type) {
}

SLAKE_API TypeDefObject::TypeDefObject(const TypeDefObject &x) : Object(x) {
	type = x.type.duplicate();
}

SLAKE_API TypeDefObject::~TypeDefObject() {
}

SLAKE_API ObjectKind TypeDefObject::getKind() const { return ObjectKind::TypeDef; }

SLAKE_API Object *TypeDefObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<TypeDefObject> slake::TypeDefObject::alloc(Runtime *rt, const Type &type) {
	using Alloc = std::pmr::polymorphic_allocator<TypeDefObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<TypeDefObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, type);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<TypeDefObject> slake::TypeDefObject::alloc(const TypeDefObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<TypeDefObject>;
	Alloc allocator(&other->associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<TypeDefObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->associatedRuntime->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::TypeDefObject::dealloc() {
	std::pmr::polymorphic_allocator<TypeDefObject> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
