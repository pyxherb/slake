#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_SSE_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_SSE_H_

#include "regular.h"

namespace slake {
	namespace jit {
		namespace x86_64 {
			SLAKE_API DiscreteInstruction emit_ssess_arithm_reg_xmm_to_reg_xmm_ins(RegisterId register_id, RegisterId src_register_id, uint8_t opcode);
			SLAKE_API DiscreteInstruction emit_ssess_arithm_mem_to_reg_xmm_ins(RegisterId register_id, const MemoryLocation &mem, uint8_t opcode);
			SLAKE_API DiscreteInstruction emit_ssesd_arithm_reg_xmm_to_reg_xmm_ins(RegisterId register_id, RegisterId src_register_id, uint8_t opcode);
			SLAKE_API DiscreteInstruction emit_ssesd_arithm_mem_to_reg_xmm_ins(RegisterId register_id, const MemoryLocation &mem, uint8_t opcode);
		}
	}
}

#endif
