#ifndef _SLAKE_LOADER_LOADER_H_
#define _SLAKE_LOADER_LOADER_H_

#include "reader.h"
#include <peff/advutils/unique_ptr.h>

namespace slake {
	namespace loader {
		class LoaderContext {
		public:
			peff::RcObjectPtr<peff::Alloc> allocator;
			peff::Set<IdRefObject *, IdRefComparator, true> loadedIdRefs;
			peff::Set<CustomTypeDefObject *> loadedCustomTypeDefs;
			peff::Set<InterfaceObject *> loadedInterfaces;
			peff::Set<ClassObject *> loadedClasses;
			peff::Set<StructObject *> loadedStructs;
			peff::Set<ScopedEnumObject *> loadedScopedEnums;
			peff::Set<FnObject *> loadedFns;
			peff::Set<ModuleObject *> loadedModules;
			HostRefHolder hostRefHolder;

			SLAKE_API LoaderContext(peff::Alloc *allocator);
			SLAKE_API ~LoaderContext();

			virtual InternalExceptionPointer locateModule(Runtime *rt, const peff::DynArray<IdRefEntry> &ref, Reader *&readerOut) = 0;
		};

		SLAKE_API InternalExceptionPointer loadType(LoaderContext &context, Runtime *runtime, Reader *reader, Object *member, TypeRef &typeOut) noexcept;
		SLAKE_API InternalExceptionPointer loadGenericParam(LoaderContext &context, Runtime *runtime, Reader *reader, Object *member, GenericParam &genericParamOut) noexcept;
		SLAKE_API InternalExceptionPointer loadValue(LoaderContext &context, Runtime *runtime, Reader *reader, Object *member, Value &valueOut) noexcept;
		SLAKE_API InternalExceptionPointer loadIdRefEntries(LoaderContext &context, Runtime *runtime, Reader *reader, Object *member, peff::DynArray<IdRefEntry> &entriesOut) noexcept;
		SLAKE_API InternalExceptionPointer loadIdRef(LoaderContext &context, Runtime *runtime, Reader *reader, Object *member, HostObjectRef<IdRefObject> &idRefOut) noexcept;
		SLAKE_API InternalExceptionPointer loadModuleMembers(LoaderContext &context, Runtime *runtime, Reader *reader, BasicModuleObject *moduleObject) noexcept;
		SLAKE_API InternalExceptionPointer loadSingleModule(LoaderContext &context, Runtime *runtime, Reader *reader, HostObjectRef<ModuleObject> &moduleObjectOut) noexcept;
		SLAKE_API InternalExceptionPointer loadModule(LoaderContext &context, Runtime *runtime, Reader *reader, HostObjectRef<ModuleObject> &moduleObjectOut) noexcept;

		SLAKE_API InternalExceptionPointer completeParentNamespaces(LoaderContext &context, Runtime *runtime, BasicModuleObject *moduleObject, const peff::DynArray<IdRefEntry> &ref) noexcept;
	}
}

#endif
