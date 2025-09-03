#ifndef _SLAKE_LOADER_LOADER_H_
#define _SLAKE_LOADER_LOADER_H_

#include "reader.h"

namespace slake {
	namespace loader {
		class LoaderContext {
		public:
			SLAKE_API LoaderContext();
			SLAKE_API ~LoaderContext();

			virtual InternalExceptionPointer locateModule(Runtime *rt, const peff::DynArray<IdRefEntry> &ref, Reader *&readerOut) = 0;
		};

		SLAKE_API InternalExceptionPointer loadType(LoaderContext &context, Runtime *runtime, Reader *reader, Object *member, Type &typeOut) noexcept;
		SLAKE_API InternalExceptionPointer loadGenericParam(LoaderContext &context, Runtime *runtime, Reader *reader, Object *member, GenericParam &genericParamOut) noexcept;
		SLAKE_API InternalExceptionPointer loadValue(LoaderContext &context, Runtime *runtime, Reader *reader, Object *member, Value &valueOut) noexcept;
		SLAKE_API InternalExceptionPointer loadIdRefEntries(LoaderContext &context, Runtime *runtime, Reader *reader, Object *member, peff::DynArray<IdRefEntry> &entriesOut) noexcept;
		SLAKE_API InternalExceptionPointer loadIdRef(LoaderContext &context, Runtime *runtime, Reader *reader, Object *member, HostObjectRef<IdRefObject> &idRefOut) noexcept;
		SLAKE_API InternalExceptionPointer loadModuleMembers(LoaderContext &context, Runtime *runtime, Reader *reader, ModuleObject *moduleObject) noexcept;
		SLAKE_API InternalExceptionPointer loadModule(LoaderContext &context, Runtime *runtime, Reader *reader, HostObjectRef<ModuleObject> &moduleObjectOut) noexcept;
	}
}

#endif
