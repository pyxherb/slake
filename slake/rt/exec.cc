#include "../runtime.h"
#include <slake/flib/math/fmod.h>
#include <slake/flib/bitop.h>
#include <slake/flib/cmp.h>
#include <peff/base/scope_guard.h>
#include <peff/utils/misc.h>
#include <cmath>

using namespace slake;

template <typename LT>
static void _cast_to_literal_value(bool nullable, const Value &x, Value &value_out) noexcept {
	switch (x.value_type) {
		case ValueType::I8:
			value_out = (static_cast<LT>(x.get_i8()));
			break;
		case ValueType::I16:
			value_out = (static_cast<LT>(x.get_i16()));
			break;
		case ValueType::I32:
			value_out = (static_cast<LT>(x.get_i32()));
			break;
		case ValueType::I64:
			value_out = (static_cast<LT>(x.get_i64()));
			break;
		case ValueType::ISize:
			value_out = (static_cast<LT>(x.get_isize()));
			break;
		case ValueType::U8:
			value_out = (static_cast<LT>(x.get_u8()));
			break;
		case ValueType::U16:
			value_out = (static_cast<LT>(x.get_u16()));
			break;
		case ValueType::U32:
			value_out = (static_cast<LT>(x.get_u32()));
			break;
		case ValueType::U64:
			value_out = (static_cast<LT>(x.get_u64()));
			break;
		case ValueType::USize:
			value_out = (static_cast<LT>(x.get_usize()));
			break;
		case ValueType::F32:
			value_out = (static_cast<LT>(x.get_f32()));
			break;
		case ValueType::F64:
			value_out = (static_cast<LT>(x.get_f64()));
			break;
		case ValueType::Bool:
			value_out = (static_cast<LT>(x.get_bool()));
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
		return alloc_oom_error_if_alloc_failed(InvalidArgumentNumberError::alloc(get_fixed_alloc(), num_args));
	}

	for (size_t i = 0; i < fn->param_types.size(); ++i) {
		TypeRef t = fn->param_types.at(i);
		if (!is_compatible(t, args[i]))
			return MismatchedVarTypeError::alloc(get_fixed_alloc(), t);
	}
	void *p_args = context->aligned_stack_alloc(sizeof(Value) * num_args, alignof(Value));
	if (!p_args)
		return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
	size_t off_args = new_major_frame->cur_coroutine ? context->stack_top - new_major_frame->cur_coroutine->off_stack_top : context->stack_top;
	memcpy(p_args, args, sizeof(Value) * num_args);
	new_major_frame->resumable_context_data.off_args = off_args;
	new_major_frame->resumable_context_data.num_args = num_args;

	return {};
}

SLAKE_API AllocaRecord *Runtime::_alloc_alloca_record(Context *context, const MajorFrame *frame, RegIndex output_reg) {
	MinorFrame *mf = _fetch_minor_frame(context, frame, frame->resumable_context_data.off_cur_minor_frame);
	AllocaRecord *record;
	if (!(record = static_cast<AllocaRecord *>(context->aligned_stack_alloc(sizeof(AllocaRecord), alignof(AllocaRecord)))))
		return nullptr;
	record->def_reg = output_reg;
	record->off_next = mf->off_alloca_records;
	mf->off_alloca_records = frame->cur_coroutine
								 ? context->stack_top - frame->cur_coroutine->off_stack_top
								 : context->stack_top;
	return record;
}

SLAKE_API MinorFrame *Runtime::_fetch_minor_frame_unchecked(
	Context *context,
	const MajorFrame *major_frame,
	size_t stack_offset) {
	size_t offset = stack_offset;
	if (major_frame->cur_coroutine) {
		offset += major_frame->cur_coroutine->off_stack_top;
	}

	return static_cast<MinorFrame *>(calc_stack_addr(context->data_stack,
		context->stack_size,
		offset));
}

SLAKE_API Value *Runtime::_fetch_arg_stack(
	char *data_stack,
	size_t stack_size,
	const MajorFrame *major_frame,
	size_t stack_offset) {
	size_t offset = stack_offset;
	if (major_frame && major_frame->cur_coroutine) {
		offset += major_frame->cur_coroutine->off_stack_top;
	}

	return static_cast<Value *>(calc_stack_addr(data_stack,
		stack_size,
		offset));
}

SLAKE_API AllocaRecord *Runtime::_fetch_alloca_record(
	Context *context,
	const MajorFrame *major_frame,
	size_t stack_offset) {
	size_t offset = stack_offset;
	if (major_frame->cur_coroutine) {
		offset += major_frame->cur_coroutine->off_stack_top;
	}

	return static_cast<AllocaRecord *>(calc_stack_addr(context->data_stack,
		context->stack_size,
		offset));
}

SLAKE_API MajorFrame *Runtime::_fetch_major_frame_unchecked(
	Context *context,
	size_t stack_offset) {
	return static_cast<MajorFrame *>(calc_stack_addr(context->data_stack,
		context->stack_size,
		stack_offset));
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

	return static_cast<ExceptHandler *>(calc_stack_addr(context->data_stack,
		context->stack_size,
		offset));
}

SLAKE_API InternalExceptionPointer slake::Runtime::_create_new_major_frame(
	ContextObject *context_object,
	Object *this_object,
	FnOverloadingObject *fn,
	const Value *args,
	size_t off_args,
	uint32_t num_args,
	RegIndex return_value_out,
	const Reference *return_struct_ref) noexcept {
	Context *const context = &context_object->_context;

	size_t prev_stack_top = context->stack_top;
	peff::ScopeGuard restore_stack_top_guard([context, prev_stack_top]() noexcept {
		context->stack_top = prev_stack_top;
	});

	// TODO: Restore resumable context data.

	MajorFrame *p_major_frame;
	if (!(p_major_frame = static_cast<MajorFrame *>(context->aligned_stack_alloc(sizeof(MajorFrame), alignof(MajorFrame)))))
		return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
	peff::construct_at<MajorFrame>(p_major_frame, this);
	MajorFrame &new_major_frame = *p_major_frame;
	new_major_frame.off_prev_frame = context->off_cur_major_frame;

	new_major_frame.cur_context = context_object;

	size_t off_major_frame = context->stack_top;

	// Create minor frame.
	if (!context->aligned_stack_alloc(sizeof(MinorFrame), alignof(MinorFrame)))
		return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));

	size_t mf_stack_off = context->stack_top;

	MinorFrame *mf = _fetch_minor_frame_unchecked(context, &new_major_frame, context->stack_top);

	peff::construct_at<MinorFrame>(mf);

	mf->off_last_minor_frame = new_major_frame.resumable_context_data.off_cur_minor_frame;
	mf->stack_base = prev_stack_top;
	new_major_frame.resumable_context_data.off_cur_minor_frame = mf_stack_off;

	if (!fn) {
		// Used in the creation of top major frame.
		// The top major frame always carries a any-typed register.
		new_major_frame.cur_fn = nullptr;
		Value *regs = static_cast<Value *>(context->aligned_stack_alloc(sizeof(Value) * 1, alignof(Value)));
		new_major_frame.resumable_context_data.regs_base_off[static_cast<size_t>(InsRegType::Any)] = context->stack_top;
		*regs = InvalidValueState{};
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
				RegularFnOverloadingObject *ol = static_cast<RegularFnOverloadingObject *>(fn);

				{
					size_t num_regs = ol->get_register_number(InsRegType::Any);
					void *regs = context->aligned_stack_alloc(sizeof(Value) * num_regs, alignof(Value));
					if (!regs)
						return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
					memset(regs, 0, sizeof(Value) * num_regs);
					new_major_frame.resumable_context_data.regs_base_off[(size_t)InsRegType::Any] = context->stack_top;
				}

				{
					size_t num_regs = ol->get_register_number(InsRegType::Object);
					void *regs = context->aligned_stack_alloc(sizeof(void *) * num_regs, alignof(void *));
					if (!regs)
						return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
					memset(regs, 0, sizeof(void *) * num_regs);
					new_major_frame.resumable_context_data.regs_base_off[(size_t)InsRegType::Any] = context->stack_top;
				}

				{
					size_t num_regs = ol->get_register_number(InsRegType::ISize);
					void *regs = context->aligned_stack_alloc(sizeof(ptrdiff_t) * num_regs, alignof(ptrdiff_t));
					if (!regs)
						return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
					memset(regs, 0, sizeof(ptrdiff_t) * num_regs);
					new_major_frame.resumable_context_data.regs_base_off[(size_t)InsRegType::ISize] = context->stack_top;
				}

				{
					size_t num_regs = ol->get_register_number(InsRegType::USize);
					void *regs = context->aligned_stack_alloc(sizeof(size_t) * num_regs, alignof(size_t));
					if (!regs)
						return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
					memset(regs, 0, sizeof(size_t) * num_regs);
					new_major_frame.resumable_context_data.regs_base_off[(size_t)InsRegType::USize] = context->stack_top;
				}

				{
					size_t num_regs = ol->get_register_number(InsRegType::F64);
					void *regs = context->aligned_stack_alloc(sizeof(double) * num_regs, alignof(double));
					if (!regs)
						return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
					memset(regs, 0, sizeof(double) * num_regs);
					new_major_frame.resumable_context_data.regs_base_off[(size_t)InsRegType::F64] = context->stack_top;
				}

				{
					size_t num_regs = ol->get_register_number(InsRegType::I64);
					void *regs = context->aligned_stack_alloc(sizeof(int64_t) * num_regs, alignof(int64_t));
					if (!regs)
						return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
					memset(regs, 0, sizeof(int64_t) * num_regs);
					new_major_frame.resumable_context_data.regs_base_off[(size_t)InsRegType::I64] = context->stack_top;
				}

				{
					size_t num_regs = ol->get_register_number(InsRegType::U64);
					void *regs = context->aligned_stack_alloc(sizeof(uint64_t) * num_regs, alignof(uint64_t));
					if (!regs)
						return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
					memset(regs, 0, sizeof(uint64_t) * num_regs);
					new_major_frame.resumable_context_data.regs_base_off[(size_t)InsRegType::U64] = context->stack_top;
				}

				{
					size_t num_regs = ol->get_register_number(InsRegType::F32);
					void *regs = context->aligned_stack_alloc(sizeof(float) * num_regs, alignof(float));
					if (!regs)
						return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
					memset(regs, 0, sizeof(float) * num_regs);
					new_major_frame.resumable_context_data.regs_base_off[(size_t)InsRegType::F32] = context->stack_top;
				}

				{
					size_t num_regs = ol->get_register_number(InsRegType::I32);
					void *regs = context->aligned_stack_alloc(sizeof(int32_t) * num_regs, alignof(int32_t));
					if (!regs)
						return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
					memset(regs, 0, sizeof(int32_t) * num_regs);
					new_major_frame.resumable_context_data.regs_base_off[(size_t)InsRegType::I32] = context->stack_top;
				}

				{
					size_t num_regs = ol->get_register_number(InsRegType::U32);
					void *regs = context->aligned_stack_alloc(sizeof(uint32_t) * num_regs, alignof(uint32_t));
					if (!regs)
						return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
					memset(regs, 0, sizeof(uint32_t) * num_regs);
					new_major_frame.resumable_context_data.regs_base_off[(size_t)InsRegType::U32] = context->stack_top;
				}

				{
					size_t num_regs = ol->get_register_number(InsRegType::I16);
					void *regs = context->aligned_stack_alloc(sizeof(int16_t) * num_regs, alignof(int16_t));
					if (!regs)
						return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
					memset(regs, 0, sizeof(int16_t) * num_regs);
					new_major_frame.resumable_context_data.regs_base_off[(size_t)InsRegType::I16] = context->stack_top;
				}

				{
					size_t num_regs = ol->get_register_number(InsRegType::U16);
					void *regs = context->aligned_stack_alloc(sizeof(uint16_t) * num_regs, alignof(uint16_t));
					if (!regs)
						return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
					memset(regs, 0, sizeof(uint16_t) * num_regs);
					new_major_frame.resumable_context_data.regs_base_off[(size_t)InsRegType::U16] = context->stack_top;
				}

				{
					size_t num_regs = ol->get_register_number(InsRegType::I8);
					void *regs = context->aligned_stack_alloc(sizeof(int8_t) * num_regs, alignof(int8_t));
					if (!regs)
						return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
					memset(regs, 0, sizeof(int8_t) * num_regs);
					new_major_frame.resumable_context_data.regs_base_off[(size_t)InsRegType::I8] = context->stack_top;
				}

				{
					size_t num_regs = ol->get_register_number(InsRegType::U8);
					void *regs = context->aligned_stack_alloc(sizeof(uint8_t) * num_regs, alignof(uint8_t));
					if (!regs)
						return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
					memset(regs, 0, sizeof(uint8_t) * num_regs);
					new_major_frame.resumable_context_data.regs_base_off[(size_t)InsRegType::U8] = context->stack_top;
				}

				{
					size_t num_regs = ol->get_register_number(InsRegType::Bool);
					void *regs = context->aligned_stack_alloc(sizeof(bool) * num_regs, alignof(bool));
					if (!regs)
						return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
					memset(regs, 0, sizeof(bool) * num_regs);
					new_major_frame.resumable_context_data.regs_base_off[(size_t)InsRegType::Bool] = context->stack_top;
				}
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
		MajorFrame *prev_mf = _fetch_major_frame(context, context->off_cur_major_frame);
		assert(context->off_cur_major_frame != off_major_frame);
		prev_mf->off_next_frame = off_major_frame;
		mf->stack_base = prev_mf->resumable_context_data.off_next_args_begin;
		prev_mf->resumable_context_data.off_next_args_begin = SIZE_MAX;
	}
	context->off_cur_major_frame = off_major_frame;
	++context->num_major_frames;

	return {};
}

SLAKE_API void Runtime::_leave_major_frame(Context *context) noexcept {
	MajorFrame *mf = _fetch_major_frame(context, context->off_cur_major_frame);

	assert(mf->off_next_frame == SIZE_MAX);

	if (mf->off_prev_frame != SIZE_MAX)
		_fetch_major_frame(context, mf->off_prev_frame)->off_next_frame = SIZE_MAX;

	context->off_cur_major_frame = mf->off_prev_frame;
	assert(mf->prev_stack_top <= context->stack_top);
	context->stack_top = mf->prev_stack_top;
	--context->num_major_frames;
}

SLAKE_FORCEINLINE InternalExceptionPointer slake::Runtime::_add_local_var(Context *context, const MajorFrame *frame, TypeRef type, RegIndex output_reg, Reference &object_ref_out) noexcept {
	size_t original_stack_top = context->stack_top;

	peff::ScopeGuard restore_stack_top_guard([original_stack_top, context]() noexcept {
		context->stack_top = original_stack_top;
	});

	switch (type.type_id) {
		case TypeId::StructInstance: {
			assert(type.get_custom_type_def()->type_object->get_object_kind() == ObjectKind::Struct);
			SLAKE_RETURN_IF_EXCEPT(prepare_struct_for_instantiation(static_cast<StructObject *>(type.get_custom_type_def()->type_object)));
			break;
		}
		case TypeId::UnionEnum: {
			assert(type.get_custom_type_def()->type_object->get_object_kind() == ObjectKind::UnionEnum);
			SLAKE_RETURN_IF_EXCEPT(prepare_union_enum_for_instantiation(static_cast<UnionEnumObject *>(type.get_custom_type_def()->type_object)));
			break;
		}
		case TypeId::UnionEnumItem: {
			assert(type.get_custom_type_def()->type_object->get_object_kind() == ObjectKind::UnionEnumItem);
			SLAKE_RETURN_IF_EXCEPT(prepare_union_enum_item_for_instantiation(static_cast<UnionEnumItemObject *>(type.get_custom_type_def()->type_object)));
			break;
		}
		default:
			break;
	}

	size_t size = sizeof_type(type), align = alignof_type(type);

	if (!context->aligned_stack_alloc(size, align))
		return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));

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
			Object **type_info = static_cast<Object **>(context->stack_alloc(sizeof(void *)));
			if (!type_info)
				return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
#ifndef _NDEBUG
			const size_t diff = alignof(void *) - ((uintptr_t)(calc_stack_addr(context->data_stack, context->stack_size, context->stack_top)) & (alignof(void *) - 1));
			assert((diff == alignof(void *) || (!diff)));
#endif
			*type_info = type.type_def;
			break;
		}
		case TypeId::ScopedEnum: {
			if (type.is_nullable())
				context->stack_alloc(sizeof(bool));
			TypeDefObject **type_info = (TypeDefObject **)context->stack_alloc(sizeof(void *));
			if (!type_info)
				return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
			memcpy(type_info, &type.type_def, sizeof(void *));
			break;
		}
		case TypeId::TypelessScopedEnum: {
			if (type.is_nullable())
				context->stack_alloc(sizeof(bool));
			TypeDefObject **type_info = (TypeDefObject **)context->stack_alloc(sizeof(void *));
			if (!type_info)
				return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
			memcpy(type_info, &type.type_def, sizeof(void *));
			break;
		}
		case TypeId::StructInstance:
		case TypeId::UnionEnum:
		case TypeId::UnionEnumItem: {
			if (type.is_nullable())
				context->stack_alloc(sizeof(bool));
			TypeDefObject **type_info = (TypeDefObject **)context->stack_alloc(sizeof(void *));
			if (!type_info)
				return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
			memcpy(type_info, &type.type_def, sizeof(void *));
			break;
		}
		default:
			std::terminate();
	}

	TypeModifier *type_modifier = static_cast<TypeModifier *>(context->stack_alloc(sizeof(TypeModifier)));
	if (!type_modifier)
		return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
	*type_modifier = type.type_modifier;

	TypeId *type_id = static_cast<TypeId *>(context->stack_alloc(sizeof(TypeId)));
	if (!type_id)
		return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
	*type_id = type.type_id;

	size_t off_out = context->stack_top;

	if (!_alloc_alloca_record(context, frame, output_reg))
		return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));

	restore_stack_top_guard.release();

	if (frame->cur_coroutine)
		object_ref_out = CoroutineLocalVarRef(frame->cur_coroutine, off_out - frame->cur_coroutine->off_stack_top);
	else
		object_ref_out = LocalVarRef(context, off_out);

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
		case TypeId::Instance:
		case TypeId::Array:
		case TypeId::Ref:
		case TypeId::StructInstance:
			write_var_with_type(object_ref_out, type, default_value_of(type));
			break;
		default:
			break;
	}

	return {};
}

SLAKE_FORCEINLINE InternalExceptionPointer larg(Context *context, MajorFrame *major_frame, Runtime *rt, uint32_t off, Reference &object_ref_out) {
	if (off >= major_frame->resumable_context_data.num_args) {
		return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(rt->get_fixed_alloc()));
	}

	if (major_frame->cur_coroutine) {
		object_ref_out = CoroutineArgRef(major_frame->cur_coroutine, off);
	} else {
		object_ref_out = ArgRef(major_frame, off);
	}
	return {};
}

#define _check_reg_type(v, t)                                      \
	if SLAKE_UNLIKELY ((v) != static_cast<uint8_t>(InsRegType::t)) \
		return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(this->get_fixed_alloc()));

#define _check_reg_index(index, opr_type)                                                        \
	if SLAKE_UNLIKELY ((index) >= ol->num_registers[static_cast<uint8_t>(InsRegType::opr_type)]) \
		return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(this->get_fixed_alloc()));

#define _access_typed_reg(index, type, opr_type) \
	static_cast<type *>(calc_stack_addr(         \
		data_stack,                              \
		stack_size,                              \
		cur_major_frame->resumable_context_data.regs_base_off[static_cast<uint8_t>(InsRegType::opr_type)] + index * sizeof(type)))

#define _access_nonlocal_typed_reg(cur_major_frame, index, type, opr_type) \
	static_cast<type *>(calc_stack_addr(                                   \
		data_stack,                                                        \
		stack_size,                                                        \
		cur_major_frame->resumable_context_data.regs_base_off[static_cast<uint8_t>(InsRegType::opr_type)] + index * sizeof(type)))

SLAKE_FORCEINLINE InternalExceptionPointer Runtime::_exec_ins(
	ContextObject *const context,
	MajorFrame *const cur_major_frame,
	char *const data_stack,
	const size_t stack_size,
	const Instruction &cur_ins,
	const RegularFnOverloadingObject *ol,
	ContextChangeType &context_changes_out) noexcept {
	switch (cur_ins.opcode) {
#define _lvalue_opcode(opcode, data_type, slake_type, slake_lower_type)                                   \
	case Opcode::opcode: {                                                                                \
		_check_reg_type(cur_ins.reg_out_type, slake_type);                                                \
		_check_reg_index(cur_ins.reg_out, slake_type);                                                    \
                                                                                                          \
		_check_reg_type(cur_ins.reg0_type, Any);                                                          \
		_check_reg_index(cur_ins.reg0, Any);                                                              \
                                                                                                          \
		Value v;                                                                                          \
		const Value *source = _access_typed_reg(cur_ins.reg0, Value, Any);                                \
		if ((!source->is_reference()) || (source->as_reference.is_object_ref()))                          \
			return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(this->get_fixed_alloc())); \
                                                                                                          \
		read_var(source->get_reference(), v);                                                             \
		if SLAKE_UNLIKELY (!v.is_##slake_lower_type())                                                                   \
			return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(this->get_fixed_alloc())); \
		*_access_typed_reg(cur_ins.reg_out, data_type, slake_type) = v.get_##slake_lower_type();          \
                                                                                                          \
		break;                                                                                            \
	}
		_lvalue_opcode(LVALUEI8, int8_t, I8, i8);
		_lvalue_opcode(LVALUEI16, int16_t, I16, i16);
		_lvalue_opcode(LVALUEI32, int32_t, I32, i32);
		_lvalue_opcode(LVALUEI64, int64_t, I64, i64);
		_lvalue_opcode(LVALUEISIZE, ptrdiff_t, ISize, isize);
		_lvalue_opcode(LVALUEU8, uint8_t, U8, u8);
		_lvalue_opcode(LVALUEU16, uint16_t, U16, u16);
		_lvalue_opcode(LVALUEU32, uint32_t, U32, u32);
		_lvalue_opcode(LVALUEU64, uint64_t, U64, u64);
		_lvalue_opcode(LVALUEUSIZE, size_t, USize, usize);
		_lvalue_opcode(LVALUEF32, float, F32, f32);
		_lvalue_opcode(LVALUEF64, double, F64, f64);
		_lvalue_opcode(LVALUEBOOL, bool, Bool, bool);
		case Opcode::LVALUE: {
			_check_reg_type(cur_ins.reg_out_type, Any);
			_check_reg_index(cur_ins.reg_out, Any);

			_check_reg_type(cur_ins.reg0_type, Any);
			_check_reg_index(cur_ins.reg0, Any);

			Value *dest = _access_typed_reg(cur_ins.reg_out, Value, Any);
			const Value *source = _access_typed_reg(cur_ins.reg0, Value, Any);
			if ((!source->is_reference()) || (source->as_reference.is_object_ref()))
				return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(this->get_fixed_alloc()));

			read_var(source->get_reference(), *dest);

			break;
		}
#define _basic_type_store_opcode(opcode, data_type, slake_type, slake_lower_type)                                           \
	case Opcode::opcode: {                                                                                                  \
		_check_reg_type(cur_ins.reg0_type, Any);                                                                            \
		_check_reg_index(cur_ins.reg0, Any);                                                                                \
                                                                                                                            \
		_check_reg_type(cur_ins.reg1_type, slake_type);                                                                     \
		_check_reg_index(cur_ins.reg1, slake_type);                                                                         \
                                                                                                                            \
		Value *dest = _access_typed_reg(cur_ins.reg0, Value, Any);                                                          \
		Value source(*_access_typed_reg(cur_ins.reg1, data_type, slake_type));                                              \
		if ((!dest->is_reference()) || (dest->as_reference.is_object_ref()))                                                \
			return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(context->get_runtime()->get_fixed_alloc())); \
                                                                                                                            \
		if (typeof_var(dest->get_reference()).type_id != TypeId::slake_type)                                                \
			return MismatchedVarTypeError::alloc(context->get_runtime()->get_fixed_alloc(), TypeId::slake_type);            \
		write_var_with_type(dest->get_reference(), TypeId::slake_type, source);                                             \
                                                                                                                            \
		break;                                                                                                              \
	}
			_basic_type_store_opcode(STOREI8, int8_t, I8, i8);
			_basic_type_store_opcode(STOREI16, int16_t, I16, i16);
			_basic_type_store_opcode(STOREI32, int32_t, I32, i32);
			_basic_type_store_opcode(STOREI64, int64_t, I64, i64);
		case Opcode::STOREISIZE: {
			_check_reg_type(cur_ins.reg0_type, Any);
			_check_reg_index(cur_ins.reg0, Any);

			_check_reg_type(cur_ins.reg1_type, ISize);
			_check_reg_index(cur_ins.reg1, ISize);

			Value *dest = _access_typed_reg(cur_ins.reg0, Value, Any);
			Value source(ExplicitISize{ *_access_typed_reg(cur_ins.reg1, ptrdiff_t, ISize) });
			if ((!dest->is_reference()) || (dest->as_reference.is_object_ref()))
				return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(context->get_runtime()->get_fixed_alloc()));

			if (typeof_var(dest->get_reference()).type_id != TypeId::ISize)
				return MismatchedVarTypeError::alloc(context->get_runtime()->get_fixed_alloc(), TypeId::ISize);
			write_var_with_type(dest->get_reference(), TypeId::ISize, source);

			break;
		}
			_basic_type_store_opcode(STOREU8, uint8_t, U8, u8);
			_basic_type_store_opcode(STOREU16, uint16_t, U16, u16);
			_basic_type_store_opcode(STOREU32, uint32_t, U32, u32);
			_basic_type_store_opcode(STOREU64, uint64_t, U64, u64);
		case Opcode::STOREUSIZE: {
			_check_reg_type(cur_ins.reg0_type, Any);
			_check_reg_index(cur_ins.reg0, Any);

			_check_reg_type(cur_ins.reg1_type, USize);
			_check_reg_index(cur_ins.reg1, USize);

			Value *dest = _access_typed_reg(cur_ins.reg0, Value, Any);
			Value source(ExplicitUSize{ *_access_typed_reg(cur_ins.reg1, size_t, USize) });
			if ((!dest->is_reference()) || (dest->as_reference.is_object_ref()))
				return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(context->get_runtime()->get_fixed_alloc()));

			if (typeof_var(dest->get_reference()).type_id != TypeId::USize)
				return MismatchedVarTypeError::alloc(context->get_runtime()->get_fixed_alloc(), TypeId::USize);
			write_var_with_type(dest->get_reference(), TypeId::USize, source);

			break;
		}
			_basic_type_store_opcode(STOREF32, float, F32, f32);
			_basic_type_store_opcode(STOREF64, double, F64, f64);
			_basic_type_store_opcode(STOREBOOL, bool, Bool, bool);
		case Opcode::STORE: {
			_check_reg_type(cur_ins.reg0_type, Any);
			_check_reg_index(cur_ins.reg0, Any);

			_check_reg_type(cur_ins.reg1_type, Any);
			_check_reg_index(cur_ins.reg1, Any);

			Value *dest = _access_typed_reg(cur_ins.reg0, Value, Any);
			const Value *source = _access_typed_reg(cur_ins.reg1, Value, Any);
			if ((!dest->is_reference()) || (dest->as_reference.is_object_ref()))
				return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(context->get_runtime()->get_fixed_alloc()));

			TypeRef type = typeof_var(dest->get_reference());
			if (!is_compatible(type, *source))
				return MismatchedVarTypeError::alloc(context->get_runtime()->get_fixed_alloc(), type);
			write_var_with_type(dest->get_reference(), type, *source);

			break;
		}
		case Opcode::LOBJ: {
			_check_reg_type(cur_ins.reg_out_type, Object);
			_check_reg_index(cur_ins.reg_out, Object);

			RegIndex dest_reg = cur_ins.reg_out;

			uint32_t obj_set_index = static_cast<uint32_t>(ol->ins_object_set.size());
			uint32_t operand_index = ins_operand_as_u32(cur_ins.operands[0]);

			if (operand_index >= obj_set_index)
				return InvalidOperandsError::alloc(context->get_runtime()->get_fixed_alloc());

			Value *dest = _access_typed_reg(dest_reg, Value, Any);

			*dest = ol->ins_object_set.at(operand_index);

			break;
		}
		case Opcode::LTYPE: {
			_check_reg_type(cur_ins.reg_out_type, Any);
			_check_reg_index(cur_ins.reg_out, Any);

			uint32_t type_set_index = static_cast<uint32_t>(ol->ins_type_set.size());
			uint32_t operand_index = ins_operand_as_u32(cur_ins.operands[0]);

			if (operand_index >= type_set_index)
				return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(context->get_runtime()->get_fixed_alloc()));

			Value *dest = _access_typed_reg(cur_ins.reg_out, Value, Any);

			*dest = ol->ins_type_set.at(operand_index);

			break;
		}
		case Opcode::JMP:
			cur_major_frame->resumable_context_data.cur_ins = ins_operand_as_u32(cur_ins.operands[0]);
			return {};
		case Opcode::BR: {
			_check_reg_type(cur_ins.reg0_type, Bool);
			_check_reg_index(cur_ins.reg0, Bool);

			cur_major_frame->resumable_context_data.cur_ins = ins_operand_as_u32(cur_ins.operands[*_access_typed_reg(cur_ins.reg0, bool, Bool) ? 0 : 1]);
			return {};
		}

#define _basic_arithm_opcode(opcode, data_type, slake_type, slake_lower_type, op_token) \
	case Opcode::opcode: {                                                              \
		data_type op0, op1;                                                             \
                                                                                        \
		if (cur_ins.flags & INS_OP0_REG) {                                              \
			_check_reg_type(cur_ins.reg0_type, slake_type);                             \
			_check_reg_index(cur_ins.reg0, slake_type);                                 \
                                                                                        \
			op0 = *_access_typed_reg(cur_ins.reg0, data_type, slake_type);              \
		} else {                                                                        \
			op0 = ins_operand_as_##slake_lower_type(cur_ins.operands[0]);               \
		}                                                                               \
                                                                                        \
		if (cur_ins.flags & INS_OP1_REG) {                                              \
			_check_reg_type(cur_ins.reg1_type, slake_type);                             \
			_check_reg_index(cur_ins.reg1, slake_type);                                 \
                                                                                        \
			op1 = *_access_typed_reg(cur_ins.reg1, data_type, slake_type);              \
		} else {                                                                        \
			op1 = ins_operand_as_##slake_lower_type(cur_ins.operands[1]);               \
		}                                                                               \
                                                                                        \
		_check_reg_type(cur_ins.reg_out_type, slake_type);                              \
		_check_reg_index(cur_ins.reg_out, slake_type);                                  \
                                                                                        \
		*_access_typed_reg(cur_ins.reg_out, data_type, slake_type) = op0 op_token op1;  \
                                                                                        \
		break;                                                                          \
	}
			_basic_arithm_opcode(ADDI8, int8_t, I8, i8, +);
			_basic_arithm_opcode(ADDI16, int16_t, I16, i16, +);
			_basic_arithm_opcode(ADDI32, int32_t, I32, i32, +);
			_basic_arithm_opcode(ADDI64, int64_t, I64, i64, +);
			_basic_arithm_opcode(ADDISIZE, ptrdiff_t, ISize, isize, +);
			_basic_arithm_opcode(ADDU8, uint8_t, U8, u8, +);
			_basic_arithm_opcode(ADDU16, uint16_t, U16, u16, +);
			_basic_arithm_opcode(ADDU32, uint32_t, U32, u32, +);
			_basic_arithm_opcode(ADDU64, uint64_t, U64, u64, +);
			_basic_arithm_opcode(ADDUSIZE, size_t, USize, usize, +);
			_basic_arithm_opcode(ADDF32, float, F32, f32, +);
			_basic_arithm_opcode(ADDF64, double, F64, f64, +);

			_basic_arithm_opcode(SUBI8, int8_t, I8, i8, -);
			_basic_arithm_opcode(SUBI16, int16_t, I16, i16, -);
			_basic_arithm_opcode(SUBI32, int32_t, I32, i32, -);
			_basic_arithm_opcode(SUBI64, int64_t, I64, i64, -);
			_basic_arithm_opcode(SUBISIZE, ptrdiff_t, ISize, isize, -);
			_basic_arithm_opcode(SUBU8, uint8_t, U8, u8, -);
			_basic_arithm_opcode(SUBU16, uint16_t, U16, u16, -);
			_basic_arithm_opcode(SUBU32, uint32_t, U32, u32, -);
			_basic_arithm_opcode(SUBU64, uint64_t, U64, u64, -);
			_basic_arithm_opcode(SUBUSIZE, size_t, USize, usize, -);
			_basic_arithm_opcode(SUBF32, float, F32, f32, -);
			_basic_arithm_opcode(SUBF64, double, F64, f64, -);

			_basic_arithm_opcode(MULI8, int8_t, I8, i8, *);
			_basic_arithm_opcode(MULI16, int16_t, I16, i16, *);
			_basic_arithm_opcode(MULI32, int32_t, I32, i32, *);
			_basic_arithm_opcode(MULI64, int64_t, I64, i64, *);
			_basic_arithm_opcode(MULISIZE, ptrdiff_t, ISize, isize, *);
			_basic_arithm_opcode(MULU8, uint8_t, U8, u8, *);
			_basic_arithm_opcode(MULU16, uint16_t, U16, u16, *);
			_basic_arithm_opcode(MULU32, uint32_t, U32, u32, *);
			_basic_arithm_opcode(MULU64, uint64_t, U64, u64, *);
			_basic_arithm_opcode(MULUSIZE, size_t, USize, usize, *);
			_basic_arithm_opcode(MULF32, float, F32, f32, *);
			_basic_arithm_opcode(MULF64, double, F64, f64, *);

			_basic_arithm_opcode(DIVI8, int8_t, I8, i8, /);
			_basic_arithm_opcode(DIVI16, int16_t, I16, i16, /);
			_basic_arithm_opcode(DIVI32, int32_t, I32, i32, /);
			_basic_arithm_opcode(DIVI64, int64_t, I64, i64, /);
			_basic_arithm_opcode(DIVISIZE, ptrdiff_t, ISize, isize, /);
			_basic_arithm_opcode(DIVU8, uint8_t, U8, u8, /);
			_basic_arithm_opcode(DIVU16, uint16_t, U16, u16, /);
			_basic_arithm_opcode(DIVU32, uint32_t, U32, u32, /);
			_basic_arithm_opcode(DIVU64, uint64_t, U64, u64, /);
			_basic_arithm_opcode(DIVUSIZE, size_t, USize, usize, /);
			_basic_arithm_opcode(DIVF32, float, F32, f32, /);
			_basic_arithm_opcode(DIVF64, double, F64, f64, /);

			_basic_arithm_opcode(MODI8, int8_t, I8, i8, %);
			_basic_arithm_opcode(MODI16, int16_t, I16, i16, %);
			_basic_arithm_opcode(MODI32, int32_t, I32, i32, %);
			_basic_arithm_opcode(MODI64, int64_t, I64, i64, %);
			_basic_arithm_opcode(MODISIZE, ptrdiff_t, ISize, isize, %);
			_basic_arithm_opcode(MODU8, uint8_t, U8, u8, %);
			_basic_arithm_opcode(MODU16, uint16_t, U16, u16, %);
			_basic_arithm_opcode(MODU32, uint32_t, U32, u32, %);
			_basic_arithm_opcode(MODU64, uint64_t, U64, u64, %);
			_basic_arithm_opcode(MODUSIZE, size_t, USize, usize, %);
		case Opcode::MODF32: {
			float op0, op1;

			if (cur_ins.flags & INS_OP0_REG) {
				_check_reg_type(cur_ins.reg0_type, F32);
				_check_reg_index(cur_ins.reg0, F32);

				op0 = *_access_typed_reg(cur_ins.reg0, float, F32);
			} else {
				op0 = ins_operand_as_f32(cur_ins.operands[0]);
			}

			if (cur_ins.flags & INS_OP1_REG) {
				_check_reg_type(cur_ins.reg1_type, F32);
				_check_reg_index(cur_ins.reg1, F32);

				op1 = *_access_typed_reg(cur_ins.reg1, float, F32);
			} else {
				op1 = ins_operand_as_f32(cur_ins.operands[1]);
			}

			_check_reg_type(cur_ins.reg_out_type, F32);
			_check_reg_index(cur_ins.reg_out, F32);

			float *output = _access_typed_reg(cur_ins.reg_out, float, F32);
			*output = flib::fmodf(op0, op1);

			break;
		}
		case Opcode::MODF64: {
			double op0, op1;

			if (cur_ins.flags & INS_OP0_REG) {
				_check_reg_type(cur_ins.reg0_type, F64);
				_check_reg_index(cur_ins.reg0, F64);

				op0 = *_access_typed_reg(cur_ins.reg0, double, F64);
			} else {
				op0 = ins_operand_as_f64(cur_ins.operands[0]);
			}

			if (cur_ins.flags & INS_OP1_REG) {
				_check_reg_type(cur_ins.reg1_type, F64);
				_check_reg_index(cur_ins.reg1, F64);

				op1 = *_access_typed_reg(cur_ins.reg1, double, F64);
			} else {
				op1 = ins_operand_as_f64(cur_ins.operands[1]);
			}

			_check_reg_type(cur_ins.reg_out_type, F64);
			_check_reg_index(cur_ins.reg_out, F64);

			double *output = _access_typed_reg(cur_ins.reg_out, double, F64);
			*output = flib::fmodf(op0, op1);

			break;
		}
			_basic_arithm_opcode(ANDI8, int8_t, I8, i8, &);
			_basic_arithm_opcode(ANDI16, int16_t, I16, i16, &);
			_basic_arithm_opcode(ANDI32, int32_t, I32, i32, &);
			_basic_arithm_opcode(ANDI64, int64_t, I64, i64, &);
			_basic_arithm_opcode(ANDISIZE, ptrdiff_t, ISize, isize, &);
			_basic_arithm_opcode(ANDU8, uint8_t, U8, u8, &);
			_basic_arithm_opcode(ANDU16, uint16_t, U16, u16, &);
			_basic_arithm_opcode(ANDU32, uint32_t, U32, u32, &);
			_basic_arithm_opcode(ANDU64, uint64_t, U64, u64, &);
			_basic_arithm_opcode(ANDUSIZE, size_t, USize, usize, &);
			_basic_arithm_opcode(ANDBOOL, bool, Bool, bool, &);

			_basic_arithm_opcode(ORI8, int8_t, I8, i8, |);
			_basic_arithm_opcode(ORI16, int16_t, I16, i16, |);
			_basic_arithm_opcode(ORI32, int32_t, I32, i32, |);
			_basic_arithm_opcode(ORI64, int64_t, I64, i64, |);
			_basic_arithm_opcode(ORISIZE, ptrdiff_t, ISize, isize, |);
			_basic_arithm_opcode(ORU8, uint8_t, U8, u8, |);
			_basic_arithm_opcode(ORU16, uint16_t, U16, u16, |);
			_basic_arithm_opcode(ORU32, uint32_t, U32, u32, |);
			_basic_arithm_opcode(ORU64, uint64_t, U64, u64, |);
			_basic_arithm_opcode(ORUSIZE, size_t, USize, usize, |);
			_basic_arithm_opcode(ORBOOL, bool, Bool, bool, |);

			_basic_arithm_opcode(XORI8, int8_t, I8, i8, ^);
			_basic_arithm_opcode(XORI16, int16_t, I16, i16, ^);
			_basic_arithm_opcode(XORI32, int32_t, I32, i32, ^);
			_basic_arithm_opcode(XORI64, int64_t, I64, i64, ^);
			_basic_arithm_opcode(XORISIZE, ptrdiff_t, ISize, isize, ^);
			_basic_arithm_opcode(XORU8, uint8_t, U8, u8, ^);
			_basic_arithm_opcode(XORU16, uint16_t, U16, u16, ^);
			_basic_arithm_opcode(XORU32, uint32_t, U32, u32, ^);
			_basic_arithm_opcode(XORU64, uint64_t, U64, u64, ^);
			_basic_arithm_opcode(XORUSIZE, size_t, USize, usize, ^);

#define _basic_comparison_opcode(opcode, data_type, slake_type, slake_lower_type, op_token) \
	case Opcode::opcode: {                                                                  \
		data_type op0, op1;                                                                 \
                                                                                            \
		if (cur_ins.flags & INS_OP0_REG) {                                                  \
			_check_reg_type(cur_ins.reg0_type, slake_type);                                 \
			_check_reg_index(cur_ins.reg0, slake_type);                                     \
                                                                                            \
			op0 = *_access_typed_reg(cur_ins.reg0, data_type, slake_type);                  \
		} else {                                                                            \
			op0 = ins_operand_as_##slake_lower_type(cur_ins.operands[0]);                   \
		}                                                                                   \
                                                                                            \
		if (cur_ins.flags & INS_OP1_REG) {                                                  \
			_check_reg_type(cur_ins.reg1_type, slake_type);                                 \
			_check_reg_index(cur_ins.reg1, slake_type);                                     \
                                                                                            \
			op1 = *_access_typed_reg(cur_ins.reg1, data_type, slake_type);                  \
		} else {                                                                            \
			op1 = ins_operand_as_##slake_lower_type(cur_ins.operands[1]);                   \
		}                                                                                   \
                                                                                            \
		_check_reg_type(cur_ins.reg_out_type, Bool);                                        \
		_check_reg_index(cur_ins.reg_out, Bool);                                            \
                                                                                            \
		*_access_typed_reg(cur_ins.reg_out, bool, Bool) = op0 op_token op1;                 \
                                                                                            \
		break;                                                                              \
	}

			_basic_comparison_opcode(EQI8, int8_t, I8, i8, ==);
			_basic_comparison_opcode(EQI16, int16_t, I16, i16, ==);
			_basic_comparison_opcode(EQI32, int32_t, I32, i32, ==);
			_basic_comparison_opcode(EQI64, int64_t, I64, i64, ==);
			_basic_comparison_opcode(EQISIZE, ptrdiff_t, ISize, isize, ==);
			_basic_comparison_opcode(EQU8, uint8_t, U8, u8, ==);
			_basic_comparison_opcode(EQU16, uint16_t, U16, u16, ==);
			_basic_comparison_opcode(EQU32, uint32_t, U32, u32, ==);
			_basic_comparison_opcode(EQU64, uint64_t, U64, u64, ==);
			_basic_comparison_opcode(EQUSIZE, size_t, USize, usize, ==);
			_basic_comparison_opcode(EQBOOL, bool, Bool, bool, ==);
		case Opcode::EQOBJ: {
			Object *op0, *op1;

			_check_reg_type(cur_ins.reg0_type, Object);
			_check_reg_index(cur_ins.reg0, Object);

			op0 = *_access_typed_reg(cur_ins.reg0, Object *, Object);

			_check_reg_type(cur_ins.reg1_type, Object);
			_check_reg_index(cur_ins.reg1, Object);

			op1 = *_access_typed_reg(cur_ins.reg1, Object *, Object);

			_check_reg_type(cur_ins.reg_out_type, Bool);
			_check_reg_index(cur_ins.reg_out, Bool);

			bool *output = _access_typed_reg(cur_ins.reg_out, bool, Bool);
			*output = op0 == op1;

			break;
		}
		case Opcode::EQTYPE: {
			Value *op0, *op1;

			_check_reg_type(cur_ins.reg0_type, Any);
			_check_reg_index(cur_ins.reg0, Any);

			op0 = _access_typed_reg(cur_ins.reg0, Value, Any);

			_check_reg_type(cur_ins.reg1_type, Object);
			_check_reg_index(cur_ins.reg1, Object);

			if (!op0->is_type_name())
				return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(this->get_fixed_alloc()));

			op1 = _access_typed_reg(cur_ins.reg1, Value, Any);

			if (!op1->is_type_name())
				return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(this->get_fixed_alloc()));

			_check_reg_type(cur_ins.reg_out_type, Bool);
			_check_reg_index(cur_ins.reg_out, Bool);

			bool *output = _access_typed_reg(cur_ins.reg_out, bool, Bool);
			*output = op0 == op1;

			break;
		}

			_basic_comparison_opcode(NEQI8, int8_t, I8, i8, !=);
			_basic_comparison_opcode(NEQI16, int16_t, I16, i16, !=);
			_basic_comparison_opcode(NEQI32, int32_t, I32, i32, !=);
			_basic_comparison_opcode(NEQI64, int64_t, I64, i64, !=);
			_basic_comparison_opcode(NEQISIZE, ptrdiff_t, ISize, isize, !=);
			_basic_comparison_opcode(NEQU8, uint8_t, U8, u8, !=);
			_basic_comparison_opcode(NEQU16, uint16_t, U16, u16, !=);
			_basic_comparison_opcode(NEQU32, uint32_t, U32, u32, !=);
			_basic_comparison_opcode(NEQU64, uint64_t, U64, u64, !=);
			_basic_comparison_opcode(NEQUSIZE, size_t, USize, usize, !=);
			_basic_comparison_opcode(NEQBOOL, bool, Bool, bool, !=);
		case Opcode::NEQOBJ: {
			Object *op0, *op1;

			_check_reg_type(cur_ins.reg0_type, Object);
			_check_reg_index(cur_ins.reg0, Object);

			op0 = *_access_typed_reg(cur_ins.reg0, Object *, Object);

			_check_reg_type(cur_ins.reg1_type, Object);
			_check_reg_index(cur_ins.reg1, Object);

			op1 = *_access_typed_reg(cur_ins.reg1, Object *, Object);

			_check_reg_type(cur_ins.reg_out_type, Bool);
			_check_reg_index(cur_ins.reg_out, Bool);

			bool *output = _access_typed_reg(cur_ins.reg_out, bool, Bool);
			*output = op0 != op1;

			break;
		}
		case Opcode::NEQTYPE: {
			Value *op0, *op1;

			_check_reg_type(cur_ins.reg0_type, Any);
			_check_reg_index(cur_ins.reg0, Any);

			op0 = _access_typed_reg(cur_ins.reg0, Value, Any);

			_check_reg_type(cur_ins.reg1_type, Object);
			_check_reg_index(cur_ins.reg1, Object);

			if (!op0->is_type_name())
				return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(this->get_fixed_alloc()));

			op1 = _access_typed_reg(cur_ins.reg1, Value, Any);

			if (!op1->is_type_name())
				return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(this->get_fixed_alloc()));

			_check_reg_type(cur_ins.reg_out_type, Bool);
			_check_reg_index(cur_ins.reg_out, Bool);

			bool *output = _access_typed_reg(cur_ins.reg_out, bool, Bool);
			*output = op0 != op1;

			break;
		}

			_basic_comparison_opcode(LTI8, int8_t, I8, i8, <);
			_basic_comparison_opcode(LTI16, int16_t, I16, i16, <);
			_basic_comparison_opcode(LTI32, int32_t, I32, i32, <);
			_basic_comparison_opcode(LTI64, int64_t, I64, i64, <);
			_basic_comparison_opcode(LTISIZE, ptrdiff_t, ISize, isize, <);
			_basic_comparison_opcode(LTU8, uint8_t, U8, u8, <);
			_basic_comparison_opcode(LTU16, uint16_t, U16, u16, <);
			_basic_comparison_opcode(LTU32, uint32_t, U32, u32, <);
			_basic_comparison_opcode(LTU64, uint64_t, U64, u64, <);
			_basic_comparison_opcode(LTUSIZE, size_t, USize, usize, <);
			_basic_comparison_opcode(LTF32, float, F32, f32, <);
			_basic_comparison_opcode(LTF64, double, F64, f64, <);

			_basic_comparison_opcode(GTI8, int8_t, I8, i8, >);
			_basic_comparison_opcode(GTI16, int16_t, I16, i16, >);
			_basic_comparison_opcode(GTI32, int32_t, I32, i32, >);
			_basic_comparison_opcode(GTI64, int64_t, I64, i64, >);
			_basic_comparison_opcode(GTISIZE, ptrdiff_t, ISize, isize, >);
			_basic_comparison_opcode(GTU8, uint8_t, U8, u8, >);
			_basic_comparison_opcode(GTU16, uint16_t, U16, u16, >);
			_basic_comparison_opcode(GTU32, uint32_t, U32, u32, >);
			_basic_comparison_opcode(GTU64, uint64_t, U64, u64, >);
			_basic_comparison_opcode(GTUSIZE, size_t, USize, usize, >);
			_basic_comparison_opcode(GTF32, float, F32, f32, >);
			_basic_comparison_opcode(GTF64, double, F64, f64, >);

			_basic_comparison_opcode(LTEQI8, int8_t, I8, i8, <=);
			_basic_comparison_opcode(LTEQI16, int16_t, I16, i16, <=);
			_basic_comparison_opcode(LTEQI32, int32_t, I32, i32, <=);
			_basic_comparison_opcode(LTEQI64, int64_t, I64, i64, <=);
			_basic_comparison_opcode(LTEQISIZE, ptrdiff_t, ISize, isize, <=);
			_basic_comparison_opcode(LTEQU8, uint8_t, U8, u8, <=);
			_basic_comparison_opcode(LTEQU16, uint16_t, U16, u16, <=);
			_basic_comparison_opcode(LTEQU32, uint32_t, U32, u32, <=);
			_basic_comparison_opcode(LTEQU64, uint64_t, U64, u64, <=);
			_basic_comparison_opcode(LTEQUSIZE, size_t, USize, usize, <=);
			_basic_comparison_opcode(LTEQF32, float, F32, f32, <=);
			_basic_comparison_opcode(LTEQF64, double, F64, f64, <=);

			_basic_comparison_opcode(GTEQI8, int8_t, I8, i8, >=);
			_basic_comparison_opcode(GTEQI16, int16_t, I16, i16, >=);
			_basic_comparison_opcode(GTEQI32, int32_t, I32, i32, >=);
			_basic_comparison_opcode(GTEQI64, int64_t, I64, i64, >=);
			_basic_comparison_opcode(GTEQISIZE, ptrdiff_t, ISize, isize, >=);
			_basic_comparison_opcode(GTEQU8, uint8_t, U8, u8, >=);
			_basic_comparison_opcode(GTEQU16, uint16_t, U16, u16, >=);
			_basic_comparison_opcode(GTEQU32, uint32_t, U32, u32, >=);
			_basic_comparison_opcode(GTEQU64, uint64_t, U64, u64, >=);
			_basic_comparison_opcode(GTEQUSIZE, size_t, USize, usize, >=);
			_basic_comparison_opcode(GTEQF32, float, F32, f32, >=);
			_basic_comparison_opcode(GTEQF64, double, F64, f64, >=);

			// TODO: Implement left-shift and right-shift.

			// TODO: Implement the three-way comparison.

			// TODO: Implement the unary operations.

		case Opcode::LOAD: {
			_check_reg_type(cur_ins.reg_out_type, Any);
			_check_reg_index(cur_ins.reg_out, Any);

			RegIndex dest_reg = cur_ins.reg_out;

			Value *dest = _access_typed_reg(dest_reg, Value, Any);

			_check_reg_type(cur_ins.reg0, Any);
			_check_reg_index(cur_ins.reg0, Any);

			Value *ref_obj = _access_typed_reg(cur_ins.reg0, Value, Any);

			if (!ref_obj->is_reference())
				return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(this->get_fixed_alloc()));

			auto &ref = ref_obj->get_reference();

			if (!ref.is_object_ref())
				return alloc_oom_error_if_alloc_failed(NullRefError::alloc(this->get_fixed_alloc()));

			Object *obj = ref.get_object_ref();
			if ((!obj) || (obj->get_object_kind() != ObjectKind::IdRef))
				return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(this->get_fixed_alloc()));

			Reference entity_ref;

			SLAKE_RETURN_IF_EXCEPT(resolve_id_ref(static_cast<IdRefObject *>(obj), entity_ref));

			if SLAKE_UNLIKELY (entity_ref.kind == ReferenceKind::Invalid)
				// TODO: Use a proper one instead.
				return alloc_oom_error_if_alloc_failed(ReferencedMemberNotFoundError::alloc(get_fixed_alloc(), static_cast<IdRefObject *>(obj)));

			*dest = entity_ref;
			break;
		}
		case Opcode::RLOAD: {
			_check_reg_type(cur_ins.reg_out_type, Any);
			_check_reg_index(cur_ins.reg_out, Any);

			RegIndex dest_reg = cur_ins.reg_out;

			Value *dest = _access_typed_reg(dest_reg, Value, Any);

			_check_reg_type(cur_ins.reg0, Any);
			_check_reg_index(cur_ins.reg0, Any);

			Value *base_obj = _access_typed_reg(cur_ins.reg0, Value, Any);

			if ((!base_obj->is_reference()) ||
				(base_obj->is_null()) || (!base_obj->get_reference().is_object_ref()))
				return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(this->get_fixed_alloc()));

			_check_reg_type(cur_ins.reg1, Any);
			_check_reg_index(cur_ins.reg1, Any);

			Value *ref_obj = _access_typed_reg(cur_ins.reg1, Value, Any);

			if (!ref_obj->is_reference())
				return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(this->get_fixed_alloc()));

			auto &ref = ref_obj->get_reference();

			if (!ref.is_object_ref())
				return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(this->get_fixed_alloc()));

			Object *obj = ref.get_object_ref();
			if ((!obj) || (obj->get_object_kind() != ObjectKind::IdRef))
				return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(this->get_fixed_alloc()));

			Reference entity_ref;

			SLAKE_RETURN_IF_EXCEPT(resolve_id_ref(static_cast<IdRefObject *>(obj), entity_ref, base_obj->get_reference().get_object_ref()));

			if SLAKE_UNLIKELY (entity_ref.kind == ReferenceKind::Invalid)
				// TODO: Use a proper one instead.
				return alloc_oom_error_if_alloc_failed(ReferencedMemberNotFoundError::alloc(get_fixed_alloc(), static_cast<IdRefObject *>(obj)));

			*dest = entity_ref;

			break;
		}
		case Opcode::LCURFN: {
			_check_reg_type(cur_ins.reg_out_type, Any);
			_check_reg_index(cur_ins.reg_out, Any);

			RegIndex dest_reg = cur_ins.reg_out;

			*_access_typed_reg(dest_reg, Value, Any) = cur_major_frame->cur_fn;
			break;
		}

		case Opcode::LARG: {
			_check_reg_type(cur_ins.reg_out_type, Any);
			_check_reg_index(cur_ins.reg_out, Any);

			RegIndex dest_reg = cur_ins.reg_out;

			Value *dest = _access_typed_reg(dest_reg, Value, Any);

			uint32_t arg_index = ins_operand_as_u32(cur_ins.operands[0]);

			*dest = Reference(ArgRef(nullptr, UINT32_MAX));
			SLAKE_RETURN_IF_EXCEPT(larg(&context->get_context(), cur_major_frame, this, arg_index, dest->get_reference()));

			break;
		}

			// TODO: Implement LAPARG.

		case Opcode::LVAR: {
			_check_reg_type(cur_ins.reg_out_type, Any);
			_check_reg_index(cur_ins.reg_out, Any);

			RegIndex dest_reg = cur_ins.reg_out;

			Value *dest = _access_typed_reg(dest_reg, Value, Any);

			_check_reg_type(cur_ins.reg0_type, Any);
			_check_reg_index(cur_ins.reg0, Any);

			Value *var_type = _access_typed_reg(cur_ins.reg0, Value, Any);

			if (!var_type->is_type_name())
				return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(this->get_fixed_alloc()));

			Reference entity_ref;
			SLAKE_RETURN_IF_EXCEPT(_add_local_var(&context->_context, cur_major_frame, var_type->get_type_name(), dest_reg, entity_ref));
			*dest = entity_ref;
			break;
		}

			// TODO: Implement ALLOCA.

		case Opcode::ENTER: {
			size_t prev_stack_top = context->get_context().stack_top;

			if (!context->_context.aligned_stack_alloc(sizeof(MinorFrame), alignof(MinorFrame)))
				return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));

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
			uint32_t level = ins_operand_as_u32(cur_ins.operands[0]);
			for (uint32_t i = 0; i < level; ++i) {
				MinorFrame *mf = _fetch_minor_frame(&context->_context, cur_major_frame, cur_major_frame->resumable_context_data.off_cur_minor_frame);

				if (mf->off_last_minor_frame == SIZE_MAX)
					return alloc_oom_error_if_alloc_failed(FrameBoundaryExceededError::alloc(get_fixed_alloc()));

				// Invalidate alloca records to prevent the runtime from dangling references.
				size_t off_alloca_record = mf->off_alloca_records;
				while (off_alloca_record != SIZE_MAX) {
					AllocaRecord *ar = _fetch_alloca_record(&context->get_context(), cur_major_frame, off_alloca_record);

					_check_reg_index(ar->def_reg, Any);
					*_access_typed_reg(ar->def_reg, Value, Any) = InvalidValueState{};

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
			Value *arg;

			_check_reg_type(cur_ins.reg0_type, Any);
			_check_reg_index(cur_ins.reg0, Any);

			arg = _access_typed_reg(cur_ins.reg0, Value, Any);

			/* if (cur_major_frame->resumable_context_data.num_next_args) {
				if (cur_major_frame->resumable_context_data.off_next_args + sizeof(Value) * cur_major_frame->resumable_context_data.num_next_args != context->get_context().stack_top)
					std::terminate();
			}*/
			size_t prev_stack_top = cur_major_frame->cur_coroutine ? context->get_context().stack_top - cur_major_frame->cur_coroutine->off_stack_top : context->get_context().stack_top;
			if (void *p = context->get_context().aligned_stack_alloc(sizeof(Value), alignof(Value)); p) {
				*static_cast<Value *>(p) = *arg;
			} else
				return alloc_oom_error_if_alloc_failed(StackOverflowError::alloc(get_fixed_alloc()));
			const size_t new_off = cur_major_frame->cur_coroutine ? context->get_context().stack_top - cur_major_frame->cur_coroutine->off_stack_top : context->get_context().stack_top;
			ResumableContextData &resumable_context_data = cur_major_frame->resumable_context_data;
			if (resumable_context_data.num_next_args) {
				if (new_off - resumable_context_data.off_next_args != sizeof(Value))
					// TODO: Use a proper one.
					return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			} else {
				resumable_context_data.off_next_args_begin = prev_stack_top;
			}
			resumable_context_data.off_next_args = new_off;
			++resumable_context_data.num_next_args;
			break;
		}
		case Opcode::CALL: {
			FnOverloadingObject *fn;

			{
				Value *arg;

				_check_reg_type(cur_ins.reg0_type, Any);
				_check_reg_index(cur_ins.reg0, Any);

				arg = _access_typed_reg(cur_ins.reg0, Value, Any);

				if ((!arg->is_reference()) || (arg->is_null()) || (!arg->get_reference().is_object_ref()))
					return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));

				if (auto obj = arg->get_reference().get_object_ref(); obj->get_object_kind() == ObjectKind::FnOverloading)
					fn = static_cast<FnOverloadingObject *>(obj);
				else
					return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			if (!fn)
				return alloc_oom_error_if_alloc_failed(NullRefError::alloc(get_fixed_alloc()));

			RegIndex output = INVALID_REG;

			if (cur_ins.reg_out != INVALID_REG) {
				_check_reg_type(cur_ins.reg_out_type, Any);
				_check_reg_index(cur_ins.reg_out, Any);
				output = cur_ins.reg_out;
			}

			ResumableContextData &resumable_context_data = cur_major_frame->resumable_context_data;

			/*if (fn->return_type.type_id == TypeId::StructInstance) {
				if (output != INVALID_REG) {
					// TODO: Untested!!!
					Reference alloca_ref;

					SLAKE_RETURN_IF_EXCEPT(_add_local_var(&context->get_context(), cur_major_frame, fn->return_type, output, alloca_ref));
					SLAKE_RETURN_IF_EXCEPT(_set_register_value(this, cur_frame_regs_ptr, cur_major_frame, output, alloca_ref));

					SLAKE_RETURN_IF_EXCEPT(_create_new_major_frame(
						context,
						nullptr,
						fn,
						nullptr,
						cur_major_frame->cur_coroutine
							? resumable_context_data.off_next_args + cur_major_frame->cur_coroutine->off_stack_top
							: resumable_context_data.off_next_args,
						cur_major_frame->resumable_context_data.num_next_args,
						INVALID_REG,
						&alloca_ref));
				} else
					SLAKE_RETURN_IF_EXCEPT(_create_new_major_frame(
						context,
						nullptr,
						fn,
						nullptr,
						cur_major_frame->cur_coroutine
							? resumable_context_data.off_next_args + cur_major_frame->cur_coroutine->off_stack_top
							: resumable_context_data.off_next_args,
						resumable_context_data.num_next_args,
						output,
						nullptr));
			} else*/
			{
				SLAKE_RETURN_IF_EXCEPT(_create_new_major_frame(
					context,
					nullptr,
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

			if (fn != cur_major_frame->cur_fn) {
				if (fn->overloading_kind != cur_major_frame->cur_fn->overloading_kind)
					context_changes_out = ContextChangeType::FnKindChanged;
				else
					context_changes_out = ContextChangeType::FnChanged;
			} else
				context_changes_out = ContextChangeType::MajorFrameChanged;
			break;
		}
		case Opcode::MCALL: {
			FnOverloadingObject *fn;
			Object *this_object = nullptr;

			{
				Value *arg;

				_check_reg_type(cur_ins.reg0_type, Any);
				_check_reg_index(cur_ins.reg0, Any);

				arg = _access_typed_reg(cur_ins.reg0, Value, Any);

				if ((!arg->is_reference()) || (arg->is_null()) || (!arg->get_reference().is_object_ref()))
					return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));

				if (auto obj = arg->get_reference().get_object_ref(); obj->get_object_kind() == ObjectKind::FnOverloading)
					fn = static_cast<FnOverloadingObject *>(obj);
				else
					return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			{
				Value *arg;

				_check_reg_type(cur_ins.reg1_type, Any);
				_check_reg_index(cur_ins.reg1, Any);

				arg = _access_typed_reg(cur_ins.reg1, Value, Any);

				if ((!arg->is_reference()) || (arg->is_null()) || (!arg->get_reference().is_object_ref()))
					return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));

				this_object = arg->get_reference().get_object_ref();
			}

			if (!fn)
				return alloc_oom_error_if_alloc_failed(NullRefError::alloc(get_fixed_alloc()));

			RegIndex output = INVALID_REG;

			if (cur_ins.reg_out != INVALID_REG) {
				_check_reg_type(cur_ins.reg_out_type, Any);
				_check_reg_index(cur_ins.reg_out, Any);
				output = cur_ins.reg_out;
			}

			ResumableContextData &resumable_context_data = cur_major_frame->resumable_context_data;

			/*if (fn->return_type.type_id == TypeId::StructInstance) {
				if (output != INVALID_REG) {
					// TODO: Untested!!!
					Reference alloca_ref;

					SLAKE_RETURN_IF_EXCEPT(_add_local_var(&context->get_context(), cur_major_frame, fn->return_type, output, alloca_ref));
					SLAKE_RETURN_IF_EXCEPT(_set_register_value(this, cur_frame_regs_ptr, cur_major_frame, output, alloca_ref));

					SLAKE_RETURN_IF_EXCEPT(_create_new_major_frame(
						context,
						this_object,
						fn,
						nullptr,
						cur_major_frame->cur_coroutine
							? resumable_context_data.off_next_args + cur_major_frame->cur_coroutine->off_stack_top
							: resumable_context_data.off_next_args,
						cur_major_frame->resumable_context_data.num_next_args,
						INVALID_REG,
						&alloca_ref));
				} else
					SLAKE_RETURN_IF_EXCEPT(_create_new_major_frame(
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
			} else */
			{
				SLAKE_RETURN_IF_EXCEPT(_create_new_major_frame(
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

			if (fn != cur_major_frame->cur_fn) {
				if (fn->overloading_kind != cur_major_frame->cur_fn->overloading_kind)
					context_changes_out = ContextChangeType::FnKindChanged;
				else
					context_changes_out = ContextChangeType::FnChanged;
			} else
				context_changes_out = ContextChangeType::MajorFrameChanged;
			break;
		}
		case Opcode::CTORCALL: {
			FnOverloadingObject *fn;
			Object *this_object = nullptr;

			{
				Value *arg;

				_check_reg_type(cur_ins.reg0_type, Any);
				_check_reg_index(cur_ins.reg0, Any);

				arg = _access_typed_reg(cur_ins.reg0, Value, Any);

				if ((!arg->is_reference()) || (arg->is_null()) || (!arg->get_reference().is_object_ref()))
					return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));

				if (auto obj = arg->get_reference().get_object_ref(); obj->get_object_kind() == ObjectKind::FnOverloading)
					fn = static_cast<FnOverloadingObject *>(obj);
				else
					return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}

			{
				Value *arg;

				_check_reg_type(cur_ins.reg1_type, Any);
				_check_reg_index(cur_ins.reg1, Any);

				arg = _access_typed_reg(cur_ins.reg1, Value, Any);

				if ((!arg->is_reference()) || (arg->is_null()) || (!arg->get_reference().is_object_ref()))
					return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));

				this_object = arg->get_reference().get_object_ref();
			}

			if (!fn)
				return alloc_oom_error_if_alloc_failed(NullRefError::alloc(get_fixed_alloc()));

			ResumableContextData &resumable_context_data = cur_major_frame->resumable_context_data;

			SLAKE_RETURN_IF_EXCEPT(_create_new_major_frame(
				context,
				this_object,
				fn,
				nullptr,
				cur_major_frame->cur_coroutine
					? resumable_context_data.off_next_args + cur_major_frame->cur_coroutine->off_stack_top
					: resumable_context_data.off_next_args,
				resumable_context_data.num_next_args,
				INVALID_REG,
				nullptr));

			resumable_context_data.off_next_args = SIZE_MAX;
			resumable_context_data.num_next_args = 0;

			if (fn != cur_major_frame->cur_fn) {
				if (fn->overloading_kind != cur_major_frame->cur_fn->overloading_kind)
					context_changes_out = ContextChangeType::FnKindChanged;
				else
					context_changes_out = ContextChangeType::FnChanged;
			} else
				context_changes_out = ContextChangeType::MajorFrameChanged;
			break;
		}
		case Opcode::RETVOID: {
			const RegIndex return_value_out_reg = cur_major_frame->return_value_out_reg;

			if SLAKE_UNLIKELY (return_value_out_reg != INVALID_REG)
				return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));

			_leave_major_frame(&context->get_context());

			context_changes_out = ContextChangeType::FnKindChanged;
			return {};
		}
		case Opcode::RET: {
			const RegIndex return_value_out_reg = cur_major_frame->return_value_out_reg;

			if (return_value_out_reg != INVALID_REG) {
				TypeRef return_type = cur_major_frame->cur_fn->return_type;

				_check_reg_type(cur_ins.reg0_type, Any);
				_check_reg_index(cur_ins.reg0, Any);
				const Value *return_value = _access_typed_reg(cur_ins.reg0, Value, Any);

				if (!is_compatible(return_type, *return_value))
					// TODO: Handle this.
					std::terminate();
				/*if (return_type.type_id == TypeId::StructInstance) {
					// TODO: Check if the return structure reference member is invalid (which means the caller does not want a return value).
					write_var(cur_major_frame->return_struct_ref, *return_value);
				} else*/
				{
					MajorFrame *mjf = _fetch_major_frame(&context->get_context(), cur_major_frame->off_prev_frame);

					if (cur_major_frame->return_value_out_reg >= ol->num_registers[static_cast<size_t>(InsRegType::Any)])
						return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(this->get_fixed_alloc()));

					*_access_nonlocal_typed_reg(mjf, cur_major_frame->return_value_out_reg, Value, Any) = *return_value;
				}
			}
			_leave_major_frame(&context->get_context());

			context_changes_out = ContextChangeType::FnKindChanged;
			return {};
		}

		case Opcode::LTHIS: {
			_check_reg_type(cur_ins.reg_out_type, Any);
			_check_reg_index(cur_ins.reg_out, Any);
			*_access_typed_reg(cur_ins.reg_out, Value, Any) = cur_major_frame->resumable_context_data.this_object;
			break;
		}

		case Opcode::NEW: {
			_check_reg_type(cur_ins.reg0_type, Any);
			_check_reg_index(cur_ins.reg0, Any);
			const Value *new_type = _access_typed_reg(cur_ins.reg0, Value, Any);

			if (!new_type->is_type_name())
				return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(this->get_fixed_alloc()));

			TypeRef type = new_type->get_type_name();

			_check_reg_type(cur_ins.reg_out_type, Any);
			_check_reg_index(cur_ins.reg_out, Any);
			Value *output = _access_typed_reg(cur_ins.reg0, Value, Any);

			switch (type.type_id) {
				case TypeId::Instance: {
					ClassObject *cls = static_cast<ClassObject *>((type.get_custom_type_def())->type_object);
					HostObjectRef<InstanceObject> instance = new_class_instance(cls, 0);
					if (!instance)
						// TODO: Return more detail exceptions.
						return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
					*output = instance.get();
					break;
				}
				default:
					return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			break;
		}
		// TODO: Implement ARRNEW.

		// TODO: Implement THROW, PUSHEH and LEXCEPT.

#define _arithm_cast_opcode(opcode, data_type, slake_type, slake_lower_type)                                      \
	case Opcode::opcode: {                                                                                        \
		_check_reg_type(cur_ins.reg_out_type, slake_type);                                                        \
		_check_reg_index(cur_ins.reg_out, slake_type);                                                            \
                                                                                                                  \
		switch (static_cast<InsRegType>(cur_ins.reg0_type)) {                                                     \
			case InsRegType::I8:                                                                                  \
				_check_reg_type(cur_ins.reg0_type, I8);                                                           \
				_check_reg_index(cur_ins.reg0, I8);                                                               \
                                                                                                                  \
				*_access_typed_reg(cur_ins.reg_out, data_type, slake_type) =                                      \
					static_cast<data_type>(*_access_typed_reg(cur_ins.reg0, int8_t, I8));                         \
				break;                                                                                            \
			case InsRegType::I16:                                                                                 \
				_check_reg_type(cur_ins.reg0_type, I16);                                                          \
				_check_reg_index(cur_ins.reg0, I16);                                                              \
                                                                                                                  \
				*_access_typed_reg(cur_ins.reg_out, data_type, slake_type) =                                      \
					static_cast<data_type>(*_access_typed_reg(cur_ins.reg0, int16_t, I16));                       \
				break;                                                                                            \
			case InsRegType::I32:                                                                                 \
				_check_reg_type(cur_ins.reg0_type, I32);                                                          \
				_check_reg_index(cur_ins.reg0, I32);                                                              \
                                                                                                                  \
				*_access_typed_reg(cur_ins.reg_out, data_type, slake_type) =                                      \
					static_cast<data_type>(*_access_typed_reg(cur_ins.reg0, int32_t, I32));                       \
				break;                                                                                            \
			case InsRegType::I64:                                                                                 \
				_check_reg_type(cur_ins.reg0_type, I64);                                                          \
				_check_reg_index(cur_ins.reg0, I64);                                                              \
                                                                                                                  \
				*_access_typed_reg(cur_ins.reg_out, data_type, slake_type) =                                      \
					static_cast<data_type>(*_access_typed_reg(cur_ins.reg0, int64_t, I64));                       \
				break;                                                                                            \
			case InsRegType::ISize:                                                                               \
				_check_reg_type(cur_ins.reg0_type, ISize);                                                        \
				_check_reg_index(cur_ins.reg0, ISize);                                                            \
                                                                                                                  \
				*_access_typed_reg(cur_ins.reg_out, data_type, slake_type) =                                      \
					static_cast<data_type>(*_access_typed_reg(cur_ins.reg0, ptrdiff_t, ISize));                   \
				break;                                                                                            \
			case InsRegType::U8:                                                                                  \
				_check_reg_type(cur_ins.reg0_type, U8);                                                           \
				_check_reg_index(cur_ins.reg0, U8);                                                               \
                                                                                                                  \
				*_access_typed_reg(cur_ins.reg_out, data_type, slake_type) =                                      \
					static_cast<data_type>(*_access_typed_reg(cur_ins.reg0, uint8_t, U8));                        \
				break;                                                                                            \
			case InsRegType::U16:                                                                                 \
				_check_reg_type(cur_ins.reg0_type, U16);                                                          \
				_check_reg_index(cur_ins.reg0, U16);                                                              \
                                                                                                                  \
				*_access_typed_reg(cur_ins.reg_out, data_type, slake_type) =                                      \
					static_cast<data_type>(*_access_typed_reg(cur_ins.reg0, uint16_t, U16));                      \
				break;                                                                                            \
			case InsRegType::U32:                                                                                 \
				_check_reg_type(cur_ins.reg0_type, U32);                                                          \
				_check_reg_index(cur_ins.reg0, U32);                                                              \
                                                                                                                  \
				*_access_typed_reg(cur_ins.reg_out, data_type, slake_type) =                                      \
					static_cast<data_type>(*_access_typed_reg(cur_ins.reg0, uint32_t, U32));                      \
				break;                                                                                            \
			case InsRegType::U64:                                                                                 \
				_check_reg_type(cur_ins.reg0_type, U64);                                                          \
				_check_reg_index(cur_ins.reg0, U64);                                                              \
                                                                                                                  \
				*_access_typed_reg(cur_ins.reg_out, data_type, slake_type) =                                      \
					static_cast<data_type>(*_access_typed_reg(cur_ins.reg0, uint64_t, U64));                      \
				break;                                                                                            \
			case InsRegType::USize:                                                                               \
				_check_reg_type(cur_ins.reg0_type, USize);                                                        \
				_check_reg_index(cur_ins.reg0, USize);                                                            \
                                                                                                                  \
				*_access_typed_reg(cur_ins.reg_out, data_type, slake_type) =                                      \
					static_cast<data_type>(*_access_typed_reg(cur_ins.reg0, size_t, USize));                      \
				break;                                                                                            \
			case InsRegType::F32:                                                                                 \
				_check_reg_type(cur_ins.reg0_type, F32);                                                          \
				_check_reg_index(cur_ins.reg0, F32);                                                              \
                                                                                                                  \
				*_access_typed_reg(cur_ins.reg_out, data_type, slake_type) =                                      \
					static_cast<data_type>(*_access_typed_reg(cur_ins.reg0, float, F32));                         \
				break;                                                                                            \
			case InsRegType::F64:                                                                                 \
				_check_reg_type(cur_ins.reg0_type, F64);                                                          \
				_check_reg_index(cur_ins.reg0, F64);                                                              \
                                                                                                                  \
				*_access_typed_reg(cur_ins.reg_out, data_type, slake_type) =                                      \
					static_cast<data_type>(*_access_typed_reg(cur_ins.reg0, double, F64));                        \
				break;                                                                                            \
			case InsRegType::Bool:                                                                                \
				_check_reg_type(cur_ins.reg0_type, Bool);                                                         \
				_check_reg_index(cur_ins.reg0, Bool);                                                             \
                                                                                                                  \
				*_access_typed_reg(cur_ins.reg_out, data_type, slake_type) =                                      \
					static_cast<data_type>(*_access_typed_reg(cur_ins.reg0, bool, Bool));                         \
				break;                                                                                            \
			case InsRegType::Any: {                                                                               \
				_check_reg_type(cur_ins.reg0_type, Any);                                                          \
				_check_reg_index(cur_ins.reg0, Any);                                                              \
                                                                                                                  \
				const Value *v = _access_typed_reg(cur_ins.reg0, Value, Any);                                     \
				if (!v->is_##slake_lower_type())                                                                  \
					return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(this->get_fixed_alloc())); \
				*_access_typed_reg(cur_ins.reg_out, data_type, slake_type) = v->get_##slake_lower_type();         \
				break;                                                                                            \
			}                                                                                                     \
			default:                                                                                              \
				return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));           \
		}                                                                                                         \
		break;                                                                                                    \
	}
			_arithm_cast_opcode(CASTI8, int8_t, I8, i8);
			_arithm_cast_opcode(CASTI16, int16_t, I16, i16);
			_arithm_cast_opcode(CASTI32, int32_t, I32, i32);
			_arithm_cast_opcode(CASTI64, int64_t, I64, i64);
			_arithm_cast_opcode(CASTISIZE, ptrdiff_t, ISize, isize);
			_arithm_cast_opcode(CASTU8, uint8_t, U8, u8);
			_arithm_cast_opcode(CASTU16, uint16_t, U16, u16);
			_arithm_cast_opcode(CASTU32, uint32_t, U32, u32);
			_arithm_cast_opcode(CASTU64, uint64_t, U64, u64);
			_arithm_cast_opcode(CASTUSIZE, size_t, USize, usize);
			_arithm_cast_opcode(CASTF32, float, F32, f32);
			_arithm_cast_opcode(CASTF64, double, F64, f64);
			_arithm_cast_opcode(CASTBOOL, bool, Bool, bool);
		case Opcode::CASTOBJ: {
			_check_reg_type(cur_ins.reg_out_type, Any);
			_check_reg_index(cur_ins.reg_out, Any);

			switch (static_cast<InsRegType>(cur_ins.reg0_type)) {
				case InsRegType::Object: {
					_check_reg_type(cur_ins.reg0_type, Any);
					_check_reg_index(cur_ins.reg0, Any);

					*_access_typed_reg(cur_ins.reg_out, Value, Any) = *_access_typed_reg(cur_ins.reg0, Object *, Object);
					break;
				}
				case InsRegType::Any: {
					_check_reg_type(cur_ins.reg0_type, Any);
					_check_reg_index(cur_ins.reg0, Any);

					const Value *v = _access_typed_reg(cur_ins.reg0, Value, Any);
					if ((!v->is_reference()) || (!v->get_reference().is_object_ref()))
						return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(this->get_fixed_alloc()));
					*_access_typed_reg(cur_ins.reg_out, Value, Any) = v->get_reference().get_object_ref();
					break;
				}
				default:
					return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			break;
		}
		case Opcode::CASTANY: {
			_check_reg_type(cur_ins.reg_out_type, Any);
			_check_reg_index(cur_ins.reg_out, Any);

			switch (static_cast<InsRegType>(cur_ins.reg0_type)) {
				case InsRegType::I8:
					_check_reg_type(cur_ins.reg0_type, I8);
					_check_reg_index(cur_ins.reg0, I8);

					*_access_typed_reg(cur_ins.reg_out, Value, Any) =
						*_access_typed_reg(cur_ins.reg0, int8_t, I8);
					break;
				case InsRegType::I16:
					_check_reg_type(cur_ins.reg0_type, I16);
					_check_reg_index(cur_ins.reg0, I16);

					*_access_typed_reg(cur_ins.reg_out, Value, Any) =
						(*_access_typed_reg(cur_ins.reg0, int16_t, I16));
					break;
				case InsRegType::I32:
					_check_reg_type(cur_ins.reg0_type, I32);
					_check_reg_index(cur_ins.reg0, I32);

					*_access_typed_reg(cur_ins.reg_out, Value, Any) =
						(*_access_typed_reg(cur_ins.reg0, int32_t, I32));
					break;
				case InsRegType::I64:
					_check_reg_type(cur_ins.reg0_type, I64);
					_check_reg_index(cur_ins.reg0, I64);

					*_access_typed_reg(cur_ins.reg_out, Value, Any) =
						(*_access_typed_reg(cur_ins.reg0, int64_t, I64));
					break;
				case InsRegType::ISize:
					_check_reg_type(cur_ins.reg0_type, ISize);
					_check_reg_index(cur_ins.reg0, ISize);

					*_access_typed_reg(cur_ins.reg_out, Value, Any) =
						ExplicitISize{ *_access_typed_reg(cur_ins.reg0, ptrdiff_t, ISize) };
					break;
				case InsRegType::U8:
					_check_reg_type(cur_ins.reg0_type, U8);
					_check_reg_index(cur_ins.reg0, U8);

					*_access_typed_reg(cur_ins.reg_out, Value, Any) =
						(*_access_typed_reg(cur_ins.reg0, uint8_t, U8));
					break;
				case InsRegType::U16:
					_check_reg_type(cur_ins.reg0_type, U16);
					_check_reg_index(cur_ins.reg0, U16);

					*_access_typed_reg(cur_ins.reg_out, Value, Any) =
						(*_access_typed_reg(cur_ins.reg0, uint16_t, U16));
					break;
				case InsRegType::U32:
					_check_reg_type(cur_ins.reg0_type, U32);
					_check_reg_index(cur_ins.reg0, U32);

					*_access_typed_reg(cur_ins.reg_out, Value, Any) =
						(*_access_typed_reg(cur_ins.reg0, uint32_t, U32));
					break;
				case InsRegType::U64:
					_check_reg_type(cur_ins.reg0_type, U64);
					_check_reg_index(cur_ins.reg0, U64);

					*_access_typed_reg(cur_ins.reg_out, Value, Any) =
						(*_access_typed_reg(cur_ins.reg0, uint64_t, U64));
					break;
				case InsRegType::USize:
					_check_reg_type(cur_ins.reg0_type, USize);
					_check_reg_index(cur_ins.reg0, USize);

					*_access_typed_reg(cur_ins.reg_out, Value, Any) =
						ExplicitUSize{ (*_access_typed_reg(cur_ins.reg0, size_t, USize)) };
					break;
				case InsRegType::F32:
					_check_reg_type(cur_ins.reg0_type, F32);
					_check_reg_index(cur_ins.reg0, F32);

					*_access_typed_reg(cur_ins.reg_out, Value, Any) =
						(*_access_typed_reg(cur_ins.reg0, float, F32));
					break;
				case InsRegType::F64:
					_check_reg_type(cur_ins.reg0_type, F64);
					_check_reg_index(cur_ins.reg0, F64);

					*_access_typed_reg(cur_ins.reg_out, Value, Any) =
						(*_access_typed_reg(cur_ins.reg0, double, F64));
					break;
				case InsRegType::Bool:
					_check_reg_type(cur_ins.reg0_type, Bool);
					_check_reg_index(cur_ins.reg0, Bool);

					*_access_typed_reg(cur_ins.reg_out, Value, Any) =
						(*_access_typed_reg(cur_ins.reg0, bool, Bool));
					break;
				case InsRegType::Object: {
					_check_reg_type(cur_ins.reg0_type, Any);
					_check_reg_index(cur_ins.reg0, Any);

					*_access_typed_reg(cur_ins.reg_out, Value, Any) = *_access_typed_reg(cur_ins.reg0, Object *, Object);
					break;
				}
				case InsRegType::Any: {
					_check_reg_type(cur_ins.reg0_type, Any);
					_check_reg_index(cur_ins.reg0, Any);

					const Value *v = _access_typed_reg(cur_ins.reg0, Value, Any);
					*_access_typed_reg(cur_ins.reg_out, Value, Any) = *v;
					break;
				}
				default:
					return alloc_oom_error_if_alloc_failed(InvalidOperandsError::alloc(get_fixed_alloc()));
			}
			break;
		}

		default:
			return alloc_oom_error_if_alloc_failed(InvalidOpcodeError::alloc(this->get_fixed_alloc(), cur_ins.opcode));
	}

	++cur_major_frame->resumable_context_data.cur_ins;
	return {};
}

SLAKE_API InternalExceptionPointer Runtime::exec_context(ContextObject *context) noexcept {
	size_t initial_major_frame_depth = context->_context.num_major_frames;
	InternalExceptionPointer except_ptr;
	ExecutionRunnable *const managed_thread = managed_thread_runnables.at(current_thread_handle());
	char *const data_stack = context->_context.data_stack;
	const size_t data_stack_size = context->_context.stack_size;

	while (context->get_context().num_major_frames >= initial_major_frame_depth) {
		MajorFrame *cur_major_frame = _fetch_major_frame(&context->get_context(), context->get_context().off_cur_major_frame);
		FnOverloadingObject *const cur_fn = cur_major_frame->cur_fn;

		if (!cur_fn) {
			break;
		}

		// Pause if the runtime is in GC
		/*while (_flags & _RT_INGC)
			yield_current_thread();*/

		switch (cur_fn->overloading_kind) {
			case FnOverloadingKind::Regular: {
				ContextChangeType context_change;
				do {
					const RegularFnOverloadingObject *const ol = static_cast<const RegularFnOverloadingObject *>(cur_major_frame->cur_fn);
					const size_t num_ins = ol->instructions.size();
					do {
						cur_major_frame = _fetch_major_frame(&context->get_context(), context->get_context().off_cur_major_frame);
						context_change = ContextChangeType::NoChange;
						do {
							// Interrupt execution if the thread is explicitly specified to be killed.
							if SLAKE_UNLIKELY (managed_thread->status == ThreadStatus::Dead) {
								return {};
							}

							if ((fixed_alloc.sz_allocated > _sz_computed_gc_limit)) {
								gc();
							}

							const uint32_t idx_cur_ins = cur_major_frame->resumable_context_data.cur_ins;
							if SLAKE_UNLIKELY (idx_cur_ins >=
											   num_ins) {
								// Raise out of fn body error.
								std::terminate();
							}
							SLAKE_RETURN_IF_EXCEPT(_exec_ins(
								context,
								cur_major_frame,
								data_stack, data_stack_size,
								ol->instructions.at(idx_cur_ins),
								ol,
								context_change));
						} while (context_change == ContextChangeType::NoChange);
					} while (context_change == ContextChangeType::MajorFrameChanged);
					cur_major_frame = _fetch_major_frame(&context->get_context(), context->get_context().off_cur_major_frame);
				} while (context_change == ContextChangeType::FnChanged);

				break;
			}
			case FnOverloadingKind::Native: {
				cur_major_frame = _fetch_major_frame(&context->get_context(), context->get_context().off_cur_major_frame);
				NativeFnOverloadingObject *ol = static_cast<NativeFnOverloadingObject *>(cur_fn);

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
				RegIndex return_value_out_reg = cur_major_frame->return_value_out_reg;
				_leave_major_frame(&context->get_context());
				if (return_value_out_reg != INVALID_REG) {
					// TODO: Set the register value.
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
	FnOverloadingObject *overloading,
	ContextObject *prev_context,
	Object *this_object,
	const Value *args,
	uint32_t num_args,
	Value &value_out) {
	HostObjectRef<ContextObject> context(prev_context);

	Context &ctxt = context->get_context();

	SLAKE_RETURN_IF_EXCEPT(_create_new_major_frame(prev_context, nullptr, nullptr, nullptr, SIZE_MAX, 0, INVALID_REG, nullptr));
	MajorFrame &bottom_frame = *_fetch_major_frame(&ctxt, ctxt.off_cur_major_frame);
	if (overloading->return_type.type_id == TypeId::StructInstance) {
		Reference struct_ref;
		SLAKE_RETURN_IF_EXCEPT(_add_local_var(&ctxt, &bottom_frame, overloading->return_type, 0, struct_ref));
		// SLAKE_RETURN_IF_EXCEPT(_set_register_value(this, regs_ptr, &bottom_frame, 0, Value(struct_ref)));
		SLAKE_RETURN_IF_EXCEPT(_create_new_major_frame(prev_context, this_object, overloading, args, SIZE_MAX, num_args, overloading->return_type == TypeId::Void ? INVALID_REG : 0, &struct_ref));
	} else {
		SLAKE_RETURN_IF_EXCEPT(_create_new_major_frame(prev_context, this_object, overloading, args, SIZE_MAX, num_args, overloading->return_type == TypeId::Void ? INVALID_REG : 0, nullptr));
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
