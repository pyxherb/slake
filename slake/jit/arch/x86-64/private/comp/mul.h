#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_COMP_MUL_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_COMP_MUL_H_

#include "../common.h"
#include <slake/opti/proganal.h>

namespace slake {
	namespace jit {
		namespace x86_64 {
			[[nodiscard]] InternalExceptionPointer compile_mul_instruction(
				JITCompileContext &compile_context,
				opti::ProgramAnalyzedInfo &analyzed_info,
				size_t off_ins,
				const Instruction &cur_ins) noexcept;
		}
	}
}

#endif
