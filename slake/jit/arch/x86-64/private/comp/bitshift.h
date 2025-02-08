#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_COMP_BITSHIFT_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_COMP_BITSHIFT_H_

#include "../common.h"
#include <slake/opti/proganal.h>

namespace slake {
	namespace jit {
		namespace x86_64 {
			[[nodiscard]] InternalExceptionPointer compileShlInstruction(
				JITCompileContext &compileContext,
				opti::ProgramAnalyzedInfo &analyzedInfo,
				size_t offIns,
				const Instruction &curIns) noexcept;
			[[nodiscard]] InternalExceptionPointer compileShrInstruction(
				JITCompileContext &compileContext,
				opti::ProgramAnalyzedInfo &analyzedInfo,
				size_t offIns,
				const Instruction &curIns) noexcept;
			[[nodiscard]] InternalExceptionPointer compileSarInstruction(
				JITCompileContext &compileContext,
				opti::ProgramAnalyzedInfo &analyzedInfo,
				size_t offIns,
				const Instruction &curIns) noexcept;
		}
	}
}

#endif
