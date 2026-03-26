#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_MUL_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_MUL_H_

#include "sse.h"

namespace slake {
	namespace jit {
		namespace x86_64 {
			SLAKE_API DiscreteInstruction emit_mul8_with_reg8_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_mul16_with_reg16_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_mul32_with_reg32_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_mul64_with_reg64_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_mul8_with_mem_ins(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_mul16_with_mem_ins(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_mul32_with_mem_ins(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_mul64_with_mem_ins(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_imul8_with_reg8_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_imul16_with_reg16_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_imul32_with_reg32_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_imul64_with_reg64_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_imul8_with_mem_ins(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_imul16_with_mem_ins(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_imul32_with_mem_ins(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_imul64_with_mem_ins(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_mulss_reg_xmm_to_reg_xmm_ins(RegisterId register_id, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_mulss_mem_to_reg_xmm_ins(RegisterId register_id, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_mulsd_reg_xmm_to_reg_xmm_ins(RegisterId register_id, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_mulsd_mem_to_reg_xmm_ins(RegisterId register_id, const MemoryLocation &mem);
		}
	}
}

#endif
