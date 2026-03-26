#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_BITSHIFT_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_BITSHIFT_H_

#include "regular.h"

namespace slake {
	namespace jit {
		namespace x86_64 {
			SLAKE_API DiscreteInstruction emit_shl_reg8_with_imm8_ins(RegisterId register_id, uint8_t times);
			SLAKE_API DiscreteInstruction emit_shl_reg16_with_imm8_ins(RegisterId register_id, uint8_t times);
			SLAKE_API DiscreteInstruction emit_shl_reg32_with_imm8_ins(RegisterId register_id, uint8_t times);
			SLAKE_API DiscreteInstruction emit_shl_reg64_with_imm8_ins(RegisterId register_id, uint8_t times);
			SLAKE_API DiscreteInstruction emit_shl_reg8_with_cl_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_shl_reg16_with_cl_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_shl_reg32_with_cl_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_shl_reg64_with_cl_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_shr_reg8_with_imm8_ins(RegisterId register_id, uint8_t times);
			SLAKE_API DiscreteInstruction emit_shr_reg16_with_imm8_ins(RegisterId register_id, uint8_t times);
			SLAKE_API DiscreteInstruction emit_shr_reg32_with_imm8_ins(RegisterId register_id, uint8_t times);
			SLAKE_API DiscreteInstruction emit_shr_reg64_with_imm8_ins(RegisterId register_id, uint8_t times);
			SLAKE_API DiscreteInstruction emit_shr_reg8_with_cl_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_shr_reg16_with_cl_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_shr_reg32_with_cl_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_shr_reg64_with_cl_ins(RegisterId register_id);

			SLAKE_API DiscreteInstruction emit_sar_reg8_with_imm8_ins(RegisterId register_id, uint8_t times);
			SLAKE_API DiscreteInstruction emit_sar_reg16_with_imm8_ins(RegisterId register_id, uint8_t times);
			SLAKE_API DiscreteInstruction emit_sar_reg32_with_imm8_ins(RegisterId register_id, uint8_t times);
			SLAKE_API DiscreteInstruction emit_sar_reg64_with_imm8_ins(RegisterId register_id, uint8_t times);
			SLAKE_API DiscreteInstruction emit_sar_reg8_with_cl_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_sar_reg16_with_cl_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_sar_reg32_with_cl_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_sar_reg64_with_cl_ins(RegisterId register_id);
		}
	}
}

#endif
