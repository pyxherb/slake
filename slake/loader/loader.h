#ifndef _SLAKE_LOADER_LOADER_H_
#define _SLAKE_LOADER_LOADER_H_

#include "reader.h"

namespace slake {
	namespace loader {
		SLAKE_API InternalExceptionPointer loadType(Runtime *runtime, Reader *reader, Object *member, Type &typeOut) noexcept;
		SLAKE_API InternalExceptionPointer loadGenericParam(Runtime *runtime, Reader *reader, Object *member, GenericParam &genericParamOut) noexcept;
		SLAKE_API InternalExceptionPointer loadValue(Runtime *runtime, Reader *reader, Object *member, Value &valueOut) noexcept;
		SLAKE_API InternalExceptionPointer loadIdRefEntries(Runtime *runtime, Reader *reader, Object *member, peff::DynArray<IdRefEntry> &entriesOut) noexcept;
		SLAKE_API InternalExceptionPointer loadIdRef(Runtime *runtime, Reader *reader, Object *member, HostObjectRef<IdRefObject> &idRefOut) noexcept;
		SLAKE_API InternalExceptionPointer loadModuleMembers(Runtime *runtime, Reader *reader, ModuleObject *moduleObject) noexcept;
		SLAKE_API InternalExceptionPointer loadModule(Runtime *runtime, Reader *reader, HostObjectRef<ModuleObject> &moduleObjectOut) noexcept;
	}
}

#endif
