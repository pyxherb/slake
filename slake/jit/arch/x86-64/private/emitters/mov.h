#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_COPY_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_COPY_H_

#include "sse.h"

namespace slake {
	namespace jit {
		namespace x86_64 {
			SLAKE_API DiscreteInstruction emit_mov_imm8_to_reg8_ins(RegisterId register_id, uint8_t imm0[1]);
			SLAKE_API DiscreteInstruction emit_mov_imm16_to_reg16_ins(RegisterId register_id, uint8_t imm0[2]);
			SLAKE_API DiscreteInstruction emit_mov_imm32_to_reg32_ins(RegisterId register_id, uint8_t imm0[4]);
			SLAKE_API DiscreteInstruction emit_mov_imm64_to_reg64_ins(RegisterId register_id, uint8_t imm0[8]);
			SLAKE_API DiscreteInstruction emit_mov_imm8_to_mem_ins(const MemoryLocation &mem, uint8_t imm0[1]);
			SLAKE_API DiscreteInstruction emit_mov_imm16_to_mem_ins(const MemoryLocation &mem, uint8_t imm0[2]);
			SLAKE_API DiscreteInstruction emit_mov_imm32_to_mem_ins(const MemoryLocation &mem, uint8_t imm0[4]);
			SLAKE_API DiscreteInstruction emit_mov_imm64_to_mem_ins(const MemoryLocation &mem, uint8_t imm0[8]);
			SLAKE_API DiscreteInstruction emit_mov_reg8_to_reg8_ins(RegisterId register_id, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_mov_reg16_to_reg16_ins(RegisterId register_id, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_mov_reg32_to_reg32_ins(RegisterId register_id, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_mov_reg64_to_reg64_ins(RegisterId register_id, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_mov_mem_to_reg8_ins(RegisterId src_register_id, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_mov_mem_to_reg16_ins(RegisterId src_register_id, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_mov_mem_to_reg32_ins(RegisterId src_register_id, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_mov_mem_to_reg64_ins(RegisterId src_register_id, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_mov_reg8_to_mem_ins(const MemoryLocation &mem, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_mov_reg16_to_mem_ins(const MemoryLocation &mem, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_mov_reg32_to_mem_ins(const MemoryLocation &mem, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_mov_reg64_to_mem_ins(const MemoryLocation &mem, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_movd_reg32_to_reg_xmm_ins(RegisterId register_id, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_movq_reg64_to_reg_xmm_ins(RegisterId register_id, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_movd_reg_xmm_to_reg32_ins(RegisterId register_id, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_movq_reg_xmm_to_reg64_ins(RegisterId register_id, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_movd_mem_to_reg_xmm_ins(RegisterId register_id, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_movq_mem_to_reg_xmm_ins(RegisterId register_id, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_movd_reg_xmm_to_mem_ins(const MemoryLocation &mem, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_movq_reg_xmm_to_mem_ins(const MemoryLocation &mem, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_movq_reg_xmm_to_reg_xmm_ins(RegisterId register_id, RegisterId src_register_id);
		}
	}
}

#endif
