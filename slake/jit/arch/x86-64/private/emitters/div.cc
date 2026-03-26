#include "div.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_div8_with_reg8_ins(RegisterId register_id) {
	return emit_ins_with_reg8_with_minor_opcode(0xf6, 6, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_div16_with_reg16_ins(RegisterId register_id) {
	return emit_ins_with_reg16_with_minor_opcode(0xf7, 6, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_div32_with_reg32_ins(RegisterId register_id) {
	return emit_ins_with_reg32_with_minor_opcode(0xf7, 6, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_div64_with_reg64_ins(RegisterId register_id) {
	return emit_ins_with_reg64_with_minor_opcode(0xf7, 6, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_div8_with_mem_ins(const MemoryLocation &mem) {
	return emit_ins_with_mem8_with_minor_opcode(0xf6, 6, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_div16_with_mem_ins(const MemoryLocation &mem) {
	return emit_ins_with_mem16_with_minor_opcode(0xf7, 6, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_div32_with_mem_ins(const MemoryLocation &mem) {
	return emit_ins_with_mem32_with_minor_opcode(0xf7, 6, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_div64_with_mem_ins(const MemoryLocation &mem) {
	return emit_ins_with_mem64_with_minor_opcode(0xf7, 6, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_idiv8_with_reg8_ins(RegisterId register_id) {
	return emit_ins_with_reg8_with_minor_opcode(0xf6, 7, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_idiv16_with_reg16_ins(RegisterId register_id) {
	return emit_ins_with_reg16_with_minor_opcode(0xf7, 7, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_idiv32_with_reg32_ins(RegisterId register_id) {
	return emit_ins_with_reg32_with_minor_opcode(0xf7, 7, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_idiv64_with_reg64_ins(RegisterId register_id) {
	return emit_ins_with_reg64_with_minor_opcode(0xf7, 7, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_idiv8_with_mem_ins(const MemoryLocation &mem) {
	return emit_ins_with_mem8_with_minor_opcode(0xf6, 7, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_idiv16_with_mem_ins(const MemoryLocation &mem) {
	return emit_ins_with_mem16_with_minor_opcode(0xf7, 7, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_idiv32_with_mem_ins(const MemoryLocation &mem) {
	return emit_ins_with_mem32_with_minor_opcode(0xf7, 7, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_idiv64_with_mem_ins(const MemoryLocation &mem) {
	return emit_ins_with_mem64_with_minor_opcode(0xf7, 7, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_divss_reg_xmm_to_reg_xmm_ins(RegisterId register_id, RegisterId src_register_id) {
	return emit_ssess_arithm_reg_xmm_to_reg_xmm_ins(register_id, src_register_id, 0x5e);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_divss_mem_to_reg_xmm_ins(RegisterId register_id, const MemoryLocation &mem) {
	return emit_ssess_arithm_mem_to_reg_xmm_ins(register_id, mem, 0x5e);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_divsd_reg_xmm_to_reg_xmm_ins(RegisterId register_id, RegisterId src_register_id) {
	return emit_ssesd_arithm_reg_xmm_to_reg_xmm_ins(register_id, src_register_id, 0x5e);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_divsd_mem_to_reg_xmm_ins(RegisterId register_id, const MemoryLocation &mem) {
	return emit_ssesd_arithm_mem_to_reg_xmm_ins(register_id, mem, 0x5e);
}
