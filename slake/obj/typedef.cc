#include "typedef.h"

#include <slake/runtime.h>

using namespace slake;

SLAKE_API TypeDefObject::TypeDefObject(Runtime *rt, const Type &type)
	: Object(rt), type(type) {
}

SLAKE_API TypeDefObject::TypeDefObject(const TypeDefObject &x, bool &succeededOut) : Object(x) {
	type = x.type.duplicate(succeededOut);
	if (!succeededOut) {
		return;
	}
}

SLAKE_API TypeDefObject::~TypeDefObject() {
}

SLAKE_API ObjectKind TypeDefObject::getKind() const { return ObjectKind::TypeDef; }

SLAKE_API Object *TypeDefObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<TypeDefObject> slake::TypeDefObject::alloc(Runtime *rt, const Type &type) {
	std::unique_ptr<TypeDefObject, util::DeallocableDeleter<TypeDefObject>> ptr(
		peff::allocAndConstruct<TypeDefObject>(
			&rt->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			rt, type));
	if (!ptr)
		return nullptr;

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<TypeDefObject> slake::TypeDefObject::alloc(const TypeDefObject *other) {
	bool succeeded = true;

	std::unique_ptr<TypeDefObject, util::DeallocableDeleter<TypeDefObject>> ptr(
		peff::allocAndConstruct<TypeDefObject>(
			&other->associatedRuntime->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			*other, succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associatedRuntime->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::TypeDefObject::dealloc() {
	peff::destroyAndRelease<TypeDefObject>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API FnTypeDefObject::FnTypeDefObject(Runtime *rt, const Type &returnType, peff::DynArray<Type> &&paramTypes)
	: Object(rt), returnType(returnType), paramTypes(std::move(paramTypes)) {
}

SLAKE_API FnTypeDefObject::FnTypeDefObject(const FnTypeDefObject &x, bool &succeededOut) : Object(x), paramTypes(&x.associatedRuntime->globalHeapPoolAlloc) {
	returnType = x.returnType.duplicate(succeededOut);
	if (!succeededOut) {
		return;
	}

	// TODO: Copy the parameter types.
}

SLAKE_API FnTypeDefObject::~FnTypeDefObject() {
}

SLAKE_API ObjectKind FnTypeDefObject::getKind() const { return ObjectKind::FnTypeDef; }

SLAKE_API Object *FnTypeDefObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<FnTypeDefObject> slake::FnTypeDefObject::alloc(Runtime *rt, const Type &returnType, peff::DynArray<Type> &&paramTypes) {
	std::unique_ptr<FnTypeDefObject, util::DeallocableDeleter<FnTypeDefObject>> ptr(
		peff::allocAndConstruct<FnTypeDefObject>(
			&rt->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			rt, returnType, std::move(paramTypes)));
	if (!ptr)
		return nullptr;

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<FnTypeDefObject> slake::FnTypeDefObject::alloc(const FnTypeDefObject *other) {
	bool succeeded = true;

	std::unique_ptr<FnTypeDefObject, util::DeallocableDeleter<FnTypeDefObject>> ptr(
		peff::allocAndConstruct<FnTypeDefObject>(
			&other->associatedRuntime->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			*other, succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associatedRuntime->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::FnTypeDefObject::dealloc() {
	peff::destroyAndRelease<FnTypeDefObject>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}
