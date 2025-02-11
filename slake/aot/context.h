#ifndef _SLAKE_AOT_CONTEXT_H_
#define _SLAKE_AOT_CONTEXT_H_

#include <slake/runtime.h>

namespace slake {
	struct AOTFnContext {
		peff::HashMap<peff::String, Type> mappedGenericArgs;
	};
}

#endif
