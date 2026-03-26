#ifndef _SLAKE_LOADER_LOADER_H_
#define _SLAKE_LOADER_LOADER_H_

#include "reader.h"
#include <peff/advutils/unique_ptr.h>

namespace slake {
	namespace loader {
		class LoaderContext {
		public:
			peff::RcObjectPtr<peff::Alloc> allocator;
			peff::Set<IdRefObject *, IdRefComparator, true> loaded_id_refs;
			peff::Set<CustomTypeDefObject *> loaded_custom_type_defs;
			peff::Set<InterfaceObject *> loaded_interfaces;
			peff::Set<ClassObject *> loaded_classes;
			peff::Set<StructObject *> loaded_structs;
			peff::Set<ScopedEnumObject *> loaded_scoped_enums;
			peff::Set<UnionEnumObject *> loaded_union_enums;
			peff::Set<UnionEnumItemObject *> loaded_union_enum_items;
			peff::Set<FnObject *> loaded_fns;
			peff::Set<ModuleObject *> loaded_modules;
			peff::List<std::pair<Reference, Value>> init_var_data;
			HostRefHolder host_ref_holder;

			SLAKE_API LoaderContext(peff::Alloc *allocator);
			SLAKE_API ~LoaderContext();

			virtual InternalExceptionPointer locate_module(Runtime *rt, const peff::DynArray<IdRefEntry> &ref, Reader *&reader_out) = 0;
		};

		SLAKE_API InternalExceptionPointer load_type(LoaderContext &context, Runtime *runtime, Reader *reader, Object *member, TypeRef &type_out) noexcept;
		SLAKE_API InternalExceptionPointer load_generic_param(LoaderContext &context, Runtime *runtime, Reader *reader, Object *member, GenericParam &generic_param_out) noexcept;
		SLAKE_API InternalExceptionPointer load_value(LoaderContext &context, Runtime *runtime, Reader *reader, Object *member, Value &value_out) noexcept;
		SLAKE_API InternalExceptionPointer load_id_ref_entries(LoaderContext &context, Runtime *runtime, Reader *reader, Object *member, peff::DynArray<IdRefEntry> &entries_out) noexcept;
		SLAKE_API InternalExceptionPointer load_id_ref(LoaderContext &context, Runtime *runtime, Reader *reader, Object *member, HostObjectRef<IdRefObject> &id_ref_out) noexcept;
		SLAKE_API InternalExceptionPointer load_module_members(LoaderContext &context, Runtime *runtime, Reader *reader, BasicModuleObject *module_object) noexcept;
		SLAKE_API InternalExceptionPointer load_single_module(LoaderContext &context, Runtime *runtime, Reader *reader, HostObjectRef<ModuleObject> &module_object_out) noexcept;
		SLAKE_API InternalExceptionPointer load_module(LoaderContext &context, Runtime *runtime, Reader *reader, HostObjectRef<ModuleObject> &module_object_out) noexcept;

		SLAKE_API InternalExceptionPointer complete_parent_namespaces(LoaderContext &context, Runtime *runtime, BasicModuleObject *module_object, const peff::DynArray<IdRefEntry> &ref) noexcept;
	}
}

#endif
