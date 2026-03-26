#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_DIV_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_DIV_H_

#include "sse.h"

namespace slake {
	namespace jit {
		namespace x86_64 {
			SLAKE_API DiscreteInstruction emit_div8_with_reg8_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_div16_with_reg16_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_div32_with_reg32_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_div64_with_reg64_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_div8_with_mem_ins(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_div16_with_mem_ins(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_div32_with_mem_ins(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_div64_with_mem_ins(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_idiv8_with_reg8_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_idiv16_with_reg16_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_idiv32_with_reg32_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_idiv64_with_reg64_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_idiv8_with_mem_ins(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_idiv16_with_mem_ins(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_idiv32_with_mem_ins(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_idiv64_with_mem_ins(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_divss_reg_xmm_to_reg_xmm_ins(RegisterId register_id, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_divss_mem_to_reg_xmm_ins(RegisterId register_id, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emit_divsd_reg_xmm_to_reg_xmm_ins(RegisterId register_id, RegisterId src_register_id);
			SLAKE_API DiscreteInstruction emit_divsd_mem_to_reg_xmm_ins(RegisterId register_id, const MemoryLocation &mem);
		}
	}
}

#endif
