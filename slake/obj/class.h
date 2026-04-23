#ifndef _SLAKE_OBJ_CLASS_H_
#define _SLAKE_OBJ_CLASS_H_

#include "fn.h"
#include "module.h"
#include "var.h"
#include <peff/advutils/unique_ptr.h>
#include <peff/base/deallocable.h>

namespace slake {
	class InterfaceObject;
	class InstanceObject;

	struct ObjectFieldRecord {
		peff::String name;
		size_t offset;
		TypeRef type;
		size_t idx_init_field_record;

		PEFF_FORCEINLINE ObjectFieldRecord(peff::Alloc *self_allocator) : name(self_allocator) {}

		SLAKE_API void replace_allocator(peff::Alloc *allocator) noexcept;
	};

	struct ObjectLayout {
		peff::RcObjectPtr<peff::Alloc> self_allocator;
		size_t total_size = 0;
		size_t alignment = 1;
		peff::DynArray<std::pair<BasicModuleObject *, size_t>> field_record_init_module_fields_number;
		peff::DynArray<ObjectFieldRecord> field_records;
		peff::HashMap<std::string_view, size_t> field_name_map;

		SLAKE_API ObjectLayout(peff::Alloc *self_allocator);

		SLAKE_API ObjectLayout *duplicate(peff::Alloc *allocator) const;

		SLAKE_API static ObjectLayout *alloc(peff::Alloc *self_allocator);
		SLAKE_API void dealloc();

		SLAKE_API void replace_allocator(peff::Alloc *allocator) noexcept;
	};

	class MethodTable {
	public:
		peff::RcObjectPtr<peff::Alloc> self_allocator;
		peff::HashMap<std::string_view, FnObject *> methods;
		peff::DynArray<FnOverloadingObject *> destructors;

		SLAKE_API MethodTable(peff::Alloc *self_allocator);

		SLAKE_API FnObject *get_method(const std::string_view &name);

		SLAKE_API MethodTable *duplicate(peff::Alloc *allocator);

		SLAKE_API static MethodTable *alloc(peff::Alloc *self_allocator);
		SLAKE_API void dealloc();

		SLAKE_API void replace_allocator(peff::Alloc *allocator) noexcept;
	};

	using ClassFlags = uint16_t;

	typedef void (*ClassNativeDestructor)(InstanceObject *instance_object);

	class ClassObject : public BasicModuleObject {
	public:
		peff::DynArray<TypeRef> generic_args;
		peff::HashMap<std::string_view, Value> mapped_generic_args;

		GenericParamList generic_params;
		peff::HashMap<std::string_view, size_t> mapped_generic_params;

		TypeRef base_type = TypeId::Invalid;
		peff::DynArray<TypeRef> impl_types;	// Implemented interfaces

		peff::UniquePtr<MethodTable, peff::DeallocableDeleter<MethodTable>> cached_instantiated_method_table;
		peff::UniquePtr<ObjectLayout, peff::DeallocableDeleter<ObjectLayout>> cached_object_layout;

		mutable ClassFlags class_flags = 0;

		SLAKE_API ClassObject(Runtime *rt, peff::Alloc *self_allocator);
		SLAKE_API ClassObject(Duplicator *duplicator, const ClassObject &x, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~ClassObject();

		SLAKE_API virtual const peff::DynArray<TypeRef> *get_generic_args() const override;

		/// @brief Check if the class has implemented the interface.
		///
		/// @param[in] p_interface Interface to check.
		///
		/// @return true if implemented, false otherwise.
		SLAKE_API bool has_implemented(InterfaceObject *p_interface) const;
		SLAKE_API bool is_base_of(const ClassObject *p_class) const;

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<ClassObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<ClassObject> alloc(Duplicator *duplicator, const ClassObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replace_allocator(peff::Alloc *allocator) noexcept override;
	};

	class InterfaceObject : public BasicModuleObject {
	private:
		friend class Runtime;
		friend class ClassObject;

	public:
		peff::DynArray<TypeRef> generic_args;
		peff::HashMap<std::string_view, Value> mapped_generic_args;

		GenericParamList generic_params;
		peff::HashMap<std::string_view, size_t> mapped_generic_params;

		peff::DynArray<TypeRef> impl_types;

		peff::Set<InterfaceObject *> impl_interface_indices;

		SLAKE_API InterfaceObject(Runtime *rt, peff::Alloc *self_allocator);
		SLAKE_API InterfaceObject(Duplicator *duplicator, const InterfaceObject &x, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~InterfaceObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API virtual const peff::DynArray<TypeRef> *get_generic_args() const override;

		SLAKE_API static HostObjectRef<InterfaceObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<InterfaceObject> alloc(Duplicator *duplicator, const InterfaceObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_FORCEINLINE void invalidate_inheritance_relationship_cache() noexcept {
			impl_interface_indices.clear();
		}
		SLAKE_API InternalExceptionPointer update_inheritance_relationship(peff::Alloc *allocator) noexcept;

		/// @brief Check if the interface is derived from specified interface
		/// @param p_interface Interface to check.
		/// @return true if the interface is derived from specified interface, false otherwise.
		SLAKE_API bool is_derived_from(InterfaceObject *p_interface) const;

		SLAKE_API virtual void replace_allocator(peff::Alloc *allocator) noexcept override;
	};

	using StructFlags = uint16_t;

	class StructObject : public BasicModuleObject {
	public:
		peff::DynArray<TypeRef> generic_args;
		peff::HashMap<std::string_view, Value> mapped_generic_args;

		GenericParamList generic_params;
		peff::HashMap<std::string_view, size_t> mapped_generic_params;

		peff::DynArray<TypeRef> impl_types;	// Implemented interfaces

		peff::UniquePtr<ObjectLayout, peff::DeallocableDeleter<ObjectLayout>> cached_object_layout;

		mutable StructFlags struct_flags = 0;

		SLAKE_API StructObject(Runtime *rt, peff::Alloc *self_allocator);
		SLAKE_API StructObject(Duplicator *duplicator, const StructObject &x, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~StructObject();

		SLAKE_API virtual const peff::DynArray<TypeRef> *get_generic_args() const override;

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API InternalExceptionPointer is_recursed(peff::Alloc *allocator) noexcept;

		SLAKE_API static HostObjectRef<StructObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<StructObject> alloc(Duplicator *duplicator, const StructObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replace_allocator(peff::Alloc *allocator) noexcept override;
	};

	class ScopedEnumObject : public BasicModuleObject {
	public:
		TypeRef base_type = TypeId::Invalid;

		SLAKE_API ScopedEnumObject(Runtime *rt, peff::Alloc *self_allocator);
		SLAKE_API ScopedEnumObject(Duplicator *duplicator, const ScopedEnumObject &x, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~ScopedEnumObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<ScopedEnumObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<ScopedEnumObject> alloc(Duplicator *duplicator, const ScopedEnumObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replace_allocator(peff::Alloc *allocator) noexcept override;
	};

	class UnionEnumItemObject : public BasicModuleObject {
	public:
		peff::UniquePtr<ObjectLayout, peff::DeallocableDeleter<ObjectLayout>> cached_object_layout;

		SLAKE_API UnionEnumItemObject(Runtime *rt, peff::Alloc *self_allocator);
		SLAKE_API UnionEnumItemObject(Duplicator *duplicator, const UnionEnumItemObject &x, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~UnionEnumItemObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<UnionEnumItemObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<UnionEnumItemObject> alloc(Duplicator *duplicator, const UnionEnumItemObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replace_allocator(peff::Alloc *allocator) noexcept override;
	};

	class UnionEnumObject : public BasicModuleObject {
	public:
		peff::DynArray<TypeRef> generic_args;
		peff::HashMap<std::string_view, Value> mapped_generic_args;

		GenericParamList generic_params;
		peff::HashMap<std::string_view, size_t> mapped_generic_params;

		size_t cached_max_size = 0, cached_max_align = 0;

		SLAKE_API UnionEnumObject(Runtime *rt, peff::Alloc *self_allocator);
		SLAKE_API UnionEnumObject(Duplicator *duplicator, const UnionEnumObject &x, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~UnionEnumObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API virtual Reference get_member(const std::string_view &name) const override;
		[[nodiscard]] SLAKE_API virtual bool add_member(MemberObject *member) override;
		SLAKE_API virtual void remove_member(const std::string_view &name) override;

		SLAKE_API static HostObjectRef<UnionEnumObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<UnionEnumObject> alloc(Duplicator *duplicator, const UnionEnumObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replace_allocator(peff::Alloc *allocator) noexcept override;
	};
}

#endif
