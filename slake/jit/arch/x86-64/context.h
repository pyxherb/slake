#ifndef _SLAKE_JIT_ARCH_X86_64_CONTEXT_H_
#define _SLAKE_JIT_ARCH_X86_64_CONTEXT_H_

#include <slake/jit/base.h>
#include "compiler.h"

namespace slake {
	namespace jit {
		namespace x86_64 {
			// JIT execution context, note that rbx is used for its storage.
			struct JITExecContext {
				Runtime *runtime;
				JITCompiledFnOverloadingObject *fn;
				Value *args;
				Value returnValue;
				InternalException *exception;
				size_t insOff;
			};
		}
	}
}

#endif
