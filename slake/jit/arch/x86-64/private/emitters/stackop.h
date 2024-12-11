#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_STACKOP_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_STACKOP_H_

#include "../emitter_base.h"

namespace slake {
	namespace jit {
		namespace x86_64 {
			SLAKE_API DiscreteInstruction emitPushReg16Ins(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitPushReg64Ins(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitPopReg16Ins(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitPopReg64Ins(RegisterId registerId);
		}
	}
}

#endif
