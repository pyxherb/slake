#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_REGULAR_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_REGULAR_H_

#include "../emitter_base.h"

namespace slake {
	namespace jit {
		namespace x86_64 {
			DiscreteInstruction emit_ins_with_imm8_and_al_reg(uint8_t opcode, uint8_t imm0[1]);
			DiscreteInstruction emit_ins_with_imm8_and_reg8_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, RegisterId register_id, uint8_t imm0[1]);
			DiscreteInstruction emit_ins_with_imm16_and_ax_reg(uint8_t opcode, uint8_t imm0[2]);
			DiscreteInstruction emit_ins_with_imm16_and_reg16_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, RegisterId register_id, uint8_t imm0[2]);
			DiscreteInstruction emit_ins_with_imm32_and_eax_reg(uint8_t opcode, uint8_t imm0[4]);
			DiscreteInstruction emit_ins_with_imm32_and_reg32_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, RegisterId register_id, uint8_t imm0[4]);
			DiscreteInstruction emit_ins_with_imm32_and_rax_reg(uint8_t opcode, uint8_t imm0[4]);
			DiscreteInstruction emit_ins_with_imm32_and_reg64_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, RegisterId register_id, uint8_t imm0[4]);
			DiscreteInstruction emit_ins_with_reg8_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, RegisterId register_id);
			DiscreteInstruction emit_ins_with_reg16_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, RegisterId register_id);
			DiscreteInstruction emit_ins_with_reg32_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, RegisterId register_id);
			DiscreteInstruction emit_ins_with_reg64_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, RegisterId register_id);
			DiscreteInstruction emit_ins_with_mem8_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, const MemoryLocation &mem);
			DiscreteInstruction emit_ins_with_mem16_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, const MemoryLocation &mem);
			DiscreteInstruction emit_ins_with_mem32_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, const MemoryLocation &mem);
			DiscreteInstruction emit_ins_with_mem64_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, const MemoryLocation &mem);
			DiscreteInstruction emit_ins_with_reg8_and_mod_rmreg(uint8_t major_opcode, RegisterId register_id, RegisterId src_register_id);
			DiscreteInstruction emit_ins_with_reg16_and_mod_rmreg(uint8_t major_opcode, RegisterId register_id, RegisterId src_register_id);
			DiscreteInstruction emit_ins_with_reg32_and_mod_rmreg(uint8_t major_opcode, RegisterId register_id, RegisterId src_register_id);
			DiscreteInstruction emit_ins_with_reg64_and_mod_rmreg(uint8_t major_opcode, RegisterId register_id, RegisterId src_register_id);
			DiscreteInstruction emit_ins_with_reg8_and_mem(uint8_t major_opcode, RegisterId src_register_id, const MemoryLocation &mem);
			DiscreteInstruction emit_ins_with_reg16_and_mem(uint8_t major_opcode, RegisterId src_register_id, const MemoryLocation &mem);
			DiscreteInstruction emit_ins_with_reg32_and_mem(uint8_t major_opcode, RegisterId src_register_id, const MemoryLocation &mem);
			DiscreteInstruction emit_ins_with_reg64_and_mem(uint8_t major_opcode, RegisterId src_register_id, const MemoryLocation &mem);
			DiscreteInstruction emit_ins_with_imm8_and_mem8_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, const MemoryLocation &mem, uint8_t imm0[1]);
			DiscreteInstruction emit_ins_with_imm16_and_mem16_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, const MemoryLocation &mem, uint8_t imm0[2]);
			DiscreteInstruction emit_ins_with_imm32_and_mem32_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, const MemoryLocation &mem, uint8_t imm0[4]);
			DiscreteInstruction emit_ins_with_imm32_and_mem64_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, const MemoryLocation &mem, uint8_t imm0[4]);
			DiscreteInstruction emit_ins_with_imm64_and_mem64_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, const MemoryLocation &mem, uint8_t imm0[8]);
			DiscreteInstruction emit_ins_with_reg8_increment(uint8_t major_opcode, RegisterId register_id);
			DiscreteInstruction emit_ins_with_reg16_increment(uint8_t major_opcode, RegisterId register_id);
			DiscreteInstruction emit_ins_with_reg32_increment(uint8_t major_opcode, RegisterId register_id);
			DiscreteInstruction emit_ins_with_reg64_increment(uint8_t major_opcode, RegisterId register_id);
			DiscreteInstruction emit_ins_with_imm8_and_reg8_increment(uint8_t major_opcode, RegisterId register_id, uint8_t imm0[1]);
			DiscreteInstruction emit_ins_with_imm16_and_reg16_increment(uint8_t major_opcode, RegisterId register_id, uint8_t imm0[2]);
			DiscreteInstruction emit_ins_with_imm32_and_reg32_increment(uint8_t major_opcode, RegisterId register_id, uint8_t imm0[4]);
			DiscreteInstruction emit_ins_with_imm64_and_reg64_increment(uint8_t major_opcode, RegisterId register_id, uint8_t imm0[8]);
		}
	}
}

#endif
