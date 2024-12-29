#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_SSE_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_SSE_H_

#include "regular.h"

namespace slake {
	namespace jit {
		namespace x86_64 {
			SLAKE_API DiscreteInstruction emitSSESsArithmRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId, uint8_t opcode);
			SLAKE_API DiscreteInstruction emitSSESsArithmMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem, uint8_t opcode);
			SLAKE_API DiscreteInstruction emitSSESdArithmRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId, uint8_t opcode);
			SLAKE_API DiscreteInstruction emitSSESdArithmMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem, uint8_t opcode);
		}
	}
}

#endif
