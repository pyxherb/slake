#include "typedef.h"

#include <slake/runtime.h>

using namespace slake;

SLAKE_API HeapTypeObject::HeapTypeObject(Runtime *rt, peff::Alloc *self_allocator)
	: Object(rt, self_allocator, ObjectKind::HeapType) {
}

SLAKE_API HeapTypeObject::HeapTypeObject(Duplicator *duplicator, const HeapTypeObject &x, peff::Alloc *allocator, bool &succeeded_out) : Object(x, allocator) {
	if (!duplicator->insert_task(DuplicationTask::make_type(&type_ref, x.type_ref))) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLAKE_API HeapTypeObject::~HeapTypeObject() {
}

SLAKE_API Object *HeapTypeObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<HeapTypeObject> slake::HeapTypeObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = rt->get_cur_gen_alloc();

	std::unique_ptr<HeapTypeObject, peff::DeallocableDeleter<HeapTypeObject>> ptr(
		peff::alloc_and_construct<HeapTypeObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			rt, cur_generation_allocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<HeapTypeObject> slake::HeapTypeObject::alloc(Duplicator *duplicator, const HeapTypeObject *other) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = other->associated_runtime->get_cur_gen_alloc();

	bool succeeded = true;

	std::unique_ptr<HeapTypeObject, peff::DeallocableDeleter<HeapTypeObject>> ptr(
		peff::alloc_and_construct<HeapTypeObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			duplicator, *other, cur_generation_allocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associated_runtime->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::HeapTypeObject::dealloc() {
	peff::destroy_and_release<HeapTypeObject>(get_allocator(), this, alignof(HeapTypeObject));
}

SLAKE_API TypeDefObject::TypeDefObject(Runtime *rt, peff::Alloc *self_allocator, TypeDefKind type_def_kind)
	: Object(rt, self_allocator, ObjectKind::TypeDef), _type_def_kind(type_def_kind) {
}

SLAKE_API TypeDefObject::TypeDefObject(Duplicator *duplicator, const TypeDefObject &x, peff::Alloc *allocator) : Object(x, allocator) {
	_type_def_kind = x._type_def_kind;
}

SLAKE_API TypeDefObject::~TypeDefObject() {
}

SLAKE_API CustomTypeDefObject::CustomTypeDefObject(Runtime *rt, peff::Alloc *self_allocator)
	: TypeDefObject(rt, self_allocator, TypeDefKind::CustomTypeDef) {
}

SLAKE_API CustomTypeDefObject::CustomTypeDefObject(Duplicator *duplicator, const CustomTypeDefObject &x, peff::Alloc *allocator, bool &succeeded_out) : TypeDefObject(duplicator, x, allocator) {
	type_object = x.type_object;

	succeeded_out = true;
}

SLAKE_API CustomTypeDefObject::~CustomTypeDefObject() {
}

SLAKE_API Object *CustomTypeDefObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<CustomTypeDefObject> slake::CustomTypeDefObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = rt->get_cur_gen_alloc();

	std::unique_ptr<CustomTypeDefObject, peff::DeallocableDeleter<CustomTypeDefObject>> ptr(
		peff::alloc_and_construct<CustomTypeDefObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			rt, cur_generation_allocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<CustomTypeDefObject> slake::CustomTypeDefObject::alloc(Duplicator *duplicator, const CustomTypeDefObject *other) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = other->associated_runtime->get_cur_gen_alloc();

	bool succeeded = true;

	std::unique_ptr<CustomTypeDefObject, peff::DeallocableDeleter<CustomTypeDefObject>> ptr(
		peff::alloc_and_construct<CustomTypeDefObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			duplicator, *other, cur_generation_allocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associated_runtime->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::CustomTypeDefObject::dealloc() {
	peff::destroy_and_release<CustomTypeDefObject>(get_allocator(), this, alignof(CustomTypeDefObject));
}

SLAKE_API ArrayTypeDefObject::ArrayTypeDefObject(Runtime *rt, peff::Alloc *self_allocator)
	: TypeDefObject(rt, self_allocator, TypeDefKind::ArrayTypeDef) {
}

SLAKE_API ArrayTypeDefObject::ArrayTypeDefObject(Duplicator *duplicator, const ArrayTypeDefObject &x, peff::Alloc *allocator, bool &succeeded_out) : TypeDefObject(duplicator, x, allocator) {
	if (!duplicator->insert_task(DuplicationTask::make_normal((Object **)&element_type, x.element_type))) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLAKE_API ArrayTypeDefObject::~ArrayTypeDefObject() {
}

SLAKE_API Object *ArrayTypeDefObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<ArrayTypeDefObject> slake::ArrayTypeDefObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = rt->get_cur_gen_alloc();

	std::unique_ptr<ArrayTypeDefObject, peff::DeallocableDeleter<ArrayTypeDefObject>> ptr(
		peff::alloc_and_construct<ArrayTypeDefObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			rt, cur_generation_allocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<ArrayTypeDefObject> slake::ArrayTypeDefObject::alloc(Duplicator *duplicator, const ArrayTypeDefObject *other) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = other->associated_runtime->get_cur_gen_alloc();

	bool succeeded = true;

	std::unique_ptr<ArrayTypeDefObject, peff::DeallocableDeleter<ArrayTypeDefObject>> ptr(
		peff::alloc_and_construct<ArrayTypeDefObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			duplicator, *other, cur_generation_allocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associated_runtime->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::ArrayTypeDefObject::dealloc() {
	peff::destroy_and_release<ArrayTypeDefObject>(get_allocator(), this, alignof(ArrayTypeDefObject));
}

SLAKE_API RefTypeDefObject::RefTypeDefObject(Runtime *rt, peff::Alloc *self_allocator)
	: TypeDefObject(rt, self_allocator, TypeDefKind::RefTypeDef) {
}

SLAKE_API RefTypeDefObject::RefTypeDefObject(Duplicator *duplicator, const RefTypeDefObject &x, peff::Alloc *allocator, bool &succeeded_out) : TypeDefObject(duplicator, x, allocator) {
	if (!duplicator->insert_task(DuplicationTask::make_normal((Object **)&referenced_type, x.referenced_type))) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLAKE_API RefTypeDefObject::~RefTypeDefObject() {
}

SLAKE_API Object *RefTypeDefObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<RefTypeDefObject> slake::RefTypeDefObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = rt->get_cur_gen_alloc();

	std::unique_ptr<RefTypeDefObject, peff::DeallocableDeleter<RefTypeDefObject>> ptr(
		peff::alloc_and_construct<RefTypeDefObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			rt, cur_generation_allocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<RefTypeDefObject> slake::RefTypeDefObject::alloc(Duplicator *duplicator, const RefTypeDefObject *other) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = other->associated_runtime->get_cur_gen_alloc();

	bool succeeded = true;

	std::unique_ptr<RefTypeDefObject, peff::DeallocableDeleter<RefTypeDefObject>> ptr(
		peff::alloc_and_construct<RefTypeDefObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			duplicator, *other, cur_generation_allocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associated_runtime->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::RefTypeDefObject::dealloc() {
	peff::destroy_and_release<RefTypeDefObject>(get_allocator(), this, alignof(RefTypeDefObject));
}

SLAKE_API GenericArgTypeDefObject::GenericArgTypeDefObject(Runtime *rt, peff::Alloc *self_allocator)
	: TypeDefObject(rt, self_allocator, TypeDefKind::GenericArgTypeDef) {
}

SLAKE_API GenericArgTypeDefObject::GenericArgTypeDefObject(Duplicator *duplicator, const GenericArgTypeDefObject &x, peff::Alloc *allocator, bool &succeeded_out) : TypeDefObject(duplicator, x, allocator) {
	owner_object = x.owner_object;
	name_object = x.name_object;

	succeeded_out = true;
}

SLAKE_API GenericArgTypeDefObject::~GenericArgTypeDefObject() {
}

SLAKE_API Object *GenericArgTypeDefObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<GenericArgTypeDefObject> slake::GenericArgTypeDefObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = rt->get_cur_gen_alloc();

	std::unique_ptr<GenericArgTypeDefObject, peff::DeallocableDeleter<GenericArgTypeDefObject>> ptr(
		peff::alloc_and_construct<GenericArgTypeDefObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			rt, cur_generation_allocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<GenericArgTypeDefObject> slake::GenericArgTypeDefObject::alloc(Duplicator *duplicator, const GenericArgTypeDefObject *other) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = other->associated_runtime->get_cur_gen_alloc();

	bool succeeded = true;

	std::unique_ptr<GenericArgTypeDefObject, peff::DeallocableDeleter<GenericArgTypeDefObject>> ptr(
		peff::alloc_and_construct<GenericArgTypeDefObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			duplicator, *other, cur_generation_allocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associated_runtime->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::GenericArgTypeDefObject::dealloc() {
	peff::destroy_and_release<GenericArgTypeDefObject>(get_allocator(), this, alignof(GenericArgTypeDefObject));
}

SLAKE_API FnTypeDefObject::FnTypeDefObject(Runtime *rt, peff::Alloc *self_allocator)
	: TypeDefObject(rt, self_allocator, TypeDefKind::FnTypeDef), param_types(self_allocator) {
}

SLAKE_API FnTypeDefObject::FnTypeDefObject(Duplicator *duplicator, const FnTypeDefObject &x, peff::Alloc *allocator, bool &succeeded_out) : TypeDefObject(duplicator, x, allocator), param_types(allocator) {
	if (!duplicator->insert_task(DuplicationTask::make_normal((Object **)&return_type, x.return_type))) {
		succeeded_out = false;
		return;
	}

	if (!param_types.resize(x.param_types.size())) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < x.param_types.size(); ++i) {
		if (!duplicator->insert_task(DuplicationTask::make_normal((Object **)&param_types.at(i), x.param_types.at(i)))) {
			succeeded_out = false;
			return;
		}
	}

	has_var_arg = x.has_var_arg;

	if (!duplicator->insert_task(DuplicationTask::make_normal((Object **)&this_type, x.this_type))) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLAKE_API FnTypeDefObject::~FnTypeDefObject() {
}

SLAKE_API Object *FnTypeDefObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<FnTypeDefObject> slake::FnTypeDefObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = rt->get_cur_gen_alloc();

	std::unique_ptr<FnTypeDefObject, peff::DeallocableDeleter<FnTypeDefObject>> ptr(
		peff::alloc_and_construct<FnTypeDefObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			rt, cur_generation_allocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<FnTypeDefObject> slake::FnTypeDefObject::alloc(Duplicator *duplicator, const FnTypeDefObject *other) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = other->associated_runtime->get_cur_gen_alloc();

	bool succeeded = true;

	std::unique_ptr<FnTypeDefObject, peff::DeallocableDeleter<FnTypeDefObject>> ptr(
		peff::alloc_and_construct<FnTypeDefObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			duplicator, *other, cur_generation_allocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associated_runtime->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::FnTypeDefObject::dealloc() {
	peff::destroy_and_release<FnTypeDefObject>(get_allocator(), this, alignof(FnTypeDefObject));
}

SLAKE_API void FnTypeDefObject::replace_allocator(peff::Alloc *allocator) noexcept {
	this->Object::replace_allocator(allocator);

	param_types.replace_allocator(allocator);
}

SLAKE_API ParamTypeListTypeDefObject::ParamTypeListTypeDefObject(Runtime *rt, peff::Alloc *self_allocator)
	: TypeDefObject(rt, self_allocator, TypeDefKind::ParamTypeListTypeDef), param_types(self_allocator) {
}

SLAKE_API ParamTypeListTypeDefObject::ParamTypeListTypeDefObject(Duplicator *duplicator, const ParamTypeListTypeDefObject &x, peff::Alloc *allocator, bool &succeeded_out) : TypeDefObject(duplicator, x, allocator), param_types(allocator) {
	if (!param_types.resize(x.param_types.size())) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < x.param_types.size(); ++i) {
		if (!duplicator->insert_task(DuplicationTask::make_normal((Object **)&param_types.at(i), x.param_types.at(i)))) {
			succeeded_out = false;
			return;
		}
	}

	has_var_arg = x.has_var_arg;

	succeeded_out = true;
}

SLAKE_API ParamTypeListTypeDefObject::~ParamTypeListTypeDefObject() {
}

SLAKE_API Object *ParamTypeListTypeDefObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<ParamTypeListTypeDefObject> slake::ParamTypeListTypeDefObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = rt->get_cur_gen_alloc();

	std::unique_ptr<ParamTypeListTypeDefObject, peff::DeallocableDeleter<ParamTypeListTypeDefObject>> ptr(
		peff::alloc_and_construct<ParamTypeListTypeDefObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			rt, cur_generation_allocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<ParamTypeListTypeDefObject> slake::ParamTypeListTypeDefObject::alloc(Duplicator *duplicator, const ParamTypeListTypeDefObject *other) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = other->associated_runtime->get_cur_gen_alloc();

	bool succeeded = true;

	std::unique_ptr<ParamTypeListTypeDefObject, peff::DeallocableDeleter<ParamTypeListTypeDefObject>> ptr(
		peff::alloc_and_construct<ParamTypeListTypeDefObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			duplicator, *other, cur_generation_allocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associated_runtime->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::ParamTypeListTypeDefObject::dealloc() {
	peff::destroy_and_release<ParamTypeListTypeDefObject>(get_allocator(), this, alignof(ParamTypeListTypeDefObject));
}

SLAKE_API void ParamTypeListTypeDefObject::replace_allocator(peff::Alloc *allocator) noexcept {
	this->Object::replace_allocator(allocator);

	param_types.replace_allocator(allocator);
}

SLAKE_API TupleTypeDefObject::TupleTypeDefObject(Runtime *rt, peff::Alloc *self_allocator)
	: TypeDefObject(rt, self_allocator, TypeDefKind::TupleTypeDef), element_types(self_allocator) {
}

SLAKE_API TupleTypeDefObject::TupleTypeDefObject(Duplicator *duplicator, const TupleTypeDefObject &x, peff::Alloc *allocator, bool &succeeded_out) : TypeDefObject(duplicator, x, allocator), element_types(allocator) {
	if (!element_types.resize(x.element_types.size())) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < x.element_types.size(); ++i) {
		if (!duplicator->insert_task(DuplicationTask::make_normal((Object **)&element_types.at(i), x.element_types.at(i)))) {
			succeeded_out = false;
			return;
		}
	}

	succeeded_out = true;
}

SLAKE_API TupleTypeDefObject::~TupleTypeDefObject() {
}

SLAKE_API Object *TupleTypeDefObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<TupleTypeDefObject> slake::TupleTypeDefObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = rt->get_cur_gen_alloc();

	std::unique_ptr<TupleTypeDefObject, peff::DeallocableDeleter<TupleTypeDefObject>> ptr(
		peff::alloc_and_construct<TupleTypeDefObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			rt, cur_generation_allocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<TupleTypeDefObject> slake::TupleTypeDefObject::alloc(Duplicator *duplicator, const TupleTypeDefObject *other) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = other->associated_runtime->get_cur_gen_alloc();

	bool succeeded = true;

	std::unique_ptr<TupleTypeDefObject, peff::DeallocableDeleter<TupleTypeDefObject>> ptr(
		peff::alloc_and_construct<TupleTypeDefObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			duplicator, *other, cur_generation_allocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associated_runtime->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::TupleTypeDefObject::dealloc() {
	peff::destroy_and_release<TupleTypeDefObject>(get_allocator(), this, alignof(TupleTypeDefObject));
}

SLAKE_API void TupleTypeDefObject::replace_allocator(peff::Alloc *allocator) noexcept {
	this->Object::replace_allocator(allocator);

	element_types.replace_allocator(allocator);
}

SLAKE_API SIMDTypeDefObject::SIMDTypeDefObject(Runtime *rt, peff::Alloc *self_allocator)
	: TypeDefObject(rt, self_allocator, TypeDefKind::SIMDTypeDef) {
}

SLAKE_API SIMDTypeDefObject::SIMDTypeDefObject(Duplicator *duplicator, const SIMDTypeDefObject &x, peff::Alloc *allocator, bool &succeeded_out) : TypeDefObject(duplicator, x, allocator) {
	if (!duplicator->insert_task(DuplicationTask::make_normal((Object **)&type, x.type))) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLAKE_API SIMDTypeDefObject::~SIMDTypeDefObject() {
}

SLAKE_API Object *SIMDTypeDefObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<SIMDTypeDefObject> slake::SIMDTypeDefObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = rt->get_cur_gen_alloc();

	std::unique_ptr<SIMDTypeDefObject, peff::DeallocableDeleter<SIMDTypeDefObject>> ptr(
		peff::alloc_and_construct<SIMDTypeDefObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			rt, cur_generation_allocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<SIMDTypeDefObject> slake::SIMDTypeDefObject::alloc(Duplicator *duplicator, const SIMDTypeDefObject *other) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = other->associated_runtime->get_cur_gen_alloc();

	bool succeeded = true;

	std::unique_ptr<SIMDTypeDefObject, peff::DeallocableDeleter<SIMDTypeDefObject>> ptr(
		peff::alloc_and_construct<SIMDTypeDefObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			duplicator, *other, cur_generation_allocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associated_runtime->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::SIMDTypeDefObject::dealloc() {
	peff::destroy_and_release<SIMDTypeDefObject>(get_allocator(), this, alignof(SIMDTypeDefObject));
}

SLAKE_API UnpackingTypeDefObject::UnpackingTypeDefObject(Runtime *rt, peff::Alloc *self_allocator)
	: TypeDefObject(rt, self_allocator, TypeDefKind::UnpackingTypeDef) {
}

SLAKE_API UnpackingTypeDefObject::UnpackingTypeDefObject(Duplicator *duplicator, const UnpackingTypeDefObject &x, peff::Alloc *allocator, bool &succeeded_out) : TypeDefObject(duplicator, x, allocator) {
	if (!duplicator->insert_task(DuplicationTask::make_normal((Object **)&type, x.type))) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLAKE_API UnpackingTypeDefObject::~UnpackingTypeDefObject() {
}

SLAKE_API Object *UnpackingTypeDefObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<UnpackingTypeDefObject> slake::UnpackingTypeDefObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = rt->get_cur_gen_alloc();

	std::unique_ptr<UnpackingTypeDefObject, peff::DeallocableDeleter<UnpackingTypeDefObject>> ptr(
		peff::alloc_and_construct<UnpackingTypeDefObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			rt, cur_generation_allocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<UnpackingTypeDefObject> slake::UnpackingTypeDefObject::alloc(Duplicator *duplicator, const UnpackingTypeDefObject *other) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = other->associated_runtime->get_cur_gen_alloc();

	bool succeeded = true;

	std::unique_ptr<UnpackingTypeDefObject, peff::DeallocableDeleter<UnpackingTypeDefObject>> ptr(
		peff::alloc_and_construct<UnpackingTypeDefObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			duplicator, *other, cur_generation_allocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associated_runtime->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::UnpackingTypeDefObject::dealloc() {
	peff::destroy_and_release<UnpackingTypeDefObject>(get_allocator(), this, alignof(UnpackingTypeDefObject));
}

SLAKE_API int TypeDefComparator::operator()(const TypeDefObject *lhs, const TypeDefObject *rhs) const noexcept {
	TypeDefKind type_def_kind;

	{
		TypeDefKind rhs_type_def_kind;
		if ((type_def_kind = lhs->get_type_def_kind()) < (rhs_type_def_kind = rhs->get_type_def_kind()))
			return -1;
		if (type_def_kind > rhs_type_def_kind)
			return 1;
	}

	switch (type_def_kind) {
		case TypeDefKind::CustomTypeDef: {
			CustomTypeDefObject *l = (CustomTypeDefObject *)lhs,
								*r = (CustomTypeDefObject *)rhs;

			if (l->type_object->get_object_kind() < r->type_object->get_object_kind())
				return -1;
			if (l->type_object->get_object_kind() > r->type_object->get_object_kind())
				return 1;

			if (l->is_loading_deferred()) {
				int result = IdRefComparator()((IdRefObject *)l->type_object, (IdRefObject *)r->type_object);
				if (result != 0)
					return result;
			} else {
				if (l->type_object < r->type_object)
					return -1;
				if (l->type_object > r->type_object)
					return 1;
			}

			break;
		}
		case TypeDefKind::ArrayTypeDef: {
			ArrayTypeDefObject *l = (ArrayTypeDefObject *)lhs,
							   *r = (ArrayTypeDefObject *)rhs;

			if (l->element_type->type_ref < r->element_type->type_ref)
				return -1;
			if (l->element_type->type_ref > r->element_type->type_ref)
				return 1;

			break;
		}
		case TypeDefKind::GenericArgTypeDef: {
			GenericArgTypeDefObject *l = (GenericArgTypeDefObject *)lhs,
									*r = (GenericArgTypeDefObject *)rhs;

			if (l->owner_object < r->owner_object)
				return -1;
			if (l->owner_object > r->owner_object)
				return 1;

			if (l->name_object->data < r->name_object->data)
				return -1;
			if (l->name_object->data > r->name_object->data)
				return 1;

			break;
		}
		case TypeDefKind::RefTypeDef: {
			RefTypeDefObject *l = (RefTypeDefObject *)lhs,
							 *r = (RefTypeDefObject *)rhs;

			if (l->referenced_type->type_ref < r->referenced_type->type_ref)
				return -1;
			if (l->referenced_type->type_ref > r->referenced_type->type_ref)
				return 1;

			break;
		}
		case TypeDefKind::FnTypeDef: {
			FnTypeDefObject *l = (FnTypeDefObject *)lhs,
							*r = (FnTypeDefObject *)rhs;

			if (l->return_type->type_ref < r->return_type->type_ref)
				return -1;
			if (l->return_type->type_ref > r->return_type->type_ref)
				return 1;

			if (l->param_types.size() < r->param_types.size())
				return -1;
			if (l->param_types.size() > r->param_types.size())
				return 1;

			for (size_t i = 0; i < l->param_types.size(); ++i) {
				const TypeRef &lt = l->param_types.at(i)->type_ref, &rt = r->param_types.at(i)->type_ref;

				if (lt < rt)
					return -1;
				if (lt > rt)
					return 1;
			}

			if ((uint8_t)l->has_var_arg < (uint8_t)r->has_var_arg)
				return -1;
			if ((uint8_t)l->has_var_arg > (uint8_t)r->has_var_arg)
				return 1;

			break;
		}
		case TypeDefKind::ParamTypeListTypeDef: {
			ParamTypeListTypeDefObject *l = (ParamTypeListTypeDefObject *)lhs,
									   *r = (ParamTypeListTypeDefObject *)rhs;

			if (l->param_types.size() < r->param_types.size())
				return -1;
			if (l->param_types.size() > r->param_types.size())
				return 1;

			for (size_t i = 0; i < l->param_types.size(); ++i) {
				const TypeRef &lt = l->param_types.at(i)->type_ref, &rt = r->param_types.at(i)->type_ref;

				if (lt < rt)
					return -1;
				if (lt > rt)
					return 1;
			}

			if ((uint8_t)l->has_var_arg < (uint8_t)r->has_var_arg)
				return -1;
			if ((uint8_t)l->has_var_arg > (uint8_t)r->has_var_arg)
				return 1;

			break;
		}
		case TypeDefKind::TupleTypeDef: {
			TupleTypeDefObject *l = (TupleTypeDefObject *)lhs,
							   *r = (TupleTypeDefObject *)rhs;

			for (size_t i = 0; i < l->element_types.size(); ++i) {
				const TypeRef &lt = l->element_types.at(i)->type_ref, &rt = r->element_types.at(i)->type_ref;

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

SLAKE_API bool slake::is_type_def_object(Object *object) {
	if (!object)
		return false;

	return object->get_object_kind() == ObjectKind::TypeDef;
}
