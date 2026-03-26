#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_COMP_MOD_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_COMP_MOD_H_

#include "../common.h"
#include <slake/opti/proganal.h>

namespace slake {
	namespace jit {
		namespace x86_64 {
			[[nodiscard]] InternalExceptionPointer compile_mod_instruction(
				JITCompileContext &compile_context,
				opti::ProgramAnalyzedInfo &analyzed_info,
				size_t off_ins,
				const Instruction &cur_ins) noexcept;

			SLAKE_API float fmodf_wrapper(float n, float d);
			SLAKE_API double fmod_wrapper(double n, double d);
		}
	}
}

#endif
