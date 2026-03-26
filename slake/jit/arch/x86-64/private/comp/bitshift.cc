#include "bitshift.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

template <typename T>
[[nodiscard]] InternalExceptionPointer compile_int_shl_instruction(
	JITCompileContext &compile_context,
	const Instruction &cur_ins,
	const Value &lhs_expected_value,
	const Value &rhs_expected_value) noexcept {
	InternalExceptionPointer exception;
	uint32_t output_reg_index = cur_ins.output;

	if (rhs_expected_value.value_type != ValueType::Undefined) {
		uint32_t lhs_reg_index = cur_ins.operands[0].get_reg_index();
		const RegisterId lhs_reg_id = compile_context.alloc_gp_reg();

		uint32_t rhs_data = cur_ins.operands[1].get_u32();

		if (compile_context.is_reg_in_use(lhs_reg_id)) {
			int32_t off;
			size_t size;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(lhs_reg_id, off, size));
		}

		if constexpr (sizeof(T) == sizeof(uint8_t)) {
			if (rhs_data >= 8) {
				uint8_t imm0 = 0;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_imm8_to_reg8_ins(
																	lhs_reg_id,
																	(uint8_t *)&imm0)));
				VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, lhs_reg_id, sizeof(T));
				if (!output_vreg_state)
					return OutOfMemoryError::alloc();
			}
		} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
			if (rhs_data >= 16) {
				uint16_t imm0 = 0;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_imm16_to_reg16_ins(
																	lhs_reg_id,
																	(uint8_t *)&imm0)));
				VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, lhs_reg_id, sizeof(T));
				if (!output_vreg_state)
					return OutOfMemoryError::alloc();
			}
		} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
			if (rhs_data >= 32) {
				uint32_t imm0 = 0;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_imm32_to_reg32_ins(
																	lhs_reg_id,
																	(uint8_t *)&imm0)));
				VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, lhs_reg_id, sizeof(T));
				if (!output_vreg_state)
					return OutOfMemoryError::alloc();
			}
		} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
			if (rhs_data >= 64) {
				uint64_t imm0 = 0;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_imm64_to_reg64_ins(
																	lhs_reg_id,
																	(uint8_t *)&imm0)));
				VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, lhs_reg_id, sizeof(T));
				if (!output_vreg_state)
					return OutOfMemoryError::alloc();
			}
		}

		uint8_t narrowed_rhs_data = (uint8_t)rhs_data;

		VirtualRegState &lhs_vreg_state = compile_context.virtual_reg_states.at(lhs_reg_index);
		if (lhs_vreg_state.save_offset != INT32_MIN) {
			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_mem_to_reg8_ins(
																	lhs_reg_id,
																	MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
			} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_mem_to_reg16_ins(
																	lhs_reg_id,
																	MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
			} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_mem_to_reg32_ins(
																	lhs_reg_id,
																	MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
			} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_mem_to_reg64_ins(
																	lhs_reg_id,
																	MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand size");
			}
		} else {
			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_reg8_to_reg8_ins(
																	lhs_reg_id,
																	lhs_vreg_state.phy_reg)));
			} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_reg16_to_reg16_ins(
																	lhs_reg_id,
																	lhs_vreg_state.phy_reg)));
			} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_reg32_to_reg32_ins(
																	lhs_reg_id,
																	lhs_vreg_state.phy_reg)));
			} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_reg64_to_reg64_ins(
																	lhs_reg_id,
																	lhs_vreg_state.phy_reg)));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand size");
			}
		}

		VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, lhs_reg_id, sizeof(T));
		if (!output_vreg_state)
			return OutOfMemoryError::alloc();

		if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_shl_reg8_with_imm8_ins(lhs_reg_id, narrowed_rhs_data)));
		} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_shl_reg16_with_imm8_ins(lhs_reg_id, narrowed_rhs_data)));
		} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_shl_reg32_with_imm8_ins(lhs_reg_id, narrowed_rhs_data)));
		} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_shl_reg64_with_imm8_ins(lhs_reg_id, narrowed_rhs_data)));
		} else {
			static_assert(!std::is_same_v<T, T>, "Invalid operand type");
		}
	} else {
		if (lhs_expected_value.value_type != ValueType::Undefined) {
			uint32_t rhs_reg_index = cur_ins.operands[1].get_reg_index();
			const RegisterId rhs_reg_id = compile_context.alloc_gp_reg();

			uint32_t lhs_data = cur_ins.operands[1].get_u32();

			if (compile_context.is_reg_in_use(rhs_reg_id)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(rhs_reg_id, off, size));
			}

			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				if (lhs_data >= 8) {
					uint8_t imm0 = 0;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_imm8_to_reg8_ins(
																		rhs_reg_id,
																		(uint8_t *)&imm0)));
					VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, rhs_reg_id, sizeof(T));
					if (!output_vreg_state)
						return OutOfMemoryError::alloc();
				}
			} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
				if (lhs_data >= 16) {
					uint16_t imm0 = 0;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_imm16_to_reg16_ins(
																		rhs_reg_id,
																		(uint8_t *)&imm0)));
					VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, rhs_reg_id, sizeof(T));
					if (!output_vreg_state)
						return OutOfMemoryError::alloc();
				}
			} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
				if (lhs_data >= 32) {
					uint32_t imm0 = 0;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_imm32_to_reg32_ins(
																		rhs_reg_id,
																		(uint8_t *)&imm0)));
					VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, rhs_reg_id, sizeof(T));
					if (!output_vreg_state)
						return OutOfMemoryError::alloc();
				}
			} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
				if (lhs_data >= 64) {
					uint64_t imm0 = 0;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_imm64_to_reg64_ins(
																		rhs_reg_id,
																		(uint8_t *)&imm0)));
					VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, rhs_reg_id, sizeof(T));
					if (!output_vreg_state)
						return OutOfMemoryError::alloc();
				}
			}

			uint8_t narrowed_lhs_data = (uint8_t)lhs_data;

			VirtualRegState &rhs_vreg_state = compile_context.virtual_reg_states.at(rhs_reg_index);
			if (rhs_vreg_state.save_offset != INT32_MIN) {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg8_ins(
																		rhs_reg_id,
																		MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg16_ins(
																		rhs_reg_id,
																		MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg32_ins(
																		rhs_reg_id,
																		MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg64_ins(
																		rhs_reg_id,
																		MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			} else {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg8_to_reg8_ins(
																		rhs_reg_id,
																		rhs_vreg_state.phy_reg)));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg16_to_reg16_ins(
																		rhs_reg_id,
																		rhs_vreg_state.phy_reg)));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg32_to_reg32_ins(
																		rhs_reg_id,
																		rhs_vreg_state.phy_reg)));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg64_to_reg64_ins(
																		rhs_reg_id,
																		rhs_vreg_state.phy_reg)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			}

			VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, rhs_reg_id, sizeof(T));
			if (!output_vreg_state)
				return OutOfMemoryError::alloc();

			if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_shl_reg8_with_imm8_ins(rhs_reg_id, narrowed_lhs_data)));
			} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_shl_reg16_with_imm8_ins(rhs_reg_id, narrowed_lhs_data)));
			} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_shl_reg32_with_imm8_ins(rhs_reg_id, narrowed_lhs_data)));
			} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_shl_reg64_with_imm8_ins(rhs_reg_id, narrowed_lhs_data)));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand type");
			}
		} else {
			uint32_t rhs_reg_index = cur_ins.operands[1].get_reg_index();

			const RegisterId lhs_reg_id = compile_context.alloc_gp_reg();

			if (compile_context.is_reg_in_use(lhs_reg_id)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(lhs_reg_id, off, size));
			}

			VirtualRegState &lhs_vreg_state = compile_context.virtual_reg_states.at(lhs_reg_id);
			if (lhs_vreg_state.save_offset != INT32_MIN) {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg8_ins(
																		lhs_reg_id,
																		MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg16_ins(
																		lhs_reg_id,
																		MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg32_ins(
																		lhs_reg_id,
																		MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg64_ins(
																		lhs_reg_id,
																		MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			} else {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg8_to_reg8_ins(
																		lhs_reg_id,
																		lhs_vreg_state.phy_reg)));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg16_to_reg16_ins(
																		lhs_reg_id,
																		lhs_vreg_state.phy_reg)));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg32_to_reg32_ins(
																		lhs_reg_id,
																		lhs_vreg_state.phy_reg)));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg64_to_reg64_ins(
																		lhs_reg_id,
																		lhs_vreg_state.phy_reg)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			}

			VirtualRegState &rhs_vreg_state = compile_context.virtual_reg_states.at(rhs_reg_index);
			int32_t cx_off = INT32_MIN;
			size_t cx_size;

			// TODO: Handle RCX >= 8, 16, 32, 64.
			if (rhs_vreg_state.save_offset != INT32_MIN) {
				if (compile_context.is_reg_in_use(REG_RCX)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg32(REG_RCX, cx_off, cx_size));
				}
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_mem_to_reg32_ins(REG_RCX, MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));

				if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_shl_reg8_with_cl_ins(lhs_reg_id)));
				} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_shl_reg16_with_cl_ins(lhs_reg_id)));
				} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_shl_reg32_with_cl_ins(lhs_reg_id)));
				} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_shl_reg64_with_cl_ins(lhs_reg_id)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}
			} else {
				if (rhs_vreg_state.phy_reg != REG_RCX && compile_context.is_reg_in_use(REG_RCX)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg8(REG_RCX, cx_off, cx_size));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg32_to_reg32_ins(REG_RCX, rhs_vreg_state.phy_reg)));
				}
				if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_shl_reg8_with_cl_ins(lhs_reg_id)));
				} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_shl_reg16_with_cl_ins(lhs_reg_id)));
				} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_shl_reg32_with_cl_ins(lhs_reg_id)));
				} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_shl_reg64_with_cl_ins(lhs_reg_id)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}
			}

			VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, lhs_reg_id, sizeof(T));
			if (!output_vreg_state)
				return OutOfMemoryError::alloc();

			if (cx_off != INT32_MIN) {
				SLAKE_RETURN_IF_EXCEPT(compile_context.pop_reg32(REG_RCX, cx_off));
			}
		}
	}

	return {};
}

InternalExceptionPointer slake::jit::x86_64::compile_shl_instruction(
	JITCompileContext &compile_context,
	opti::ProgramAnalyzedInfo &analyzed_info,
	size_t off_ins,
	const Instruction &cur_ins) noexcept {
	InternalExceptionPointer exception;
	uint32_t output_reg_index = UINT32_MAX;
	auto &output_reg_info = analyzed_info.analyzed_reg_info.at(output_reg_index);

	Value lhs = cur_ins.operands[0], rhs = cur_ins.operands[1];
	Value lhs_expected_value(ValueType::Undefined), rhs_expected_value(ValueType::Undefined);

	switch (lhs.value_type) {
		case ValueType::I8:
		case ValueType::I16:
		case ValueType::I32:
		case ValueType::I64:
		case ValueType::U8:
		case ValueType::U16:
		case ValueType::U32:
		case ValueType::U64:
			lhs_expected_value = lhs;
			break;
		case ValueType::RegIndex:
			lhs_expected_value = analyzed_info.analyzed_reg_info.at(lhs.get_reg_index()).expected_value;
			break;
		default:
			// Malformed function
			std::terminate();
	}

	switch (rhs.value_type) {
		case ValueType::I8:
		case ValueType::I16:
		case ValueType::I32:
		case ValueType::I64:
		case ValueType::U8:
		case ValueType::U16:
		case ValueType::U32:
		case ValueType::U64:
			lhs_expected_value = rhs;
			break;
		case ValueType::RegIndex:
			lhs_expected_value = analyzed_info.analyzed_reg_info.at(rhs.get_reg_index()).expected_value;
			break;
		default:
			// Malformed function
			std::terminate();
	}

	switch (output_reg_info.type.type_id) {
		case TypeId::I8: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_shl_instruction<int8_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::I16: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_shl_instruction<int16_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::I32: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_shl_instruction<int32_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::I64: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_shl_instruction<int64_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::U8: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_shl_instruction<uint8_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::U16: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_shl_instruction<uint16_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::U32: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_shl_instruction<uint32_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::U64: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_shl_instruction<uint64_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		default:
			// The function is malformed
			std::terminate();
	}

	return {};
}

template <typename T>
[[nodiscard]] InternalExceptionPointer compile_int_shr_instruction(
	JITCompileContext &compile_context,
	const Instruction &cur_ins,
	const Value &lhs_expected_value,
	const Value &rhs_expected_value) noexcept {
	InternalExceptionPointer exception;
	uint32_t output_reg_index = cur_ins.output;

	if (rhs_expected_value.value_type != ValueType::Undefined) {
		uint32_t lhs_reg_index = cur_ins.operands[0].get_reg_index();
		const RegisterId lhs_reg_id = compile_context.alloc_gp_reg();

		uint32_t rhs_data = cur_ins.operands[1].get_u32();

		if (compile_context.is_reg_in_use(lhs_reg_id)) {
			int32_t off;
			size_t size;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(lhs_reg_id, off, size));
		}

		if constexpr (sizeof(T) == sizeof(uint8_t)) {
			if (rhs_data >= 8) {
				uint8_t imm0 = 0;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_imm8_to_reg8_ins(
																	lhs_reg_id,
																	(uint8_t *)&imm0)));
				VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, lhs_reg_id, sizeof(T));
				if (!output_vreg_state)
					return OutOfMemoryError::alloc();
			}
		} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
			if (rhs_data >= 16) {
				uint16_t imm0 = 0;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_imm16_to_reg16_ins(
																	lhs_reg_id,
																	(uint8_t *)&imm0)));
				VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, lhs_reg_id, sizeof(T));
				if (!output_vreg_state)
					return OutOfMemoryError::alloc();
			}
		} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
			if (rhs_data >= 32) {
				uint32_t imm0 = 0;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_imm32_to_reg32_ins(
																	lhs_reg_id,
																	(uint8_t *)&imm0)));
				VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, lhs_reg_id, sizeof(T));
				if (!output_vreg_state)
					return OutOfMemoryError::alloc();
			}
		} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
			if (rhs_data >= 64) {
				uint64_t imm0 = 0;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_imm64_to_reg64_ins(
																	lhs_reg_id,
																	(uint8_t *)&imm0)));
				VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, lhs_reg_id, sizeof(T));
				if (!output_vreg_state)
					return OutOfMemoryError::alloc();
			}
		}

		uint8_t narrowed_rhs_data = (uint8_t)rhs_data;

		VirtualRegState &lhs_vreg_state = compile_context.virtual_reg_states.at(lhs_reg_index);
		if (lhs_vreg_state.save_offset != INT32_MIN) {
			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_mem_to_reg8_ins(
																	lhs_reg_id,
																	MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
			} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_mem_to_reg16_ins(
																	lhs_reg_id,
																	MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
			} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_mem_to_reg32_ins(
																	lhs_reg_id,
																	MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
			} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_mem_to_reg64_ins(
																	lhs_reg_id,
																	MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand size");
			}
		} else {
			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_reg8_to_reg8_ins(
																	lhs_reg_id,
																	lhs_vreg_state.phy_reg)));
			} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_reg16_to_reg16_ins(
																	lhs_reg_id,
																	lhs_vreg_state.phy_reg)));
			} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_reg32_to_reg32_ins(
																	lhs_reg_id,
																	lhs_vreg_state.phy_reg)));
			} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_reg64_to_reg64_ins(
																	lhs_reg_id,
																	lhs_vreg_state.phy_reg)));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand size");
			}
		}

		VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, lhs_reg_id, sizeof(T));
		if (!output_vreg_state)
			return OutOfMemoryError::alloc();

		if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_shr_reg8_with_imm8_ins(lhs_reg_id, narrowed_rhs_data)));
		} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_shr_reg16_with_imm8_ins(lhs_reg_id, narrowed_rhs_data)));
		} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_shr_reg32_with_imm8_ins(lhs_reg_id, narrowed_rhs_data)));
		} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_shr_reg64_with_imm8_ins(lhs_reg_id, narrowed_rhs_data)));
		} else {
			static_assert(!std::is_same_v<T, T>, "Invalid operand type");
		}
	} else {
		if (lhs_expected_value.value_type != ValueType::Undefined) {
			uint32_t rhs_reg_index = cur_ins.operands[1].get_reg_index();
			const RegisterId rhs_reg_id = compile_context.alloc_gp_reg();

			uint32_t lhs_data = cur_ins.operands[1].get_u32();

			if (compile_context.is_reg_in_use(rhs_reg_id)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(rhs_reg_id, off, size));
			}

			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				if (lhs_data >= 8) {
					uint8_t imm0 = 0;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_imm8_to_reg8_ins(
																		rhs_reg_id,
																		(uint8_t *)&imm0)));
					VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, rhs_reg_id, sizeof(T));
					if (!output_vreg_state)
						return OutOfMemoryError::alloc();
				}
			} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
				if (lhs_data >= 16) {
					uint16_t imm0 = 0;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_imm16_to_reg16_ins(
																		rhs_reg_id,
																		(uint8_t *)&imm0)));
					VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, rhs_reg_id, sizeof(T));
					if (!output_vreg_state)
						return OutOfMemoryError::alloc();
				}
			} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
				if (lhs_data >= 32) {
					uint32_t imm0 = 0;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_imm32_to_reg32_ins(
																		rhs_reg_id,
																		(uint8_t *)&imm0)));
					VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, rhs_reg_id, sizeof(T));
					if (!output_vreg_state)
						return OutOfMemoryError::alloc();
				}
			} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
				if (lhs_data >= 64) {
					uint64_t imm0 = 0;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_imm64_to_reg64_ins(
																		rhs_reg_id,
																		(uint8_t *)&imm0)));
					VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, rhs_reg_id, sizeof(T));
					if (!output_vreg_state)
						return OutOfMemoryError::alloc();
				}
			}

			uint8_t narrowed_lhs_data = (uint8_t)lhs_data;

			VirtualRegState &rhs_vreg_state = compile_context.virtual_reg_states.at(rhs_reg_index);
			if (rhs_vreg_state.save_offset != INT32_MIN) {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg8_ins(
																		rhs_reg_id,
																		MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg16_ins(
																		rhs_reg_id,
																		MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg32_ins(
																		rhs_reg_id,
																		MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg64_ins(
																		rhs_reg_id,
																		MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			} else {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg8_to_reg8_ins(
																		rhs_reg_id,
																		rhs_vreg_state.phy_reg)));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg16_to_reg16_ins(
																		rhs_reg_id,
																		rhs_vreg_state.phy_reg)));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg32_to_reg32_ins(
																		rhs_reg_id,
																		rhs_vreg_state.phy_reg)));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg64_to_reg64_ins(
																		rhs_reg_id,
																		rhs_vreg_state.phy_reg)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			}

			VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, rhs_reg_id, sizeof(T));
			if (!output_vreg_state)
				return OutOfMemoryError::alloc();

			if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_shr_reg8_with_imm8_ins(rhs_reg_id, narrowed_lhs_data)));
			} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_shr_reg16_with_imm8_ins(rhs_reg_id, narrowed_lhs_data)));
			} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_shr_reg32_with_imm8_ins(rhs_reg_id, narrowed_lhs_data)));
			} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_shr_reg64_with_imm8_ins(rhs_reg_id, narrowed_lhs_data)));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand type");
			}
		} else {
			uint32_t rhs_reg_index = cur_ins.operands[1].get_reg_index();

			const RegisterId lhs_reg_id = compile_context.alloc_gp_reg();

			if (compile_context.is_reg_in_use(lhs_reg_id)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(lhs_reg_id, off, size));
			}

			VirtualRegState &lhs_vreg_state = compile_context.virtual_reg_states.at(lhs_reg_id);
			if (lhs_vreg_state.save_offset != INT32_MIN) {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg8_ins(
																		lhs_reg_id,
																		MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg16_ins(
																		lhs_reg_id,
																		MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg32_ins(
																		lhs_reg_id,
																		MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg64_ins(
																		lhs_reg_id,
																		MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			} else {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg8_to_reg8_ins(
																		lhs_reg_id,
																		lhs_vreg_state.phy_reg)));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg16_to_reg16_ins(
																		lhs_reg_id,
																		lhs_vreg_state.phy_reg)));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg32_to_reg32_ins(
																		lhs_reg_id,
																		lhs_vreg_state.phy_reg)));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg64_to_reg64_ins(
																		lhs_reg_id,
																		lhs_vreg_state.phy_reg)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			}

			VirtualRegState &rhs_vreg_state = compile_context.virtual_reg_states.at(rhs_reg_index);
			int32_t cx_off = INT32_MIN;
			size_t cx_size;

			// TODO: Handle RCX >= 8, 16, 32, 64.
			if (rhs_vreg_state.save_offset != INT32_MIN) {
				if (compile_context.is_reg_in_use(REG_RCX)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg32(REG_RCX, cx_off, cx_size));
				}
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_mem_to_reg32_ins(REG_RCX, MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));

				if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_shr_reg8_with_cl_ins(lhs_reg_id)));
				} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_shr_reg16_with_cl_ins(lhs_reg_id)));
				} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_shr_reg32_with_cl_ins(lhs_reg_id)));
				} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_shr_reg64_with_cl_ins(lhs_reg_id)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}
			} else {
				if (rhs_vreg_state.phy_reg != REG_RCX && compile_context.is_reg_in_use(REG_RCX)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg8(REG_RCX, cx_off, cx_size));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg32_to_reg32_ins(REG_RCX, rhs_vreg_state.phy_reg)));
				}
				if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_shr_reg8_with_cl_ins(lhs_reg_id)));
				} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_shr_reg16_with_cl_ins(lhs_reg_id)));
				} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_shr_reg32_with_cl_ins(lhs_reg_id)));
				} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_shr_reg64_with_cl_ins(lhs_reg_id)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}
			}

			VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, lhs_reg_id, sizeof(T));
			if (!output_vreg_state)
				return OutOfMemoryError::alloc();

			if (cx_off != INT32_MIN) {
				SLAKE_RETURN_IF_EXCEPT(compile_context.pop_reg32(REG_RCX, cx_off));
			}
		}
	}

	return {};
}

InternalExceptionPointer slake::jit::x86_64::compile_shr_instruction(
	JITCompileContext &compile_context,
	opti::ProgramAnalyzedInfo &analyzed_info,
	size_t off_ins,
	const Instruction &cur_ins) noexcept {
	InternalExceptionPointer exception;
	uint32_t output_reg_index = UINT32_MAX;
	auto &output_reg_info = analyzed_info.analyzed_reg_info.at(output_reg_index);

	Value lhs = cur_ins.operands[0], rhs = cur_ins.operands[1];
	Value lhs_expected_value(ValueType::Undefined), rhs_expected_value(ValueType::Undefined);

	switch (lhs.value_type) {
		case ValueType::I8:
		case ValueType::I16:
		case ValueType::I32:
		case ValueType::I64:
		case ValueType::U8:
		case ValueType::U16:
		case ValueType::U32:
		case ValueType::U64:
			lhs_expected_value = lhs;
			break;
		case ValueType::RegIndex:
			lhs_expected_value = analyzed_info.analyzed_reg_info.at(lhs.get_reg_index()).expected_value;
			break;
		default:
			// Malformed function
			std::terminate();
	}

	switch (rhs.value_type) {
		case ValueType::I8:
		case ValueType::I16:
		case ValueType::I32:
		case ValueType::I64:
		case ValueType::U8:
		case ValueType::U16:
		case ValueType::U32:
		case ValueType::U64:
			lhs_expected_value = rhs;
			break;
		case ValueType::RegIndex:
			lhs_expected_value = analyzed_info.analyzed_reg_info.at(rhs.get_reg_index()).expected_value;
			break;
		default:
			// Malformed function
			std::terminate();
	}

	switch (output_reg_info.type.type_id) {
		case TypeId::I8: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_shr_instruction<int8_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::I16: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_shr_instruction<int16_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::I32: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_shr_instruction<int32_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::I64: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_shr_instruction<int64_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::U8: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_shr_instruction<uint8_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::U16: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_shr_instruction<uint16_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::U32: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_shr_instruction<uint32_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::U64: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_shr_instruction<uint64_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		default:
			// The function is malformed
			std::terminate();
	}

	return {};
}

template <typename T>
[[nodiscard]] InternalExceptionPointer compile_int_sar_instruction(
	JITCompileContext &compile_context,
	const Instruction &cur_ins,
	const Value &lhs_expected_value,
	const Value &rhs_expected_value) noexcept {
	InternalExceptionPointer exception;
	uint32_t output_reg_index = cur_ins.output;

	if (rhs_expected_value.value_type != ValueType::Undefined) {
		uint32_t lhs_reg_index = cur_ins.operands[0].get_reg_index();
		const RegisterId lhs_reg_id = compile_context.alloc_gp_reg();

		uint32_t rhs_data = cur_ins.operands[1].get_u32();

		if (compile_context.is_reg_in_use(lhs_reg_id)) {
			int32_t off;
			size_t size;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(lhs_reg_id, off, size));
		}

		if constexpr (sizeof(T) == sizeof(uint8_t)) {
			if (rhs_data >= 8) {
				uint8_t imm0 = 0;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_imm8_to_reg8_ins(
																	lhs_reg_id,
																	(uint8_t *)&imm0)));
				VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, lhs_reg_id, sizeof(T));
				if (!output_vreg_state)
					return OutOfMemoryError::alloc();
			}
		} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
			if (rhs_data >= 16) {
				uint16_t imm0 = 0;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_imm16_to_reg16_ins(
																	lhs_reg_id,
																	(uint8_t *)&imm0)));
				VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, lhs_reg_id, sizeof(T));
				if (!output_vreg_state)
					return OutOfMemoryError::alloc();
			}
		} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
			if (rhs_data >= 32) {
				uint32_t imm0 = 0;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_imm32_to_reg32_ins(
																	lhs_reg_id,
																	(uint8_t *)&imm0)));
				VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, lhs_reg_id, sizeof(T));
				if (!output_vreg_state)
					return OutOfMemoryError::alloc();
			}
		} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
			if (rhs_data >= 64) {
				uint64_t imm0 = 0;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_imm64_to_reg64_ins(
																	lhs_reg_id,
																	(uint8_t *)&imm0)));
				VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, lhs_reg_id, sizeof(T));
				if (!output_vreg_state)
					return OutOfMemoryError::alloc();
			}
		}

		uint8_t narrowed_rhs_data = (uint8_t)rhs_data;

		VirtualRegState &lhs_vreg_state = compile_context.virtual_reg_states.at(lhs_reg_index);
		if (lhs_vreg_state.save_offset != INT32_MIN) {
			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_mem_to_reg8_ins(
																	lhs_reg_id,
																	MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
			} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_mem_to_reg16_ins(
																	lhs_reg_id,
																	MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
			} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_mem_to_reg32_ins(
																	lhs_reg_id,
																	MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
			} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_mem_to_reg64_ins(
																	lhs_reg_id,
																	MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand size");
			}
		} else {
			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_reg8_to_reg8_ins(
																	lhs_reg_id,
																	lhs_vreg_state.phy_reg)));
			} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_reg16_to_reg16_ins(
																	lhs_reg_id,
																	lhs_vreg_state.phy_reg)));
			} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_reg32_to_reg32_ins(
																	lhs_reg_id,
																	lhs_vreg_state.phy_reg)));
			} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_reg64_to_reg64_ins(
																	lhs_reg_id,
																	lhs_vreg_state.phy_reg)));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand size");
			}
		}

		VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, lhs_reg_id, sizeof(T));
		if (!output_vreg_state)
			return OutOfMemoryError::alloc();

		if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_sar_reg8_with_imm8_ins(lhs_reg_id, narrowed_rhs_data)));
		} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_sar_reg16_with_imm8_ins(lhs_reg_id, narrowed_rhs_data)));
		} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_sar_reg32_with_imm8_ins(lhs_reg_id, narrowed_rhs_data)));
		} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_sar_reg64_with_imm8_ins(lhs_reg_id, narrowed_rhs_data)));
		} else {
			static_assert(!std::is_same_v<T, T>, "Invalid operand type");
		}
	} else {
		if (lhs_expected_value.value_type != ValueType::Undefined) {
			uint32_t rhs_reg_index = cur_ins.operands[1].get_reg_index();
			const RegisterId rhs_reg_id = compile_context.alloc_gp_reg();

			uint32_t lhs_data = cur_ins.operands[1].get_u32();

			if (compile_context.is_reg_in_use(rhs_reg_id)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(rhs_reg_id, off, size));
			}

			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				if (lhs_data >= 8) {
					uint8_t imm0 = 0;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_imm8_to_reg8_ins(
																		rhs_reg_id,
																		(uint8_t *)&imm0)));
					VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, rhs_reg_id, sizeof(T));
					if (!output_vreg_state)
						return OutOfMemoryError::alloc();
				}
			} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
				if (lhs_data >= 16) {
					uint16_t imm0 = 0;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_imm16_to_reg16_ins(
																		rhs_reg_id,
																		(uint8_t *)&imm0)));
					VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, rhs_reg_id, sizeof(T));
					if (!output_vreg_state)
						return OutOfMemoryError::alloc();
				}
			} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
				if (lhs_data >= 32) {
					uint32_t imm0 = 0;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_imm32_to_reg32_ins(
																		rhs_reg_id,
																		(uint8_t *)&imm0)));
					VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, rhs_reg_id, sizeof(T));
					if (!output_vreg_state)
						return OutOfMemoryError::alloc();
				}
			} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
				if (lhs_data >= 64) {
					uint64_t imm0 = 0;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_imm64_to_reg64_ins(
																		rhs_reg_id,
																		(uint8_t *)&imm0)));
					VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, rhs_reg_id, sizeof(T));
					if (!output_vreg_state)
						return OutOfMemoryError::alloc();
				}
			}

			uint8_t narrowed_lhs_data = (uint8_t)lhs_data;

			VirtualRegState &rhs_vreg_state = compile_context.virtual_reg_states.at(rhs_reg_index);
			if (rhs_vreg_state.save_offset != INT32_MIN) {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg8_ins(
																		rhs_reg_id,
																		MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg16_ins(
																		rhs_reg_id,
																		MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg32_ins(
																		rhs_reg_id,
																		MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg64_ins(
																		rhs_reg_id,
																		MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			} else {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg8_to_reg8_ins(
																		rhs_reg_id,
																		rhs_vreg_state.phy_reg)));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg16_to_reg16_ins(
																		rhs_reg_id,
																		rhs_vreg_state.phy_reg)));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg32_to_reg32_ins(
																		rhs_reg_id,
																		rhs_vreg_state.phy_reg)));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg64_to_reg64_ins(
																		rhs_reg_id,
																		rhs_vreg_state.phy_reg)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			}

			VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, rhs_reg_id, sizeof(T));
			if (!output_vreg_state)
				return OutOfMemoryError::alloc();

			if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_sar_reg8_with_imm8_ins(rhs_reg_id, narrowed_lhs_data)));
			} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_sar_reg16_with_imm8_ins(rhs_reg_id, narrowed_lhs_data)));
			} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_sar_reg32_with_imm8_ins(rhs_reg_id, narrowed_lhs_data)));
			} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_sar_reg64_with_imm8_ins(rhs_reg_id, narrowed_lhs_data)));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand type");
			}
		} else {
			uint32_t rhs_reg_index = cur_ins.operands[1].get_reg_index();

			const RegisterId lhs_reg_id = compile_context.alloc_gp_reg();

			if (compile_context.is_reg_in_use(lhs_reg_id)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(lhs_reg_id, off, size));
			}

			VirtualRegState &lhs_vreg_state = compile_context.virtual_reg_states.at(lhs_reg_id);
			if (lhs_vreg_state.save_offset != INT32_MIN) {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg8_ins(
																		lhs_reg_id,
																		MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg16_ins(
																		lhs_reg_id,
																		MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg32_ins(
																		lhs_reg_id,
																		MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg64_ins(
																		lhs_reg_id,
																		MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			} else {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg8_to_reg8_ins(
																		lhs_reg_id,
																		lhs_vreg_state.phy_reg)));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg16_to_reg16_ins(
																		lhs_reg_id,
																		lhs_vreg_state.phy_reg)));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg32_to_reg32_ins(
																		lhs_reg_id,
																		lhs_vreg_state.phy_reg)));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg64_to_reg64_ins(
																		lhs_reg_id,
																		lhs_vreg_state.phy_reg)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			}

			VirtualRegState &rhs_vreg_state = compile_context.virtual_reg_states.at(rhs_reg_index);
			int32_t cx_off = INT32_MIN;
			size_t cx_size;

			// TODO: Handle RCX >= 8, 16, 32, 64.
			if (rhs_vreg_state.save_offset != INT32_MIN) {
				if (compile_context.is_reg_in_use(REG_RCX)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg32(REG_RCX, cx_off, cx_size));
				}
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_mem_to_reg32_ins(REG_RCX, MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));

				if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_sar_reg8_with_cl_ins(lhs_reg_id)));
				} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_sar_reg16_with_cl_ins(lhs_reg_id)));
				} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_sar_reg32_with_cl_ins(lhs_reg_id)));
				} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_sar_reg64_with_cl_ins(lhs_reg_id)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}
			} else {
				if (rhs_vreg_state.phy_reg != REG_RCX && compile_context.is_reg_in_use(REG_RCX)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg8(REG_RCX, cx_off, cx_size));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg32_to_reg32_ins(REG_RCX, rhs_vreg_state.phy_reg)));
				}
				if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_sar_reg8_with_cl_ins(lhs_reg_id)));
				} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_sar_reg16_with_cl_ins(lhs_reg_id)));
				} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_sar_reg32_with_cl_ins(lhs_reg_id)));
				} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_sar_reg64_with_cl_ins(lhs_reg_id)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}
			}

			VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, lhs_reg_id, sizeof(T));
			if (!output_vreg_state)
				return OutOfMemoryError::alloc();

			if (cx_off != INT32_MIN) {
				SLAKE_RETURN_IF_EXCEPT(compile_context.pop_reg32(REG_RCX, cx_off));
			}
		}
	}

	return {};
}

InternalExceptionPointer slake::jit::x86_64::compile_sar_instruction(
	JITCompileContext &compile_context,
	opti::ProgramAnalyzedInfo &analyzed_info,
	size_t off_ins,
	const Instruction &cur_ins) noexcept {
	InternalExceptionPointer exception;
	uint32_t output_reg_index = UINT32_MAX;
	auto &output_reg_info = analyzed_info.analyzed_reg_info.at(output_reg_index);

	Value lhs = cur_ins.operands[0], rhs = cur_ins.operands[1];
	Value lhs_expected_value(ValueType::Undefined), rhs_expected_value(ValueType::Undefined);

	switch (lhs.value_type) {
		case ValueType::I8:
		case ValueType::I16:
		case ValueType::I32:
		case ValueType::I64:
		case ValueType::U8:
		case ValueType::U16:
		case ValueType::U32:
		case ValueType::U64:
			lhs_expected_value = lhs;
			break;
		case ValueType::RegIndex:
			lhs_expected_value = analyzed_info.analyzed_reg_info.at(lhs.get_reg_index()).expected_value;
			break;
		default:
			// Malformed function
			std::terminate();
	}

	switch (rhs.value_type) {
		case ValueType::I8:
		case ValueType::I16:
		case ValueType::I32:
		case ValueType::I64:
		case ValueType::U8:
		case ValueType::U16:
		case ValueType::U32:
		case ValueType::U64:
			lhs_expected_value = rhs;
			break;
		case ValueType::RegIndex:
			lhs_expected_value = analyzed_info.analyzed_reg_info.at(rhs.get_reg_index()).expected_value;
			break;
		default:
			// Malformed function
			std::terminate();
	}

	switch (output_reg_info.type.type_id) {
		case TypeId::I8: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_sar_instruction<int8_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::I16: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_sar_instruction<int16_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::I32: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_sar_instruction<int32_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::I64: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_sar_instruction<int64_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::U8: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_sar_instruction<uint8_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::U16: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_sar_instruction<uint16_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::U32: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_sar_instruction<uint32_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::U64: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_sar_instruction<uint64_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		default:
			// The function is malformed
			std::terminate();
	}

	return {};
}
