#ifndef _SLAKE_LOADER_LOADER_H_
#define _SLAKE_LOADER_LOADER_H_

#include "reader.h"

namespace slake {
	namespace loader {
		SLAKE_API InternalExceptionPointer loadModule(Runtime *runtime, Reader *reader, ModuleObject *&moduleObjectOut) noexcept;
	}
}

#endif
