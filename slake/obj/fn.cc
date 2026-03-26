#include <slake/runtime.h>
#include <peff/base/scope_guard.h>

using namespace slake;

SLAKE_API Instruction::Instruction()
	: opcode((Opcode)0xff),
	  num_operands(0),
	  output(UINT32_MAX),
	  operands(nullptr),
	  operands_allocator(nullptr) {
}

SLAKE_API Instruction::Instruction(Instruction &&rhs)
	: off_source_loc_desc(rhs.off_source_loc_desc),
	  opcode(rhs.opcode),
	  num_operands(rhs.num_operands),
	  output(rhs.output),
	  operands(rhs.operands),
	  operands_allocator(rhs.operands_allocator) {
	rhs.off_source_loc_desc = SIZE_MAX;
	rhs.opcode = (Opcode)0xff;
	rhs.num_operands = 0;
	rhs.output = UINT32_MAX;
	rhs.operands = nullptr;
	rhs.operands_allocator = nullptr;
}

SLAKE_API Instruction::~Instruction() {
	clear_operands();
}

SLAKE_API void Instruction::clear_operands() {
	if (num_operands) {
		operands_allocator->release(operands, sizeof(Value) * num_operands, alignof(Value));
		operands = nullptr;
		num_operands = 0;
	} else {
		assert(!operands);
	}
	operands_allocator = nullptr;
}

[[nodiscard]] SLAKE_API bool Instruction::reserve_operands(peff::Alloc *allocator, uint32_t num_operands) {
	clear_operands();
	if (num_operands) {
		if (!(operands = (Value *)allocator->alloc(sizeof(Value) * num_operands, alignof(Value)))) {
			return false;
		}
		operands_allocator = allocator;
	}
	this->num_operands = num_operands;
	return true;
}

SLAKE_API void Instruction::replace_allocator(peff::Alloc *allocator) noexcept {
	peff::verify_replaceable(operands_allocator.get(), allocator);

	operands_allocator = allocator;
}

SLAKE_API bool Instruction::operator==(const Instruction &rhs) const {
	if (opcode != rhs.opcode)
		return false;
	if (output != rhs.output)
		return false;
	if (num_operands != rhs.num_operands)
		return false;
	for (size_t i = 0; i < num_operands; ++i) {
		if (operands[i] != rhs.operands[i])
			return false;
	}
	return true;
}

SLAKE_API bool Instruction::operator<(const Instruction &rhs) const {
	if (opcode < rhs.opcode)
		return true;
	if (opcode > rhs.opcode)
		return false;
	if (output < rhs.output)
		return true;
	if (num_operands < rhs.num_operands)
		return true;
	if (num_operands > rhs.num_operands)
		return false;
	for (size_t i = 0; i < num_operands; ++i) {
		if (operands[i] < rhs.operands[i])
			return true;
		if (operands[i] != rhs.operands[i])
			return false;
	}
	return false;
}

SLAKE_API Instruction &Instruction::operator=(Instruction &&rhs) {
	peff::construct_at<Instruction>(this, std::move(rhs));
	return *this;
}

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
	  mapped_generic_params(allocator),  // No need to copy
	  mapped_generic_args(allocator),	   // No need to copy
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
	  num_registers(0),
	  source_loc_descs(self_allocator),
	  instructions(self_allocator) {}

SLAKE_API RegularFnOverloadingObject::RegularFnOverloadingObject(const RegularFnOverloadingObject &other, peff::Alloc *allocator, bool &succeeded_out) : FnOverloadingObject(other, allocator, succeeded_out), source_loc_descs(allocator), instructions(allocator) {
	if (succeeded_out) {
		if (!source_loc_descs.resize(other.source_loc_descs.size())) {
			succeeded_out = false;
			return;
		}
		memcpy(source_loc_descs.data(), other.source_loc_descs.data(), source_loc_descs.size() * sizeof(slxfmt::SourceLocDesc));

		if (!instructions.resize(other.instructions.size())) {
			succeeded_out = false;
			return;
		}
		for (size_t i = 0; i < instructions.size(); ++i) {
			Instruction &cur_ins = instructions.at(i);
			const Instruction &other_cur_ins = other.instructions.at(i);
			cur_ins.opcode = other_cur_ins.opcode;

			cur_ins.output = other_cur_ins.output;

			if (!cur_ins.reserve_operands(allocator, other_cur_ins.num_operands)) {
				succeeded_out = false;
				return;
			}
			for (size_t j = 0; j < other_cur_ins.num_operands; ++j) {
				cur_ins.operands[j] = Value(ValueType::Undefined);
			}

			// Duplicate each of the operands.
			for (size_t j = 0; j < other_cur_ins.num_operands; ++j) {
				auto &operand = other_cur_ins.operands[j];

				if (operand.value_type == ValueType::Reference) {
					const Reference &entity_ref = operand.get_reference();
					switch (entity_ref.kind) {
						case ReferenceKind::ObjectRef:
							if (entity_ref.as_object)
								cur_ins.operands[j] = Reference(entity_ref.as_object->duplicate(nullptr));
							else
								cur_ins.operands[j] = operand;
							break;
						default:
							cur_ins.operands[j] = operand;
					}
				} else
					cur_ins.operands[j] = operand;
			}
		}

		num_registers = other.num_registers;
	}
}

SLAKE_API RegularFnOverloadingObject::~RegularFnOverloadingObject() {
}

SLAKE_API Object *slake::RegularFnOverloadingObject::duplicate(Duplicator *duplicator) const {
	return alloc(this).get();
}

SLAKE_API HostObjectRef<RegularFnOverloadingObject> slake::RegularFnOverloadingObject::alloc(
	FnObject *fn_object) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = fn_object->associated_runtime->get_cur_gen_alloc();

	std::unique_ptr<RegularFnOverloadingObject, peff::DeallocableDeleter<RegularFnOverloadingObject>> ptr(
		peff::alloc_and_construct<RegularFnOverloadingObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			fn_object,
			cur_generation_allocator.get()));
	if (!ptr)
		return nullptr;

	if (!fn_object->associated_runtime->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<RegularFnOverloadingObject> slake::RegularFnOverloadingObject::alloc(const RegularFnOverloadingObject *other) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = other->fn_object->associated_runtime->get_cur_gen_alloc();

	bool succeeded = true;

	std::unique_ptr<RegularFnOverloadingObject, peff::DeallocableDeleter<RegularFnOverloadingObject>> ptr(
		peff::alloc_and_construct<RegularFnOverloadingObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			*other, cur_generation_allocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associated_runtime->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::RegularFnOverloadingObject::dealloc() {
	peff::destroy_and_release<RegularFnOverloadingObject>(self_allocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API void RegularFnOverloadingObject::replace_allocator(peff::Alloc *allocator) noexcept {
	this->FnOverloadingObject::replace_allocator(allocator);

	source_loc_descs.replace_allocator(allocator);

	instructions.replace_allocator(allocator);

	for (auto &i : instructions) {
		i.replace_allocator(allocator);
	}
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
			sizeof(std::max_align_t),
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
			sizeof(std::max_align_t),
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
	peff::destroy_and_release<NativeFnOverloadingObject>(self_allocator.get(), this, sizeof(std::max_align_t));
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
			sizeof(std::max_align_t),
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
			sizeof(std::max_align_t),
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
	peff::destroy_and_release<FnObject>(self_allocator.get(), this, sizeof(std::max_align_t));
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

	overloadings = peff::Map<FnSignature, FnOverloadingObject *, FnSignatureComparator, true>(self_allocator.get());

	for (auto [k, v] : old_overloadings) {
		if (!overloadings.insert(FnSignature(k), +v))
			return OutOfMemoryError::alloc();
	}

	return {};
}
