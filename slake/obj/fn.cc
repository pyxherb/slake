#include <slake/runtime.h>
#include <peff/base/scope_guard.h>

using namespace slake;

SLAKE_API FnOverloadingObject::FnOverloadingObject(
	FnOverloadingKind overloading_kind,
	FnObject *fn_object,
	peff::Alloc *self_allocator)
	: Object(fn_object->associated_runtime, self_allocator, ObjectKind::FnOverloading),
	  overloading_kind(overloading_kind),
	  fn_object(fn_object),
	  generic_params(self_allocator),
	  mapped_generic_params(self_allocator),
	  mapped_generic_args(self_allocator),
	  param_types(self_allocator),
	  return_type(TypeRef{ TypeId::Void }) {
}

SLAKE_API FnOverloadingObject::FnOverloadingObject(const FnOverloadingObject &other, peff::Alloc *allocator, bool &succeeded_out)
	: Object(other, allocator),
	  generic_params(allocator),
	  mapped_generic_params(allocator),	 // No need to copy
	  mapped_generic_args(allocator),	 // No need to copy
	  param_types(allocator) {
	fn_object = other.fn_object;

	access = other.access;

	if (!generic_params.resize_uninit(other.generic_params.size())) {
		succeeded_out = false;
		return;
	}
	for (size_t i = 0; i < other.generic_params.size(); ++i) {
		if (!other.generic_params.at(i).copy(generic_params.at(i))) {
			for (size_t j = i; j; --j) {
				peff::destroy_at<GenericParam>(&generic_params.at(j - 1));
			}
			succeeded_out = false;
			return;
		}
	}
	for (auto [k, v] : other.mapped_generic_args) {
		peff::String name(allocator);

		if (!name.build(k)) {
			succeeded_out = false;
			return;
		}

		if (!(mapped_generic_args.insert(std::move(name), TypeRef(v)))) {
			succeeded_out = false;
			return;
		}
	}

	if (!param_types.resize(other.param_types.size())) {
		succeeded_out = false;
		return;
	}
	memcpy(param_types.data(), other.param_types.data(), param_types.size() * sizeof(TypeRef));
	return_type = other.return_type;

	overloading_flags = other.overloading_flags;
	overloading_kind = other.overloading_kind;
	overriden_type = other.overriden_type;
}

SLAKE_API FnOverloadingObject::~FnOverloadingObject() {
}

SLAKE_API void FnOverloadingObject::replace_allocator(peff::Alloc *allocator) noexcept {
	this->Object::replace_allocator(allocator);

	generic_params.replace_allocator(allocator);

	for (auto &i : generic_params) {
		i.replace_allocator(allocator);
	}

	mapped_generic_params.replace_allocator(allocator);
	mapped_generic_args.replace_allocator(allocator);

	param_types.replace_allocator(allocator);
}

SLAKE_API RegularFnOverloadingObject::RegularFnOverloadingObject(
	FnObject *fn_object,
	peff::Alloc *self_allocator)
	: FnOverloadingObject(
		  FnOverloadingKind::Regular,
		  fn_object,
		  self_allocator),
	  source_loc_descs(self_allocator),
	  instructions(self_allocator),
	  ins_object_set(self_allocator),
	  ins_type_set(self_allocator) {
	memset(num_registers, 0, sizeof(num_registers));
}

SLAKE_API RegularFnOverloadingObject::RegularFnOverloadingObject(
	Duplicator *duplicator,
	const RegularFnOverloadingObject &other,
	peff::Alloc *allocator,
	bool &succeeded_out)
	: FnOverloadingObject(other, allocator, succeeded_out),
	  source_loc_descs(allocator),
	  instructions(allocator),
	  ins_object_set(allocator),
	  ins_type_set(allocator) {
	if (succeeded_out) {
		if (!source_loc_descs.resize_uninit(other.source_loc_descs.size())) {
			succeeded_out = false;
			return;
		}
		memcpy(source_loc_descs.data(), other.source_loc_descs.data(), source_loc_descs.size() * sizeof(slxfmt::SourceLocDesc));

		if (!instructions.resize_uninit(other.instructions.size())) {
			succeeded_out = false;
			return;
		}

		memcpy(instructions.data(), other.instructions.data(), instructions.size() * sizeof(Instruction));

		memcpy(num_registers, other.num_registers, sizeof(num_registers));

		if (!ins_object_set.resize_uninit(other.ins_object_set.size())) {
			succeeded_out = false;
			return;
		}
		memcpy(ins_object_set.data(), other.ins_object_set.data(), ins_object_set.size() * sizeof(Object *));

		if (!ins_type_set.build(other.ins_type_set)) {
			succeeded_out = false;
			return;
		}
	}
}

SLAKE_API RegularFnOverloadingObject::~RegularFnOverloadingObject() {
}

SLAKE_API Object *slake::RegularFnOverloadingObject::duplicate(Duplicator *duplicator) const {
	return alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<RegularFnOverloadingObject> slake::RegularFnOverloadingObject::alloc(
	FnObject *fn_object) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = fn_object->associated_runtime->get_cur_gen_alloc();

	std::unique_ptr<RegularFnOverloadingObject, peff::DeallocableDeleter<RegularFnOverloadingObject>> ptr(
		peff::alloc_and_construct<RegularFnOverloadingObject>(
			cur_generation_allocator.get(),
			alignof(RegularFnOverloadingObject),
			fn_object,
			cur_generation_allocator.get()));
	if (!ptr)
		return nullptr;

	if (!fn_object->associated_runtime->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<RegularFnOverloadingObject> slake::RegularFnOverloadingObject::alloc(Duplicator *duplicator, const RegularFnOverloadingObject *other) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = other->fn_object->associated_runtime->get_cur_gen_alloc();

	bool succeeded = true;

	std::unique_ptr<RegularFnOverloadingObject, peff::DeallocableDeleter<RegularFnOverloadingObject>> ptr(
		peff::alloc_and_construct<RegularFnOverloadingObject>(
			cur_generation_allocator.get(),
			alignof(RegularFnOverloadingObject),
			duplicator, *other, cur_generation_allocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associated_runtime->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::RegularFnOverloadingObject::dealloc() {
	peff::destroy_and_release<RegularFnOverloadingObject>(get_allocator(), this, alignof(RegularFnOverloadingObject));
}

SLAKE_API void RegularFnOverloadingObject::replace_allocator(peff::Alloc *allocator) noexcept {
	this->FnOverloadingObject::replace_allocator(allocator);

	source_loc_descs.replace_allocator(allocator);

	instructions.replace_allocator(allocator);

	ins_object_set.replace_allocator(allocator);

	ins_type_set.replace_allocator(allocator);
}

SLAKE_API NativeFnOverloadingObject::NativeFnOverloadingObject(
	FnObject *fn_object,
	peff::Alloc *self_allocator,
	NativeFnCallback callback)
	: FnOverloadingObject(
		  FnOverloadingKind::Native,
		  fn_object,
		  self_allocator),
	  callback(callback) {}

SLAKE_API NativeFnOverloadingObject::NativeFnOverloadingObject(const NativeFnOverloadingObject &other, peff::Alloc *allocator, bool &succeeded_out) : FnOverloadingObject(other, allocator, succeeded_out) {
	if (succeeded_out) {
		callback = other.callback;
	}
}

SLAKE_API NativeFnOverloadingObject::~NativeFnOverloadingObject() {
}

SLAKE_API FnOverloadingObject *slake::NativeFnOverloadingObject::duplicate(Duplicator *duplicator) const {
	return (FnOverloadingObject *)alloc(this).get();
}

SLAKE_API HostObjectRef<NativeFnOverloadingObject> slake::NativeFnOverloadingObject::alloc(
	FnObject *fn_object,
	NativeFnCallback callback) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = fn_object->associated_runtime->get_cur_gen_alloc();

	std::unique_ptr<NativeFnOverloadingObject, peff::DeallocableDeleter<NativeFnOverloadingObject>> ptr(
		peff::alloc_and_construct<NativeFnOverloadingObject>(
			cur_generation_allocator.get(),
			alignof(NativeFnOverloadingObject),
			fn_object, cur_generation_allocator.get(), callback));
	if (!ptr)
		return nullptr;

	if (!fn_object->associated_runtime->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<NativeFnOverloadingObject> slake::NativeFnOverloadingObject::alloc(const NativeFnOverloadingObject *other) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = other->associated_runtime->get_cur_gen_alloc();

	bool succeeded = true;

	std::unique_ptr<NativeFnOverloadingObject, peff::DeallocableDeleter<NativeFnOverloadingObject>> ptr(
		peff::alloc_and_construct<NativeFnOverloadingObject>(
			cur_generation_allocator.get(),
			alignof(NativeFnOverloadingObject),
			*other, cur_generation_allocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associated_runtime->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::NativeFnOverloadingObject::dealloc() {
	peff::destroy_and_release<NativeFnOverloadingObject>(get_allocator(), this, alignof(NativeFnOverloadingObject));
}

SLAKE_API int FnSignatureComparator::operator()(const FnSignature &lhs, const FnSignature &rhs) const noexcept {
	int result = inner_comparator(lhs.param_types, rhs.param_types);
	if (result)
		return result;

	if (((int)lhs.has_var_arg) < ((int)rhs.has_var_arg))
		return -1;
	if (((int)rhs.has_var_arg) > ((int)rhs.has_var_arg))
		return 1;

	if (lhs.num_generic_params < rhs.num_generic_params) {
		return -1;
	}
	if (lhs.num_generic_params > rhs.num_generic_params) {
		return 1;
	}

	if (lhs.overriden_type < rhs.overriden_type) {
		return -1;
	}
	if (lhs.overriden_type > rhs.overriden_type) {
		return 1;
	}

	return 0;
}

SLAKE_API FnObject::FnObject(Runtime *rt, peff::Alloc *self_allocator) : MemberObject(rt, self_allocator, ObjectKind::Fn), overloadings(self_allocator) {
}

SLAKE_API FnObject::FnObject(const FnObject &x, peff::Alloc *allocator, bool &succeeded_out) : MemberObject(x, allocator, succeeded_out), overloadings(allocator) {
	if (succeeded_out) {
		for (auto [k, v] : x.overloadings) {
			FnOverloadingObject *ol = (FnOverloadingObject *)v->duplicate(nullptr);

			if (!ol) {
				succeeded_out = false;
				return;
			}

			ol->fn_object = this;

			if (!overloadings.insert({ ol->param_types, ol->is_with_var_args(), ol->generic_params.size(), ol->overriden_type }, +ol)) {
				succeeded_out = false;
				return;
			}
		}
	}
}

SLAKE_API FnObject::~FnObject() {
}

SLAKE_API Object *FnObject::duplicate(Duplicator *duplicator) const {
	SLAKE_REFERENCED_PARAM(duplicator);

	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<FnObject> slake::FnObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = rt->get_cur_gen_alloc();

	std::unique_ptr<FnObject, peff::DeallocableDeleter<FnObject>> ptr(
		peff::alloc_and_construct<FnObject>(
			cur_generation_allocator.get(),
			alignof(FnObject),
			rt, cur_generation_allocator.get()));

	if (!rt->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<FnObject> slake::FnObject::alloc(const FnObject *other) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = other->associated_runtime->get_cur_gen_alloc();

	bool succeeded = true;

	std::unique_ptr<FnObject, peff::DeallocableDeleter<FnObject>> ptr(
		peff::alloc_and_construct<FnObject>(
			cur_generation_allocator.get(),
			alignof(FnObject),
			*other, cur_generation_allocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associated_runtime->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::FnObject::dealloc() {
	peff::destroy_and_release<FnObject>(get_allocator(), this, alignof(FnObject));
}

SLAKE_API void FnObject::replace_allocator(peff::Alloc *allocator) noexcept {
	this->MemberObject::replace_allocator(allocator);

	overloadings.replace_allocator(allocator);
}

SLAKE_API InternalExceptionPointer FnObject::resort_overloadings() noexcept {
	// Resort the overloading map.
	// TODO: Can we check if any one of the overloadings is changed to
	// implement on-demand resorting?
	auto old_overloadings = std::move(overloadings);

	overloadings = peff::Map<FnSignature, FnOverloadingObject *, FnSignatureComparator, true>(get_allocator());

	for (auto [k, v] : old_overloadings) {
		if (!overloadings.insert(FnSignature(k), +v))
			return OutOfMemoryError::alloc();
	}

	return {};
}
