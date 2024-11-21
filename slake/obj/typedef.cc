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

	rt->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<TypeDefObject> slake::TypeDefObject::alloc(const TypeDefObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<TypeDefObject>;
	Alloc allocator(&other->associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<TypeDefObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->associatedRuntime->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::TypeDefObject::dealloc() {
	std::pmr::polymorphic_allocator<TypeDefObject> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API FnTypeDefObject::FnTypeDefObject(Runtime *rt, const Type &returnType, std::pmr::vector<Type> &&paramTypes)
	: Object(rt), returnType(returnType), paramTypes(paramTypes) {
}

SLAKE_API FnTypeDefObject::FnTypeDefObject(const FnTypeDefObject &x) : Object(x) {
	returnType = x.returnType.duplicate();
}

SLAKE_API FnTypeDefObject::~FnTypeDefObject() {
}

SLAKE_API ObjectKind FnTypeDefObject::getKind() const { return ObjectKind::FnTypeDef; }

SLAKE_API Object *FnTypeDefObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<FnTypeDefObject> slake::FnTypeDefObject::alloc(Runtime *rt, const Type &returnType, std::pmr::vector<Type> &&paramTypes) {
	using Alloc = std::pmr::polymorphic_allocator<FnTypeDefObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<FnTypeDefObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, returnType, std::move(paramTypes));

	rt->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<FnTypeDefObject> slake::FnTypeDefObject::alloc(const FnTypeDefObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<FnTypeDefObject>;
	Alloc allocator(&other->associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<FnTypeDefObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->associatedRuntime->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::FnTypeDefObject::dealloc() {
	std::pmr::polymorphic_allocator<FnTypeDefObject> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
