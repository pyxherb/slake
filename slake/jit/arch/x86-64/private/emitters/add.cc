#include "add.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_add_imm8_to_reg8_ins(RegisterId register_id, uint8_t imm0[1]) {
	switch (register_id) {
		case REG_RAX:
			return emit_ins_with_imm8_and_al_reg(0x04, imm0);
		default:
			return emit_ins_with_imm8_and_reg8_with_minor_opcode(0x80, 0, register_id, imm0);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_add_imm16_to_reg16_ins(RegisterId register_id, uint8_t imm0[2]) {
	switch (register_id) {
		case REG_RAX:
			return emit_ins_with_imm16_and_ax_reg(0x05, imm0);
		default:
			return emit_ins_with_imm16_and_reg16_with_minor_opcode(0x81, 0, register_id, imm0);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_add_imm32_to_reg32_ins(RegisterId register_id, uint8_t imm0[4]) {
	switch (register_id) {
		case REG_RAX:
			return emit_ins_with_imm32_and_eax_reg(0x05, imm0);
		default:
			return emit_ins_with_imm32_and_reg32_with_minor_opcode(0x81, 0, register_id, imm0);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_add_imm32_to_reg64_ins(RegisterId register_id, uint8_t imm0[4]) {
	switch (register_id) {
		case REG_RAX:
			return emit_ins_with_imm32_and_rax_reg(0x05, imm0);
		default:
			return emit_ins_with_imm32_and_reg64_with_minor_opcode(0x81, 0, register_id, imm0);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_add_reg8_to_reg8_ins(RegisterId register_id, RegisterId src_register_id) {
	return emit_ins_with_reg8_and_mod_rmreg(0x88, register_id, src_register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_add_reg16_to_reg16_ins(RegisterId register_id, RegisterId src_register_id) {
	return emit_ins_with_reg16_and_mod_rmreg(0x89, register_id, src_register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_add_reg32_to_reg32_ins(RegisterId register_id, RegisterId src_register_id) {
	return emit_ins_with_reg32_and_mod_rmreg(0x89, register_id, src_register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_add_reg64_to_reg64_ins(RegisterId register_id, RegisterId src_register_id) {
	return emit_ins_with_reg64_and_mod_rmreg(0x89, register_id, src_register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_add_mem_to_reg8_ins(RegisterId src_register_id, const MemoryLocation &mem) {
	return emit_ins_with_reg8_and_mem(0x02, src_register_id, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_add_mem_to_reg16_ins(RegisterId src_register_id, const MemoryLocation &mem) {
	return emit_ins_with_reg16_and_mem(0x03, src_register_id, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_add_mem_to_reg32_ins(RegisterId src_register_id, const MemoryLocation &mem) {
	return emit_ins_with_reg32_and_mem(0x03, src_register_id, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_add_mem_to_reg64_ins(RegisterId src_register_id, const MemoryLocation &mem) {
	return emit_ins_with_reg64_and_mem(0x03, src_register_id, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_addss_reg_xmm_to_reg_xmm_ins(RegisterId register_id, RegisterId src_register_id) {
	return emit_ssess_arithm_reg_xmm_to_reg_xmm_ins(register_id, src_register_id, 0x58);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_addss_mem_to_reg_xmm_ins(RegisterId register_id, const MemoryLocation &mem) {
	return emit_ssess_arithm_mem_to_reg_xmm_ins(register_id, mem, 0x58);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_addsd_reg_xmm_to_reg_xmm_ins(RegisterId register_id, RegisterId src_register_id) {
	return emit_ssesd_arithm_reg_xmm_to_reg_xmm_ins(register_id, src_register_id, 0x58);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_addsd_mem_to_reg_xmm_ins(RegisterId register_id, const MemoryLocation &mem) {
	return emit_ssesd_arithm_mem_to_reg_xmm_ins(register_id, mem, 0x58);
}
