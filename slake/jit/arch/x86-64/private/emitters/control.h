#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_CONTROL_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_CONTROL_H_

#include "regular.h"

namespace slake {
	namespace jit {
		namespace x86_64 {
			SLAKE_API DiscreteInstruction emit_near_ret_ins();
			SLAKE_API DiscreteInstruction emit_far_ret_ins();
		}
	}
}

#endif
