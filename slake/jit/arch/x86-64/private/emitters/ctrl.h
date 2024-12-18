#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_CTRL_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_CTRL_H_

#include "../emitter_base.h"

namespace slake {
	namespace jit {
		namespace x86_64 {
			SLAKE_API DiscreteInstruction emitJmpWithOff32(size_t &offDestOut);
			SLAKE_API DiscreteInstruction emitCallWithOff32(size_t &offDestOut);
			SLAKE_API DiscreteInstruction emitJmpWithReg64Ins(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitCallWithReg64Ins(RegisterId registerId);
		}
	}
}

#endif
