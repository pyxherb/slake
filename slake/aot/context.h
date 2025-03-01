#ifndef _SLAKE_AOT_CONTEXT_H_
#define _SLAKE_AOT_CONTEXT_H_

#include <slake/runtime.h>

namespace slake {
	struct AOTFnExecContext {
		Runtime *runtime;
		ContextObject *context;
		HostObjectRef<ContextObject> hostContext;
		peff::HashMap<peff::String, Type> *mappedGenericArgs;
		void *mappedObjects;
	};
}

#endif
