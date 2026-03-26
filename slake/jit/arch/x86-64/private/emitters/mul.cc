#include "mul.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mul8_with_reg8_ins(RegisterId register_id) {
	return emit_ins_with_reg8_with_minor_opcode(0xf6, 4, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mul16_with_reg16_ins(RegisterId register_id) {
	return emit_ins_with_reg16_with_minor_opcode(0xf7, 4, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mul32_with_reg32_ins(RegisterId register_id) {
	return emit_ins_with_reg32_with_minor_opcode(0xf7, 4, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mul64_with_reg64_ins(RegisterId register_id) {
	return emit_ins_with_reg64_with_minor_opcode(0xf7, 4, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mul8_with_mem_ins(const MemoryLocation &mem) {
	return emit_ins_with_mem8_with_minor_opcode(0xf6, 4, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mul16_with_mem_ins(const MemoryLocation &mem) {
	return emit_ins_with_mem16_with_minor_opcode(0xf7, 4, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mul32_with_mem_ins(const MemoryLocation &mem) {
	return emit_ins_with_mem32_with_minor_opcode(0xf7, 4, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mul64_with_mem_ins(const MemoryLocation &mem) {
	return emit_ins_with_mem64_with_minor_opcode(0xf7, 4, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_imul8_with_reg8_ins(RegisterId register_id) {
	return emit_ins_with_reg8_with_minor_opcode(0xf6, 5, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_imul16_with_reg16_ins(RegisterId register_id) {
	return emit_ins_with_reg16_with_minor_opcode(0xf7, 5, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_imul32_with_reg32_ins(RegisterId register_id) {
	return emit_ins_with_reg32_with_minor_opcode(0xf7, 5, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_imul64_with_reg64_ins(RegisterId register_id) {
	return emit_ins_with_reg64_with_minor_opcode(0xf7, 5, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_imul8_with_mem_ins(const MemoryLocation &mem) {
	return emit_ins_with_mem8_with_minor_opcode(0xf6, 5, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_imul16_with_mem_ins(const MemoryLocation &mem) {
	return emit_ins_with_mem16_with_minor_opcode(0xf7, 5, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_imul32_with_mem_ins(const MemoryLocation &mem) {
	return emit_ins_with_mem32_with_minor_opcode(0xf7, 5, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_imul64_with_mem_ins(const MemoryLocation &mem) {
	return emit_ins_with_mem64_with_minor_opcode(0xf7, 5, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mulss_reg_xmm_to_reg_xmm_ins(RegisterId register_id, RegisterId src_register_id) {
	return emit_ssess_arithm_reg_xmm_to_reg_xmm_ins(register_id, src_register_id, 0x59);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mulss_mem_to_reg_xmm_ins(RegisterId register_id, const MemoryLocation &mem) {
	return emit_ssess_arithm_mem_to_reg_xmm_ins(register_id, mem, 0x59);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mulsd_reg_xmm_to_reg_xmm_ins(RegisterId register_id, RegisterId src_register_id) {
	return emit_ssesd_arithm_reg_xmm_to_reg_xmm_ins(register_id, src_register_id, 0x59);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mulsd_mem_to_reg_xmm_ins(RegisterId register_id, const MemoryLocation &mem) {
	return emit_ssesd_arithm_mem_to_reg_xmm_ins(register_id, mem, 0x59);
}
