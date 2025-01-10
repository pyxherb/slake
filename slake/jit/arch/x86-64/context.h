#ifndef _SLAKE_JIT_ARCH_X86_64_CONTEXT_H_
#define _SLAKE_JIT_ARCH_X86_64_CONTEXT_H_

#include <slake/jit/base.h>

namespace slake {
	namespace jit {
		namespace x86_64 {
			struct JITArgRegistry {
				Value value;
				Type type;
			};

			// JIT execution context, note that rbx is used for its storage.
			struct JITExecContext {
				Runtime *runtime;
				JITCompiledFnOverloadingObject *fn;
				JITArgRegistry *args;
				Value returnValue;
				InternalException *exception;
				StackOverflowError *stackOverflowError;
				size_t insOff;
				void *stackBase;
				void *stackLimit;
			};

			typedef void (*JITCompiledBody)(JITExecContext *execContext);
		}
	}
}

#endif
