#include "typedef.h"

#include <slake/runtime.h>

using namespace slake;

SLAKE_API TypeDefObject::TypeDefObject(Runtime *rt, peff::Alloc *selfAllocator)
	: Object(rt, selfAllocator, ObjectKind::TypeDef) {
}

SLAKE_API TypeDefObject::TypeDefObject(Duplicator *duplicator, const TypeDefObject &x, peff::Alloc *allocator, bool &succeededOut) : Object(x, allocator) {
	if (!duplicator->insertTask(DuplicationTask::makeType(&type, x.type))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLAKE_API TypeDefObject::~TypeDefObject() {
}

SLAKE_API Object *TypeDefObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<TypeDefObject> slake::TypeDefObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<TypeDefObject, util::DeallocableDeleter<TypeDefObject>> ptr(
		peff::allocAndConstruct<TypeDefObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			rt, curGenerationAllocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<TypeDefObject> slake::TypeDefObject::alloc(Duplicator *duplicator, const TypeDefObject *other) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = other->associatedRuntime->getCurGenAlloc();

	bool succeeded = true;

	std::unique_ptr<TypeDefObject, util::DeallocableDeleter<TypeDefObject>> ptr(
		peff::allocAndConstruct<TypeDefObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			duplicator, *other, curGenerationAllocator.get(), succeeded));
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

SLAKE_API FnTypeDefObject::FnTypeDefObject(Runtime *rt, peff::Alloc *selfAllocator)
	: Object(rt, selfAllocator, ObjectKind::FnTypeDef), paramTypes(selfAllocator) {
}

SLAKE_API FnTypeDefObject::FnTypeDefObject(Duplicator *duplicator, const FnTypeDefObject &x, peff::Alloc *allocator, bool &succeededOut) : Object(x, allocator), paramTypes(allocator) {
	returnType = TypeId::Void;
	if (!duplicator->insertTask(DuplicationTask::makeType(&returnType, x.returnType))) {
		succeededOut = false;
		return;
	}

	if (!paramTypes.resize(x.paramTypes.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < x.paramTypes.size(); ++i) {
		paramTypes.at(i) = TypeId::Void;
	}

	for (size_t i = 0; i < x.paramTypes.size(); ++i) {
		if (!duplicator->insertTask(DuplicationTask::makeType(&paramTypes.at(i), x.paramTypes.at(i)))) {
			succeededOut = false;
			return;
		}
	}

	hasVarArg = x.hasVarArg;

	succeededOut = true;
}

SLAKE_API FnTypeDefObject::~FnTypeDefObject() {
}

SLAKE_API Object *FnTypeDefObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<FnTypeDefObject> slake::FnTypeDefObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<FnTypeDefObject, util::DeallocableDeleter<FnTypeDefObject>> ptr(
		peff::allocAndConstruct<FnTypeDefObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			rt, curGenerationAllocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<FnTypeDefObject> slake::FnTypeDefObject::alloc(Duplicator *duplicator, const FnTypeDefObject *other) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = other->associatedRuntime->getCurGenAlloc();

	bool succeeded = true;

	std::unique_ptr<FnTypeDefObject, util::DeallocableDeleter<FnTypeDefObject>> ptr(
		peff::allocAndConstruct<FnTypeDefObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			duplicator, *other, curGenerationAllocator.get(), succeeded));
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

SLAKE_API ParamTypeListTypeDefObject::ParamTypeListTypeDefObject(Runtime *rt, peff::Alloc *selfAllocator)
	: Object(rt, selfAllocator, ObjectKind::ParamTypeListTypeDef), paramTypes(selfAllocator) {
}

SLAKE_API ParamTypeListTypeDefObject::ParamTypeListTypeDefObject(Duplicator *duplicator, const ParamTypeListTypeDefObject &x, peff::Alloc *allocator, bool &succeededOut) : Object(x, allocator), paramTypes(allocator) {
	if (!paramTypes.resize(x.paramTypes.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < x.paramTypes.size(); ++i) {
		paramTypes.at(i) = TypeId::Void;
	}

	for (size_t i = 0; i < x.paramTypes.size(); ++i) {
		if (!duplicator->insertTask(DuplicationTask::makeType(&paramTypes.at(i), x.paramTypes.at(i)))) {
			succeededOut = false;
			return;
		}
	}

	hasVarArg = x.hasVarArg;

	succeededOut = true;
}

SLAKE_API ParamTypeListTypeDefObject::~ParamTypeListTypeDefObject() {
}

SLAKE_API Object *ParamTypeListTypeDefObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<ParamTypeListTypeDefObject> slake::ParamTypeListTypeDefObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<ParamTypeListTypeDefObject, util::DeallocableDeleter<ParamTypeListTypeDefObject>> ptr(
		peff::allocAndConstruct<ParamTypeListTypeDefObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			rt, curGenerationAllocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<ParamTypeListTypeDefObject> slake::ParamTypeListTypeDefObject::alloc(Duplicator *duplicator, const ParamTypeListTypeDefObject *other) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = other->associatedRuntime->getCurGenAlloc();

	bool succeeded = true;

	std::unique_ptr<ParamTypeListTypeDefObject, util::DeallocableDeleter<ParamTypeListTypeDefObject>> ptr(
		peff::allocAndConstruct<ParamTypeListTypeDefObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			duplicator, *other, curGenerationAllocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associatedRuntime->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::ParamTypeListTypeDefObject::dealloc() {
	peff::destroyAndRelease<ParamTypeListTypeDefObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API void ParamTypeListTypeDefObject::replaceAllocator(peff::Alloc *allocator) noexcept {
	this->Object::replaceAllocator(allocator);

	paramTypes.replaceAllocator(allocator);
}

SLAKE_API TupleTypeListTypeDefObject::TupleTypeListTypeDefObject(Runtime *rt, peff::Alloc *selfAllocator)
	: Object(rt, selfAllocator, ObjectKind::TupleTypeDef), paramTypes(selfAllocator) {
}

SLAKE_API TupleTypeListTypeDefObject::TupleTypeListTypeDefObject(Duplicator *duplicator, const TupleTypeListTypeDefObject &x, peff::Alloc *allocator, bool &succeededOut) : Object(x, allocator), paramTypes(allocator) {
	if (!paramTypes.resize(x.paramTypes.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < x.paramTypes.size(); ++i) {
		paramTypes.at(i) = TypeId::Void;
	}

	for (size_t i = 0; i < x.paramTypes.size(); ++i) {
		if (!duplicator->insertTask(DuplicationTask::makeType(&paramTypes.at(i), x.paramTypes.at(i)))) {
			succeededOut = false;
			return;
		}
	}

	succeededOut = true;
}

SLAKE_API TupleTypeListTypeDefObject::~TupleTypeListTypeDefObject() {
}

SLAKE_API Object *TupleTypeListTypeDefObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<TupleTypeListTypeDefObject> slake::TupleTypeListTypeDefObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<TupleTypeListTypeDefObject, util::DeallocableDeleter<TupleTypeListTypeDefObject>> ptr(
		peff::allocAndConstruct<TupleTypeListTypeDefObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			rt, curGenerationAllocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<TupleTypeListTypeDefObject> slake::TupleTypeListTypeDefObject::alloc(Duplicator *duplicator, const TupleTypeListTypeDefObject *other) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = other->associatedRuntime->getCurGenAlloc();

	bool succeeded = true;

	std::unique_ptr<TupleTypeListTypeDefObject, util::DeallocableDeleter<TupleTypeListTypeDefObject>> ptr(
		peff::allocAndConstruct<TupleTypeListTypeDefObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			duplicator, *other, curGenerationAllocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associatedRuntime->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::TupleTypeListTypeDefObject::dealloc() {
	peff::destroyAndRelease<TupleTypeListTypeDefObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API void TupleTypeListTypeDefObject::replaceAllocator(peff::Alloc *allocator) noexcept {
	this->Object::replaceAllocator(allocator);

	paramTypes.replaceAllocator(allocator);
}

SLAKE_API SIMDTypeDefObject::SIMDTypeDefObject(Runtime *rt, peff::Alloc *selfAllocator)
	: Object(rt, selfAllocator, ObjectKind::SIMDTypeDef) {
}

SLAKE_API SIMDTypeDefObject::SIMDTypeDefObject(Duplicator *duplicator, const SIMDTypeDefObject &x, peff::Alloc *allocator, bool &succeededOut) : Object(x, allocator) {
	if (!duplicator->insertTask(DuplicationTask::makeType(&type, x.type))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLAKE_API SIMDTypeDefObject::~SIMDTypeDefObject() {
}

SLAKE_API Object *SIMDTypeDefObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<SIMDTypeDefObject> slake::SIMDTypeDefObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<SIMDTypeDefObject, util::DeallocableDeleter<SIMDTypeDefObject>> ptr(
		peff::allocAndConstruct<SIMDTypeDefObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			rt, curGenerationAllocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<SIMDTypeDefObject> slake::SIMDTypeDefObject::alloc(Duplicator *duplicator, const SIMDTypeDefObject *other) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = other->associatedRuntime->getCurGenAlloc();

	bool succeeded = true;

	std::unique_ptr<SIMDTypeDefObject, util::DeallocableDeleter<SIMDTypeDefObject>> ptr(
		peff::allocAndConstruct<SIMDTypeDefObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			duplicator, *other, curGenerationAllocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associatedRuntime->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::SIMDTypeDefObject::dealloc() {
	peff::destroyAndRelease<SIMDTypeDefObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}
