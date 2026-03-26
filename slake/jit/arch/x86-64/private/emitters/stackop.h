#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_STACKOP_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_STACKOP_H_

#include "regular.h"

namespace slake {
	namespace jit {
		namespace x86_64 {
			SLAKE_API DiscreteInstruction emit_push_reg16_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_push_reg64_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_pop_reg16_ins(RegisterId register_id);
			SLAKE_API DiscreteInstruction emit_pop_reg64_ins(RegisterId register_id);
		}
	}
}

#endif
