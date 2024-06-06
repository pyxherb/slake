#ifndef _SLAKE_JIT_ARCH_X86_CONTEXT_H_
#define _SLAKE_JIT_ARCH_X86_CONTEXT_H_

#include <slake/runtime.h>

namespace slake {
	struct X86JITContext {
		Object* args;
	};
}

#endif
