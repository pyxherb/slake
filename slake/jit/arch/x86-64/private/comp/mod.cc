#include "mod.h"
#include <slake/flib/math/fmod.h>

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

template <typename T>
[[nodiscard]] InternalExceptionPointer compile_int_mod_instruction(
	JITCompileContext &compile_context,
	const Instruction &cur_ins,
	const Value &lhs_expected_value,
	const Value &rhs_expected_value) noexcept {
	InternalExceptionPointer exception;
	uint32_t output_reg_index = cur_ins.output;

	if (rhs_expected_value.value_type != ValueType::Undefined) {
		uint32_t lhs_reg_index = cur_ins.operands[0].get_reg_index();
		int32_t saved_rax_off = INT32_MIN;
		size_t saved_rax_size;
		RegisterId lhs_reg_id;

		if constexpr (sizeof(T) == sizeof(uint8_t)) {
			lhs_reg_id = REG_RAX;
		} else {
			lhs_reg_id = REG_RDX;
			if (compile_context.is_reg_in_use(REG_RAX)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(REG_RAX, saved_rax_off, saved_rax_size));
			}
		}
		if (compile_context.is_reg_in_use(lhs_reg_id)) {
			int32_t off;
			size_t size;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(lhs_reg_id, off, size));
		}

		VirtualRegState &lhs_vreg_state = compile_context.virtual_reg_states.at(lhs_reg_index);
		if (lhs_vreg_state.save_offset != INT32_MIN) {
			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_mem_to_reg8_ins(
																	REG_RAX,
																	MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
			} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_mem_to_reg16_ins(
																	REG_RAX,
																	MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
			} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_mem_to_reg32_ins(
																	REG_RAX,
																	MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
			} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_mem_to_reg64_ins(
																	REG_RAX,
																	MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand size");
			}
		} else {
			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_reg8_to_reg8_ins(
																	REG_RAX,
																	lhs_vreg_state.phy_reg)));
			} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_reg16_to_reg16_ins(
																	REG_RAX,
																	lhs_vreg_state.phy_reg)));
			} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_reg32_to_reg32_ins(
																	REG_RAX,
																	lhs_vreg_state.phy_reg)));
			} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																emit_mov_reg64_to_reg64_ins(
																	REG_RAX,
																	lhs_vreg_state.phy_reg)));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand size");
			}
		}

		// Try to allocate a new temporary register to store the right operand.
		const RegisterId tmp_reg_id = compile_context.alloc_gp_reg();
		if (compile_context.is_reg_in_use(tmp_reg_id)) {
			int32_t off;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.stack_alloc_aligned(sizeof(T), sizeof(T), off));
			if constexpr (std::is_same_v<T, int8_t>) {
				int8_t rhs_data = cur_ins.operands[1].get_i8();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm8_to_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&rhs_data)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_div8_with_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
			} else if constexpr (std::is_same_v<T, int16_t>) {
				int16_t rhs_data = cur_ins.operands[1].get_i16();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm16_to_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&rhs_data)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_div16_with_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
			} else if constexpr (std::is_same_v<T, int32_t>) {
				int32_t rhs_data = cur_ins.operands[1].get_i32();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm32_to_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&rhs_data)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_div32_with_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
			} else if constexpr (std::is_same_v<T, int64_t>) {
				int64_t rhs_data = cur_ins.operands[1].get_i64();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&rhs_data)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_div64_with_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
			} else if constexpr (std::is_same_v<T, uint8_t>) {
				uint8_t rhs_data = cur_ins.operands[1].get_u8();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm8_to_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&rhs_data)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_idiv8_with_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
			} else if constexpr (std::is_same_v<T, uint16_t>) {
				uint16_t rhs_data = cur_ins.operands[1].get_u16();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm16_to_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&rhs_data)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_idiv16_with_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
			} else if constexpr (std::is_same_v<T, uint32_t>) {
				uint32_t rhs_data = cur_ins.operands[1].get_u32();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm32_to_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&rhs_data)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_idiv32_with_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
			} else if constexpr (std::is_same_v<T, uint64_t>) {
				uint64_t rhs_data = cur_ins.operands[1].get_u64();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&rhs_data)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_idiv64_with_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand type");
			}

			compile_context.stack_free(off, sizeof(T));
		} else {
			if constexpr (std::is_same_v<T, int8_t>) {
				int8_t rhs_data = cur_ins.operands[1].get_i8();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm8_to_reg8_ins(tmp_reg_id, (uint8_t *)&rhs_data)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_div8_with_reg8_ins(tmp_reg_id)));
			} else if constexpr (std::is_same_v<T, int16_t>) {
				int16_t rhs_data = cur_ins.operands[1].get_i16();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm16_to_reg16_ins(tmp_reg_id, (uint8_t *)&rhs_data)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_div16_with_reg16_ins(tmp_reg_id)));
			} else if constexpr (std::is_same_v<T, int32_t>) {
				int32_t rhs_data = cur_ins.operands[1].get_i32();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm32_to_reg32_ins(tmp_reg_id, (uint8_t *)&rhs_data)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_div32_with_reg32_ins(tmp_reg_id)));
			} else if constexpr (std::is_same_v<T, int64_t>) {
				int64_t rhs_data = cur_ins.operands[1].get_i64();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_reg64_ins(tmp_reg_id, (uint8_t *)&rhs_data)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_div64_with_reg64_ins(tmp_reg_id)));
			} else if constexpr (std::is_same_v<T, uint8_t>) {
				uint8_t rhs_data = cur_ins.operands[1].get_u8();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm8_to_reg8_ins(tmp_reg_id, (uint8_t *)&rhs_data)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_idiv8_with_reg8_ins(tmp_reg_id)));
			} else if constexpr (std::is_same_v<T, uint16_t>) {
				uint16_t rhs_data = cur_ins.operands[1].get_u16();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm16_to_reg16_ins(tmp_reg_id, (uint8_t *)&rhs_data)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_idiv16_with_reg16_ins(tmp_reg_id)));
			} else if constexpr (std::is_same_v<T, uint32_t>) {
				uint32_t rhs_data = cur_ins.operands[1].get_u32();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm32_to_reg32_ins(tmp_reg_id, (uint8_t *)&rhs_data)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_idiv32_with_reg32_ins(tmp_reg_id)));
			} else if constexpr (std::is_same_v<T, uint64_t>) {
				uint64_t rhs_data = cur_ins.operands[1].get_u64();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_reg64_ins(tmp_reg_id, (uint8_t *)&rhs_data)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_idiv64_with_reg64_ins(tmp_reg_id)));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand type");
			}
			compile_context.unalloc_reg(tmp_reg_id);
		}

		if constexpr (sizeof(T) == sizeof(uint8_t)) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_shr_reg16_with_imm8_ins(REG_RAX, 8)));
		} else if constexpr (sizeof(T) > sizeof(uint8_t)) {
			if (saved_rax_off != INT32_MIN) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.pop_reg(REG_RAX, saved_rax_off, saved_rax_size));
			}
		}

		VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, lhs_reg_id, sizeof(T));
		if (!output_vreg_state)
			return OutOfMemoryError::alloc();
	} else {
		if (lhs_expected_value.value_type != ValueType::Undefined) {  // The RHS is an expectable value so we can just simply add it with a register.
			uint32_t rhs_reg_index = cur_ins.operands[1].get_reg_index();
			int32_t saved_rax_off = INT32_MIN;
			size_t saved_rax_size;
			RegisterId rhs_reg_id;

			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				rhs_reg_id = REG_RAX;
			} else {
				rhs_reg_id = REG_RDX;
				if (compile_context.is_reg_in_use(REG_RAX)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(REG_RAX, saved_rax_off, saved_rax_size));
				}
			}
			if (compile_context.is_reg_in_use(rhs_reg_id)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(rhs_reg_id, off, size));
			}

			VirtualRegState &rhs_vreg_state = compile_context.virtual_reg_states.at(rhs_reg_index);
			if (rhs_vreg_state.save_offset != INT32_MIN) {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg8_ins(
																		REG_RAX,
																		MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg16_ins(
																		REG_RAX,
																		MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg32_ins(
																		REG_RAX,
																		MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg64_ins(
																		REG_RAX,
																		MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			} else {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg8_to_reg8_ins(
																		REG_RAX,
																		rhs_vreg_state.phy_reg)));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg16_to_reg16_ins(
																		REG_RAX,
																		rhs_vreg_state.phy_reg)));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg32_to_reg32_ins(
																		REG_RAX,
																		rhs_vreg_state.phy_reg)));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg64_to_reg64_ins(
																		REG_RAX,
																		rhs_vreg_state.phy_reg)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			}

			// Try to allocate a new temporary register to store the right operand.
			const RegisterId tmp_reg_id = compile_context.alloc_gp_reg();
			if (compile_context.is_reg_in_use(tmp_reg_id)) {
				int32_t off;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.stack_alloc_aligned(sizeof(T), sizeof(T), off));
				if constexpr (std::is_same_v<T, int8_t>) {
					int8_t lhs_data = cur_ins.operands[0].get_i8();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm8_to_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&lhs_data)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_div8_with_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, int16_t>) {
					int16_t lhs_data = cur_ins.operands[0].get_i16();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm16_to_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&lhs_data)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_div16_with_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, int32_t>) {
					int32_t lhs_data = cur_ins.operands[0].get_i32();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm32_to_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&lhs_data)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_div32_with_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, int64_t>) {
					int64_t lhs_data = cur_ins.operands[0].get_i64();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&lhs_data)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_div64_with_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, uint8_t>) {
					uint8_t lhs_data = cur_ins.operands[0].get_u8();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm8_to_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&lhs_data)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_idiv8_with_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, uint16_t>) {
					uint16_t lhs_data = cur_ins.operands[0].get_u16();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm16_to_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&lhs_data)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_idiv16_with_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, uint32_t>) {
					uint32_t lhs_data = cur_ins.operands[0].get_u32();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm32_to_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&lhs_data)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_idiv32_with_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, uint64_t>) {
					uint64_t lhs_data = cur_ins.operands[0].get_u64();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&lhs_data)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_idiv64_with_mem_ins(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}

				compile_context.stack_free(off, sizeof(T));
			} else {
				if constexpr (std::is_same_v<T, int8_t>) {
					int8_t lhs_data = cur_ins.operands[1].get_i8();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm8_to_reg8_ins(tmp_reg_id, (uint8_t *)&lhs_data)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_div8_with_reg8_ins(tmp_reg_id)));
				} else if constexpr (std::is_same_v<T, int16_t>) {
					int16_t lhs_data = cur_ins.operands[1].get_i16();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm16_to_reg16_ins(tmp_reg_id, (uint8_t *)&lhs_data)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_div16_with_reg16_ins(tmp_reg_id)));
				} else if constexpr (std::is_same_v<T, int32_t>) {
					int32_t lhs_data = cur_ins.operands[1].get_i32();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm32_to_reg32_ins(tmp_reg_id, (uint8_t *)&lhs_data)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_div32_with_reg32_ins(tmp_reg_id)));
				} else if constexpr (std::is_same_v<T, int64_t>) {
					int64_t lhs_data = cur_ins.operands[1].get_i64();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_reg64_ins(tmp_reg_id, (uint8_t *)&lhs_data)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_div64_with_reg64_ins(tmp_reg_id)));
				} else if constexpr (std::is_same_v<T, uint8_t>) {
					uint8_t lhs_data = cur_ins.operands[1].get_u8();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm8_to_reg8_ins(tmp_reg_id, (uint8_t *)&lhs_data)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_idiv8_with_reg8_ins(tmp_reg_id)));
				} else if constexpr (std::is_same_v<T, uint16_t>) {
					uint16_t lhs_data = cur_ins.operands[1].get_u16();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm16_to_reg16_ins(tmp_reg_id, (uint8_t *)&lhs_data)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_idiv16_with_reg16_ins(tmp_reg_id)));
				} else if constexpr (std::is_same_v<T, uint32_t>) {
					uint32_t lhs_data = cur_ins.operands[1].get_u32();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm32_to_reg32_ins(tmp_reg_id, (uint8_t *)&lhs_data)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_idiv32_with_reg32_ins(tmp_reg_id)));
				} else if constexpr (std::is_same_v<T, uint64_t>) {
					uint64_t lhs_data = cur_ins.operands[1].get_u64();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_reg64_ins(tmp_reg_id, (uint8_t *)&lhs_data)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_idiv64_with_reg64_ins(tmp_reg_id)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}
				compile_context.unalloc_reg(tmp_reg_id);
			}

			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_shr_reg16_with_imm8_ins(REG_RAX, 8)));
			} else if constexpr (sizeof(T) > sizeof(uint8_t)) {
				if (saved_rax_off != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.pop_reg(REG_RAX, saved_rax_off, saved_rax_size));
				}
			}

			VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, rhs_reg_id, sizeof(T));
			if (!output_vreg_state)
				return OutOfMemoryError::alloc();
		} else {
			uint32_t rhs_reg_index = cur_ins.operands[1].get_reg_index();
			int32_t saved_rax_off = INT32_MIN;
			size_t saved_rax_size;
			RegisterId lhs_reg_id;

			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				lhs_reg_id = REG_RAX;
			} else {
				lhs_reg_id = REG_RDX;
				if (compile_context.is_reg_in_use(REG_RAX)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(REG_RAX, saved_rax_off, saved_rax_size));
				}
			}
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
																		REG_RAX,
																		MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg16_ins(
																		REG_RAX,
																		MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg32_ins(
																		REG_RAX,
																		MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_mem_to_reg64_ins(
																		REG_RAX,
																		MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			} else {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg8_to_reg8_ins(
																		REG_RAX,
																		lhs_vreg_state.phy_reg)));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg16_to_reg16_ins(
																		REG_RAX,
																		lhs_vreg_state.phy_reg)));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg32_to_reg32_ins(
																		REG_RAX,
																		lhs_vreg_state.phy_reg)));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_reg64_to_reg64_ins(
																		REG_RAX,
																		lhs_vreg_state.phy_reg)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			}

			VirtualRegState &rhs_vreg_state = compile_context.virtual_reg_states.at(rhs_reg_index);
			if (rhs_vreg_state.save_offset != INT32_MIN) {
				if constexpr (std::is_same_v<T, int8_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_idiv8_with_mem_ins(MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, int16_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_idiv16_with_mem_ins(MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, int32_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_idiv32_with_mem_ins(MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, int64_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_idiv64_with_mem_ins(MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, uint8_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_div8_with_mem_ins(MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, uint16_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_div16_with_mem_ins(MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, uint32_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_div32_with_mem_ins(MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, uint64_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_div64_with_mem_ins(MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}
			} else {
				if constexpr (std::is_same_v<T, int8_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_idiv8_with_reg8_ins(rhs_vreg_state.phy_reg)));
				} else if constexpr (std::is_same_v<T, int16_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_idiv16_with_reg16_ins(rhs_vreg_state.phy_reg)));
				} else if constexpr (std::is_same_v<T, int32_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_idiv32_with_reg32_ins(rhs_vreg_state.phy_reg)));
				} else if constexpr (std::is_same_v<T, int64_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_idiv64_with_reg64_ins(rhs_vreg_state.phy_reg)));
				} else if constexpr (std::is_same_v<T, uint8_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_div8_with_reg8_ins(rhs_vreg_state.phy_reg)));
				} else if constexpr (std::is_same_v<T, uint16_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_div16_with_reg16_ins(rhs_vreg_state.phy_reg)));
				} else if constexpr (std::is_same_v<T, uint32_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_div32_with_reg32_ins(rhs_vreg_state.phy_reg)));
				} else if constexpr (std::is_same_v<T, uint64_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_div64_with_reg64_ins(rhs_vreg_state.phy_reg)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}
			}

			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_shr_reg16_with_imm8_ins(REG_RAX, 8)));
			} else if constexpr (sizeof(T) > sizeof(uint8_t)) {
				if (saved_rax_off != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.pop_reg(REG_RAX, saved_rax_off, saved_rax_size));
				}
			}

			VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, lhs_reg_id, sizeof(T));
			if (!output_vreg_state)
				return OutOfMemoryError::alloc();
		}
	}

	return {};
}

template <typename T>
[[nodiscard]] InternalExceptionPointer compile_fp_mod_instruction(
	JITCompileContext &compile_context,
	const Instruction &cur_ins,
	const Value &lhs_expected_value,
	const Value &rhs_expected_value) noexcept {
	InternalExceptionPointer exception;
	uint32_t output_reg_index = cur_ins.output;

	uint32_t lhs_reg_index = cur_ins.operands[0].get_reg_index(),
			 rhs_reg_index = cur_ins.operands[1].get_reg_index();

	VirtualRegState &lhs_vreg_state = compile_context.virtual_reg_states.at(lhs_reg_index);
	VirtualRegState &rhs_vreg_state = compile_context.virtual_reg_states.at(rhs_reg_index);
	const RegisterId lhs_xmm_reg_id = REG_XMM0, rhs_xmm_reg_id = REG_XMM1;
	int32_t rhs_off = INT32_MIN;
	size_t rhs_size;

	if (compile_context.is_reg_in_use(lhs_xmm_reg_id)) {
		int32_t off;
		size_t size;
		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg_xmm(lhs_xmm_reg_id, off, size));
	}

	if (compile_context.is_reg_in_use(rhs_xmm_reg_id)) {
		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg_xmm(rhs_xmm_reg_id, rhs_off, rhs_size));
	}

	VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, lhs_xmm_reg_id, sizeof(T));
	if (!output_vreg_state)
		return OutOfMemoryError::alloc();

	if constexpr (std::is_same_v<T, float>) {
		if (lhs_vreg_state.save_offset != INT32_MIN) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_movd_mem_to_reg_xmm_ins(lhs_xmm_reg_id, MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
		} else {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_movq_reg_xmm_to_reg_xmm_ins(lhs_xmm_reg_id, lhs_vreg_state.phy_reg)));
		}

		if (rhs_vreg_state.save_offset != INT32_MIN) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_movd_mem_to_reg_xmm_ins(rhs_xmm_reg_id, MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
		} else {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_movq_reg_xmm_to_reg_xmm_ins(rhs_xmm_reg_id, rhs_vreg_state.phy_reg)));
		}

		size_t padding = compile_context.cur_stack_size % 16;
		int32_t padding_off;

		if (padding) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.stack_alloc_aligned(16 - padding, 1, padding_off));
		}

		CallingRegSavingInfo calling_reg_saving_info;

		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.save_calling_regs(calling_reg_saving_info));

		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_call_ins((void *)fmodf_wrapper)));

		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.restore_calling_regs(calling_reg_saving_info));

		if (padding) {
			compile_context.stack_free(padding_off, 16 - padding);
		}
	} else if constexpr (std::is_same_v<T, double>) {
		if (lhs_vreg_state.save_offset != INT32_MIN) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_movq_mem_to_reg_xmm_ins(lhs_xmm_reg_id, MemoryLocation{ REG_RBP, lhs_vreg_state.save_offset, REG_MAX, 0 })));
		} else {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_movq_reg_xmm_to_reg_xmm_ins(lhs_xmm_reg_id, lhs_vreg_state.phy_reg)));
		}

		if (rhs_vreg_state.save_offset != INT32_MIN) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_movq_mem_to_reg_xmm_ins(rhs_xmm_reg_id, MemoryLocation{ REG_RBP, rhs_vreg_state.save_offset, REG_MAX, 0 })));
		} else {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_movq_reg_xmm_to_reg_xmm_ins(rhs_xmm_reg_id, rhs_vreg_state.phy_reg)));
		}

		uint32_t padding = (uint32_t)compile_context.cur_stack_size % 16;

		if (padding) {
			uint32_t sz_diff = 16 - padding;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.check_and_push_stack_pointer(sz_diff));
		}

		CallingRegSavingInfo calling_reg_saving_info;

		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.save_calling_regs(calling_reg_saving_info));

		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_call_ins((void *)fmod_wrapper)));

		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.restore_calling_regs(calling_reg_saving_info));

		if (padding) {
			uint32_t sz_diff = 16 - padding;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_add_imm32_to_reg64_ins(REG_RSP, (uint8_t *)&sz_diff)));
			compile_context.sub_stack_ptr(16 - padding);
		}
	} else {
		static_assert(!std::is_same_v<T, T>, "Invalid operand type");
	}

	if (rhs_off != INT32_MIN) {
		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.pop_reg_xmm(rhs_xmm_reg_id, rhs_off, rhs_size));
	}

	return {};
}

InternalExceptionPointer slake::jit::x86_64::compile_mod_instruction(
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
		case ValueType::F32:
		case ValueType::F64:
		case ValueType::Bool:
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
		case ValueType::F32:
		case ValueType::F64:
		case ValueType::Bool:
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
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_mod_instruction<int8_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::I16: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_mod_instruction<int16_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::I32: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_mod_instruction<int32_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::I64: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_mod_instruction<int64_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::U8: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_mod_instruction<uint8_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::U16: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_mod_instruction<uint16_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::U32: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_mod_instruction<uint32_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::U64: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_int_mod_instruction<uint64_t>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::F32: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_fp_mod_instruction<float>(
															compile_context,
															cur_ins,
															lhs_expected_value,
															rhs_expected_value));
			break;
		}
		case TypeId::F64: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_fp_mod_instruction<double>(
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

SLAKE_API float slake::jit::x86_64::fmodf_wrapper(float n, float d) {
	return flib::fmodf(n, d);
}

SLAKE_API double slake::jit::x86_64::fmod_wrapper(double n, double d) {
	return flib::fmod(n, d);
}
