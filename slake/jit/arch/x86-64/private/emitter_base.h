#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTER_BASE_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTER_BASE_H_

#include <slake/jit/arch/x86-64/basedefs.h>

namespace slake {
	namespace jit {
		namespace x86_64 {
			SLAKE_FORCEINLINE DiscreteInstruction emitRawIns(size_t szIns, const uint8_t *buffer) {
				DiscreteInstruction ins;

				ins.data.asRawInsData.szIns = szIns;
				memcpy(ins.data.asRawInsData.buffer, buffer, szIns);
				ins.insType = DiscreteInstructionType::Raw;

				return ins;
			}

			SLAKE_FORCEINLINE DiscreteInstruction emitJumpIns(void* dest) {
				DiscreteInstruction ins;
				ins.insType = DiscreteInstructionType::Jump;
				ins.data.asJumpInsData.dest = dest;
				return ins;
			}

			SLAKE_FORCEINLINE DiscreteInstruction emitCallIns(void *dest) {
				DiscreteInstruction ins;
				ins.insType = DiscreteInstructionType::Call;
				ins.data.asJumpInsData.dest = dest;
				return ins;
			}
		}
	}
}

#endif
