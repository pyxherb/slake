#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_CMP_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_CMP_H_

#include "sse.h"

namespace slake {
	namespace jit {
		namespace x86_64 {
			SLAKE_API DiscreteInstruction emit_cmp_imm8_to_reg8_ins(RegisterId register_id, uint8_t imm0[1]);
			SLAKE_API DiscreteInstruction emit_cmp_imm16_to_reg16_ins(RegisterId register_id, uint8_t imm0[2]);
			SLAKE_API DiscreteInstruction emit_cmp_imm32_to_reg32_ins(RegisterId register_id, uint8_t imm0[4]);
			SLAKE_API DiscreteInstruction emit_cmp_imm32_to_reg64_ins(RegisterId register_id, uint8_t imm0[4]);
			SLAKE_API DiscreteInstruction emit_cmp_imm8_to_mem_ins(const MemoryLocation &mem, uint8_t imm0[1]);
			SLAKE_API DiscreteInstruction emit_cmp_imm16_to_mem_ins(const MemoryLocation &mem, uint8_t imm0[2]);
			SLAKE_API DiscreteInstruction emit_cmp_imm32_to_mem_ins(const MemoryLocation &mem, uint8_t imm0[4]);
			SLAKE_API DiscreteInstruction emit_cmp_imm64_to_mem_ins(const MemoryLocation &mem, uint8_t imm0[8]);
			SLAKE_API DiscreteInstruction emit_cmp_reg8_to_reg8_ins(RegisterId register_id, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_cmp_reg16_to_reg16_ins(RegisterId register_id, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_cmp_reg32_to_reg32_ins(RegisterId register_id, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_cmp_reg64_to_reg64_ins(RegisterId register_id, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_cmp_mem_to_reg8_ins(RegisterId src_register_id, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_cmp_mem_to_reg16_ins(RegisterId src_register_id, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_cmp_mem_to_reg32_ins(RegisterId src_register_id, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_cmp_mem_to_reg64_ins(RegisterId src_register_id, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_cmp_reg8_to_mem_ins(const MemoryLocation &mem, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_cmp_reg16_to_mem_ins(const MemoryLocation &mem, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_cmp_reg32_to_mem_ins(const MemoryLocation &mem, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_cmp_reg64_to_mem_ins(const MemoryLocation &mem, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_cmpss_mem_to_reg_xmm_ins(RegisterId register_id, const MemoryLocation &mem, uint8_t imm0);
			SLAKE_API DiscreteInstruction emit_cmpsd_mem_to_reg_xmm_ins(RegisterId register_id, const MemoryLocation &mem, uint8_t imm0);
			SLAKE_API DiscreteInstruction emit_cmpss_reg_xmm_to_reg_xmm_ins(RegisterId register_id, RegisterId src_register_id, uint8_t imm0);
			SLAKE_API DiscreteInstruction emit_cmpsd_reg_xmm_to_reg_xmm_ins(RegisterId register_id, RegisterId src_register_id, uint8_t imm0);
		}
	}
}

#endif
