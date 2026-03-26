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

			/// @brief The JIT execution context.
			struct JITExecContext {
				Runtime *runtime;
				JITCompiledFnOverloadingObject *fn;
				JITArgRegistry *args;
				Value return_value;
				InternalException *exception;
				StackOverflowError *stack_overflow_error;
				size_t ins_off;
				void *stack_base;
				void *stack_limit;
			};

			typedef void (*JITCompiledBody)(JITExecContext *exec_context);
		}
	}
}

#endif
