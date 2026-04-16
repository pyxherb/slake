#include "../proganal.h"

using namespace slake;
using namespace slake::opti;

InternalExceptionPointer slake::opti::analyze_cast_ins(
	ProgramAnalyzeContext &analyze_context,
	uint32_t reg_index) {
	Instruction &cur_ins = analyze_context.fn_object->instructions.at(analyze_context.idx_cur_ins);

	if (cur_ins.num_operands != 2) {
		return alloc_oom_error_if_alloc_failed(
			MalformedProgramError::alloc(
				analyze_context.runtime->get_fixed_alloc(),
				analyze_context.fn_object,
				analyze_context.idx_cur_ins));
	}

	if (cur_ins.operands[0].value_type != ValueType::TypeName) {
		return alloc_oom_error_if_alloc_failed(
			MalformedProgramError::alloc(
				analyze_context.runtime->get_fixed_alloc(),
				analyze_context.fn_object,
				analyze_context.idx_cur_ins));
	}

	Value const_src(ValueType::Undefined);
	TypeRef src_type, dest_type = cur_ins.operands[0].get_type_name();
	SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, cur_ins.operands[1], const_src));
	SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[1], src_type));

	switch (src_type.type_id) {
		case TypeId::I8:
			switch (dest_type.type_id) {
				case TypeId::I8:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int8_t)const_src.get_i8());
					}
					break;
				case TypeId::I16:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int16_t)const_src.get_i8());
					}
					break;
				case TypeId::I32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int32_t)const_src.get_i8());
					}
					break;
				case TypeId::I64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int64_t)const_src.get_i8());
					}
					break;
				case TypeId::U8:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint8_t)const_src.get_i8());
					}
					break;
				case TypeId::U16:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint16_t)const_src.get_i8());
					}
					break;
				case TypeId::U32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint32_t)const_src.get_i8());
					}
					break;
				case TypeId::U64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint64_t)const_src.get_i8());
					}
					break;
				case TypeId::F32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((float)const_src.get_i8());
					}
					break;
				case TypeId::F64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((double)const_src.get_i8());
					}
					break;
				case TypeId::Bool:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((bool)const_src.get_i8());
					}
					break;
				default: {
					return alloc_oom_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}
			break;
		case TypeId::I16:
			switch (dest_type.type_id) {
				case TypeId::I8:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int8_t)const_src.get_i16());
					}
					break;
				case TypeId::I16:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int16_t)const_src.get_i16());
					}
					break;
				case TypeId::I32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int32_t)const_src.get_i16());
					}
					break;
				case TypeId::I64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int64_t)const_src.get_i16());
					}
					break;
				case TypeId::U8:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint8_t)const_src.get_i16());
					}
					break;
				case TypeId::U16:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint16_t)const_src.get_i16());
					}
					break;
				case TypeId::U32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint32_t)const_src.get_i16());
					}
					break;
				case TypeId::U64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint64_t)const_src.get_i16());
					}
					break;
				case TypeId::F32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((float)const_src.get_i16());
					}
					break;
				case TypeId::F64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((double)const_src.get_i16());
					}
					break;
				case TypeId::Bool:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((bool)const_src.get_i16());
					}
					break;
				default: {
					return alloc_oom_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}
			break;
			break;
		case TypeId::I32:
			switch (dest_type.type_id) {
				case TypeId::I8:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int8_t)const_src.get_i32());
					}
					break;
				case TypeId::I16:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int16_t)const_src.get_i32());
					}
					break;
				case TypeId::I32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int32_t)const_src.get_i32());
					}
					break;
				case TypeId::I64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int64_t)const_src.get_i32());
					}
					break;
				case TypeId::U8:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint8_t)const_src.get_i32());
					}
					break;
				case TypeId::U16:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint16_t)const_src.get_i32());
					}
					break;
				case TypeId::U32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint32_t)const_src.get_i32());
					}
					break;
				case TypeId::U64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint64_t)const_src.get_i32());
					}
					break;
				case TypeId::F32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((float)const_src.get_i32());
					}
					break;
				case TypeId::F64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((double)const_src.get_i32());
					}
					break;
				case TypeId::Bool:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((bool)const_src.get_i32());
					}
					break;
					break;
				default: {
					return alloc_oom_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}
			break;
		case TypeId::I64:
			switch (dest_type.type_id) {
				case TypeId::I8:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int8_t)const_src.get_i64());
					}
					break;
				case TypeId::I16:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int16_t)const_src.get_i64());
					}
					break;
				case TypeId::I32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int32_t)const_src.get_i64());
					}
					break;
				case TypeId::I64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int64_t)const_src.get_i64());
					}
					break;
				case TypeId::U8:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint8_t)const_src.get_i64());
					}
					break;
				case TypeId::U16:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint16_t)const_src.get_i64());
					}
					break;
				case TypeId::U32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint32_t)const_src.get_i64());
					}
					break;
				case TypeId::U64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint64_t)const_src.get_i64());
					}
					break;
				case TypeId::F32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((float)const_src.get_i64());
					}
					break;
				case TypeId::F64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((double)const_src.get_i64());
					}
					break;
				case TypeId::Bool:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((bool)const_src.get_i64());
					}
					break;
				default: {
					return alloc_oom_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}
			break;
		case TypeId::U8:
			switch (dest_type.type_id) {
				case TypeId::I8:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int8_t)const_src.get_u8());
					}
					break;
				case TypeId::I16:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int16_t)const_src.get_u8());
					}
					break;
				case TypeId::I32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int32_t)const_src.get_u8());
					}
					break;
				case TypeId::I64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int64_t)const_src.get_u8());
					}
					break;
				case TypeId::U8:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint8_t)const_src.get_u8());
					}
					break;
				case TypeId::U16:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint16_t)const_src.get_u8());
					}
					break;
				case TypeId::U32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint32_t)const_src.get_u8());
					}
					break;
				case TypeId::U64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint64_t)const_src.get_u8());
					}
					break;
				case TypeId::F32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((float)const_src.get_u8());
					}
					break;
				case TypeId::F64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((double)const_src.get_u8());
					}
					break;
				case TypeId::Bool:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((bool)const_src.get_u8());
					}
					break;
					break;
				default: {
					return alloc_oom_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}
			break;
		case TypeId::U16:
			switch (dest_type.type_id) {
				case TypeId::I8:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int8_t)const_src.get_u16());
					}
					break;
				case TypeId::I16:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int16_t)const_src.get_u16());
					}
					break;
				case TypeId::I32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int32_t)const_src.get_u16());
					}
					break;
				case TypeId::I64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int64_t)const_src.get_u16());
					}
					break;
				case TypeId::U8:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint8_t)const_src.get_u16());
					}
					break;
				case TypeId::U16:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint16_t)const_src.get_u16());
					}
					break;
				case TypeId::U32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint32_t)const_src.get_u16());
					}
					break;
				case TypeId::U64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint64_t)const_src.get_u16());
					}
					break;
				case TypeId::F32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((float)const_src.get_u16());
					}
					break;
				case TypeId::F64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((double)const_src.get_u16());
					}
					break;
				case TypeId::Bool:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((bool)const_src.get_u16());
					}
					break;
				default: {
					return alloc_oom_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}
			break;
		case TypeId::U32:
			switch (dest_type.type_id) {
				case TypeId::I8:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int8_t)const_src.get_u32());
					}
					break;
				case TypeId::I16:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int16_t)const_src.get_u32());
					}
					break;
				case TypeId::I32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int32_t)const_src.get_u32());
					}
					break;
				case TypeId::I64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int64_t)const_src.get_u32());
					}
					break;
				case TypeId::U8:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint8_t)const_src.get_u32());
					}
					break;
				case TypeId::U16:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint16_t)const_src.get_u32());
					}
					break;
				case TypeId::U32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint32_t)const_src.get_u32());
					}
					break;
				case TypeId::U64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint64_t)const_src.get_u32());
					}
					break;
				case TypeId::F32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((float)const_src.get_u32());
					}
					break;
				case TypeId::F64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((double)const_src.get_u32());
					}
					break;
				case TypeId::Bool:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((bool)const_src.get_u32());
					}
					break;
				default: {
					return alloc_oom_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}
			break;
		case TypeId::U64:
			switch (dest_type.type_id) {
				case TypeId::I8:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int8_t)const_src.get_u64());
					}
					break;
				case TypeId::I16:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int16_t)const_src.get_u64());
					}
					break;
				case TypeId::I32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int32_t)const_src.get_u64());
					}
					break;
				case TypeId::I64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int64_t)const_src.get_u64());
					}
					break;
				case TypeId::U8:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint8_t)const_src.get_u64());
					}
					break;
				case TypeId::U16:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint16_t)const_src.get_u64());
					}
					break;
				case TypeId::U32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint32_t)const_src.get_u64());
					}
					break;
				case TypeId::U64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint64_t)const_src.get_u64());
					}
					break;
				case TypeId::F32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((float)const_src.get_u64());
					}
					break;
				case TypeId::F64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((double)const_src.get_u64());
					}
					break;
				case TypeId::Bool:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((bool)const_src.get_u64());
					}
					break;
				default: {
					return alloc_oom_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}
			break;
		case TypeId::Bool:
			switch (dest_type.type_id) {
				case TypeId::I8:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int8_t)const_src.get_bool());
					}
					break;
				case TypeId::I16:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int16_t)const_src.get_bool());
					}
					break;
				case TypeId::I32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int32_t)const_src.get_bool());
					}
					break;
				case TypeId::I64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int64_t)const_src.get_bool());
					}
					break;
				case TypeId::U8:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint8_t)const_src.get_bool());
					}
					break;
				case TypeId::U16:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint16_t)const_src.get_bool());
					}
					break;
				case TypeId::U32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint32_t)const_src.get_bool());
					}
					break;
				case TypeId::U64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint64_t)const_src.get_bool());
					}
					break;
				case TypeId::F32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((float)const_src.get_bool());
					}
					break;
				case TypeId::F64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((double)const_src.get_bool());
					}
					break;
				case TypeId::Bool:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((bool)const_src.get_bool());
					}
					break;
				default: {
					return alloc_oom_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}
			break;
		case TypeId::F32:
			switch (dest_type.type_id) {
				case TypeId::I8:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int8_t)const_src.get_f32());
					}
					break;
				case TypeId::I16:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int16_t)const_src.get_f32());
					}
					break;
				case TypeId::I32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int32_t)const_src.get_f32());
					}
					break;
				case TypeId::I64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int64_t)const_src.get_f32());
					}
					break;
				case TypeId::U8:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint8_t)const_src.get_f32());
					}
					break;
				case TypeId::U16:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint16_t)const_src.get_f32());
					}
					break;
				case TypeId::U32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint32_t)const_src.get_f32());
					}
					break;
				case TypeId::U64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint64_t)const_src.get_f32());
					}
					break;
				case TypeId::F32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((float)const_src.get_f32());
					}
					break;
				case TypeId::F64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((double)const_src.get_f32());
					}
					break;
				case TypeId::Bool:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((bool)const_src.get_f32());
					}
					break;
				default: {
					return alloc_oom_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}
			break;
		case TypeId::F64:
			switch (dest_type.type_id) {
				case TypeId::I8:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int8_t)const_src.get_f64());
					}
					break;
				case TypeId::I16:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int16_t)const_src.get_f64());
					}
					break;
				case TypeId::I32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int32_t)const_src.get_f64());
					}
					break;
				case TypeId::I64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((int64_t)const_src.get_f64());
					}
					break;
				case TypeId::U8:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint8_t)const_src.get_f64());
					}
					break;
				case TypeId::U16:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint16_t)const_src.get_f64());
					}
					break;
				case TypeId::U32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint32_t)const_src.get_f64());
					}
					break;
				case TypeId::U64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((uint64_t)const_src.get_f64());
					}
					break;
				case TypeId::F32:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((float)const_src.get_f64());
					}
					break;
				case TypeId::F64:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((double)const_src.get_f64());
					}
					break;
				case TypeId::Bool:
					if (const_src.value_type != ValueType::Undefined) {
						analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = Value((bool)const_src.get_f64());
					}
					break;
				default: {
					return alloc_oom_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				} break;
			}
			break;
		case TypeId::Instance: {
			break;
		}
		default: {
			return alloc_oom_error_if_alloc_failed(
				MalformedProgramError::alloc(
					analyze_context.runtime->get_fixed_alloc(),
					analyze_context.fn_object,
					analyze_context.idx_cur_ins));
		}
	}

	if (reg_index != UINT32_MAX) {
		analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).type = dest_type;
	}

	return {};
}
