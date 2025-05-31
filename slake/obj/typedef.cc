#include "typedef.h"

#include <slake/runtime.h>

using namespace slake;

SLAKE_API TypeDefObject::TypeDefObject(Runtime *rt, peff::Alloc *selfAllocator, const Type &type)
	: Object(rt, selfAllocator), type(type) {
}

SLAKE_API TypeDefObject::TypeDefObject(const TypeDefObject &x, peff::Alloc *allocator, bool &succeededOut) : Object(x, allocator) {
	type = x.type.duplicate(succeededOut);
	if (!succeededOut) {
		return;
	}
}

SLAKE_API TypeDefObject::~TypeDefObject() {
}

SLAKE_API ObjectKind TypeDefObject::getKind() const { return ObjectKind::TypeDef; }

SLAKE_API Object *TypeDefObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<TypeDefObject> slake::TypeDefObject::alloc(Runtime *rt, const Type &type) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<TypeDefObject, util::DeallocableDeleter<TypeDefObject>> ptr(
		peff::allocAndConstruct<TypeDefObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			rt, curGenerationAllocator.get(), type));
	if (!ptr)
		return nullptr;

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<TypeDefObject> slake::TypeDefObject::alloc(const TypeDefObject *other) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = other->associatedRuntime->getCurGenAlloc();

	bool succeeded = true;

	std::unique_ptr<TypeDefObject, util::DeallocableDeleter<TypeDefObject>> ptr(
		peff::allocAndConstruct<TypeDefObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			*other, curGenerationAllocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associatedRuntime->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::TypeDefObject::dealloc() {
	peff::destroyAndRelease<TypeDefObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API FnTypeDefObject::FnTypeDefObject(Runtime *rt, peff::Alloc *selfAllocator, const Type &returnType, peff::DynArray<Type> &&paramTypes)
	: Object(rt, selfAllocator), returnType(returnType), paramTypes(std::move(paramTypes)) {
}

SLAKE_API FnTypeDefObject::FnTypeDefObject(const FnTypeDefObject &x, peff::Alloc *allocator, bool &succeededOut) : Object(x, allocator), paramTypes(allocator) {
	returnType = x.returnType.duplicate(succeededOut);
	if (!succeededOut) {
		return;
	}

	// TODO: Copy the parameter types.
}

SLAKE_API FnTypeDefObject::~FnTypeDefObject() {
}

SLAKE_API ObjectKind FnTypeDefObject::getKind() const { return ObjectKind::FnTypeDef; }

SLAKE_API Object *FnTypeDefObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<FnTypeDefObject> slake::FnTypeDefObject::alloc(Runtime *rt, const Type &returnType, peff::DynArray<Type> &&paramTypes) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<FnTypeDefObject, util::DeallocableDeleter<FnTypeDefObject>> ptr(
		peff::allocAndConstruct<FnTypeDefObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			rt, curGenerationAllocator.get(), returnType, std::move(paramTypes)));
	if (!ptr)
		return nullptr;

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<FnTypeDefObject> slake::FnTypeDefObject::alloc(const FnTypeDefObject *other) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = other->associatedRuntime->getCurGenAlloc();

	bool succeeded = true;

	std::unique_ptr<FnTypeDefObject, util::DeallocableDeleter<FnTypeDefObject>> ptr(
		peff::allocAndConstruct<FnTypeDefObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			*other, curGenerationAllocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associatedRuntime->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::FnTypeDefObject::dealloc() {
	peff::destroyAndRelease<FnTypeDefObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API void FnTypeDefObject::replaceAllocator(peff::Alloc *allocator) noexcept {
	this->Object::replaceAllocator(allocator);

	paramTypes.replaceAllocator(allocator);
}
