#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_COMP_SUB_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_COMP_SUB_H_

#include "../common.h"
#include <slake/opti/proganal.h>

namespace slake {
	namespace jit {
		namespace x86_64 {
			InternalExceptionPointer compileSubInstruction(
				JITCompileContext &compileContext,
				opti::ProgramAnalyzedInfo &analyzedInfo,
				size_t offIns,
				const Instruction &curIns);
		}
	}
}

#endif
