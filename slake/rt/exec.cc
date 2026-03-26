#include "../runtime.h"
#include <slake/flib/math/fmod.h>
#include <slake/flib/bitop.h>
#include <slake/flib/cmp.h>
#include <peff/base/scope_guard.h>
#include <cmath>

using namespace slake;

template <bool has_output, size_t num_operands>
[[nodiscard]] static SLAKE_FORCEINLINE InternalExceptionPointer _check_operand_count(
	Runtime *runtime,
	size_t output,
	size_t num_operands_in) noexcept {
	if constexpr (has_output) {
		if ((output == UINT32_MAX) || (num_operands != num_operands_in))
			return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(runtime->get_fixed_alloc()));
	} else {
		if (num_operands != num_operands_in) {
			return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(runtime->get_fixed_alloc()));
		}
	}

	return {};
}

template <ValueType value_type>
[[nodiscard]] static SLAKE_FORCEINLINE InternalExceptionPointer _check_operand_type(
	Runtime *runtime,
	const Value &operand) noexcept {
	if (operand.value_type != value_type)
		return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(runtime->get_fixed_alloc()));
	return {};
}

template <ReferenceKind kind>
[[nodiscard]] static SLAKE_FORCEINLINE InternalExceptionPointer _check_object_ref_operand_type(
	Runtime *runtime,
	const Reference &operand) noexcept {
	if (operand.kind != kind) {
		return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(runtime->get_fixed_alloc()));
	}
	return {};
}

template <ObjectKind type_id>
[[nodiscard]] static SLAKE_FORCEINLINE InternalExceptionPointer _check_object_operand_type(
	Runtime *runtime,
	const Object *const object) noexcept {
	if (object && object->get_object_kind() != type_id) {
		return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(runtime->get_fixed_alloc()));
	}
	return {};
}

#define _is_register_valid(cur_major_frame, index) ((index) < (cur_major_frame)->resumable_context_data.num_regs)

static SLAKE_FORCEINLINE Value *_calc_reg_ptr(
	char *stack_data,
	size_t stack_size,
	const MajorFrame *cur_major_frame,
	uint32_t index) noexcept {
	return &((Value *)calc_stack_addr(stack_data, stack_size, cur_major_frame->off_regs))[index];
}

static SLAKE_FORCEINLINE const Value *_calc_reg_ptr(
	const char *stack_data,
	size_t stack_size,
	const MajorFrame *cur_major_frame,
	uint32_t index) noexcept {
	return &((const Value *)calc_stack_addr(stack_data, stack_size, cur_major_frame->off_regs))[index];
}

[[nodiscard]] static SLAKE_FORCEINLINE InternalExceptionPointer _set_register_value(
	Runtime *runtime,
	char *stack_data,
	const size_t stack_size,
	const MajorFrame *cur_major_frame,
	uint32_t index,
	const Value &value) noexcept {
	if (!_is_register_valid(cur_major_frame, index)) {
		// The register does not present.
		return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(runtime->get_fixed_alloc()));
	}
	Value *val = _calc_reg_ptr(stack_data, stack_size, cur_major_frame, index);
	*_calc_reg_ptr(stack_data, stack_size, cur_major_frame, index) = value;
	return {};
}

[[nodiscard]] static SLAKE_FORCEINLINE InternalExceptionPointer _unwrap_reg_operand(
	Runtime *const runtime,
	const char *stack_data,
	const size_t stack_size,
	const MajorFrame *cur_major_frame,
	const Value &value,
	Value &value_out) noexcept {
	if (value.value_type == ValueType::RegIndex) {
		if (value.as_u32 >= cur_major_frame->resumable_context_data.num_regs) {
			// The register does not present.
			return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(runtime->get_fixed_alloc()));
		}
		value_out = *_calc_reg_ptr(stack_data, stack_size, cur_major_frame, value.as_u32);
		return {};
	}
	value_out = value;
	return {};
}

[[nodiscard]] static SLAKE_FORCEINLINE InternalExceptionPointer _unwrap_reg_operand_into_ptr(
	Runtime *const runtime,
	const char *stack_data,
	const size_t stack_size,
	const MajorFrame *cur_major_frame,
	const Value &value,
	const Value *&value_out) noexcept {
	if (value.value_type == ValueType::RegIndex) {
		if (value.as_u32 >= cur_major_frame->resumable_context_data.num_regs) {
			// The register does not present.
			return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(runtime->get_fixed_alloc()));
		}
		value_out = _calc_reg_ptr(stack_data, stack_size, cur_major_frame, value.as_u32);
		return {};
	}
	value_out = &value;
	return {};
}

template <typename LT>
static void _cast_to_literal_value(bool nullable, const Value &x, Value &value_out) noexcept {
	switch (x.value_type) {
		case ValueType::I8:
			value_out = ((LT)(x.get_i8()));
			break;
		case ValueType::I16:
			value_out = ((LT)(x.get_i16()));
			break;
		case ValueType::I32:
			value_out = ((LT)(x.get_i32()));
			break;
		case ValueType::I64:
			value_out = ((LT)(x.get_i64()));
			break;
		case ValueType::ISize:
			value_out = ((LT)(x.get_isize()));
			break;
		case ValueType::U8:
			value_out = ((LT)(x.get_u8()));
			break;
		case ValueType::U16:
			value_out = ((LT)(x.get_u16()));
			break;
		case ValueType::U32:
			value_out = ((LT)(x.get_u32()));
			break;
		case ValueType::U64:
			value_out = ((LT)(x.get_u64()));
			break;
		case ValueType::USize:
			value_out = ((LT)(x.get_usize()));
			break;
		case ValueType::F32:
			value_out = ((LT)(x.get_f32()));
			break;
		case ValueType::F64:
			value_out = ((LT)(x.get_f64()));
			break;
		case ValueType::Bool:
			value_out = ((LT)(x.get_bool()));
			break;
		case ValueType::Reference:
			if (nullable) {
				if (x.is_null())
					value_out = nullptr;
				else
					std::terminate();
			}
			break;
		default:
			std::terminate();
	}
}

SLAKE_API InternalExceptionPointer Runtime::_fill_args(
	Context *context,
	MajorFrame *new_major_frame,
	const FnOverloadingObject *fn,
	const Value *args,
	uint32_t num_args) {
	if (num_args < fn->param_types.size()) {
		return alloc_out_of_memory_error_if_alloc_failed(InvalidArgumentNumberError::alloc(get_fixed_alloc(), num_args));
	}

	for (size_t i = 0; i < fn->param_types.size(); ++i) {
		TypeRef t = fn->param_types.at(i);
		if (!is_compatible(t, args[i]))
			return MismatchedVarTypeError::alloc(get_fixed_alloc());
	}
	char *p_args = context->aligned_stack_alloc(sizeof(Value) * num_args, alignof(Value));
	if (!p_args)
		return alloc_out_of_memory_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
	size_t off_args = new_major_frame->cur_coroutine ? context->stack_top - new_major_frame->cur_coroutine->off_stack_top : context->stack_top;
	memcpy(p_args, args, sizeof(Value) * num_args);
	new_major_frame->resumable_context_data.off_args = off_args;
	new_major_frame->resumable_context_data.num_args = num_args;

	return {};
}

SLAKE_API AllocaRecord *Runtime::_alloc_alloca_record(Context *context, const MajorFrame *frame, uint32_t output_reg) {
	MinorFrame *mf = _fetch_minor_frame(context, frame, frame->resumable_context_data.off_cur_minor_frame);
	char *p_record;
	if (!(p_record = context->aligned_stack_alloc(sizeof(AllocaRecord), alignof(AllocaRecord))))
		return nullptr;
	AllocaRecord *record = (AllocaRecord *)p_record;
	record->def_reg = output_reg;
	record->off_next = mf->off_alloca_records;
	mf->off_alloca_records = frame->cur_coroutine
							   ? context->stack_top - frame->cur_coroutine->off_stack_top
							   : context->stack_top;
	return record;
}

SLAKE_API MinorFrame *Runtime::_fetch_minor_frame(
	Context *context,
	const MajorFrame *major_frame,
	size_t stack_offset) {
	MinorFrame *mf = (MinorFrame *)_fetch_minor_frame_unchecked(context, major_frame, stack_offset);
	assert(mf->magic == MINOR_FRAME_MAGIC);
	return mf;
}

SLAKE_API MinorFrame *Runtime::_fetch_minor_frame_unchecked(
	Context *context,
	const MajorFrame *major_frame,
	size_t stack_offset) {
	size_t offset;
	if (major_frame->cur_coroutine) {
		offset = stack_offset + major_frame->cur_coroutine->off_stack_top;
	} else {
		offset = stack_offset;
	}

	return (MinorFrame *)calc_stack_addr(context->data_stack,
		context->stack_size,
		offset);
}

SLAKE_API Value *Runtime::_fetch_arg_stack(
	char *data_stack,
	size_t stack_size,
	const MajorFrame *major_frame,
	size_t stack_offset) {
	size_t offset;
	if (major_frame && major_frame->cur_coroutine) {
		offset = stack_offset + major_frame->cur_coroutine->off_stack_top;
	} else {
		offset = stack_offset;
	}

	return (Value *)calc_stack_addr(data_stack,
		stack_size,
		offset);
}

SLAKE_API AllocaRecord *Runtime::_fetch_alloca_record(
	Context *context,
	const MajorFrame *major_frame,
	size_t stack_offset) {
	size_t offset;
	if (major_frame->cur_coroutine) {
		offset = stack_offset + major_frame->cur_coroutine->off_stack_top;
	} else {
		offset = stack_offset;
	}

	return (AllocaRecord *)calc_stack_addr(context->data_stack,
		context->stack_size,
		offset);
}

SLAKE_API MajorFrame *Runtime::_fetch_major_frame(
	Context *context,
	size_t stack_offset) {
	MajorFrame *mf = _fetch_major_frame_unchecked(context, stack_offset);
	assert(mf->magic == MAJOR_FRAME_MAGIC);
	return mf;
}

SLAKE_API MajorFrame *Runtime::_fetch_major_frame_unchecked(
	Context *context,
	size_t stack_offset) {
	return (MajorFrame *)calc_stack_addr(context->data_stack,
		context->stack_size,
		stack_offset);
}

SLAKE_API ExceptHandler *Runtime::_fetch_except_handler(
	Context *context,
	MajorFrame *major_frame,
	size_t stack_offset) {
	size_t offset;
	if (major_frame->cur_coroutine) {
		offset = stack_offset + major_frame->cur_coroutine->off_stack_top;
	} else {
		offset = stack_offset;
	}

	return (ExceptHandler *)calc_stack_addr(context->data_stack,
		context->stack_size,
		offset);
}

SLAKE_API InternalExceptionPointer Runtime::_create_new_coroutine_major_frame(
	Context *context,
	CoroutineObject *coroutine,
	uint32_t return_value_out,
	const Reference *return_struct_ref) noexcept {
	HostRefHolder holder(context->runtime->get_fixed_alloc());

	size_t prev_stack_top = context->stack_top;
	peff::ScopeGuard restore_stack_top_guard([context, prev_stack_top, coroutine]() noexcept {
		context->stack_top = prev_stack_top;
		coroutine->off_stack_top = 0;
	});

	// TODO: Restore resumable context data.

	char *p_major_frame;
	if (!(p_major_frame = context->aligned_stack_alloc(sizeof(MajorFrame), alignof(MajorFrame))))
		return alloc_out_of_memory_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
	peff::construct_at<MajorFrame>((MajorFrame *)p_major_frame, this);
	MajorFrame &new_major_frame = *(MajorFrame *)p_major_frame;
	new_major_frame.off_prev_frame = context->off_cur_major_frame;

	if (coroutine->resumable.has_value()) {
		new_major_frame.resumable_context_data = coroutine->resumable.move();
	} else {
		peff::construct_at<ResumableContextData>(&new_major_frame.resumable_context_data);
	}

	new_major_frame.cur_fn = coroutine->overloading;
	new_major_frame.cur_coroutine = coroutine;

	size_t off_major_frame = context->stack_top;

	if (coroutine->stack_data) {
		// Note: code commented causes stack addressing error, fix them and re-enable them.
		if (!context->align_stack(alignof(std::max_align_t)))
			return alloc_out_of_memory_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
		coroutine->off_stack_top = context->stack_top;
		new_major_frame.off_regs = context->stack_top + coroutine->off_regs;
		char *initial_data = context->aligned_stack_alloc(coroutine->len_stack_data, alignof(std::max_align_t));
		if (!initial_data) {
			return alloc_out_of_memory_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
		}
		memcpy(initial_data, coroutine->stack_data, coroutine->len_stack_data);
		coroutine->release_stack_data();
	} else {
		// Create minor frame.
		if (!context->aligned_stack_alloc(sizeof(MinorFrame), alignof(MinorFrame)))
			return alloc_out_of_memory_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));

		size_t mf_stack_off = context->stack_top;

		MinorFrame *mf = _fetch_minor_frame_unchecked(context, &new_major_frame, context->stack_top);

		peff::construct_at<MinorFrame>(mf);

		if (new_major_frame.cur_coroutine) {
			mf_stack_off -= new_major_frame.cur_coroutine->off_stack_top;
		}

		mf->off_last_minor_frame = new_major_frame.resumable_context_data.off_cur_minor_frame;
		mf->stack_base = new_major_frame.cur_coroutine ? prev_stack_top - new_major_frame.cur_coroutine->off_stack_top : prev_stack_top;
		new_major_frame.resumable_context_data.off_cur_minor_frame = mf_stack_off;

		switch (coroutine->overloading->overloading_kind) {
			case FnOverloadingKind::Regular: {
				RegularFnOverloadingObject *ol = (RegularFnOverloadingObject *)coroutine->overloading;
				new_major_frame.resumable_context_data.num_regs = ol->num_registers;
				Value *regs = (Value *)context->aligned_stack_alloc(sizeof(Value) * ol->num_registers, alignof(Value));
				new_major_frame.off_regs = context->stack_top;
				if (!regs)
					return alloc_out_of_memory_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
				for (size_t i = 0; i < ol->num_registers; ++i)
					regs[i] = Value(ValueType::Undefined);
				break;
			}
			default:;
		}
	}

	new_major_frame.return_value_out_reg = return_value_out;
	if (return_struct_ref)
		new_major_frame.return_struct_ref = *return_struct_ref;

	new_major_frame.prev_stack_top = prev_stack_top;

	coroutine->bind_to_context(context, &new_major_frame);

	restore_stack_top_guard.release();

	if (context->off_cur_major_frame != SIZE_MAX) {
		MajorFrame *pmf = _fetch_major_frame(context, context->off_cur_major_frame);
		assert(context->off_cur_major_frame != off_major_frame);
		pmf->off_next_frame = off_major_frame;
	}
	context->off_cur_major_frame = off_major_frame;
	++context->num_major_frames;
	return {};
}

SLAKE_API InternalExceptionPointer slake::Runtime::_create_new_major_frame(
	ContextObject *context_object,
	Object *this_object,
	const FnOverloadingObject *fn,
	const Value *args,
	size_t off_args,
	uint32_t num_args,
	uint32_t return_value_out,
	const Reference *return_struct_ref) noexcept {
	Context *const context = &context_object->_context;

	size_t prev_stack_top = context->stack_top;
	peff::ScopeGuard restore_stack_top_guard([context, prev_stack_top]() noexcept {
		context->stack_top = prev_stack_top;
	});

	// TODO: Restore resumable context data.

	char *p_major_frame;
	if (!(p_major_frame = context->aligned_stack_alloc(sizeof(MajorFrame), alignof(MajorFrame))))
		return alloc_out_of_memory_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
	peff::construct_at<MajorFrame>((MajorFrame *)p_major_frame, this);
	MajorFrame &new_major_frame = *(MajorFrame *)p_major_frame;
	new_major_frame.off_prev_frame = context->off_cur_major_frame;

	new_major_frame.cur_context = context_object;
	peff::construct_at<ResumableContextData>(&new_major_frame.resumable_context_data);

	size_t off_major_frame = context->stack_top;

	// Create minor frame.
	if (!context->aligned_stack_alloc(sizeof(MinorFrame), alignof(MinorFrame)))
		return alloc_out_of_memory_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));

	size_t mf_stack_off = context->stack_top;

	MinorFrame *mf = _fetch_minor_frame_unchecked(context, &new_major_frame, context->stack_top);

	peff::construct_at<MinorFrame>(mf);

	mf->off_last_minor_frame = new_major_frame.resumable_context_data.off_cur_minor_frame;
	mf->stack_base = prev_stack_top;
	new_major_frame.resumable_context_data.off_cur_minor_frame = mf_stack_off;

	if (!fn) {
		// Used in the creation of top major frame.
		new_major_frame.cur_fn = nullptr;
		new_major_frame.resumable_context_data.num_regs = 1;
		Value *regs = (Value *)context->aligned_stack_alloc(sizeof(Value) * 1, alignof(Value));
		new_major_frame.off_regs = context->stack_top;
		*regs = Value(ValueType::Undefined);
	} else {
		new_major_frame.cur_fn = fn;
		new_major_frame.resumable_context_data.this_object = this_object;

		if (args)
			SLAKE_RETURN_IF_EXCEPT(_fill_args(context, &new_major_frame, fn, args, num_args));
		else {
			if (num_args)
				new_major_frame.resumable_context_data.off_args = off_args;
			new_major_frame.resumable_context_data.num_args = num_args;
		}

		switch (fn->overloading_kind) {
			case FnOverloadingKind::Regular: {
				RegularFnOverloadingObject *ol = (RegularFnOverloadingObject *)fn;
				new_major_frame.resumable_context_data.num_regs = ol->num_registers;
				Value *regs = (Value *)context->aligned_stack_alloc(sizeof(Value) * ol->num_registers, alignof(Value));
				memset(regs, 0, sizeof(Value) * ol->num_registers);
				new_major_frame.off_regs = context->stack_top;
				break;
			}
			default:
				break;
		}
	}

	new_major_frame.return_value_out_reg = return_value_out;
	if (return_struct_ref)
		new_major_frame.return_struct_ref = *return_struct_ref;
	new_major_frame.prev_stack_top = prev_stack_top;

	restore_stack_top_guard.release();

	if (context->off_cur_major_frame != SIZE_MAX) {
		MajorFrame *pmf = _fetch_major_frame(context, context->off_cur_major_frame);
		assert(context->off_cur_major_frame != off_major_frame);
		pmf->off_next_frame = off_major_frame;
		mf->stack_base = pmf->resumable_context_data.off_next_args_begin;
		pmf->resumable_context_data.off_next_args_begin = SIZE_MAX;
	}
	context->off_cur_major_frame = off_major_frame;
	++context->num_major_frames;

	return {};
}

SLAKE_API void Runtime::_leave_major_frame(Context *context) noexcept {
	MajorFrame *mf = _fetch_major_frame(context, context->off_cur_major_frame), *pmf;

	assert(mf->off_next_frame == SIZE_MAX);

	if (mf->off_prev_frame != SIZE_MAX) {
		pmf = _fetch_major_frame(context, mf->off_prev_frame);

		pmf->off_next_frame = SIZE_MAX;
	}

	context->off_cur_major_frame = mf->off_prev_frame;
	assert(mf->prev_stack_top <= context->stack_top);
	context->stack_top = mf->prev_stack_top;
	--context->num_major_frames;

	std::destroy_at<MajorFrame>(mf);
}

SLAKE_FORCEINLINE InternalExceptionPointer slake::Runtime::_add_local_var(Context *context, const MajorFrame *frame, TypeRef type, uint32_t output_reg, Reference &object_ref_out) noexcept {
	size_t original_stack_top = context->stack_top;

	peff::ScopeGuard restore_stack_top_guard([original_stack_top, context]() noexcept {
		context->stack_top = original_stack_top;
	});

	switch (type.type_id) {
		case TypeId::StructInstance: {
			assert(type.get_custom_type_def()->type_object->get_object_kind() == ObjectKind::Struct);
			SLAKE_RETURN_IF_EXCEPT(prepare_struct_for_instantiation((StructObject *)type.get_custom_type_def()->type_object));
			break;
		}
		case TypeId::UnionEnum: {
			assert(type.get_custom_type_def()->type_object->get_object_kind() == ObjectKind::UnionEnum);
			SLAKE_RETURN_IF_EXCEPT(prepare_union_enum_for_instantiation((UnionEnumObject *)type.get_custom_type_def()->type_object));
			break;
		}
		case TypeId::UnionEnumItem: {
			assert(type.get_custom_type_def()->type_object->get_object_kind() == ObjectKind::UnionEnumItem);
			SLAKE_RETURN_IF_EXCEPT(prepare_union_enum_item_for_instantiation((UnionEnumItemObject *)type.get_custom_type_def()->type_object));
			break;
		}
		default:
			break;
	}

	size_t size = sizeof_type(type), align = alignof_type(type);

	if (!context->aligned_stack_alloc(size, align))
		return alloc_out_of_memory_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));

	switch (type.type_id) {
		case TypeId::I8:
		case TypeId::I16:
		case TypeId::I32:
		case TypeId::I64:
		case TypeId::U8:
		case TypeId::U16:
		case TypeId::U32:
		case TypeId::U64:
		case TypeId::F32:
		case TypeId::F64:
		case TypeId::Bool:
		case TypeId::String:
		case TypeId::Any:
			if (type.is_nullable())
				context->stack_alloc(sizeof(bool));
			break;
		case TypeId::Instance:
		case TypeId::GenericArg:
		case TypeId::Array:
		case TypeId::Ref: {
			// The data is already aligned, just directly assign to them.
			Object **type_info = (Object **)context->stack_alloc(sizeof(void *));
			if (!type_info)
				return alloc_out_of_memory_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
#ifndef _NDEBUG
			const size_t diff = alignof(void *) - ((uintptr_t)(calc_stack_addr(context->data_stack, context->stack_size, context->stack_top)) % alignof(void *));
			assert((diff == alignof(void *) || (!diff)));
#endif
			*type_info = type.type_def;
			break;
		}
		case TypeId::ScopedEnum: {
			TypeDefObject **type_info = (TypeDefObject **)context->stack_alloc(sizeof(void *));
			if (!type_info)
				return alloc_out_of_memory_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
			memcpy(type_info, &type.type_def, sizeof(void *));
			break;
		}
		case TypeId::TypelessScopedEnum: {
			TypeDefObject **type_info = (TypeDefObject **)context->stack_alloc(sizeof(void *));
			if (!type_info)
				return alloc_out_of_memory_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
			memcpy(type_info, &type.type_def, sizeof(void *));
			break;
		}
		case TypeId::StructInstance:
		case TypeId::UnionEnum:
		case TypeId::UnionEnumItem: {
			TypeDefObject **type_info = (TypeDefObject **)context->stack_alloc(sizeof(void *));
			if (!type_info)
				return alloc_out_of_memory_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
			memcpy(type_info, &type.type_def, sizeof(void *));
			break;
		}
		default:
			std::terminate();
	}

	TypeModifier *type_modifier = (TypeModifier *)context->stack_alloc(sizeof(TypeModifier));
	if (!type_modifier)
		return alloc_out_of_memory_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
	*type_modifier = type.type_modifier;

	TypeId *type_id = (TypeId *)context->stack_alloc(sizeof(TypeId));
	if (!type_id)
		return alloc_out_of_memory_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
	*type_id = type.type_id;

	size_t off_out = context->stack_top;

	if (!_alloc_alloca_record(context, frame, output_reg))
		return alloc_out_of_memory_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));

	restore_stack_top_guard.release();

	if (frame->cur_coroutine)
		object_ref_out = CoroutineLocalVarRef(frame->cur_coroutine, off_out - frame->cur_coroutine->off_stack_top);
	else
		object_ref_out = LocalVarRef(context, off_out);
	return {};
}

SLAKE_FORCEINLINE InternalExceptionPointer larg(Context *context, MajorFrame *major_frame, Runtime *rt, uint32_t off, Reference &object_ref_out) {
	if (off >= major_frame->resumable_context_data.num_args) {
		return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(rt->get_fixed_alloc()));
	}

	if (major_frame->cur_coroutine) {
		object_ref_out = CoroutineArgRef(major_frame->cur_coroutine, off);
	} else {
		object_ref_out = ArgRef(major_frame, off);
	}
	return {};
}

InternalExceptionPointer Runtime::_exec_ins(ContextObject *const context, MajorFrame *const cur_major_frame, const Opcode opcode, const size_t output, const size_t num_operands, const Value *const operands, bool &is_context_changed_out) noexcept {
	InternalExceptionPointer except_ptr;
	char *const data_stack = context->_context.data_stack;
	const size_t stack_size = context->_context.stack_size;

	switch (opcode) {
		case Opcode::LVALUE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 1>(this, output, num_operands));

			const Value *dest;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], dest));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::Reference>(this, *dest));

			if ((!_is_register_valid(cur_major_frame, output)) || (dest->is_invalid()))
				// The register does not present.
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			read_var(dest->get_reference(), *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output));

			break;
		}
		case Opcode::LARGV: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 1>(this, output, num_operands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::U32>(this, operands[0]));

			Reference ref;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, larg(&context->get_context(), cur_major_frame, this, operands[0].get_u32(), ref));

			if (!_is_register_valid(cur_major_frame, output))
				// The register does not present.
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			read_var(ref, *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output));

			break;
		}
		case Opcode::STORE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<false, 2>(this, output, num_operands));

			const Value *dest_value;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], dest_value));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::Reference>(this, *dest_value));

			const Value *data;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[1], data));

			const Reference &ref = dest_value->get_reference();
			TypeRef t = typeof_var(ref);
			if (!is_compatible(t, *data))
				return MismatchedVarTypeError::alloc(get_fixed_alloc());
			write_var_with_type(ref, t, *data);
			break;
		}
		case Opcode::JMP: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<false, 1>(this, output, num_operands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::U32>(this, operands[0]));

			cur_major_frame->resumable_context_data.last_jump_src = cur_major_frame->resumable_context_data.cur_ins;
			cur_major_frame->resumable_context_data.cur_ins = operands[0].get_u32();
			return {};
		}
		case Opcode::BR: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<false, 3>(this, output, num_operands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::U32>(this, operands[1]));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::U32>(this, operands[2]));
			const Value *condition;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], condition));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::Bool>(this, *condition));

			cur_major_frame->resumable_context_data.last_jump_src = cur_major_frame->resumable_context_data.cur_ins;
			cur_major_frame->resumable_context_data.cur_ins = operands[((uint8_t)!condition->get_bool()) + 1].get_u32();
			return {};
		}
		case Opcode::ADD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 2>(this, output, num_operands));

			if (!_is_register_valid(cur_major_frame, output)) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			Value &value_out = *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[1], y));
			if (x->value_type != y->value_type) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			switch (x->value_type) {
				case ValueType::I8:
					value_out = (int8_t)(x->get_i8() + y->get_i8());
					break;
				case ValueType::I16:
					value_out = (int16_t)(x->get_i16() + y->get_i16());
					break;
				case ValueType::I32:
					value_out = (int32_t)(x->get_i32() + y->get_i32());
					break;
				case ValueType::I64:
					value_out = (int64_t)(x->get_i64() + y->get_i64());
					break;
				case ValueType::U8:
					value_out = (uint8_t)(x->get_u8() + y->get_u8());
					break;
				case ValueType::U16:
					value_out = (uint16_t)(x->get_u16() + y->get_u16());
					break;
				case ValueType::U32:
					value_out = (uint32_t)(x->get_u32() + y->get_u32());
					break;
				case ValueType::U64:
					value_out = (uint64_t)(x->get_u64() + y->get_u64());
					break;
				case ValueType::F32:
					value_out = (float)(x->get_f32() + y->get_f32());
					break;
				case ValueType::F64:
					value_out = (double)(x->get_f64() + y->get_f64());
					break;
				default:
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			break;
		}
		case Opcode::SUB: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 2>(this, output, num_operands));

			if (!_is_register_valid(cur_major_frame, output)) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			Value &value_out = *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[1], y));
			if (x->value_type != y->value_type) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			switch (x->value_type) {
				case ValueType::I8:
					value_out = (int8_t)(x->get_i8() - y->get_i8());
					break;
				case ValueType::I16:
					value_out = (int16_t)(x->get_i16() - y->get_i16());
					break;
				case ValueType::I32:
					value_out = (int32_t)(x->get_i32() - y->get_i32());
					break;
				case ValueType::I64:
					value_out = (int64_t)(x->get_i64() - y->get_i64());
					break;
				case ValueType::U8:
					value_out = (uint8_t)(x->get_u8() - y->get_u8());
					break;
				case ValueType::U16:
					value_out = (uint16_t)(x->get_u16() - y->get_u16());
					break;
				case ValueType::U32:
					value_out = (uint32_t)(x->get_u32() - y->get_u32());
					break;
				case ValueType::U64:
					value_out = (uint64_t)(x->get_u64() - y->get_u64());
					break;
				case ValueType::F32:
					value_out = (float)(x->get_f32() - y->get_f32());
					break;
				case ValueType::F64:
					value_out = (double)(x->get_f64() - y->get_f64());
					break;
				default:
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			break;
		}
		case Opcode::MUL: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 2>(this, output, num_operands));

			if (!_is_register_valid(cur_major_frame, output)) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			Value &value_out = *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[1], y));
			if (x->value_type != y->value_type) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			switch (x->value_type) {
				case ValueType::I8:
					value_out = (int8_t)(x->get_i8() * y->get_i8());
					break;
				case ValueType::I16:
					value_out = (int16_t)(x->get_i16() * y->get_i16());
					break;
				case ValueType::I32:
					value_out = (int32_t)(x->get_i32() * y->get_i32());
					break;
				case ValueType::I64:
					value_out = (int64_t)(x->get_i64() * y->get_i64());
					break;
				case ValueType::U8:
					value_out = (uint8_t)(x->get_u8() * y->get_u8());
					break;
				case ValueType::U16:
					value_out = (uint16_t)(x->get_u16() * y->get_u16());
					break;
				case ValueType::U32:
					value_out = (uint32_t)(x->get_u32() * y->get_u32());
					break;
				case ValueType::U64:
					value_out = (uint64_t)(x->get_u64() * y->get_u64());
					break;
				case ValueType::F32:
					value_out = (float)(x->get_f32() * y->get_f32());
					break;
				case ValueType::F64:
					value_out = (double)(x->get_f64() * y->get_f64());
					break;
				default:
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			break;
		}
		case Opcode::DIV: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 2>(this, output, num_operands));

			if (!_is_register_valid(cur_major_frame, output)) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			Value &value_out = *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[1], y));
			if (x->value_type != y->value_type) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			switch (x->value_type) {
				case ValueType::I8:
					value_out = (int8_t)(x->get_i8() / y->get_i8());
					break;
				case ValueType::I16:
					value_out = (int16_t)(x->get_i16() / y->get_i16());
					break;
				case ValueType::I32:
					value_out = (int32_t)(x->get_i32() / y->get_i32());
					break;
				case ValueType::I64:
					value_out = (int64_t)(x->get_i64() / y->get_i64());
					break;
				case ValueType::U8:
					value_out = (uint8_t)(x->get_u8() / y->get_u8());
					break;
				case ValueType::U16:
					value_out = (uint16_t)(x->get_u16() / y->get_u16());
					break;
				case ValueType::U32:
					value_out = (uint32_t)(x->get_u32() / y->get_u32());
					break;
				case ValueType::U64:
					value_out = (uint64_t)(x->get_u64() / y->get_u64());
					break;
				case ValueType::F32:
					value_out = (float)(x->get_f32() / y->get_f32());
					break;
				case ValueType::F64:
					value_out = (double)(x->get_f64() / y->get_f64());
					break;
				default:
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			break;
		}
		case Opcode::MOD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 2>(this, output, num_operands));

			if (!_is_register_valid(cur_major_frame, output)) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			Value &value_out = *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[1], y));
			if (x->value_type != y->value_type) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			switch (x->value_type) {
				case ValueType::I8:
					value_out = (int8_t)(x->get_i8() % y->get_i8());
					break;
				case ValueType::I16:
					value_out = (int16_t)(x->get_i16() % y->get_i16());
					break;
				case ValueType::I32:
					value_out = (int32_t)(x->get_i32() % y->get_i32());
					break;
				case ValueType::I64:
					value_out = (int64_t)(x->get_i64() % y->get_i64());
					break;
				case ValueType::U8:
					value_out = (uint8_t)(x->get_u8() % y->get_u8());
					break;
				case ValueType::U16:
					value_out = (uint16_t)(x->get_u16() % y->get_u16());
					break;
				case ValueType::U32:
					value_out = (uint32_t)(x->get_u32() % y->get_u32());
					break;
				case ValueType::U64:
					value_out = (uint64_t)(x->get_u64() % y->get_u64());
					break;
				case ValueType::F32:
					value_out = (float)flib::fmodf(x->get_f32(), y->get_f32());
					break;
				case ValueType::F64:
					value_out = (double)flib::fmod(x->get_f64(), y->get_f64());
					break;
				default:
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			break;
		}
		case Opcode::AND: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 2>(this, output, num_operands));

			if (!_is_register_valid(cur_major_frame, output)) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			Value &value_out = *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[1], y));
			if (x->value_type != y->value_type) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			switch (x->value_type) {
				case ValueType::I8:
					value_out = (int8_t)(x->get_i8() & y->get_i8());
					break;
				case ValueType::I16:
					value_out = (int16_t)(x->get_i16() & y->get_i16());
					break;
				case ValueType::I32:
					value_out = (int32_t)(x->get_i32() & y->get_i32());
					break;
				case ValueType::I64:
					value_out = (int64_t)(x->get_i64() & y->get_i64());
					break;
				case ValueType::U8:
					value_out = (uint8_t)(x->get_u8() & y->get_u8());
					break;
				case ValueType::U16:
					value_out = (uint16_t)(x->get_u16() & y->get_u16());
					break;
				case ValueType::U32:
					value_out = (uint32_t)(x->get_u32() & y->get_u32());
					break;
				case ValueType::U64:
					value_out = (uint64_t)(x->get_u64() & y->get_u64());
					break;
				default:
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			break;
		}
		case Opcode::OR: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 2>(this, output, num_operands));

			if (!_is_register_valid(cur_major_frame, output)) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			Value &value_out = *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[1], y));
			if (x->value_type != y->value_type) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			switch (x->value_type) {
				case ValueType::I8:
					value_out = (int8_t)(x->get_i8() | y->get_i8());
					break;
				case ValueType::I16:
					value_out = (int16_t)(x->get_i16() | y->get_i16());
					break;
				case ValueType::I32:
					value_out = (int32_t)(x->get_i32() | y->get_i32());
					break;
				case ValueType::I64:
					value_out = (int64_t)(x->get_i64() | y->get_i64());
					break;
				case ValueType::U8:
					value_out = (uint8_t)(x->get_u8() | y->get_u8());
					break;
				case ValueType::U16:
					value_out = (uint16_t)(x->get_u16() | y->get_u16());
					break;
				case ValueType::U32:
					value_out = (uint32_t)(x->get_u32() | y->get_u32());
					break;
				case ValueType::U64:
					value_out = (uint64_t)(x->get_u64() | y->get_u64());
					break;
				default:
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			break;
		}
		case Opcode::XOR: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 2>(this, output, num_operands));

			if (!_is_register_valid(cur_major_frame, output)) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			Value &value_out = *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[1], y));
			if (x->value_type != y->value_type) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			switch (x->value_type) {
				case ValueType::I8:
					value_out = (int8_t)(x->get_i8() ^ y->get_i8());
					break;
				case ValueType::I16:
					value_out = (int16_t)(x->get_i16() ^ y->get_i16());
					break;
				case ValueType::I32:
					value_out = (int32_t)(x->get_i32() ^ y->get_i32());
					break;
				case ValueType::I64:
					value_out = (int64_t)(x->get_i64() ^ y->get_i64());
					break;
				case ValueType::U8:
					value_out = (uint8_t)(x->get_u8() ^ y->get_u8());
					break;
				case ValueType::U16:
					value_out = (uint16_t)(x->get_u16() ^ y->get_u16());
					break;
				case ValueType::U32:
					value_out = (uint32_t)(x->get_u32() ^ y->get_u32());
					break;
				case ValueType::U64:
					value_out = (uint64_t)(x->get_u64() ^ y->get_u64());
					break;
				default:
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			break;
		}
		case Opcode::LAND: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 2>(this, output, num_operands));

			if (!_is_register_valid(cur_major_frame, output)) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			Value &value_out = *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[1], y));
			if (x->value_type != y->value_type) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			switch (x->value_type) {
				case ValueType::Bool:
					value_out = Value((int16_t)(x->get_bool() && y->get_bool()));
					break;
				default:
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			break;
		}
		case Opcode::LOR: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 2>(this, output, num_operands));

			if (!_is_register_valid(cur_major_frame, output)) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			Value &value_out = *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[1], y));
			if (x->value_type != y->value_type) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			switch (x->value_type) {
				case ValueType::Bool:
					value_out = Value((int16_t)(x->get_bool() || y->get_bool()));
					break;
				default:
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			break;
		}
		case Opcode::EQ: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 2>(this, output, num_operands));

			if (!_is_register_valid(cur_major_frame, output)) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			Value &value_out = *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[1], y));
			if (x->value_type != y->value_type) {
				if ((!x->is_null()) || (!y->is_null()))
					value_out = false;
				else
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			} else {
				switch (x->value_type) {
					case ValueType::I8:
						value_out = (bool)(x->get_i8() == y->get_i8());
						break;
					case ValueType::I16:
						value_out = (bool)(x->get_i16() == y->get_i16());
						break;
					case ValueType::I32:
						value_out = (bool)(x->get_i32() == y->get_i32());
						break;
					case ValueType::I64:
						value_out = (bool)(x->get_i64() == y->get_i64());
						break;
					case ValueType::U8:
						value_out = (bool)(x->get_u8() == y->get_u8());
						break;
					case ValueType::U16:
						value_out = (bool)(x->get_u16() == y->get_u16());
						break;
					case ValueType::U32:
						value_out = (bool)(x->get_u32() == y->get_u32());
						break;
					case ValueType::U64:
						value_out = (bool)(x->get_u64() == y->get_u64());
						break;
					case ValueType::F32:
						value_out = (bool)(x->get_f32() == y->get_f32());
						break;
					case ValueType::F64:
						value_out = (bool)(x->get_f64() == y->get_f64());
						break;
					case ValueType::Bool:
						value_out = (bool)(x->get_bool() == y->get_bool());
						break;
					default:
						return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
				}
			}
			break;
		}
		case Opcode::NEQ: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 2>(this, output, num_operands));

			if (!_is_register_valid(cur_major_frame, output)) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			Value &value_out = *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[1], y));
			if (x->value_type != y->value_type) {
				if ((!x->is_null()) || (!y->is_null()))
					value_out = true;
				else
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			} else {
				switch (x->value_type) {
					case ValueType::I8:
						value_out = (bool)(x->get_i8() != y->get_i8());
						break;
					case ValueType::I16:
						value_out = (bool)(x->get_i16() != y->get_i16());
						break;
					case ValueType::I32:
						value_out = (bool)(x->get_i32() != y->get_i32());
						break;
					case ValueType::I64:
						value_out = (bool)(x->get_i64() != y->get_i64());
						break;
					case ValueType::U8:
						value_out = (bool)(x->get_u8() != y->get_u8());
						break;
					case ValueType::U16:
						value_out = (bool)(x->get_u16() != y->get_u16());
						break;
					case ValueType::U32:
						value_out = (bool)(x->get_u32() != y->get_u32());
						break;
					case ValueType::U64:
						value_out = (bool)(x->get_u64() != y->get_u64());
						break;
					case ValueType::F32:
						value_out = (bool)(x->get_f32() != y->get_f32());
						break;
					case ValueType::F64:
						value_out = (bool)(x->get_f64() != y->get_f64());
						break;
					case ValueType::Bool:
						value_out = (bool)(x->get_bool() != y->get_bool());
						break;
					default:
						return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
				}
			}
			break;
		}
		case Opcode::LT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 2>(this, output, num_operands));

			if (!_is_register_valid(cur_major_frame, output)) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			Value &value_out = *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[1], y));
			if (x->value_type != y->value_type) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			switch (x->value_type) {
				case ValueType::I8:
					value_out = (bool)(x->get_i8() < y->get_i8());
					break;
				case ValueType::I16:
					value_out = (bool)(x->get_i16() < y->get_i16());
					break;
				case ValueType::I32:
					value_out = (bool)(x->get_i32() < y->get_i32());
					break;
				case ValueType::I64:
					value_out = (bool)(x->get_i64() < y->get_i64());
					break;
				case ValueType::U8:
					value_out = (bool)(x->get_u8() < y->get_u8());
					break;
				case ValueType::U16:
					value_out = (bool)(x->get_u16() < y->get_u16());
					break;
				case ValueType::U32:
					value_out = (bool)(x->get_u32() < y->get_u32());
					break;
				case ValueType::U64:
					value_out = (bool)(x->get_u64() < y->get_u64());
					break;
				case ValueType::F32:
					value_out = (bool)(x->get_f32() < y->get_f32());
					break;
				case ValueType::F64:
					value_out = (bool)(x->get_f64() < y->get_f64());
					break;
				default:
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			break;
		}
		case Opcode::GT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 2>(this, output, num_operands));

			if (!_is_register_valid(cur_major_frame, output)) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			Value &value_out = *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[1], y));
			if (x->value_type != y->value_type) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			switch (x->value_type) {
				case ValueType::I8:
					value_out = (bool)(x->get_i8() > y->get_i8());
					break;
				case ValueType::I16:
					value_out = (bool)(x->get_i16() > y->get_i16());
					break;
				case ValueType::I32:
					value_out = (bool)(x->get_i32() > y->get_i32());
					break;
				case ValueType::I64:
					value_out = (bool)(x->get_i64() > y->get_i64());
					break;
				case ValueType::U8:
					value_out = (bool)(x->get_u8() > y->get_u8());
					break;
				case ValueType::U16:
					value_out = (bool)(x->get_u16() > y->get_u16());
					break;
				case ValueType::U32:
					value_out = (bool)(x->get_u32() > y->get_u32());
					break;
				case ValueType::U64:
					value_out = (bool)(x->get_u64() > y->get_u64());
					break;
				case ValueType::F32:
					value_out = (bool)(x->get_f32() > y->get_f32());
					break;
				case ValueType::F64:
					value_out = (bool)(x->get_f64() > y->get_f64());
					break;
				default:
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			break;
		}
		case Opcode::LTEQ: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 2>(this, output, num_operands));

			if (!_is_register_valid(cur_major_frame, output)) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			Value &value_out = *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[1], y));
			if (x->value_type != y->value_type) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			switch (x->value_type) {
				case ValueType::I8:
					value_out = (bool)(x->get_i8() <= y->get_i8());
					break;
				case ValueType::I16:
					value_out = (bool)(x->get_i16() <= y->get_i16());
					break;
				case ValueType::I32:
					value_out = (bool)(x->get_i32() <= y->get_i32());
					break;
				case ValueType::I64:
					value_out = (bool)(x->get_i64() <= y->get_i64());
					break;
				case ValueType::U8:
					value_out = (bool)(x->get_u8() <= y->get_u8());
					break;
				case ValueType::U16:
					value_out = (bool)(x->get_u16() <= y->get_u16());
					break;
				case ValueType::U32:
					value_out = (bool)(x->get_u32() <= y->get_u32());
					break;
				case ValueType::U64:
					value_out = (bool)(x->get_u64() <= y->get_u64());
					break;
				case ValueType::F32:
					value_out = (bool)(x->get_f32() <= y->get_f32());
					break;
				case ValueType::F64:
					value_out = (bool)(x->get_f64() <= y->get_f64());
					break;
				default:
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			break;
		}
		case Opcode::GTEQ: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 2>(this, output, num_operands));

			if (!_is_register_valid(cur_major_frame, output)) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			Value &value_out = *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[1], y));
			if (x->value_type != y->value_type) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			switch (x->value_type) {
				case ValueType::I8:
					value_out = (bool)(x->get_i8() >= y->get_i8());
					break;
				case ValueType::I16:
					value_out = (bool)(x->get_i16() >= y->get_i16());
					break;
				case ValueType::I32:
					value_out = (bool)(x->get_i32() >= y->get_i32());
					break;
				case ValueType::I64:
					value_out = (bool)(x->get_i64() >= y->get_i64());
					break;
				case ValueType::U8:
					value_out = (bool)(x->get_u8() >= y->get_u8());
					break;
				case ValueType::U16:
					value_out = (bool)(x->get_u16() >= y->get_u16());
					break;
				case ValueType::U32:
					value_out = (bool)(x->get_u32() >= y->get_u32());
					break;
				case ValueType::U64:
					value_out = (bool)(x->get_u64() >= y->get_u64());
					break;
				case ValueType::F32:
					value_out = (bool)(x->get_f32() >= y->get_f32());
					break;
				case ValueType::F64:
					value_out = (bool)(x->get_f64() >= y->get_f64());
					break;
				default:
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			uint64_t lhs = x->get_u64(), rhs = y->get_u64();
			if (lhs > rhs) {
				value_out = Value((int32_t)1);
			} else if (lhs < rhs) {
				value_out = Value((int32_t)-1);
			} else
				value_out = Value((int32_t)0);
			break;
		}
		case Opcode::CMP: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 2>(this, output, num_operands));

			if (!_is_register_valid(cur_major_frame, output)) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			Value &value_out = *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[1], y));
			if (x->value_type != y->value_type) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			switch (x->value_type) {
				case ValueType::I8:
					value_out = (int32_t)flib::compare_i8(x->get_i8(), y->get_i8());
					break;
				case ValueType::I16:
					value_out = (int32_t)flib::compare_i16(x->get_i16(), y->get_i16());
					break;
				case ValueType::I32:
					value_out = (int32_t)flib::compare_i32(x->get_i32(), y->get_i32());
					break;
				case ValueType::I64:
					value_out = (int32_t)flib::compare_i64(x->get_i64(), y->get_i64());
					break;
				case ValueType::U8:
					value_out = (int32_t)flib::compare_u8(x->get_u8(), y->get_u8());
					break;
				case ValueType::U16:
					value_out = (int32_t)flib::compare_u16(x->get_u16(), y->get_u16());
					break;
				case ValueType::U32:
					value_out = (int32_t)flib::compare_u32(x->get_u32(), y->get_u32());
					break;
				case ValueType::U64:
					value_out = (int32_t)flib::compare_u64(x->get_u64(), y->get_u64());
					break;
				case ValueType::F32:
					value_out = (int32_t)flib::compare_f32(x->get_f32(), y->get_f32());
					break;
				case ValueType::F64:
					value_out = (int32_t)flib::compare_f64(x->get_f64(), y->get_f64());
					break;
				default:
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			break;
		}
		case Opcode::LSH: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 2>(this, output, num_operands));

			if (!_is_register_valid(cur_major_frame, output)) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			Value &value_out = *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[1], y));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::U32>(this, *y));

			uint32_t rhs = y->get_u32();

			switch (x->value_type) {
				case ValueType::I8:
					value_out = flib::shl_signed8(x->get_i8(), rhs);
					break;
				case ValueType::I16:
					value_out = flib::shl_signed16(x->get_i16(), rhs);
					break;
				case ValueType::I32:
					value_out = flib::shl_signed32(x->get_i32(), rhs);
					break;
				case ValueType::I64:
					value_out = flib::shl_signed64(x->get_i64(), rhs);
					break;
				case ValueType::U8:
					value_out = flib::shl_unsigned8(x->get_u8(), rhs);
					break;
				case ValueType::U16:
					value_out = flib::shl_unsigned16(x->get_u16(), rhs);
					break;
				case ValueType::U32:
					value_out = flib::shl_unsigned32(x->get_u32(), rhs);
					break;
				case ValueType::U64:
					value_out = flib::shl_unsigned64(x->get_u64(), rhs);
					break;
				default:
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			break;
		}
		case Opcode::RSH: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 2>(this, output, num_operands));

			if (!_is_register_valid(cur_major_frame, output)) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			Value &value_out = *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[1], y));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::U32>(this, *y));

			uint32_t rhs = y->get_u32();

			switch (x->value_type) {
				case ValueType::I8:
					value_out = flib::shr_signed8(x->get_i8(), rhs);
					break;
				case ValueType::I16:
					value_out = flib::shr_signed16(x->get_i16(), rhs);
					break;
				case ValueType::I32:
					value_out = flib::shr_signed32(x->get_i32(), rhs);
					break;
				case ValueType::I64:
					value_out = flib::shr_signed64(x->get_i64(), rhs);
					break;
				case ValueType::U8:
					value_out = flib::shr_unsigned8(x->get_u8(), rhs);
					break;
				case ValueType::U16:
					value_out = flib::shr_unsigned16(x->get_u16(), rhs);
					break;
				case ValueType::U32:
					value_out = flib::shr_unsigned32(x->get_u32(), rhs);
					break;
				case ValueType::U64:
					value_out = flib::shr_unsigned64(x->get_u64(), rhs);
					break;
				default:
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			break;
		}
		case Opcode::NOT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 1>(this, output, num_operands));

			if (!_is_register_valid(cur_major_frame, output)) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			Value &value_out = *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);

			const Value *x;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[1], x));
			switch (x->value_type) {
				case ValueType::I8:
					value_out = (int8_t)(~x->get_i8());
					break;
				case ValueType::I16:
					value_out = (int16_t)(~x->get_i16());
					break;
				case ValueType::I32:
					value_out = (int32_t)(~x->get_i32());
					break;
				case ValueType::I64:
					value_out = (int64_t)(~x->get_i64());
					break;
				case ValueType::U8:
					value_out = (uint8_t)(~x->get_u8());
					break;
				case ValueType::U16:
					value_out = (uint16_t)(~x->get_u16());
					break;
				case ValueType::U32:
					value_out = (uint32_t)(~x->get_u32());
					break;
				case ValueType::U64:
					value_out = (uint64_t)(~x->get_u64());
					break;
				default:
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			break;
		}
		case Opcode::LNOT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 1>(this, output, num_operands));

			if (!_is_register_valid(cur_major_frame, output)) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			Value &value_out = *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);

			const Value *x;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[1], x));
			switch (x->value_type) {
				case ValueType::I8:
					value_out = (bool)(!x->get_i8());
					break;
				case ValueType::I16:
					value_out = (bool)(!x->get_i16());
					break;
				case ValueType::I32:
					value_out = (bool)(!x->get_i32());
					break;
				case ValueType::I64:
					value_out = (bool)(!x->get_i64());
					break;
				case ValueType::U8:
					value_out = (bool)(!x->get_i8());
					break;
				case ValueType::U16:
					value_out = (bool)(!x->get_u16());
					break;
				case ValueType::U32:
					value_out = (bool)(!x->get_u32());
					break;
				case ValueType::U64:
					value_out = (bool)(!x->get_u64());
					break;
				case ValueType::F32:
					value_out = (bool)(!x->get_f32());
					break;
				case ValueType::F64:
					value_out = (bool)(!x->get_f64());
					break;
				case ValueType::Bool:
					value_out = (bool)(!x->get_u64());
					break;
				default:
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			break;
		}
		case Opcode::NEG: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 1>(this, output, num_operands));

			if (!_is_register_valid(cur_major_frame, output)) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			Value &value_out = *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);

			const Value *x;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[1], x));
			switch (x->value_type) {
				case ValueType::I8:
					value_out = (int8_t)(-x->get_i8());
					break;
				case ValueType::I16:
					value_out = (int16_t)(-x->get_i16());
					break;
				case ValueType::I32:
					value_out = (int32_t)(-x->get_i32());
					break;
				case ValueType::I64:
					value_out = (int64_t)(-x->get_i64());
					break;
				case ValueType::U8:
					value_out = (uint8_t)(x->get_u8());
					break;
				case ValueType::U16:
					value_out = (uint16_t)(x->get_u16());
					break;
				case ValueType::U32:
					value_out = (uint32_t)(x->get_u32());
					break;
				case ValueType::U64:
					value_out = (uint64_t)(x->get_u64());
					break;
				case ValueType::F32:
					value_out = (float)(-x->get_f32());
					break;
				case ValueType::F64:
					value_out = (double)(-x->get_f64());
					break;
				default:
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			break;
		}
		case Opcode::AT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 2>(this, output, num_operands));

			Value array_value;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand(this, data_stack, stack_size, cur_major_frame, operands[0], array_value));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::Reference>(this, array_value));

			Value index;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand(this, data_stack, stack_size, cur_major_frame, operands[1], index));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::U32>(this, index));

			auto array_in = array_value.get_reference();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_object_ref_operand_type<ReferenceKind::ObjectRef>(this, array_in));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_object_operand_type<ObjectKind::Array>(this, array_in.as_object));
			ArrayObject *array_object = (ArrayObject *)array_in.as_object;

			uint32_t index_in = index.get_u32();

			if (index_in > array_object->length) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidArrayIndexError::alloc(get_fixed_alloc(), index_in));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _set_register_value(this, data_stack, stack_size, cur_major_frame, output, Value(Reference(ArrayElementRef(array_object, index_in)))));

			break;
		}
		case Opcode::LOAD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 1>(this, output, num_operands));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::Reference>(this, operands[0]));
			auto ref_ptr = operands[0].get_reference();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_object_ref_operand_type<ReferenceKind::ObjectRef>(this, ref_ptr));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_object_operand_type<ObjectKind::IdRef>(this, ref_ptr.as_object));

			Reference entity_ref;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, resolve_id_ref((IdRefObject *)ref_ptr.as_object, entity_ref));

			if (entity_ref.kind == ReferenceKind::Invalid)
				// TODO: Use a proper one instead.
				return alloc_out_of_memory_error_if_alloc_failed(ReferencedMemberNotFoundError::alloc(get_fixed_alloc(), (IdRefObject *)ref_ptr.as_object));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _set_register_value(this, data_stack, stack_size, cur_major_frame, output, Value(entity_ref)));
			break;
		}
		case Opcode::RLOAD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 2>(this, output, num_operands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::RegIndex>(this, operands[0]));

			Value lhs;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand(this, data_stack, stack_size, cur_major_frame, operands[0], lhs));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::Reference>(this, lhs));

			Value rhs;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand(this, data_stack, stack_size, cur_major_frame, operands[1], rhs));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::Reference>(this, rhs));

			auto &lhs_entity_ref = lhs.get_reference();

			auto &id_ref_entity_ref = rhs.get_reference();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_object_ref_operand_type<ReferenceKind::ObjectRef>(this, id_ref_entity_ref));

			if (!lhs_entity_ref) {
				return alloc_out_of_memory_error_if_alloc_failed(NullRefError::alloc(get_fixed_alloc()));
			}

			if (!id_ref_entity_ref) {
				return alloc_out_of_memory_error_if_alloc_failed(NullRefError::alloc(get_fixed_alloc()));
			}

			if (id_ref_entity_ref.as_object->get_object_kind() != ObjectKind::IdRef) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			IdRefObject *id_ref = (IdRefObject *)id_ref_entity_ref.as_object;

			Reference entity_ref;

			switch (lhs_entity_ref.kind) {
				case ReferenceKind::StaticFieldRef:
				case ReferenceKind::LocalVarRef:
				case ReferenceKind::CoroutineLocalVarRef:
				case ReferenceKind::ObjectFieldRef:
				case ReferenceKind::ArrayElementRef:
				case ReferenceKind::ArgRef:
				case ReferenceKind::CoroutineArgRef: {
					TypeRef type = typeof_var(lhs_entity_ref);

					if (type.type_id == TypeId::StructInstance) {
						StructObject *struct_object = (StructObject *)(((CustomTypeDefObject *)type.type_def)->type_object);
						IdRefEntry &cur_name = id_ref->entries.at(0);

						if (auto it = struct_object->cached_object_layout->field_name_map.find(cur_name.name); it != struct_object->cached_object_layout->field_name_map.end()) {
							entity_ref = lhs_entity_ref;
							((uint8_t &)entity_ref.kind) |= 0x80;
							entity_ref.struct_field_index = it.value();
						} else {
							entity_ref = struct_object->get_member(cur_name.name);

							if (cur_name.generic_args.size()) {
								peff::NullAlloc null_alloc;
								GenericInstantiationContext generic_instantiation_context(&null_alloc, get_fixed_alloc());

								generic_instantiation_context.generic_args = &cur_name.generic_args;
								MemberObject *m;
								SLAKE_RETURN_IF_EXCEPT(instantiate_generic_object((MemberObject *)entity_ref.as_object, m, &generic_instantiation_context));
								entity_ref = Reference(m);
							}
						}
					}

					break;
				}
				case ReferenceKind::ObjectRef:
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, resolve_id_ref(id_ref, entity_ref, lhs_entity_ref.as_object));
					break;
				default:
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			if (!entity_ref) {
				std::terminate();
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _set_register_value(this, data_stack, stack_size, cur_major_frame, output, Value(entity_ref)));
			break;
		}
		case Opcode::LCURFN: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 0>(this, output, num_operands));

			if (_is_register_valid(cur_major_frame, output)) {
				Value &output_reg = *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);
				output_reg.value_type = ValueType::Reference;
				output_reg.get_reference().kind = ReferenceKind::ObjectRef;
				output_reg.get_reference().as_object = const_cast<FnOverloadingObject *>(cur_major_frame->cur_fn);
			}
			break;
		}
		case Opcode::COPYI8: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 1>(this, output, num_operands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::I8>(this, operands[0]));

			if (output != UINT32_MAX) {
				if (!_is_register_valid(cur_major_frame, output)) {
					// The register does not present.
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
				}
				*_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output) = operands[0].get_i8();
			}
			break;
		}
		case Opcode::COPYI16: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 1>(this, output, num_operands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::I16>(this, operands[0]));

			if (output != UINT32_MAX) {
				if (!_is_register_valid(cur_major_frame, output)) {
					// The register does not present.
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
				}
				*_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output) = operands[0].get_i16();
			}
			break;
		}
		case Opcode::COPYI32: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 1>(this, output, num_operands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::I32>(this, operands[0]));

			if (output != UINT32_MAX) {
				if (!_is_register_valid(cur_major_frame, output)) {
					// The register does not present.
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
				}
				*_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output) = operands[0].get_i32();
			}
			break;
		}
		case Opcode::COPYI64: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 1>(this, output, num_operands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::I64>(this, operands[0]));

			if (output != UINT32_MAX) {
				if (!_is_register_valid(cur_major_frame, output)) {
					// The register does not present.
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
				}
				*_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output) = operands[0].get_i64();
			}
			break;
		}
		case Opcode::COPYISIZE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 1>(this, output, num_operands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::ISize>(this, operands[0]));

			if (output != UINT32_MAX) {
				if (!_is_register_valid(cur_major_frame, output)) {
					// The register does not present.
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
				}
				Value *const val = _calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);
				val->as_usize = operands[0].get_isize();
				val->value_type = ValueType::ISize;
				val->value_flags = 0;
			}
			break;
		}
		case Opcode::COPYU8: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 1>(this, output, num_operands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::U8>(this, operands[0]));

			if (output != UINT32_MAX) {
				if (!_is_register_valid(cur_major_frame, output)) {
					// The register does not present.
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
				}
				*_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output) = operands[0].get_u8();
			}
			break;
		}
		case Opcode::COPYU16: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 1>(this, output, num_operands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::U16>(this, operands[0]));

			if (output != UINT32_MAX) {
				if (!_is_register_valid(cur_major_frame, output)) {
					// The register does not present.
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
				}
				*_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output) = operands[0].get_u16();
			}
			break;
		}
		case Opcode::COPYU32: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 1>(this, output, num_operands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::U32>(this, operands[0]));

			if (output != UINT32_MAX) {
				if (!_is_register_valid(cur_major_frame, output)) {
					// The register does not present.
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
				}
				*_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output) = operands[0].get_u32();
			}
			break;
		}
		case Opcode::COPYU64: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 1>(this, output, num_operands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::U64>(this, operands[0]));

			if (output != UINT32_MAX) {
				if (!_is_register_valid(cur_major_frame, output)) {
					// The register does not present.
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
				}
				*_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output) = operands[0].get_u64();
			}
			break;
		}
		case Opcode::COPYUSIZE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 1>(this, output, num_operands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::USize>(this, operands[0]));

			if (output != UINT32_MAX) {
				if (!_is_register_valid(cur_major_frame, output)) {
					// The register does not present.
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
				}
				Value *const val = _calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);
				val->as_usize = operands[0].get_usize();
				val->value_type = ValueType::USize;
				val->value_flags = 0;
			}
			break;
		}
		case Opcode::COPYF32: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 1>(this, output, num_operands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::F32>(this, operands[0]));

			if (output != UINT32_MAX) {
				if (!_is_register_valid(cur_major_frame, output)) {
					// The register does not present.
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
				}
				*_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output) = operands[0].get_f32();
			}
			break;
		}
		case Opcode::COPYF64: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 1>(this, output, num_operands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::F64>(this, operands[0]));

			if (output != UINT32_MAX) {
				if (!_is_register_valid(cur_major_frame, output)) {
					// The register does not present.
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
				}
				*_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output) = operands[0].get_f64();
			}
			break;
		}
		case Opcode::COPYBOOL: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 1>(this, output, num_operands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::Bool>(this, operands[0]));

			if (output != UINT32_MAX) {
				if (!_is_register_valid(cur_major_frame, output)) {
					// The register does not present.
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
				}
				*_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output) = operands[0].get_bool();
			}
			break;
		}
		case Opcode::COPYNULL: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 0>(this, output, num_operands));

			if (output != UINT32_MAX) {
				if (!_is_register_valid(cur_major_frame, output)) {
					// The register does not present.
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
				}
				*_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output) = nullptr;
			}
			break;
		}
		case Opcode::COPY: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 1>(this, output, num_operands));

			const Value *value;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], value));

			if (output != UINT32_MAX) {
				if (!_is_register_valid(cur_major_frame, output)) {
					// The register does not present.
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
				}
				Value *output_reg = _calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);
				output_reg->value_flags = value->value_flags;
				switch (value->value_type) {
					case ValueType::I8:
						output_reg->as_i8 = value->get_i8();
						output_reg->value_type = ValueType::I8;
						break;
					case ValueType::I16:
						output_reg->as_i16 = value->get_i16();
						output_reg->value_type = ValueType::I16;
						break;
					case ValueType::I32:
						output_reg->as_i32 = value->get_i32();
						output_reg->value_type = ValueType::I32;
						break;
					case ValueType::I64:
						output_reg->as_i64 = value->get_i64();
						output_reg->value_type = ValueType::I64;
						break;
					case ValueType::ISize:
						output_reg->as_isize = value->get_isize();
						output_reg->value_type = ValueType::ISize;
						break;
					case ValueType::U8:
						output_reg->as_u8 = value->get_u8();
						output_reg->value_type = ValueType::U8;
						break;
					case ValueType::U16:
						output_reg->as_u16 = value->get_u16();
						output_reg->value_type = ValueType::U16;
						break;
					case ValueType::U32:
						output_reg->as_u32 = value->get_u32();
						output_reg->value_type = ValueType::U32;
						break;
					case ValueType::U64:
						output_reg->as_u64 = value->get_u64();
						output_reg->value_type = ValueType::U64;
						break;
					case ValueType::USize:
						output_reg->as_usize = value->get_usize();
						output_reg->value_type = ValueType::USize;
						break;
					case ValueType::F32:
						output_reg->as_f32 = value->get_f32();
						output_reg->value_type = ValueType::F32;
						break;
					case ValueType::F64:
						output_reg->as_f64 = value->get_f64();
						output_reg->value_type = ValueType::F64;
						break;
					case ValueType::Bool:
						output_reg->as_bool = value->get_bool();
						output_reg->value_type = ValueType::Bool;
						break;
					case ValueType::Reference: {
						const Reference &ref = value->get_reference();
						switch (ref.kind) {
							case ReferenceKind::Invalid:
								return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
							case ReferenceKind::LocalVarRef:
								// if (value->as_reference.as_local_var.stack_off >= cur_major_frame->stack_base)
								return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
								break;
							case ReferenceKind::CoroutineLocalVarRef: {
								// if ((value->as_reference.as_coroutine_local_var.stack_off + value->as_reference.as_coroutine_local_var.coroutine->off_stack_top) >=
								//(cur_major_frame->cur_coroutine ? cur_major_frame->stack_base + cur_major_frame->cur_coroutine->off_stack_top : cur_major_frame->stack_base))
								return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
								break;
							}
							default:
								output_reg->as_reference = ref;
								break;
						}
						output_reg->value_type = ValueType::Reference;
						break;
					}
					case ValueType::TypelessScopedEnum:
						output_reg->as_typeless_scoped_enum = value->get_typeless_scoped_enum();
						output_reg->value_type = ValueType::Bool;
						break;
					case ValueType::RegIndex:
						output_reg->as_u32 = value->get_reg_index();
						output_reg->value_type = ValueType::RegIndex;
						break;
					case ValueType::TypeName:
						output_reg->as_u32 = value->get_reg_index();
						output_reg->value_type = ValueType::TypeName;
						break;
					case ValueType::Label:
						output_reg->as_u32 = value->get_label();
						output_reg->value_type = ValueType::Label;
						break;
					default:
						std::terminate();
				}
			}
			break;
		}
		case Opcode::LARG: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 1>(this, output, num_operands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::U32>(this, operands[0]));

			Reference entity_ref;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, larg(&context->get_context(), cur_major_frame, this, operands[0].get_u32(), entity_ref));
			if (!_is_register_valid(cur_major_frame, output)) {
				// The register does not present.
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			Value *output_reg = _calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);
			output_reg->value_type = ValueType::Reference;
			output_reg->get_reference() = entity_ref;
			break;
		}
		case Opcode::LAPARG:
			return InvalidOpcodeError::alloc(get_fixed_alloc(), opcode);
		case Opcode::LVAR: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 1>(this, output, num_operands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::TypeName>(this, operands[0]));

			TypeRef type = operands[0].get_type_name();

			Reference entity_ref;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _add_local_var(&context->_context, cur_major_frame, type, output, entity_ref));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _set_register_value(this, data_stack, stack_size, cur_major_frame, output, entity_ref));
			break;
		}
		case Opcode::ALLOCA:
			return InvalidOpcodeError::alloc(get_fixed_alloc(), opcode);
		case Opcode::ENTER: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<false, 0>(this, output, num_operands));

			size_t prev_stack_top = context->get_context().stack_top;

			if (!context->_context.aligned_stack_alloc(sizeof(MinorFrame), alignof(MinorFrame)))
				return alloc_out_of_memory_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));

			size_t mf_stack_off = context->_context.stack_top;

			if (cur_major_frame->cur_coroutine) {
				mf_stack_off -= cur_major_frame->cur_coroutine->off_stack_top;
			}

			MinorFrame *mf = _fetch_minor_frame_unchecked(&context->get_context(), cur_major_frame, mf_stack_off);

			peff::construct_at<MinorFrame>(mf);

			mf->off_last_minor_frame = cur_major_frame->resumable_context_data.off_cur_minor_frame;
			mf->stack_base = cur_major_frame->cur_coroutine ? prev_stack_top - cur_major_frame->cur_coroutine->off_stack_top : prev_stack_top;
			cur_major_frame->resumable_context_data.off_cur_minor_frame = mf_stack_off;
			break;
		}
		case Opcode::LEAVE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<false, 1>(this, output, num_operands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::U32>(this, operands[0]));
			uint32_t level = operands[0].get_u32();
			for (uint32_t i = 0; i < level; ++i) {
				MinorFrame *mf = _fetch_minor_frame(&context->_context, cur_major_frame, cur_major_frame->resumable_context_data.off_cur_minor_frame);

				if (mf->off_last_minor_frame == SIZE_MAX)
					return alloc_out_of_memory_error_if_alloc_failed(FrameBoundaryExceededError::alloc(get_fixed_alloc()));

				// Invalidate alloca records to prevent the runtime from dangling references.
				size_t off_alloca_record = mf->off_alloca_records;
				while (off_alloca_record != SIZE_MAX) {
					AllocaRecord *ar = _fetch_alloca_record(&context->get_context(), cur_major_frame, off_alloca_record);

					if (!_is_register_valid(cur_major_frame, ar->def_reg)) {
						return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
						// TODO: Use a more proper kind of exception.
					}
					_calc_reg_ptr(data_stack, stack_size, cur_major_frame, ar->def_reg)->value_type = ValueType::Invalid;

					off_alloca_record = ar->off_next;
				}

				size_t off_last_minor_frame = mf->off_last_minor_frame;
				context->_context.stack_top = cur_major_frame->cur_coroutine
												 ? cur_major_frame->cur_coroutine->off_stack_top + mf->stack_base
												 : mf->stack_base;
				cur_major_frame->resumable_context_data.off_cur_minor_frame = off_last_minor_frame;
			}
			break;
		}
		case Opcode::PUSHARG: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<false, 1>(this, output, num_operands));

			const Value *value;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], value));
			/* if (cur_major_frame->resumable_context_data.num_next_args) {
				if (cur_major_frame->resumable_context_data.off_next_args + sizeof(Value) * cur_major_frame->resumable_context_data.num_next_args != context->get_context().stack_top)
					std::terminate();
			}*/
			size_t prev_stack_top = cur_major_frame->cur_coroutine ? context->get_context().stack_top - cur_major_frame->cur_coroutine->off_stack_top : context->get_context().stack_top;
			if (char *p = context->get_context().aligned_stack_alloc(sizeof(Value), alignof(Value)); p) {
				*(Value *)p = *value;
			} else
				return alloc_out_of_memory_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
			const size_t new_off = cur_major_frame->cur_coroutine ? context->get_context().stack_top - cur_major_frame->cur_coroutine->off_stack_top : context->get_context().stack_top;
			if (cur_major_frame->resumable_context_data.num_next_args) {
				if (new_off - cur_major_frame->resumable_context_data.off_next_args != sizeof(Value))
					// TODO: Use a proper one.
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			} else {
				cur_major_frame->resumable_context_data.off_next_args_begin = prev_stack_top;
			}
			cur_major_frame->resumable_context_data.off_next_args = new_off;
			++cur_major_frame->resumable_context_data.num_next_args;
			break;
		}
		case Opcode::PUSHAP:
			std::terminate();
		case Opcode::CTORCALL:
		case Opcode::MCALL:
		case Opcode::CALL: {
			FnOverloadingObject *fn;
			Object *this_object = nullptr;

			switch (opcode) {
				case Opcode::CTORCALL:
				case Opcode::MCALL: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<false, 2>(this, output, num_operands));

					Value fn_value;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand(this, data_stack, stack_size, cur_major_frame, operands[0], fn_value));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::Reference>(this, fn_value));
					const Reference &fn_object_ref = fn_value.get_reference();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_object_ref_operand_type<ReferenceKind::ObjectRef>(this, fn_object_ref));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_object_operand_type<ObjectKind::FnOverloading>(this, fn_object_ref.as_object));
					fn = (FnOverloadingObject *)fn_object_ref.as_object;

					const Value *this_object_value;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[1], this_object_value));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::Reference>(this, *this_object_value));
					const Reference &this_object_ref = this_object_value->get_reference();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_object_ref_operand_type<ReferenceKind::ObjectRef>(this, this_object_ref));
					this_object = this_object_ref.as_object;
					break;
				}
				case Opcode::CALL: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<false, 1>(this, output, num_operands));

					const Value *fn_value;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], fn_value));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::Reference>(this, *fn_value));
					const Reference &fn_object_ref = fn_value->get_reference();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_object_ref_operand_type<ReferenceKind::ObjectRef>(this, fn_object_ref));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_object_operand_type<ObjectKind::FnOverloading>(this, fn_object_ref.as_object));
					fn = (FnOverloadingObject *)fn_object_ref.as_object;
					break;
				}
				default:
					std::terminate();
			}

			if (!fn)
				return alloc_out_of_memory_error_if_alloc_failed(NullRefError::alloc(get_fixed_alloc()));

			ResumableContextData &resumable_context_data = cur_major_frame->resumable_context_data;

			if (fn->return_type.type_id == TypeId::StructInstance) {
				if (output != UINT32_MAX) {
					// TODO: Untested!!!
					Reference alloca_ref;

					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _add_local_var(&context->get_context(), cur_major_frame, fn->return_type, output, alloca_ref));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _set_register_value(this, data_stack, stack_size, cur_major_frame, output, alloca_ref));

					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _create_new_major_frame(
																	context,
																	this_object,
																	fn,
																	nullptr,
																	cur_major_frame->cur_coroutine
																		? resumable_context_data.off_next_args + cur_major_frame->cur_coroutine->off_stack_top
																		: resumable_context_data.off_next_args,
																	cur_major_frame->resumable_context_data.num_next_args,
																	UINT32_MAX,
																	&alloca_ref));
				} else
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _create_new_major_frame(
																	context,
																	this_object,
																	fn,
																	nullptr,
																	cur_major_frame->cur_coroutine
																		? resumable_context_data.off_next_args + cur_major_frame->cur_coroutine->off_stack_top
																		: resumable_context_data.off_next_args,
																	resumable_context_data.num_next_args,
																	output,
																	nullptr));
			} else {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _create_new_major_frame(
																context,
																this_object,
																fn,
																nullptr,
																cur_major_frame->cur_coroutine
																	? resumable_context_data.off_next_args + cur_major_frame->cur_coroutine->off_stack_top
																	: resumable_context_data.off_next_args,
																resumable_context_data.num_next_args,
																output,
																nullptr));
			}

			resumable_context_data.off_next_args = SIZE_MAX;
			resumable_context_data.num_next_args = 0;

			is_context_changed_out = true;
			break;
		}
		case Opcode::RET: {
			const uint32_t return_value_out_reg = cur_major_frame->return_value_out_reg;

			const Value *return_value;

			switch (num_operands) {
				case 0:
					break;
				case 1:
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[0], return_value));
					break;
				default:
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			if (CoroutineObject *co = cur_major_frame->cur_coroutine; co) {
				co->resumable = std::move(cur_major_frame->resumable_context_data);
				// TODO: Implement returning structure.
				if (return_value->value_type != ValueType::Invalid)
					co->final_result = return_value;
				co->set_done();
			}

			TypeRef return_type = cur_major_frame->cur_fn->return_type;

			if ((!return_value) || return_value->value_type == ValueType::Invalid) {
				if (return_type.type_id != TypeId::Void)
					// TODO: Handle this.
					std::terminate();
				if (return_value_out_reg != UINT32_MAX)
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
				_leave_major_frame(&context->get_context());
			} else {
				if (!is_compatible(return_type, *return_value))
					// TODO: Handle this.
					std::terminate();
				_leave_major_frame(&context->get_context());
				if (return_type.type_id == TypeId::StructInstance) {
					write_var(cur_major_frame->return_struct_ref, *return_value);
				} else if (return_value_out_reg != UINT32_MAX) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _set_register_value(this, data_stack, stack_size, _fetch_major_frame(&context->get_context(), cur_major_frame->off_prev_frame), return_value_out_reg, *return_value));
				}
			}

			is_context_changed_out = true;
			return {};
		}
		case Opcode::COCALL:
		case Opcode::COMCALL: {
			// stub
			FnOverloadingObject *fn;
			Object *this_object = nullptr;
			uint32_t return_value_output_reg = UINT32_MAX;

			if (output != UINT32_MAX) {
				return_value_output_reg = output;
			}

			switch (opcode) {
				case Opcode::COMCALL: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<false, 2>(this, output, num_operands));

					Value fn_value;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand(this, data_stack, stack_size, cur_major_frame, operands[0], fn_value));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::Reference>(this, fn_value));
					const Reference &fn_object_ref = fn_value.get_reference();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_object_ref_operand_type<ReferenceKind::ObjectRef>(this, fn_object_ref));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_object_operand_type<ObjectKind::FnOverloading>(this, fn_object_ref.as_object));
					fn = (FnOverloadingObject *)fn_object_ref.as_object;

					Value this_object_value;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand(this, data_stack, stack_size, cur_major_frame, operands[1], this_object_value));
					const Reference &this_object_ref = this_object_value.get_reference();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::Reference>(this, this_object_value));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_object_ref_operand_type<ReferenceKind::ObjectRef>(this, this_object_ref));
					this_object = this_object_ref.as_object;
					break;
				}
				case Opcode::COCALL: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<false, 1>(this, output, num_operands));

					Value fn_value;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand(this, data_stack, stack_size, cur_major_frame, operands[0], fn_value));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::Reference>(this, fn_value));
					const Reference &fn_object_ref = fn_value.get_reference();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_object_ref_operand_type<ReferenceKind::ObjectRef>(this, fn_object_ref));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_object_operand_type<ObjectKind::FnOverloading>(this, fn_object_ref.as_object));
					fn = (FnOverloadingObject *)fn_object_ref.as_object;
					break;
				}
				default:
					std::terminate();
			}

			if (!fn) {
				return alloc_out_of_memory_error_if_alloc_failed(NullRefError::alloc(get_fixed_alloc()));
			}

			HostObjectRef<CoroutineObject> co;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, create_coroutine_instance(
															fn,
															this_object,
															_fetch_arg_stack(context->get_context().data_stack, context->get_context().stack_size, cur_major_frame, cur_major_frame->resumable_context_data.off_next_args),
															cur_major_frame->resumable_context_data.num_next_args,
															co));
			cur_major_frame->resumable_context_data.off_next_args = SIZE_MAX;
			cur_major_frame->resumable_context_data.num_next_args = 0;

			if (return_value_output_reg != UINT32_MAX) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _set_register_value(this, data_stack, stack_size, cur_major_frame, return_value_output_reg, Reference(co.get())));
			}
			break;
		}
		case Opcode::YIELD: {
			if (!cur_major_frame->cur_coroutine) {
				// TODO: Return an exception,
				std::terminate();
			}

			uint32_t return_value_out_reg = cur_major_frame->return_value_out_reg;
			Value return_value = Value(ValueType::Invalid);

			switch (num_operands) {
				case 0:
					break;
				case 1:
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand(this, data_stack, stack_size, cur_major_frame, operands[0], return_value));
					break;
				default:
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			if (size_t sz_frame = context->_context.stack_top - cur_major_frame->cur_coroutine->off_stack_top; sz_frame) {
				char *p = cur_major_frame->cur_coroutine->alloc_stack_data(sz_frame);
				if (!p) {
					return OutOfMemoryError::alloc();
				}
				memcpy(p, calc_stack_addr(context->_context.data_stack, context->_context.stack_size, context->_context.stack_top), sz_frame);
			}

			cur_major_frame->cur_coroutine->unbind_context();
			++cur_major_frame->resumable_context_data.cur_ins;

			cur_major_frame->cur_coroutine->off_regs = cur_major_frame->off_regs - cur_major_frame->cur_coroutine->off_stack_top;
			cur_major_frame->cur_coroutine->resumable = std::move(cur_major_frame->resumable_context_data);

			if (return_value == ValueType::Invalid) {
				if (return_value_out_reg != UINT32_MAX) {
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
				}
				_leave_major_frame(&context->get_context());
			} else {
				_leave_major_frame(&context->get_context());
				MajorFrame *prev_frame = _fetch_major_frame(&context->get_context(), cur_major_frame->off_prev_frame);
				if (return_value_out_reg != UINT32_MAX) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _set_register_value(this, data_stack, stack_size, prev_frame, return_value_out_reg, return_value));
				}
			}

			is_context_changed_out = true;
			return {};
		}
		case Opcode::RESUME: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 1>(this, output, num_operands));

			HostObjectRef<CoroutineObject> co;

			Value fn_value;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand(this, data_stack, stack_size, cur_major_frame, operands[0], fn_value));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::Reference>(this, fn_value));
			const Reference &fn_object_ref = fn_value.get_reference();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_object_ref_operand_type<ReferenceKind::ObjectRef>(this, fn_object_ref));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_object_operand_type<ObjectKind::FnOverloading>(this, fn_object_ref.as_object));
			co = (CoroutineObject *)fn_object_ref.as_object;

			if (co->is_done()) {
				if (output != UINT32_MAX) {
					SLAKE_RETURN_IF_EXCEPT(_set_register_value(this, data_stack, stack_size, cur_major_frame, output, co->final_result));
				}
			} else {
				if (co->overloading->return_type.type_id == TypeId::StructInstance) {
					Reference ref;

					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _add_local_var(&context->get_context(), cur_major_frame, co->overloading->return_type, output, ref));

					SLAKE_RETURN_IF_EXCEPT(_create_new_coroutine_major_frame(&context->_context, co.get(), output, &ref));
				} else
					SLAKE_RETURN_IF_EXCEPT(_create_new_coroutine_major_frame(&context->_context, co.get(), output, nullptr));
			}

			is_context_changed_out = true;
			break;
		}
		case Opcode::CODONE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 1>(this, output, num_operands));

			HostObjectRef<CoroutineObject> co;

			Value fn_value;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand(this, data_stack, stack_size, cur_major_frame, operands[0], fn_value));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::Reference>(this, fn_value));
			const Reference &fn_object_ref = fn_value.get_reference();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_object_ref_operand_type<ReferenceKind::ObjectRef>(this, fn_object_ref));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_object_operand_type<ObjectKind::FnOverloading>(this, fn_object_ref.as_object));
			co = (CoroutineObject *)fn_object_ref.as_object;

			SLAKE_RETURN_IF_EXCEPT(_set_register_value(this, data_stack, stack_size, cur_major_frame, output, co->is_done()));
			break;
		}
		case Opcode::LTHIS: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 0>(this, output, num_operands));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _set_register_value(this, data_stack, stack_size, cur_major_frame, output, Reference(cur_major_frame->resumable_context_data.this_object)));
			break;
		}
		case Opcode::NEW: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 1>(this, output, num_operands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::TypeName>(this, operands[0]));

			TypeRef type = operands[0].get_type_name();

			switch (type.type_id) {
				case TypeId::Instance: {
					ClassObject *cls = (ClassObject *)(type.get_custom_type_def())->type_object;
					HostObjectRef<InstanceObject> instance = new_class_instance(cls, 0);
					if (!instance)
						// TODO: Return more detail exceptions.
						return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _set_register_value(this, data_stack, stack_size, cur_major_frame, output, Reference(instance.get())));
					break;
				}
				default:
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			break;
		}
		case Opcode::ARRNEW: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 2>(this, output, num_operands));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::TypeName>(this, operands[0]));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::U32>(this, operands[1]));

			TypeRef type = operands[0].get_type_name();
			uint32_t size = operands[1].get_u32();

			auto instance = new_array_instance(this, type, size);
			if (!instance)
				// TODO: Return more detailed exceptions.
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _set_register_value(this, data_stack, stack_size, cur_major_frame, output, Reference(instance.get())));

			break;
		}
		// case Opcode::THROW: {
		//  TODO: Implement it.
		//  return alloc_out_of_memory_error_if_alloc_failed(UncaughtExceptionError::alloc(get_fixed_alloc(), x));
		//}
		case Opcode::PUSHEH: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<false, 2>(this, output, num_operands));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::TypeName>(this, operands[0]));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::U32>(this, operands[1]));

			MinorFrame *mf = _fetch_minor_frame(&context->get_context(), cur_major_frame, cur_major_frame->resumable_context_data.off_cur_minor_frame);

			if (!context->_context.aligned_stack_alloc(sizeof(ExceptHandler), alignof(ExceptHandler)))
				return alloc_out_of_memory_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));

			size_t eh_stack_off = context->_context.stack_top;

			if (cur_major_frame->cur_coroutine) {
				eh_stack_off -= cur_major_frame->cur_coroutine->off_stack_top;
			}

			ExceptHandler *eh = _fetch_except_handler(&context->get_context(), cur_major_frame, context->get_context().stack_top);

			eh->type = operands[0].get_type_name();
			eh->off_next = mf->off_except_handler;

			mf->off_except_handler = eh_stack_off;
			break;
		}
		case Opcode::CAST: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_count<true, 2>(this, output, num_operands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::TypeName>(this, operands[0]));
			if (!_is_register_valid(cur_major_frame, output)) {
				// The register does not present.
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			const Value *v;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand_into_ptr(this, data_stack, stack_size, cur_major_frame, operands[1], v));

			auto t = operands[0].get_type_name();
			Value &value_out = *_calc_reg_ptr(data_stack, stack_size, cur_major_frame, output);

			switch (t.type_id) {
				case TypeId::I8:
					_cast_to_literal_value<int8_t>(t.is_nullable(), *v, value_out);
					break;
				case TypeId::I16:
					_cast_to_literal_value<int16_t>(t.is_nullable(), *v, value_out);
					break;
				case TypeId::I32:
					_cast_to_literal_value<int32_t>(t.is_nullable(), *v, value_out);
					break;
				case TypeId::I64:
					_cast_to_literal_value<int64_t>(t.is_nullable(), *v, value_out);
					break;
				case TypeId::U8:
					_cast_to_literal_value<uint8_t>(t.is_nullable(), *v, value_out);
					break;
				case TypeId::U16:
					_cast_to_literal_value<uint16_t>(t.is_nullable(), *v, value_out);
					break;
				case TypeId::U32:
					_cast_to_literal_value<uint32_t>(t.is_nullable(), *v, value_out);
					break;
				case TypeId::U64:
					_cast_to_literal_value<uint64_t>(t.is_nullable(), *v, value_out);
					break;
				case TypeId::Bool:
					_cast_to_literal_value<bool>(t.is_nullable(), *v, value_out);
					break;
				case TypeId::F32:
					_cast_to_literal_value<float>(t.is_nullable(), *v, value_out);
					break;
				case TypeId::F64:
					_cast_to_literal_value<double>(t.is_nullable(), *v, value_out);
					break;
				case TypeId::Instance:
					value_out.as_reference = v->get_reference();
					value_out.value_type = ValueType::Reference;
					break;
				default:
					return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			break;
		}
		case Opcode::PHI: {
			if (output == UINT32_MAX) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			if ((num_operands < 2) || ((num_operands & 1))) {
				return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			Value v;

			for (size_t i = 0; i < num_operands; i += 2) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _check_operand_type<ValueType::U32>(this, operands[i]));

				uint32_t off = operands[i].get_u32();

				if (off == cur_major_frame->resumable_context_data.last_jump_src) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _unwrap_reg_operand(this, data_stack, stack_size, cur_major_frame, operands[i + 1], v));

					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _set_register_value(this, data_stack, stack_size, cur_major_frame, output, v));

					goto succeeded;
				}
			}

			return alloc_out_of_memory_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));

		succeeded:
			break;
		}
		default:
			return alloc_out_of_memory_error_if_alloc_failed(InvalidOpcodeError::alloc(get_fixed_alloc(), opcode));
	}
	++cur_major_frame->resumable_context_data.cur_ins;
	return {};
}

SLAKE_API InternalExceptionPointer Runtime::exec_context(ContextObject *context) noexcept {
	size_t initial_major_frame_depth = context->_context.num_major_frames;
	InternalExceptionPointer except_ptr;
	ExecutionRunnable *const managed_thread = managed_thread_runnables.at(current_thread_handle());

	while (context->get_context().num_major_frames >= initial_major_frame_depth) {
		MajorFrame *const cur_major_frame = _fetch_major_frame(&context->get_context(), context->get_context().off_cur_major_frame);
		const FnOverloadingObject *const cur_fn = cur_major_frame->cur_fn;

		if (!cur_fn) {
			break;
		}

		// Pause if the runtime is in GC
		/*while (_flags & _RT_INGC)
			yield_current_thread();*/

		switch (cur_fn->overloading_kind) {
			case FnOverloadingKind::Regular: {
				const RegularFnOverloadingObject *const ol = (RegularFnOverloadingObject *)cur_fn;

				const size_t num_ins = ol->instructions.size();

				bool is_context_changed = false;
				while (!is_context_changed) {
					const size_t idx_cur_ins = cur_major_frame->resumable_context_data.cur_ins;
					if (idx_cur_ins >=
						num_ins) {
						// Raise out of fn body error.
						std::terminate();
					}

					// Interrupt execution if the thread is explicitly specified to be killed.
					if (managed_thread->status == ThreadStatus::Dead) {
						return {};
					}

					if ((fixed_alloc.sz_allocated > _sz_computed_gc_limit)) {
						gc();
					}

					const Instruction *const instruction = ol->instructions.data() + idx_cur_ins;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _exec_ins(context, cur_major_frame, instruction->opcode, instruction->output, instruction->num_operands, instruction->operands, is_context_changed));
				}

				break;
			}
			case FnOverloadingKind::Native: {
				NativeFnOverloadingObject *ol = (NativeFnOverloadingObject *)cur_fn;

				MinorFrame *mf = _fetch_minor_frame(&context->get_context(), cur_major_frame, cur_major_frame->resumable_context_data.off_cur_minor_frame);
				const Value *args = _fetch_arg_stack(
					context->get_context().data_stack,
					context->get_context().stack_size,
					cur_major_frame,
					cur_major_frame->resumable_context_data.off_args);
				Value return_value;
				{
					peff::ScopeGuard dec_ref_args_guard([this, cur_major_frame, args]() noexcept {
						for (size_t i = 0; i < cur_major_frame->resumable_context_data.num_args; ++i) {
							if (args[i].is_reference()) {
								const Reference &ref = args[i].get_reference();
								if (ref.is_object_ref())
									ref.get_object_ref()->dec_host_ref();
							}
						}
					});
					for (size_t i = 0; i < cur_major_frame->resumable_context_data.num_args; ++i) {
						if (args[i].is_reference()) {
							const Reference &ref = args[i].get_reference();
							if (ref.is_object_ref())
								ref.get_object_ref()->inc_host_ref();
						}
					}
					return_value = ol->callback(
						&context->get_context(),
						cur_major_frame);
				}
				uint32_t return_value_out_reg = cur_major_frame->return_value_out_reg;
				_leave_major_frame(&context->get_context());
				if (return_value_out_reg != UINT32_MAX) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(except_ptr, _set_register_value(this, context->_context.data_stack, context->_context.stack_size, cur_major_frame, return_value_out_reg, return_value));
				}

				break;
			}
			default:
				std::terminate();
		}
	}

	context->_context.flags |= CTX_DONE;
	return {};
}

SLAKE_API InternalExceptionPointer Runtime::exec_fn(
	const FnOverloadingObject *overloading,
	ContextObject *prev_context,
	Object *this_object,
	const Value *args,
	uint32_t num_args,
	Value &value_out) {
	HostObjectRef<ContextObject> context(prev_context);

	Context &ctxt = context->get_context();

	SLAKE_RETURN_IF_EXCEPT(_create_new_major_frame(prev_context, nullptr, nullptr, nullptr, SIZE_MAX, 0, UINT32_MAX, nullptr));
	MajorFrame &bottom_frame = *_fetch_major_frame(&ctxt, ctxt.off_cur_major_frame);
	if (overloading->return_type.type_id == TypeId::StructInstance) {
		Reference struct_ref;
		SLAKE_RETURN_IF_EXCEPT(_add_local_var(&ctxt, &bottom_frame, overloading->return_type, 0, struct_ref));
		SLAKE_RETURN_IF_EXCEPT(_set_register_value(this, ctxt.data_stack, ctxt.stack_size, &bottom_frame, 0, Value(struct_ref)));
		SLAKE_RETURN_IF_EXCEPT(_create_new_major_frame(prev_context, this_object, overloading, args, SIZE_MAX, num_args, 0, &struct_ref));
	} else {
		SLAKE_RETURN_IF_EXCEPT(_create_new_major_frame(prev_context, this_object, overloading, args, SIZE_MAX, num_args, 0, nullptr));
	}

	ExecutionRunnable runnable;

	runnable.context = context;

	if (!managed_thread_runnables.insert(current_thread_handle(), &runnable)) {
		return OutOfMemoryError::alloc();
	}

	NativeThreadHandle thread_handle = current_thread_handle();

	peff::ScopeGuard remove_managed_thread_runnables_guard([this, thread_handle]() noexcept {
		managed_thread_runnables.remove(thread_handle);
	});

	runnable.run();

	InternalExceptionPointer except_ptr = std::move(runnable.except_ptr);

	return except_ptr;
}

SLAKE_API InternalExceptionPointer Runtime::exec_fn_with_separated_execution_thread(
	const FnOverloadingObject *overloading,
	ContextObject *prev_context,
	Object *this_object,
	const Value *args,
	uint32_t num_args,
	HostObjectRef<ContextObject> &context_out) {
	HostObjectRef<ContextObject> context(prev_context);

	Context &ctxt = context->get_context();

	SLAKE_RETURN_IF_EXCEPT(_create_new_major_frame(prev_context, nullptr, nullptr, nullptr, SIZE_MAX, 0, UINT32_MAX, nullptr));
	MajorFrame &bottom_frame = *_fetch_major_frame(&ctxt, ctxt.off_cur_major_frame);
	if (overloading->return_type.type_id == TypeId::StructInstance) {
		Reference struct_ref;
		SLAKE_RETURN_IF_EXCEPT(_add_local_var(&ctxt, &bottom_frame, overloading->return_type, 0, struct_ref));
		SLAKE_RETURN_IF_EXCEPT(_set_register_value(this, ctxt.data_stack, ctxt.stack_size, &bottom_frame, 0, Value(struct_ref)));
		SLAKE_RETURN_IF_EXCEPT(_create_new_major_frame(prev_context, this_object, overloading, args, SIZE_MAX, num_args, 0, &struct_ref));
	} else {
		SLAKE_RETURN_IF_EXCEPT(_create_new_major_frame(prev_context, this_object, overloading, args, SIZE_MAX, num_args, 0, nullptr));
	}

	ExecutionRunnable runnable;

	runnable.context = context;

	if (!managed_thread_runnables.insert(current_thread_handle(), &runnable)) {
		return OutOfMemoryError::alloc();
	}

	NativeThreadHandle thread_handle = current_thread_handle();

	peff::ScopeGuard remove_managed_thread_runnables_guard([this, thread_handle]() noexcept {
		managed_thread_runnables.remove(thread_handle);
	});

	std::unique_ptr<Thread, peff::DeallocableDeleter<Thread>> t(Thread::alloc(get_fixed_alloc(), &runnable, SLAKE_NATIVE_STACK_SIZE_MAX));

	t->join();

	InternalExceptionPointer except_ptr = std::move(runnable.except_ptr);

	return except_ptr;
}

SLAKE_API InternalExceptionPointer Runtime::create_coroutine_instance(
	const FnOverloadingObject *fn,
	Object *this_object,
	const Value *args,
	uint32_t num_args,
	HostObjectRef<CoroutineObject> &coroutine_out) {
	HostRefHolder holder(get_fixed_alloc());
	HostObjectRef<CoroutineObject> co = CoroutineObject::alloc(this);

	if (!co)
		return OutOfMemoryError::alloc();

	co->overloading = fn;

	MajorFrame new_major_frame(this);

	peff::construct_at<ResumableContextData>(&new_major_frame.resumable_context_data);

	if (num_args) {
		char *p = co->alloc_stack_data(sizeof(Value) * num_args);
		if (!p)
			return OutOfMemoryError::alloc();
		// TODO: Check if the arguments and the parameters are matched.
		memcpy(p, args, sizeof(Value) * num_args);
	}
	new_major_frame.resumable_context_data.this_object = this_object;

	coroutine_out = co;
	return {};
}

SLAKE_API InternalExceptionPointer Runtime::resume_coroutine(
	ContextObject *context,
	CoroutineObject *coroutine,
	Value &result_out,
	void *native_stack_base_current_ptr,
	size_t native_stack_size) {
	if (coroutine->is_done()) {
		result_out = coroutine->final_result;
		return {};
	}

	HostObjectRef<ContextObject> context_ref(context);
	Context &ctxt = context->get_context();

	SLAKE_RETURN_IF_EXCEPT(_create_new_major_frame(context, nullptr, nullptr, nullptr, SIZE_MAX, 0, UINT32_MAX, nullptr));
	MajorFrame &bottom_frame = *_fetch_major_frame(&ctxt, ctxt.off_cur_major_frame);
	if (coroutine->overloading->return_type.type_id == TypeId::StructInstance) {
		Reference struct_ref;
		SLAKE_RETURN_IF_EXCEPT(_add_local_var(&ctxt, &bottom_frame, coroutine->overloading->return_type, 0, struct_ref));
		SLAKE_RETURN_IF_EXCEPT(_set_register_value(this, ctxt.data_stack, ctxt.stack_size, &bottom_frame, 0, Value(struct_ref)));
		SLAKE_RETURN_IF_EXCEPT(_create_new_coroutine_major_frame(&context->_context, coroutine, 0, &struct_ref));
	} else
		SLAKE_RETURN_IF_EXCEPT(_create_new_coroutine_major_frame(&context->_context, coroutine, 0, nullptr));

	{
		ExecutionRunnable runnable;

		runnable.context = context;

		{
			if (!managed_thread_runnables.insert(current_thread_handle(), &runnable)) {
				_leave_major_frame(&context->get_context());

				return OutOfMemoryError::alloc();
			}

			NativeThreadHandle thread_handle = current_thread_handle();

			peff::ScopeGuard remove_managed_thread_runnables_guard([this, thread_handle]() noexcept {
				managed_thread_runnables.remove(thread_handle);
			});

			runnable.run();
		}

		InternalExceptionPointer except_ptr = std::move(runnable.except_ptr);

		if (except_ptr) {
			return except_ptr;
		}

		result_out = *_calc_reg_ptr(context->_context.data_stack, context->_context.stack_size, &bottom_frame, 0);

		_leave_major_frame(&context->get_context());
	}

	return {};
}
