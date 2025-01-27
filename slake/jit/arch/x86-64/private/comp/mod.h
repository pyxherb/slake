#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_COMP_MOD_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_COMP_MOD_H_

#include "../common.h"
#include <slake/opti/proganal.h>

namespace slake {
	namespace jit {
		namespace x86_64 {
			[[nodiscard]] InternalExceptionPointer compileModInstruction(
				JITCompileContext &compileContext,
				opti::ProgramAnalyzedInfo &analyzedInfo,
				size_t offIns,
				const Instruction &curIns);

			SLAKE_API float fmodfWrapper(float n, float d);
			SLAKE_API double fmodWrapper(double n, double d);
		}
	}
}

#endif
