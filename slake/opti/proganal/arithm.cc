#include "../proganal.h"
#include <slake/flib/math/fmod.h>

using namespace slake;
using namespace slake::opti;

InternalExceptionPointer slake::opti::analyze_arithmetic_ins(
	ProgramAnalyzeContext &analyze_context,
	uint32_t reg_index) noexcept {
	Instruction &cur_ins = analyze_context.fn_object->instructions.at(analyze_context.idx_cur_ins);

	Value lhs = cur_ins.operands[0],
		  rhs = cur_ins.operands[1],
		  evaluated_lhs(ValueType::Undefined),
		  evaluated_rhs(ValueType::Undefined),
		  result = (ValueType::Undefined);
	TypeRef lhs_type, rhs_type, result_type;

	switch (cur_ins.opcode) {
		case Opcode::ADD: {
			if (cur_ins.num_operands != 2) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[0], lhs_type));
			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[1], rhs_type));

			if (lhs_type != rhs_type) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, rhs, rhs));

			switch (lhs_type.type_id) {
				case TypeId::I8:
					result_type = TypeId::I8;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int8_t)evaluated_lhs.get_i8() + evaluated_rhs.get_i8());
					}
					break;
				case TypeId::I16:
					result_type = TypeId::I16;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int16_t)evaluated_lhs.get_i16() + evaluated_rhs.get_i16());
					}
					break;
				case TypeId::I32:
					result_type = TypeId::I32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int32_t)evaluated_lhs.get_i32() + evaluated_rhs.get_i32());
					}
					break;
				case TypeId::I64:
					result_type = TypeId::I64;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int64_t)evaluated_lhs.get_i64() + evaluated_rhs.get_i64());
					}
					break;
				case TypeId::U8:
					result_type = TypeId::U8;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint8_t)evaluated_lhs.get_u8() + evaluated_rhs.get_u8());
					}
					break;
				case TypeId::U16:
					result_type = TypeId::U16;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint16_t)evaluated_lhs.get_u16() + evaluated_rhs.get_u16());
					}
					break;
				case TypeId::U32:
					result_type = TypeId::U32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint32_t)evaluated_lhs.get_u32() + evaluated_rhs.get_u32());
					}
					break;
				case TypeId::U64:
					result_type = TypeId::U64;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint64_t)evaluated_lhs.get_u64() + evaluated_rhs.get_u64());
					}
					break;
				case TypeId::F32:
					result_type = TypeId::F32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((float)evaluated_lhs.get_f32() + evaluated_rhs.get_f32());
					}
					break;
				case TypeId::F64:
					result_type = TypeId::F64;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((double)evaluated_lhs.get_f64() + evaluated_rhs.get_f64());
					}
					break;
				default: {
					return alloc_out_of_memory_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}

			if (reg_index != UINT32_MAX) {
				analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).type = result_type;
				if (result.value_type != ValueType::Undefined) {
					analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = result;
				}
			}
			break;
		}
		case Opcode::SUB: {
			if (cur_ins.num_operands != 2) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[0], lhs_type));
			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[1], rhs_type));

			if (lhs_type != rhs_type) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, rhs, rhs));

			switch (lhs_type.type_id) {
				case TypeId::I8:
					result_type = TypeId::I8;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int8_t)evaluated_lhs.get_i8() - evaluated_rhs.get_i8());
					}
					break;
				case TypeId::I16:
					result_type = TypeId::I16;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int16_t)evaluated_lhs.get_i16() - evaluated_rhs.get_i16());
					}
					break;
				case TypeId::I32:
					result_type = TypeId::I32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int32_t)evaluated_lhs.get_i32() - evaluated_rhs.get_i32());
					}
					break;
				case TypeId::I64:
					result_type = TypeId::I64;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int64_t)evaluated_lhs.get_i64() - evaluated_rhs.get_i64());
					}
					break;
				case TypeId::U8:
					result_type = TypeId::U8;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint8_t)evaluated_lhs.get_u8() - evaluated_rhs.get_u8());
					}
					break;
				case TypeId::U16:
					result_type = TypeId::U16;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint16_t)evaluated_lhs.get_u16() - evaluated_rhs.get_u16());
					}
					break;
				case TypeId::U32:
					result_type = TypeId::U32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint32_t)evaluated_lhs.get_u32() - evaluated_rhs.get_u32());
					}
					break;
				case TypeId::U64:
					result_type = TypeId::U64;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint64_t)evaluated_lhs.get_u64() - evaluated_rhs.get_u64());
					}
					break;
				case TypeId::F32:
					result_type = TypeId::F32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((float)evaluated_lhs.get_f32() - evaluated_rhs.get_f32());
					}
					break;
				case TypeId::F64:
					result_type = TypeId::F64;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((double)evaluated_lhs.get_f64() - evaluated_rhs.get_f64());
					}
					break;
				default: {
					return alloc_out_of_memory_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}

			if (reg_index != UINT32_MAX) {
				analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).type = result_type;
				if (result.value_type != ValueType::Undefined) {
					analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = result;
				}
			}
			break;
		}
		case Opcode::MUL: {
			if (cur_ins.num_operands != 2) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[0], lhs_type));
			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[1], rhs_type));

			if (lhs_type != rhs_type) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, rhs, rhs));

			switch (lhs_type.type_id) {
				case TypeId::I8:
					result_type = TypeId::I8;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int8_t)evaluated_lhs.get_i8() * evaluated_rhs.get_i8());
					}
					break;
				case TypeId::I16:
					result_type = TypeId::I16;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int16_t)evaluated_lhs.get_i16() * evaluated_rhs.get_i16());
					}
					break;
				case TypeId::I32:
					result_type = TypeId::I32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int32_t)evaluated_lhs.get_i32() * evaluated_rhs.get_i32());
					}
					break;
				case TypeId::I64:
					result_type = TypeId::I64;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int64_t)evaluated_lhs.get_i64() * evaluated_rhs.get_i64());
					}
					break;
				case TypeId::U8:
					result_type = TypeId::U8;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint8_t)evaluated_lhs.get_u8() * evaluated_rhs.get_u8());
					}
					break;
				case TypeId::U16:
					result_type = TypeId::U16;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint16_t)evaluated_lhs.get_u16() * evaluated_rhs.get_u16());
					}
					break;
				case TypeId::U32:
					result_type = TypeId::U32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint32_t)evaluated_lhs.get_u32() * evaluated_rhs.get_u32());
					}
					break;
				case TypeId::U64:
					result_type = TypeId::U64;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint64_t)evaluated_lhs.get_u64() * evaluated_rhs.get_u64());
					}
					break;
				case TypeId::F32:
					result_type = TypeId::F32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((float)evaluated_lhs.get_f32() * evaluated_rhs.get_f32());
					}
					break;
				case TypeId::F64:
					result_type = TypeId::F64;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((double)evaluated_lhs.get_f64() * evaluated_rhs.get_f64());
					}
					break;
				default: {
					return alloc_out_of_memory_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}

			if (reg_index != UINT32_MAX) {
				analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).type = result_type;
				if (result.value_type != ValueType::Undefined) {
					analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = result;
				}
			}
			break;
		}
		case Opcode::DIV: {
			if (cur_ins.num_operands != 2) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[0], lhs_type));
			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[1], rhs_type));

			if (lhs_type != rhs_type) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, rhs, rhs));

			switch (lhs_type.type_id) {
				case TypeId::I8:
					result_type = TypeId::I8;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int8_t)evaluated_lhs.get_i8() / evaluated_rhs.get_i8());
					}
					break;
				case TypeId::I16:
					result_type = TypeId::I16;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int16_t)evaluated_lhs.get_i16() / evaluated_rhs.get_i16());
					}
					break;
				case TypeId::I32:
					result_type = TypeId::I32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int32_t)evaluated_lhs.get_i32() / evaluated_rhs.get_i32());
					}
					break;
				case TypeId::I64:
					result_type = TypeId::I64;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int64_t)evaluated_lhs.get_i64() / evaluated_rhs.get_i64());
					}
					break;
				case TypeId::U8:
					result_type = TypeId::U8;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint8_t)evaluated_lhs.get_u8() / evaluated_rhs.get_u8());
					}
					break;
				case TypeId::U16:
					result_type = TypeId::U16;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint16_t)evaluated_lhs.get_u16() / evaluated_rhs.get_u16());
					}
					break;
				case TypeId::U32:
					result_type = TypeId::U32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint32_t)evaluated_lhs.get_u32() / evaluated_rhs.get_u32());
					}
					break;
				case TypeId::U64:
					result_type = TypeId::U64;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint64_t)evaluated_lhs.get_u64() / evaluated_rhs.get_u64());
					}
					break;
				case TypeId::F32:
					result_type = TypeId::F32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((float)evaluated_lhs.get_f32() / evaluated_rhs.get_f32());
					}
					break;
				case TypeId::F64:
					result_type = TypeId::F64;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((double)evaluated_lhs.get_f64() / evaluated_rhs.get_f64());
					}
					break;
				default: {
					return alloc_out_of_memory_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}

			if (reg_index != UINT32_MAX) {
				analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).type = result_type;
				if (result.value_type != ValueType::Undefined) {
					analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = result;
				}
			}
			break;
		}
		case Opcode::MOD: {
			if (cur_ins.num_operands != 2) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[0], lhs_type));
			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[1], rhs_type));

			if (lhs_type != rhs_type) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, rhs, rhs));

			switch (lhs_type.type_id) {
				case TypeId::I8:
					result_type = TypeId::I8;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int8_t)evaluated_lhs.get_i8() % evaluated_rhs.get_i8());
					}
					break;
				case TypeId::I16:
					result_type = TypeId::I16;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int16_t)evaluated_lhs.get_i16() % evaluated_rhs.get_i16());
					}
					break;
				case TypeId::I32:
					result_type = TypeId::I32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int32_t)evaluated_lhs.get_i32() % evaluated_rhs.get_i32());
					}
					break;
				case TypeId::I64:
					result_type = TypeId::I64;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int64_t)evaluated_lhs.get_i64() % evaluated_rhs.get_i64());
					}
					break;
				case TypeId::U8:
					result_type = TypeId::U8;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint8_t)evaluated_lhs.get_u8() % evaluated_rhs.get_u8());
					}
					break;
				case TypeId::U16:
					result_type = TypeId::U16;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint16_t)evaluated_lhs.get_u16() % evaluated_rhs.get_u16());
					}
					break;
				case TypeId::U32:
					result_type = TypeId::U32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint32_t)evaluated_lhs.get_u32() % evaluated_rhs.get_u32());
					}
					break;
				case TypeId::U64:
					result_type = TypeId::U64;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint64_t)evaluated_lhs.get_u64() % evaluated_rhs.get_u64());
					}
					break;
				case TypeId::F32:
					result_type = TypeId::F32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((float)flib::fmodf(evaluated_lhs.get_f32(), evaluated_rhs.get_f32()));
					}
					break;
				case TypeId::F64:
					result_type = TypeId::F64;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((double)flib::fmod(evaluated_lhs.get_f64(), evaluated_rhs.get_f64()));
					}
					break;
				default: {
					return alloc_out_of_memory_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}

			if (reg_index != UINT32_MAX) {
				analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).type = result_type;
				if (result.value_type != ValueType::Undefined) {
					analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = result;
				}
			}
			break;
		}
		case Opcode::AND: {
			if (cur_ins.num_operands != 2) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[0], lhs_type));
			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[1], rhs_type));

			if (lhs_type != rhs_type) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, rhs, rhs));

			switch (lhs_type.type_id) {
				case TypeId::I8:
					result_type = TypeId::I8;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int8_t)evaluated_lhs.get_i8() & evaluated_rhs.get_i8());
					}
					break;
				case TypeId::I16:
					result_type = TypeId::I16;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int16_t)evaluated_lhs.get_i16() & evaluated_rhs.get_i16());
					}
					break;
				case TypeId::I32:
					result_type = TypeId::I32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int32_t)evaluated_lhs.get_i32() & evaluated_rhs.get_i32());
					}
					break;
				case TypeId::I64:
					result_type = TypeId::I64;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int64_t)evaluated_lhs.get_i64() & evaluated_rhs.get_i64());
					}
					break;
				case TypeId::U8:
					result_type = TypeId::U8;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint8_t)evaluated_lhs.get_u8() & evaluated_rhs.get_u8());
					}
					break;
				case TypeId::U16:
					result_type = TypeId::U16;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint16_t)evaluated_lhs.get_u16() & evaluated_rhs.get_u16());
					}
					break;
				case TypeId::U32:
					result_type = TypeId::U32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint32_t)evaluated_lhs.get_u32() & evaluated_rhs.get_u32());
					}
					break;
				case TypeId::U64:
					result_type = TypeId::U64;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint64_t)evaluated_lhs.get_u64() & evaluated_rhs.get_u64());
					}
					break;
				default: {
					return alloc_out_of_memory_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}

			if (reg_index != UINT32_MAX) {
				analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).type = result_type;
				if (result.value_type != ValueType::Undefined) {
					analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = result;
				}
			}
			break;
		}
		case Opcode::OR: {
			if (cur_ins.num_operands != 2) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[0], lhs_type));
			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[1], rhs_type));

			if (lhs_type != rhs_type) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, rhs, rhs));

			switch (lhs_type.type_id) {
				case TypeId::I8:
					result_type = TypeId::I8;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int8_t)evaluated_lhs.get_i8() | evaluated_rhs.get_i8());
					}
					break;
				case TypeId::I16:
					result_type = TypeId::I16;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int16_t)evaluated_lhs.get_i16() | evaluated_rhs.get_i16());
					}
					break;
				case TypeId::I32:
					result_type = TypeId::I32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int32_t)evaluated_lhs.get_i32() | evaluated_rhs.get_i32());
					}
					break;
				case TypeId::I64:
					result_type = TypeId::I64;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int64_t)evaluated_lhs.get_i64() | evaluated_rhs.get_i64());
					}
					break;
				case TypeId::U8:
					result_type = TypeId::U8;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint8_t)evaluated_lhs.get_u8() | evaluated_rhs.get_u8());
					}
					break;
				case TypeId::U16:
					result_type = TypeId::U16;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint16_t)evaluated_lhs.get_u16() | evaluated_rhs.get_u16());
					}
					break;
				case TypeId::U32:
					result_type = TypeId::U32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint32_t)evaluated_lhs.get_u32() | evaluated_rhs.get_u32());
					}
					break;
				case TypeId::U64:
					result_type = TypeId::U64;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint64_t)evaluated_lhs.get_u64() | evaluated_rhs.get_u64());
					}
					break;
				default: {
					return alloc_out_of_memory_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}

			if (reg_index != UINT32_MAX) {
				analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).type = result_type;
				if (result.value_type != ValueType::Undefined) {
					analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = result;
				}
			}
			break;
		}
		case Opcode::XOR: {
			if (cur_ins.num_operands != 2) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[0], lhs_type));
			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[1], rhs_type));

			if (lhs_type != rhs_type) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, rhs, rhs));

			switch (lhs_type.type_id) {
				case TypeId::I8:
					result_type = TypeId::I8;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int8_t)evaluated_lhs.get_i8() ^ evaluated_rhs.get_i8());
					}
					break;
				case TypeId::I16:
					result_type = TypeId::I16;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int16_t)evaluated_lhs.get_i16() ^ evaluated_rhs.get_i16());
					}
					break;
				case TypeId::I32:
					result_type = TypeId::I32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int32_t)evaluated_lhs.get_i32() ^ evaluated_rhs.get_i32());
					}
					break;
				case TypeId::I64:
					result_type = TypeId::I64;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int64_t)evaluated_lhs.get_i64() ^ evaluated_rhs.get_i64());
					}
					break;
				case TypeId::U8:
					result_type = TypeId::U8;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint8_t)evaluated_lhs.get_u8() ^ evaluated_rhs.get_u8());
					}
					break;
				case TypeId::U16:
					result_type = TypeId::U16;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint16_t)evaluated_lhs.get_u16() ^ evaluated_rhs.get_u16());
					}
					break;
				case TypeId::U32:
					result_type = TypeId::U32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint32_t)evaluated_lhs.get_u32() ^ evaluated_rhs.get_u32());
					}
					break;
				case TypeId::U64:
					result_type = TypeId::U64;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint64_t)evaluated_lhs.get_u64() ^ evaluated_rhs.get_u64());
					}
					break;
				default: {
					return alloc_out_of_memory_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}

			if (reg_index != UINT32_MAX) {
				analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).type = result_type;
				if (result.value_type != ValueType::Undefined) {
					analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = result;
				}
			}
			break;
		}
		case Opcode::LAND: {
			if (cur_ins.num_operands != 2) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[0], lhs_type));
			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[1], rhs_type));

			int cmp_result;

			if (lhs_type != TypeId::Bool) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			if (lhs_type.compares_to(rhs_type)) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, rhs, rhs));

			switch (lhs_type.type_id) {
				case TypeId::Bool:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((bool)evaluated_lhs.get_bool() && evaluated_rhs.get_bool());
					}
					break;
				default: {
					return alloc_out_of_memory_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}

			if (reg_index != UINT32_MAX) {
				analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).type = result_type;
				if (result.value_type != ValueType::Undefined) {
					analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = result;
				}
			}
			break;
		}
		case Opcode::LOR: {
			if (cur_ins.num_operands != 2) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[0], lhs_type));
			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[1], rhs_type));

			if (lhs_type != TypeId::Bool) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}
			if (lhs_type != rhs_type) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, rhs, rhs));

			switch (lhs_type.type_id) {
				case TypeId::Bool:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((bool)evaluated_lhs.get_bool() || evaluated_rhs.get_bool());
					}
					break;
				default: {
					return alloc_out_of_memory_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}

			if (reg_index != UINT32_MAX) {
				analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).type = result_type;
				if (result.value_type != ValueType::Undefined) {
					analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = result;
				}
			}
			break;
		}
		case Opcode::EQ: {
			if (cur_ins.num_operands != 2) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[0], lhs_type));
			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[1], rhs_type));

			if (lhs_type != rhs_type) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, rhs, rhs));

			switch (lhs_type.type_id) {
				case TypeId::I8:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_i8() == evaluated_rhs.get_i8());
					}
					break;
				case TypeId::I16:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_i16() == evaluated_rhs.get_i16());
					}
					break;
				case TypeId::I32:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_i32() == evaluated_rhs.get_i32());
					}
					break;
				case TypeId::I64:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_i64() == evaluated_rhs.get_i64());
					}
					break;
				case TypeId::U8:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_u8() == evaluated_rhs.get_u8());
					}
					break;
				case TypeId::U16:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_u16() == evaluated_rhs.get_u16());
					}
					break;
				case TypeId::U32:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_u32() == evaluated_rhs.get_u32());
					}
					break;
				case TypeId::U64:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_u64() == evaluated_rhs.get_u64());
					}
					break;
				case TypeId::F32:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_f32() == evaluated_rhs.get_f32());
					}
					break;
				case TypeId::F64:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_f64() == evaluated_rhs.get_f64());
					}
					break;
				case TypeId::Bool:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_bool() == evaluated_rhs.get_bool());
					}
					break;
				default: {
					return alloc_out_of_memory_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}

			if (reg_index != UINT32_MAX) {
				analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).type = result_type;
				if (result.value_type != ValueType::Undefined) {
					analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = result;
				}
			}
			break;
		}
		case Opcode::NEQ: {
			if (cur_ins.num_operands != 2) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[0], lhs_type));
			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[1], rhs_type));

			if (lhs_type != rhs_type) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, rhs, rhs));

			switch (lhs_type.type_id) {
				case TypeId::I8:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_i8() != evaluated_rhs.get_i8());
					}
					break;
				case TypeId::I16:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_i16() != evaluated_rhs.get_i16());
					}
					break;
				case TypeId::I32:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_i32() != evaluated_rhs.get_i32());
					}
					break;
				case TypeId::I64:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_i64() != evaluated_rhs.get_i64());
					}
					break;
				case TypeId::U8:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_u8() != evaluated_rhs.get_u8());
					}
					break;
				case TypeId::U16:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_u16() != evaluated_rhs.get_u16());
					}
					break;
				case TypeId::U32:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_u32() != evaluated_rhs.get_u32());
					}
					break;
				case TypeId::U64:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_u64() != evaluated_rhs.get_u64());
					}
					break;
				case TypeId::F32:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_f32() != evaluated_rhs.get_f32());
					}
					break;
				case TypeId::F64:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_f64() != evaluated_rhs.get_f64());
					}
					break;
				case TypeId::Bool:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_bool() != evaluated_rhs.get_bool());
					}
					break;
				default: {
					return alloc_out_of_memory_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}

			if (reg_index != UINT32_MAX) {
				analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).type = result_type;
				if (result.value_type != ValueType::Undefined) {
					analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = result;
				}
			}
			break;
		}
		case Opcode::LT: {
			if (cur_ins.num_operands != 2) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[0], lhs_type));
			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[1], rhs_type));

			if (lhs_type != rhs_type) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, rhs, rhs));

			switch (lhs_type.type_id) {
				case TypeId::I8:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_i8() < evaluated_rhs.get_i8());
					}
					break;
				case TypeId::I16:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_i16() < evaluated_rhs.get_i16());
					}
					break;
				case TypeId::I32:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_i32() < evaluated_rhs.get_i32());
					}
					break;
				case TypeId::I64:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_i64() < evaluated_rhs.get_i64());
					}
					break;
				case TypeId::U8:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_u8() < evaluated_rhs.get_u8());
					}
					break;
				case TypeId::U16:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_u16() < evaluated_rhs.get_u16());
					}
					break;
				case TypeId::U32:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_u32() < evaluated_rhs.get_u32());
					}
					break;
				case TypeId::U64:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_u64() < evaluated_rhs.get_u64());
					}
					break;
				case TypeId::F32:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_f32() < evaluated_rhs.get_f32());
					}
					break;
				case TypeId::F64:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_f64() < evaluated_rhs.get_f64());
					}
					break;
				default: {
					return alloc_out_of_memory_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}

			if (reg_index != UINT32_MAX) {
				analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).type = result_type;
				if (result.value_type != ValueType::Undefined) {
					analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = result;
				}
			}
			break;
		}
		case Opcode::GT: {
			if (cur_ins.num_operands != 2) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[0], lhs_type));
			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[1], rhs_type));

			if (lhs_type != rhs_type) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, rhs, rhs));

			switch (lhs_type.type_id) {
				case TypeId::I8:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_i8() > evaluated_rhs.get_i8());
					}
					break;
				case TypeId::I16:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_i16() > evaluated_rhs.get_i16());
					}
					break;
				case TypeId::I32:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_i32() > evaluated_rhs.get_i32());
					}
					break;
				case TypeId::I64:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_i64() > evaluated_rhs.get_i64());
					}
					break;
				case TypeId::U8:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_u8() > evaluated_rhs.get_u8());
					}
					break;
				case TypeId::U16:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_u16() > evaluated_rhs.get_u16());
					}
					break;
				case TypeId::U32:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_u32() > evaluated_rhs.get_u32());
					}
					break;
				case TypeId::U64:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_u64() > evaluated_rhs.get_u64());
					}
					break;
				case TypeId::F32:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_f32() > evaluated_rhs.get_f32());
					}
					break;
				case TypeId::F64:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_f64() > evaluated_rhs.get_f64());
					}
					break;
				default: {
					return alloc_out_of_memory_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}

			if (reg_index != UINT32_MAX) {
				analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).type = result_type;
				if (result.value_type != ValueType::Undefined) {
					analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = result;
				}
			}
			break;
		}
		case Opcode::LTEQ: {
			if (cur_ins.num_operands != 2) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[0], lhs_type));
			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[1], rhs_type));

			if (lhs_type != rhs_type) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, rhs, rhs));

			switch (lhs_type.type_id) {
				case TypeId::I8:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_i8() <= evaluated_rhs.get_i8());
					}
					break;
				case TypeId::I16:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_i16() <= evaluated_rhs.get_i16());
					}
					break;
				case TypeId::I32:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_i32() <= evaluated_rhs.get_i32());
					}
					break;
				case TypeId::I64:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_i64() <= evaluated_rhs.get_i64());
					}
					break;
				case TypeId::U8:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_u8() <= evaluated_rhs.get_u8());
					}
					break;
				case TypeId::U16:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_u16() <= evaluated_rhs.get_u16());
					}
					break;
				case TypeId::U32:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_u32() <= evaluated_rhs.get_u32());
					}
					break;
				case TypeId::U64:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_u64() <= evaluated_rhs.get_u64());
					}
					break;
				case TypeId::F32:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_f32() <= evaluated_rhs.get_f32());
					}
					break;
				case TypeId::F64:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_f64() <= evaluated_rhs.get_f64());
					}
					break;
				default: {
					return alloc_out_of_memory_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}

			if (reg_index != UINT32_MAX) {
				analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).type = result_type;
				if (result.value_type != ValueType::Undefined) {
					analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = result;
				}
			}
			break;
		}
		case Opcode::GTEQ: {
			if (cur_ins.num_operands != 2) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[0], lhs_type));
			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[1], rhs_type));

			if (lhs_type != rhs_type) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, rhs, rhs));

			switch (lhs_type.type_id) {
				case TypeId::I8:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_i8() >= evaluated_rhs.get_i8());
					}
					break;
				case TypeId::I16:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_i16() >= evaluated_rhs.get_i16());
					}
					break;
				case TypeId::I32:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_i32() >= evaluated_rhs.get_i32());
					}
					break;
				case TypeId::I64:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_i64() >= evaluated_rhs.get_i64());
					}
					break;
				case TypeId::U8:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_u8() >= evaluated_rhs.get_u8());
					}
					break;
				case TypeId::U16:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_u16() >= evaluated_rhs.get_u16());
					}
					break;
				case TypeId::U32:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_u32() >= evaluated_rhs.get_u32());
					}
					break;
				case TypeId::U64:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_u64() >= evaluated_rhs.get_u64());
					}
					break;
				case TypeId::F32:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_f32() >= evaluated_rhs.get_f32());
					}
					break;
				case TypeId::F64:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value(evaluated_lhs.get_f64() >= evaluated_rhs.get_f64());
					}
					break;
				default: {
					return alloc_out_of_memory_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}

			if (reg_index != UINT32_MAX) {
				analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).type = result_type;
				if (result.value_type != ValueType::Undefined) {
					analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = result;
				}
			}
			break;
		}
		case Opcode::CMP: {
			if (cur_ins.num_operands != 2) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[0], lhs_type));
			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[1], rhs_type));

			if (lhs_type != rhs_type) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, rhs, rhs));

			switch (lhs_type.type_id) {
				case TypeId::I8:
					result_type = TypeId::I32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						int8_t lhs_data = evaluated_lhs.get_i8(), rhs_data = evaluated_rhs.get_i8();
						if (lhs_data < rhs_data) {
							result = Value((int32_t)-1);
						} else if (lhs_data > rhs_data) {
							result = Value((int32_t)1);
						} else {
							result = Value((int32_t)0);
						}
					}
					break;
				case TypeId::I16:
					result_type = TypeId::I32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						int16_t lhs_data = evaluated_lhs.get_i16(), rhs_data = evaluated_rhs.get_i16();
						if (lhs_data < rhs_data) {
							result = Value((int32_t)-1);
						} else if (lhs_data > rhs_data) {
							result = Value((int32_t)1);
						} else {
							result = Value((int32_t)0);
						}
					}
					break;
				case TypeId::I32:
					result_type = TypeId::I32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						int32_t lhs_data = evaluated_lhs.get_i32(), rhs_data = evaluated_rhs.get_i32();
						if (lhs_data < rhs_data) {
							result = Value((int32_t)-1);
						} else if (lhs_data > rhs_data) {
							result = Value((int32_t)1);
						} else {
							result = Value((int32_t)0);
						}
					}
					break;
				case TypeId::I64:
					result_type = TypeId::I32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						int64_t lhs_data = evaluated_lhs.get_i64(), rhs_data = evaluated_rhs.get_i64();
						if (lhs_data < rhs_data) {
							result = Value((int32_t)-1);
						} else if (lhs_data > rhs_data) {
							result = Value((int32_t)1);
						} else {
							result = Value((int32_t)0);
						}
					}
					break;
				case TypeId::U8:
					result_type = TypeId::I32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						uint8_t lhs_data = evaluated_lhs.get_u8(), rhs_data = evaluated_rhs.get_u8();
						if (lhs_data < rhs_data) {
							result = Value((int32_t)-1);
						} else if (lhs_data > rhs_data) {
							result = Value((int32_t)1);
						} else {
							result = Value((int32_t)0);
						}
					}
					break;
				case TypeId::U16:
					result_type = TypeId::I32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						uint16_t lhs_data = evaluated_lhs.get_u16(), rhs_data = evaluated_rhs.get_u16();
						if (lhs_data < rhs_data) {
							result = Value((int32_t)-1);
						} else if (lhs_data > rhs_data) {
							result = Value((int32_t)1);
						} else {
							result = Value((int32_t)0);
						}
					}
					break;
				case TypeId::U32:
					result_type = TypeId::I32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						uint32_t lhs_data = evaluated_lhs.get_u32(), rhs_data = evaluated_rhs.get_u32();
						if (lhs_data < rhs_data) {
							result = Value((int32_t)-1);
						} else if (lhs_data > rhs_data) {
							result = Value((int32_t)1);
						} else {
							result = Value((int32_t)0);
						}
					}
					break;
				case TypeId::U64:
					result_type = TypeId::I32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						uint64_t lhs_data = evaluated_lhs.get_u64(), rhs_data = evaluated_rhs.get_u64();
						if (lhs_data < rhs_data) {
							result = Value((int32_t)-1);
						} else if (lhs_data > rhs_data) {
							result = Value((int32_t)1);
						} else {
							result = Value((int32_t)0);
						}
					}
					break;
				case TypeId::F32:
					result_type = TypeId::I32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						float lhs_data = evaluated_lhs.get_f32(), rhs_data = evaluated_rhs.get_f32();
						if (lhs_data < rhs_data) {
							result = Value((int32_t)-1);
						} else if (lhs_data > rhs_data) {
							result = Value((int32_t)1);
						} else {
							result = Value((int32_t)0);
						}
					}
					break;
				case TypeId::F64:
					result_type = TypeId::I32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						double lhs_data = evaluated_lhs.get_f64(), rhs_data = evaluated_rhs.get_f64();
						if (lhs_data < rhs_data) {
							result = Value((int32_t)-1);
						} else if (lhs_data > rhs_data) {
							result = Value((int32_t)1);
						} else {
							result = Value((int32_t)0);
						}
					}
					break;
				default: {
					return alloc_out_of_memory_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}

			if (reg_index != UINT32_MAX) {
				analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).type = result_type;
				if (result.value_type != ValueType::Undefined) {
					analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = result;
				}
			}
			break;
		}
		case Opcode::LSH: {
			if (cur_ins.num_operands != 2) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[0], lhs_type));
			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[1], rhs_type));

			if (rhs_type != TypeId::U32) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, rhs, rhs));

			switch (lhs_type.type_id) {
				case TypeId::I8:
					result_type = TypeId::I8;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int8_t)evaluated_lhs.get_i8() << evaluated_rhs.get_u32());
					}
					break;
				case TypeId::I16:
					result_type = TypeId::I16;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int16_t)evaluated_lhs.get_i16() << evaluated_rhs.get_u32());
					}
					break;
				case TypeId::I32:
					result_type = TypeId::I32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int32_t)evaluated_lhs.get_i32() << evaluated_rhs.get_u32());
					}
					break;
				case TypeId::I64:
					result_type = TypeId::I64;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int64_t)evaluated_lhs.get_i64() << evaluated_rhs.get_u32());
					}
					break;
				case TypeId::U8:
					result_type = TypeId::U8;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint8_t)evaluated_lhs.get_u8() << evaluated_rhs.get_u32());
					}
					break;
				case TypeId::U16:
					result_type = TypeId::U16;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint16_t)evaluated_lhs.get_u16() << evaluated_rhs.get_u32());
					}
					break;
				case TypeId::U32:
					result_type = TypeId::U32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint32_t)evaluated_lhs.get_u32() << evaluated_rhs.get_u32());
					}
					break;
				case TypeId::U64:
					result_type = TypeId::U64;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint64_t)evaluated_lhs.get_u64() << evaluated_rhs.get_u32());
					}
					break;
				default: {
					return alloc_out_of_memory_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}

			if (reg_index != UINT32_MAX) {
				analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).type = result_type;
				if (result.value_type != ValueType::Undefined) {
					analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = result;
				}
			}
			break;
		}
		case Opcode::RSH: {
			if (cur_ins.num_operands != 2) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[0], lhs_type));
			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, cur_ins.operands[1], rhs_type));

			if (rhs_type != TypeId::U32) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, rhs, rhs));

			switch (lhs_type.type_id) {
				case TypeId::I8:
					result_type = TypeId::I8;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int8_t)evaluated_lhs.get_i8() >> evaluated_rhs.get_u32());
					}
					break;
				case TypeId::I16:
					result_type = TypeId::I16;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int16_t)evaluated_lhs.get_i16() >> evaluated_rhs.get_u32());
					}
					break;
				case TypeId::I32:
					result_type = TypeId::I32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int32_t)evaluated_lhs.get_i32() >> evaluated_rhs.get_u32());
					}
					break;
				case TypeId::I64:
					result_type = TypeId::I64;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((int64_t)evaluated_lhs.get_i64() >> evaluated_rhs.get_u32());
					}
					break;
				case TypeId::U8:
					result_type = TypeId::U8;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint8_t)evaluated_lhs.get_u8() >> evaluated_rhs.get_u32());
					}
					break;
				case TypeId::U16:
					result_type = TypeId::U16;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint16_t)evaluated_lhs.get_u16() >> evaluated_rhs.get_u32());
					}
					break;
				case TypeId::U32:
					result_type = TypeId::U32;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint32_t)evaluated_lhs.get_u32() >> evaluated_rhs.get_u32());
					}
					break;
				case TypeId::U64:
					result_type = TypeId::U64;
					if (evaluated_lhs.value_type != ValueType::Undefined &&
						evaluated_rhs.value_type != ValueType::Undefined) {
						result = Value((uint64_t)evaluated_lhs.get_u64() >> evaluated_rhs.get_u32());
					}
					break;
				default: {
					return alloc_out_of_memory_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}

			if (reg_index != UINT32_MAX) {
				analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).type = result_type;
				if (result.value_type != ValueType::Undefined) {
					analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = result;
				}
			}
			break;
		}
		case Opcode::NOT: {
			if (cur_ins.num_operands != 1) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			Value lhs = cur_ins.operands[0],
				  rhs = cur_ins.operands[1],
				  evaluated_lhs(ValueType::Undefined),
				  evaluated_rhs(ValueType::Undefined),
				  result = (ValueType::Undefined);
			TypeRef lhs_type, result_type;

			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, lhs, lhs_type));

			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, lhs, lhs));

			switch (lhs_type.type_id) {
				case TypeId::I8:
					result_type = TypeId::I8;
					if (evaluated_lhs.value_type != ValueType::Undefined) {
						result = Value((int8_t)~evaluated_lhs.get_i8());
					}
					break;
				case TypeId::I16:
					result_type = TypeId::I16;
					if (evaluated_lhs.value_type != ValueType::Undefined) {
						result = Value((int16_t)~evaluated_lhs.get_i16());
					}
					break;
				case TypeId::I32:
					result_type = TypeId::I32;
					if (evaluated_lhs.value_type != ValueType::Undefined) {
						result = Value((int32_t)~evaluated_lhs.get_i32());
					}
					break;
				case TypeId::I64:
					result_type = TypeId::I64;
					if (evaluated_lhs.value_type != ValueType::Undefined) {
						result = Value((int64_t)~evaluated_lhs.get_i64());
					}
					break;
				case TypeId::U8:
					result_type = TypeId::U8;
					if (evaluated_lhs.value_type != ValueType::Undefined) {
						result = Value((uint8_t)~evaluated_lhs.get_u8());
					}
					break;
				case TypeId::U16:
					result_type = TypeId::U16;
					if (evaluated_lhs.value_type != ValueType::Undefined) {
						result = Value((uint16_t)~evaluated_lhs.get_u16());
					}
					break;
				case TypeId::U32:
					result_type = TypeId::U32;
					if (evaluated_lhs.value_type != ValueType::Undefined) {
						result = Value((uint32_t)~evaluated_lhs.get_u32());
					}
					break;
				case TypeId::U64:
					result_type = TypeId::U64;
					if (evaluated_lhs.value_type != ValueType::Undefined) {
						result = Value((uint64_t)~evaluated_lhs.get_u64());
					}
					break;
				default: {
					return alloc_out_of_memory_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}

			if (reg_index != UINT32_MAX) {
				if (result.value_type != ValueType::Undefined) {
					analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = result;
				}
			}
			break;
		}
		case Opcode::LNOT: {
			if (cur_ins.num_operands != 1) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			Value lhs = cur_ins.operands[0],
				  rhs = cur_ins.operands[1],
				  evaluated_lhs(ValueType::Undefined),
				  evaluated_rhs(ValueType::Undefined),
				  result = (ValueType::Undefined);
			TypeRef lhs_type, result_type;

			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, lhs, lhs_type));

			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, lhs, lhs));

			switch (lhs_type.type_id) {
				case TypeId::Bool:
					result_type = TypeId::Bool;
					if (evaluated_lhs.value_type != ValueType::Undefined) {
						result = Value((bool)!evaluated_lhs.get_bool());
					}
					break;
				default: {
					return alloc_out_of_memory_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}

			if (reg_index != UINT32_MAX) {
				if (result.value_type != ValueType::Undefined) {
					analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = result;
				}
			}
			break;
		}
		case Opcode::NEG: {
			if (cur_ins.num_operands != 1) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MalformedProgramError::alloc(
						analyze_context.runtime->get_fixed_alloc(),
						analyze_context.fn_object,
						analyze_context.idx_cur_ins));
			}

			Value lhs = cur_ins.operands[0],
				  rhs = cur_ins.operands[1],
				  evaluated_lhs(ValueType::Undefined),
				  evaluated_rhs(ValueType::Undefined),
				  result = (ValueType::Undefined);
			TypeRef lhs_type, result_type;

			SLAKE_RETURN_IF_EXCEPT(eval_value_type(analyze_context, lhs, lhs_type));

			SLAKE_RETURN_IF_EXCEPT(eval_const_value(analyze_context, lhs, lhs));

			switch (lhs_type.type_id) {
				case TypeId::I8:
					result_type = TypeId::I8;
					if (evaluated_lhs.value_type != ValueType::Undefined) {
						result = Value((int8_t)-evaluated_lhs.get_i8());
					}
					break;
				case TypeId::I16:
					result_type = TypeId::I16;
					if (evaluated_lhs.value_type != ValueType::Undefined) {
						result = Value((int16_t)-evaluated_lhs.get_i16());
					}
					break;
				case TypeId::I32:
					result_type = TypeId::I32;
					if (evaluated_lhs.value_type != ValueType::Undefined) {
						result = Value((int32_t)-evaluated_lhs.get_i32());
					}
					break;
				case TypeId::I64:
					result_type = TypeId::I64;
					if (evaluated_lhs.value_type != ValueType::Undefined) {
						result = Value((int64_t)-evaluated_lhs.get_i64());
					}
					break;
				case TypeId::U8:
					result_type = TypeId::U8;
					if (evaluated_lhs.value_type != ValueType::Undefined) {
						result = Value((uint8_t)evaluated_lhs.get_u8());
					}
					break;
				case TypeId::U16:
					result_type = TypeId::U16;
					if (evaluated_lhs.value_type != ValueType::Undefined) {
						result = Value((uint16_t)evaluated_lhs.get_u16());
					}
					break;
				case TypeId::U32:
					result_type = TypeId::U32;
					if (evaluated_lhs.value_type != ValueType::Undefined) {
						result = Value((uint32_t)evaluated_lhs.get_u32());
					}
					break;
				case TypeId::U64:
					result_type = TypeId::U64;
					if (evaluated_lhs.value_type != ValueType::Undefined) {
						result = Value((uint64_t)evaluated_lhs.get_u64());
					}
					break;
				case TypeId::F32:
					result_type = TypeId::F32;
					if (evaluated_lhs.value_type != ValueType::Undefined) {
						result = Value(-evaluated_lhs.get_f32());
					}
					break;
				case TypeId::F64:
					result_type = TypeId::F64;
					if (evaluated_lhs.value_type != ValueType::Undefined) {
						result = Value(-evaluated_lhs.get_f64());
					}
					break;
				default: {
					return alloc_out_of_memory_error_if_alloc_failed(
						MalformedProgramError::alloc(
							analyze_context.runtime->get_fixed_alloc(),
							analyze_context.fn_object,
							analyze_context.idx_cur_ins));
				}
			}

			if (reg_index != UINT32_MAX) {
				if (result.value_type != ValueType::Undefined) {
					analyze_context.analyzed_info_out.analyzed_reg_info.at(reg_index).expected_value = result;
				}
			}
			break;
		}
		default:
			std::terminate();
	}

	return {};
}
