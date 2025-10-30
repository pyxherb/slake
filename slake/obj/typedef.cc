#include "typedef.h"

#include <slake/runtime.h>

using namespace slake;

SLAKE_API HeapTypeObject::HeapTypeObject(Runtime *rt, peff::Alloc *selfAllocator)
	: Object(rt, selfAllocator, ObjectKind::HeapType) {
}

SLAKE_API HeapTypeObject::HeapTypeObject(Duplicator *duplicator, const HeapTypeObject &x, peff::Alloc *allocator, bool &succeededOut) : Object(x, allocator) {
	if (!duplicator->insertTask(DuplicationTask::makeType(&typeRef, x.typeRef))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLAKE_API HeapTypeObject::~HeapTypeObject() {
}

SLAKE_API Object *HeapTypeObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<HeapTypeObject> slake::HeapTypeObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<HeapTypeObject, peff::DeallocableDeleter<HeapTypeObject>> ptr(
		peff::allocAndConstruct<HeapTypeObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			rt, curGenerationAllocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<HeapTypeObject> slake::HeapTypeObject::alloc(Duplicator *duplicator, const HeapTypeObject *other) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = other->associatedRuntime->getCurGenAlloc();

	bool succeeded = true;

	std::unique_ptr<HeapTypeObject, peff::DeallocableDeleter<HeapTypeObject>> ptr(
		peff::allocAndConstruct<HeapTypeObject>(
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

SLAKE_API void slake::HeapTypeObject::dealloc() {
	peff::destroyAndRelease<HeapTypeObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API TypeDefObject::TypeDefObject(Runtime *rt, peff::Alloc *selfAllocator, TypeDefKind typeDefKind)
	: Object(rt, selfAllocator, ObjectKind::TypeDef), _typeDefKind(typeDefKind) {
}

SLAKE_API TypeDefObject::TypeDefObject(Duplicator *duplicator, const TypeDefObject &x, peff::Alloc *allocator) : Object(x, allocator) {
	_typeDefKind = x._typeDefKind;
}

SLAKE_API TypeDefObject::~TypeDefObject() {
}

SLAKE_API CustomTypeDefObject::CustomTypeDefObject(Runtime *rt, peff::Alloc *selfAllocator)
	: TypeDefObject(rt, selfAllocator, TypeDefKind::CustomTypeDef) {
}

SLAKE_API CustomTypeDefObject::CustomTypeDefObject(Duplicator *duplicator, const CustomTypeDefObject &x, peff::Alloc *allocator, bool &succeededOut) : TypeDefObject(duplicator, x, allocator) {
	typeObject = x.typeObject;

	succeededOut = true;
}

SLAKE_API CustomTypeDefObject::~CustomTypeDefObject() {
}

SLAKE_API Object *CustomTypeDefObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<CustomTypeDefObject> slake::CustomTypeDefObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<CustomTypeDefObject, peff::DeallocableDeleter<CustomTypeDefObject>> ptr(
		peff::allocAndConstruct<CustomTypeDefObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			rt, curGenerationAllocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<CustomTypeDefObject> slake::CustomTypeDefObject::alloc(Duplicator *duplicator, const CustomTypeDefObject *other) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = other->associatedRuntime->getCurGenAlloc();

	bool succeeded = true;

	std::unique_ptr<CustomTypeDefObject, peff::DeallocableDeleter<CustomTypeDefObject>> ptr(
		peff::allocAndConstruct<CustomTypeDefObject>(
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

SLAKE_API void slake::CustomTypeDefObject::dealloc() {
	peff::destroyAndRelease<CustomTypeDefObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API ArrayTypeDefObject::ArrayTypeDefObject(Runtime *rt, peff::Alloc *selfAllocator)
	: TypeDefObject(rt, selfAllocator, TypeDefKind::ArrayTypeDef) {
}

SLAKE_API ArrayTypeDefObject::ArrayTypeDefObject(Duplicator *duplicator, const ArrayTypeDefObject &x, peff::Alloc *allocator, bool &succeededOut) : TypeDefObject(duplicator, x, allocator) {
	if (!duplicator->insertTask(DuplicationTask::makeNormal((Object **)&elementType, x.elementType))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLAKE_API ArrayTypeDefObject::~ArrayTypeDefObject() {
}

SLAKE_API Object *ArrayTypeDefObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<ArrayTypeDefObject> slake::ArrayTypeDefObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<ArrayTypeDefObject, peff::DeallocableDeleter<ArrayTypeDefObject>> ptr(
		peff::allocAndConstruct<ArrayTypeDefObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			rt, curGenerationAllocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<ArrayTypeDefObject> slake::ArrayTypeDefObject::alloc(Duplicator *duplicator, const ArrayTypeDefObject *other) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = other->associatedRuntime->getCurGenAlloc();

	bool succeeded = true;

	std::unique_ptr<ArrayTypeDefObject, peff::DeallocableDeleter<ArrayTypeDefObject>> ptr(
		peff::allocAndConstruct<ArrayTypeDefObject>(
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

SLAKE_API void slake::ArrayTypeDefObject::dealloc() {
	peff::destroyAndRelease<ArrayTypeDefObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API RefTypeDefObject::RefTypeDefObject(Runtime *rt, peff::Alloc *selfAllocator)
	: TypeDefObject(rt, selfAllocator, TypeDefKind::RefTypeDef) {
}

SLAKE_API RefTypeDefObject::RefTypeDefObject(Duplicator *duplicator, const RefTypeDefObject &x, peff::Alloc *allocator, bool &succeededOut) : TypeDefObject(duplicator, x, allocator) {
	if (!duplicator->insertTask(DuplicationTask::makeNormal((Object **)&referencedType, x.referencedType))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLAKE_API RefTypeDefObject::~RefTypeDefObject() {
}

SLAKE_API Object *RefTypeDefObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<RefTypeDefObject> slake::RefTypeDefObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<RefTypeDefObject, peff::DeallocableDeleter<RefTypeDefObject>> ptr(
		peff::allocAndConstruct<RefTypeDefObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			rt, curGenerationAllocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<RefTypeDefObject> slake::RefTypeDefObject::alloc(Duplicator *duplicator, const RefTypeDefObject *other) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = other->associatedRuntime->getCurGenAlloc();

	bool succeeded = true;

	std::unique_ptr<RefTypeDefObject, peff::DeallocableDeleter<RefTypeDefObject>> ptr(
		peff::allocAndConstruct<RefTypeDefObject>(
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

SLAKE_API void slake::RefTypeDefObject::dealloc() {
	peff::destroyAndRelease<RefTypeDefObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API GenericArgTypeDefObject::GenericArgTypeDefObject(Runtime *rt, peff::Alloc *selfAllocator)
	: TypeDefObject(rt, selfAllocator, TypeDefKind::GenericArgTypeDef) {
}

SLAKE_API GenericArgTypeDefObject::GenericArgTypeDefObject(Duplicator *duplicator, const GenericArgTypeDefObject &x, peff::Alloc *allocator, bool &succeededOut) : TypeDefObject(duplicator, x, allocator) {
	ownerObject = x.ownerObject;
	nameObject = x.nameObject;

	succeededOut = true;
}

SLAKE_API GenericArgTypeDefObject::~GenericArgTypeDefObject() {
}

SLAKE_API Object *GenericArgTypeDefObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<GenericArgTypeDefObject> slake::GenericArgTypeDefObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<GenericArgTypeDefObject, peff::DeallocableDeleter<GenericArgTypeDefObject>> ptr(
		peff::allocAndConstruct<GenericArgTypeDefObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			rt, curGenerationAllocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<GenericArgTypeDefObject> slake::GenericArgTypeDefObject::alloc(Duplicator *duplicator, const GenericArgTypeDefObject *other) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = other->associatedRuntime->getCurGenAlloc();

	bool succeeded = true;

	std::unique_ptr<GenericArgTypeDefObject, peff::DeallocableDeleter<GenericArgTypeDefObject>> ptr(
		peff::allocAndConstruct<GenericArgTypeDefObject>(
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

SLAKE_API void slake::GenericArgTypeDefObject::dealloc() {
	peff::destroyAndRelease<GenericArgTypeDefObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API FnTypeDefObject::FnTypeDefObject(Runtime *rt, peff::Alloc *selfAllocator)
	: TypeDefObject(rt, selfAllocator, TypeDefKind::FnTypeDef), paramTypes(selfAllocator) {
}

SLAKE_API FnTypeDefObject::FnTypeDefObject(Duplicator *duplicator, const FnTypeDefObject &x, peff::Alloc *allocator, bool &succeededOut) : TypeDefObject(duplicator, x, allocator), paramTypes(allocator) {
	if (!duplicator->insertTask(DuplicationTask::makeNormal((Object **)&returnType, x.returnType))) {
		succeededOut = false;
		return;
	}

	if (!paramTypes.resize(x.paramTypes.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < x.paramTypes.size(); ++i) {
		if (!duplicator->insertTask(DuplicationTask::makeNormal((Object **)&paramTypes.at(i), x.paramTypes.at(i)))) {
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

	std::unique_ptr<FnTypeDefObject, peff::DeallocableDeleter<FnTypeDefObject>> ptr(
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

	std::unique_ptr<FnTypeDefObject, peff::DeallocableDeleter<FnTypeDefObject>> ptr(
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
	: TypeDefObject(rt, selfAllocator, TypeDefKind::ParamTypeListTypeDef), paramTypes(selfAllocator) {
}

SLAKE_API ParamTypeListTypeDefObject::ParamTypeListTypeDefObject(Duplicator *duplicator, const ParamTypeListTypeDefObject &x, peff::Alloc *allocator, bool &succeededOut) : TypeDefObject(duplicator, x, allocator), paramTypes(allocator) {
	if (!paramTypes.resize(x.paramTypes.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < x.paramTypes.size(); ++i) {
		if (!duplicator->insertTask(DuplicationTask::makeNormal((Object **)&paramTypes.at(i), x.paramTypes.at(i)))) {
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

	std::unique_ptr<ParamTypeListTypeDefObject, peff::DeallocableDeleter<ParamTypeListTypeDefObject>> ptr(
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

	std::unique_ptr<ParamTypeListTypeDefObject, peff::DeallocableDeleter<ParamTypeListTypeDefObject>> ptr(
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

SLAKE_API TupleTypeDefObject::TupleTypeDefObject(Runtime *rt, peff::Alloc *selfAllocator)
	: TypeDefObject(rt, selfAllocator, TypeDefKind::TupleTypeDef), elementTypes(selfAllocator) {
}

SLAKE_API TupleTypeDefObject::TupleTypeDefObject(Duplicator *duplicator, const TupleTypeDefObject &x, peff::Alloc *allocator, bool &succeededOut) : TypeDefObject(duplicator, x, allocator), elementTypes(allocator) {
	if (!elementTypes.resize(x.elementTypes.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < x.elementTypes.size(); ++i) {
		if (!duplicator->insertTask(DuplicationTask::makeNormal((Object **)&elementTypes.at(i), x.elementTypes.at(i)))) {
			succeededOut = false;
			return;
		}
	}

	succeededOut = true;
}

SLAKE_API TupleTypeDefObject::~TupleTypeDefObject() {
}

SLAKE_API Object *TupleTypeDefObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<TupleTypeDefObject> slake::TupleTypeDefObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<TupleTypeDefObject, peff::DeallocableDeleter<TupleTypeDefObject>> ptr(
		peff::allocAndConstruct<TupleTypeDefObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			rt, curGenerationAllocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<TupleTypeDefObject> slake::TupleTypeDefObject::alloc(Duplicator *duplicator, const TupleTypeDefObject *other) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = other->associatedRuntime->getCurGenAlloc();

	bool succeeded = true;

	std::unique_ptr<TupleTypeDefObject, peff::DeallocableDeleter<TupleTypeDefObject>> ptr(
		peff::allocAndConstruct<TupleTypeDefObject>(
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

SLAKE_API void slake::TupleTypeDefObject::dealloc() {
	peff::destroyAndRelease<TupleTypeDefObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API void TupleTypeDefObject::replaceAllocator(peff::Alloc *allocator) noexcept {
	this->Object::replaceAllocator(allocator);

	elementTypes.replaceAllocator(allocator);
}

SLAKE_API SIMDTypeDefObject::SIMDTypeDefObject(Runtime *rt, peff::Alloc *selfAllocator)
	: TypeDefObject(rt, selfAllocator, TypeDefKind::SIMDTypeDef) {
}

SLAKE_API SIMDTypeDefObject::SIMDTypeDefObject(Duplicator *duplicator, const SIMDTypeDefObject &x, peff::Alloc *allocator, bool &succeededOut) : TypeDefObject(duplicator, x, allocator) {
	if (!duplicator->insertTask(DuplicationTask::makeNormal((Object **)&type, x.type))) {
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

	std::unique_ptr<SIMDTypeDefObject, peff::DeallocableDeleter<SIMDTypeDefObject>> ptr(
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

	std::unique_ptr<SIMDTypeDefObject, peff::DeallocableDeleter<SIMDTypeDefObject>> ptr(
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

SLAKE_API UnpackingTypeDefObject::UnpackingTypeDefObject(Runtime *rt, peff::Alloc *selfAllocator)
	: TypeDefObject(rt, selfAllocator, TypeDefKind::UnpackingTypeDef) {
}

SLAKE_API UnpackingTypeDefObject::UnpackingTypeDefObject(Duplicator *duplicator, const UnpackingTypeDefObject &x, peff::Alloc *allocator, bool &succeededOut) : TypeDefObject(duplicator, x, allocator) {
	if (!duplicator->insertTask(DuplicationTask::makeNormal((Object **)&type, x.type))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLAKE_API UnpackingTypeDefObject::~UnpackingTypeDefObject() {
}

SLAKE_API Object *UnpackingTypeDefObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<UnpackingTypeDefObject> slake::UnpackingTypeDefObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<UnpackingTypeDefObject, peff::DeallocableDeleter<UnpackingTypeDefObject>> ptr(
		peff::allocAndConstruct<UnpackingTypeDefObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			rt, curGenerationAllocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<UnpackingTypeDefObject> slake::UnpackingTypeDefObject::alloc(Duplicator *duplicator, const UnpackingTypeDefObject *other) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = other->associatedRuntime->getCurGenAlloc();

	bool succeeded = true;

	std::unique_ptr<UnpackingTypeDefObject, peff::DeallocableDeleter<UnpackingTypeDefObject>> ptr(
		peff::allocAndConstruct<UnpackingTypeDefObject>(
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

SLAKE_API void slake::UnpackingTypeDefObject::dealloc() {
	peff::destroyAndRelease<UnpackingTypeDefObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API int TypeDefComparator::operator()(const TypeDefObject *lhs, const TypeDefObject *rhs) const noexcept {
	TypeDefKind typeDefKind;

	{
		TypeDefKind rhsTypeDefKind;
		if ((typeDefKind = lhs->getTypeDefKind()) < (rhsTypeDefKind = rhs->getTypeDefKind()))
			return -1;
		if (typeDefKind > rhsTypeDefKind)
			return 1;
	}

	switch (typeDefKind) {
		case TypeDefKind::CustomTypeDef: {
			CustomTypeDefObject *l = (CustomTypeDefObject *)lhs,
								*r = (CustomTypeDefObject *)rhs;

			if (l->typeObject->getObjectKind() < r->typeObject->getObjectKind())
				return -1;
			if (l->typeObject->getObjectKind() > r->typeObject->getObjectKind())
				return 1;

			if (l->isLoadingDeferred()) {
				int result = IdRefComparator()((IdRefObject *)l->typeObject, (IdRefObject *)r->typeObject);
				if (result != 0)
					return result;
			} else {
				if (l->typeObject < r->typeObject)
					return -1;
				if (l->typeObject > r->typeObject)
					return 1;
			}

			break;
		}
		case TypeDefKind::ArrayTypeDef: {
			ArrayTypeDefObject *l = (ArrayTypeDefObject *)lhs,
							   *r = (ArrayTypeDefObject *)rhs;

			if (l->elementType->typeRef < r->elementType->typeRef)
				return -1;
			if (l->elementType->typeRef > r->elementType->typeRef)
				return 1;

			break;
		}
		case TypeDefKind::GenericArgTypeDef: {
			GenericArgTypeDefObject *l = (GenericArgTypeDefObject *)lhs,
									*r = (GenericArgTypeDefObject *)rhs;

			if (l->ownerObject < r->ownerObject)
				return -1;
			if (l->ownerObject > r->ownerObject)
				return 1;

			if (l->nameObject->data < r->nameObject->data)
				return -1;
			if (l->nameObject->data > r->nameObject->data)
				return 1;

			break;
		}
		case TypeDefKind::RefTypeDef: {
			RefTypeDefObject *l = (RefTypeDefObject *)lhs,
							 *r = (RefTypeDefObject *)rhs;

			if (l->referencedType->typeRef < r->referencedType->typeRef)
				return -1;
			if (l->referencedType->typeRef > r->referencedType->typeRef)
				return 1;

			break;
		}
		case TypeDefKind::FnTypeDef: {
			FnTypeDefObject *l = (FnTypeDefObject *)lhs,
							*r = (FnTypeDefObject *)rhs;

			if (l->returnType->typeRef < r->returnType->typeRef)
				return -1;
			if (l->returnType->typeRef > r->returnType->typeRef)
				return 1;

			if (l->paramTypes.size() < r->paramTypes.size())
				return -1;
			if (l->paramTypes.size() > r->paramTypes.size())
				return 1;

			for (size_t i = 0; i < l->paramTypes.size(); ++i) {
				const TypeRef &lt = l->paramTypes.at(i)->typeRef, &rt = r->paramTypes.at(i)->typeRef;

				if (lt < rt)
					return -1;
				if (lt > rt)
					return 1;
			}

			if ((uint8_t)l->hasVarArg < (uint8_t)r->hasVarArg)
				return -1;
			if ((uint8_t)l->hasVarArg > (uint8_t)r->hasVarArg)
				return 1;

			break;
		}
		case TypeDefKind::ParamTypeListTypeDef: {
			ParamTypeListTypeDefObject *l = (ParamTypeListTypeDefObject *)lhs,
									   *r = (ParamTypeListTypeDefObject *)rhs;

			if (l->paramTypes.size() < r->paramTypes.size())
				return -1;
			if (l->paramTypes.size() > r->paramTypes.size())
				return 1;

			for (size_t i = 0; i < l->paramTypes.size(); ++i) {
				const TypeRef &lt = l->paramTypes.at(i)->typeRef, &rt = r->paramTypes.at(i)->typeRef;

				if (lt < rt)
					return -1;
				if (lt > rt)
					return 1;
			}

			if ((uint8_t)l->hasVarArg < (uint8_t)r->hasVarArg)
				return -1;
			if ((uint8_t)l->hasVarArg > (uint8_t)r->hasVarArg)
				return 1;

			break;
		}
		case TypeDefKind::TupleTypeDef: {
			TupleTypeDefObject *l = (TupleTypeDefObject *)lhs,
							   *r = (TupleTypeDefObject *)rhs;

			for (size_t i = 0; i < l->elementTypes.size(); ++i) {
				const TypeRef &lt = l->elementTypes.at(i)->typeRef, &rt = r->elementTypes.at(i)->typeRef;

				if (lt < rt)
					return -1;
				if (lt > rt)
					return 1;
			}

			break;
		}
		case TypeDefKind::SIMDTypeDef: {
			SIMDTypeDefObject *l = (SIMDTypeDefObject *)lhs,
							  *r = (SIMDTypeDefObject *)rhs;

			if (l->type < r->type)
				return -1;
			if (l->type > r->type)
				return 1;

			if (l->width < r->width)
				return -1;
			if (l->width > r->width)
				return 1;

			break;
		}
		case TypeDefKind::UnpackingTypeDef: {
			UnpackingTypeDefObject *l = (UnpackingTypeDefObject *)lhs,
								   *r = (UnpackingTypeDefObject *)rhs;

			if (l->type < r->type)
				return -1;
			if (l->type > r->type)
				return 1;

			break;
		}
		default:
			std::terminate();
	}

	return 0;
}

SLAKE_API bool slake::isTypeDefObject(Object *object) {
	if (!object)
		return false;

	return object->getObjectKind() == ObjectKind::TypeDef;
}
