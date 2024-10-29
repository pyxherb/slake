#ifndef _SLAKE_JIT_ARCH_X86_CONTEXT_H_
#define _SLAKE_JIT_ARCH_X86_CONTEXT_H_

#include "compiler.h"

namespace slake {
	namespace jit {
		namespace x86_64 {
			struct JITExecContext {
				Runtime *runtime;
				Object *args;
			};
		}
	}
}

#endif
