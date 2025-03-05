#ifndef _SLAKE_AOT_CONTEXT_H_
#define _SLAKE_AOT_CONTEXT_H_

#include <slake/runtime.h>

namespace slake {
	struct AOTFnExecContext {
		Runtime *runtime;
		HostObjectRef<ContextObject> hostContext;
		peff::HashMap<peff::String, Type> *mappedGenericArgs;
		void *mappedObjects;
		Value thisObject;
		Value returnValue;
	};
}

#endif
