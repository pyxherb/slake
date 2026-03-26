#ifndef _SLAKE_AOT_CONTEXT_H_
#define _SLAKE_AOT_CONTEXT_H_

#include <slake/runtime.h>

namespace slake {
	struct AOTFnExecContext {
		Runtime *runtime;
		HostObjectRef<ContextObject> host_context;
		peff::HashMap<peff::String, Type> *mapped_generic_args;
		Value *args;
		void *mapped_objects;
		Value this_object;
		Value return_value;
	};
}

#endif
