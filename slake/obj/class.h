#ifndef _SLAKE_OBJ_CLASS_H_
#define _SLAKE_OBJ_CLASS_H_

#include "fn.h"
#include "module.h"
#include "var.h"

namespace slake {
	class InterfaceObject;
	class InstanceObject;

	struct ObjectFieldRecord {
		peff::String name;
		size_t offset;
		TypeRef type;
		size_t idxInitFieldRecord;

		PEFF_FORCEINLINE ObjectFieldRecord(peff::Alloc *selfAllocator) : name(selfAllocator) {}

		SLAKE_API void replaceAllocator(peff::Alloc *allocator) noexcept;
	};

	struct ObjectLayout {
		peff::RcObjectPtr<peff::Alloc> selfAllocator;
		size_t totalSize = 0;
		size_t alignment = 1;
		peff::DynArray<std::pair<BasicModuleObject *, size_t>> fieldRecordInitModuleFieldsNumber;
		peff::DynArray<ObjectFieldRecord> fieldRecords;
		peff::HashMap<std::string_view, size_t> fieldNameMap;

		SLAKE_API ObjectLayout(peff::Alloc *selfAllocator);

		SLAKE_API ObjectLayout *duplicate(peff::Alloc *allocator) const;

		SLAKE_API static ObjectLayout *alloc(peff::Alloc *selfAllocator);
		SLAKE_API void dealloc();

		SLAKE_API void replaceAllocator(peff::Alloc *allocator) noexcept;
	};

	class MethodTable {
	public:
		peff::RcObjectPtr<peff::Alloc> selfAllocator;
		peff::HashMap<std::string_view, FnObject *> methods;
		peff::DynArray<FnOverloadingObject *> destructors;

		SLAKE_API MethodTable(peff::Alloc *selfAllocator);

		SLAKE_API FnObject *getMethod(const std::string_view &name);

		SLAKE_API MethodTable *duplicate(peff::Alloc *allocator);

		SLAKE_API static MethodTable *alloc(peff::Alloc *selfAllocator);
		SLAKE_API void dealloc();

		SLAKE_API void replaceAllocator(peff::Alloc *allocator) noexcept;
	};

	using ClassFlags = uint16_t;

	class ClassObject : public BasicModuleObject {
	public:
		peff::DynArray<Value> genericArgs;
		peff::HashMap<std::string_view, Value> mappedGenericArgs;

		GenericParamList genericParams;
		peff::HashMap<std::string_view, size_t> mappedGenericParams;

		TypeRef baseType = TypeId::Invalid;
		peff::DynArray<TypeRef> implTypes;	// Implemented interfaces

		MethodTable *cachedInstantiatedMethodTable = nullptr;
		ObjectLayout *cachedObjectLayout = nullptr;

		mutable ClassFlags classFlags = 0;

		SLAKE_API ClassObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API ClassObject(Duplicator *duplicator, const ClassObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~ClassObject();

		SLAKE_API virtual const peff::DynArray<Value> *getGenericArgs() const override;

		/// @brief Check if the class has implemented the interface.
		///
		/// @param[in] pInterface Interface to check.
		///
		/// @return true if implemented, false otherwise.
		SLAKE_API bool hasImplemented(InterfaceObject *pInterface) const;
		SLAKE_API bool isBaseOf(const ClassObject *pClass) const;

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<ClassObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<ClassObject> alloc(Duplicator *duplicator, const ClassObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept override;
	};

	class InterfaceObject : public BasicModuleObject {
	private:
		friend class Runtime;
		friend class ClassObject;

	public:
		peff::DynArray<Value> genericArgs;
		peff::HashMap<std::string_view, Value> mappedGenericArgs;

		GenericParamList genericParams;
		peff::HashMap<std::string_view, size_t> mappedGenericParams;

		peff::DynArray<TypeRef> implTypes;

		peff::Set<InterfaceObject *> implInterfaceIndices;

		SLAKE_API InterfaceObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API InterfaceObject(Duplicator *duplicator, const InterfaceObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~InterfaceObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API virtual const peff::DynArray<Value> *getGenericArgs() const override;

		SLAKE_API static HostObjectRef<InterfaceObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<InterfaceObject> alloc(Duplicator *duplicator, const InterfaceObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_FORCEINLINE void invalidateInheritanceRelationshipCache() noexcept {
			implInterfaceIndices.clear();
		}
		SLAKE_API InternalExceptionPointer updateInheritanceRelationship(peff::Alloc *allocator) noexcept;

		/// @brief Check if the interface is derived from specified interface
		/// @param pInterface Interface to check.
		/// @return true if the interface is derived from specified interface, false otherwise.
		SLAKE_API bool isDerivedFrom(InterfaceObject *pInterface) const;

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept override;
	};

	using StructFlags = uint16_t;

	class StructObject : public BasicModuleObject {
	public:
		peff::DynArray<Value> genericArgs;
		peff::HashMap<std::string_view, Value> mappedGenericArgs;

		GenericParamList genericParams;
		peff::HashMap<std::string_view, size_t> mappedGenericParams;

		peff::DynArray<TypeRef> implTypes;	// Implemented interfaces

		ObjectLayout *cachedObjectLayout = nullptr;

		mutable StructFlags structFlags = 0;

		SLAKE_API StructObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API StructObject(Duplicator *duplicator, const StructObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~StructObject();

		SLAKE_API virtual const peff::DynArray<Value> *getGenericArgs() const override;

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API InternalExceptionPointer isRecursed(peff::Alloc *allocator) noexcept;

		SLAKE_API static HostObjectRef<StructObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<StructObject> alloc(Duplicator *duplicator, const StructObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept override;
	};

	class EnumModuleObject : public MemberObject {
	public:
		peff::HashMap<std::string_view, MemberObject *> members;

		SLAKE_API EnumModuleObject(Runtime *rt, peff::Alloc *selfAllocator, ObjectKind objectKind);
		SLAKE_API EnumModuleObject(Duplicator *duplicator, const EnumModuleObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~EnumModuleObject();

		SLAKE_API virtual Reference getMember(const std::string_view &name) const override;
		[[nodiscard]] SLAKE_API virtual bool addMember(MemberObject *member);
		[[nodiscard]] SLAKE_API virtual bool removeMember(const std::string_view &name);

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept override;
	};

	class ScopedEnumObject : public EnumModuleObject {
	public:
		TypeRef baseType = TypeId::Invalid;

		SLAKE_API ScopedEnumObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API ScopedEnumObject(Duplicator *duplicator, const ScopedEnumObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~ScopedEnumObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API virtual Reference getMember(const std::string_view &name) const override;
		[[nodiscard]] SLAKE_API virtual bool addMember(MemberObject *member);
		[[nodiscard]] SLAKE_API virtual bool removeMember(const std::string_view &name);

		SLAKE_API static HostObjectRef<ScopedEnumObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<ScopedEnumObject> alloc(Duplicator *duplicator, const ScopedEnumObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept override;
	};

	class UnionEnumItemObject : public BasicModuleObject {
	public:
		SLAKE_API UnionEnumItemObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API UnionEnumItemObject(Duplicator *duplicator, const UnionEnumItemObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~UnionEnumItemObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<UnionEnumItemObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<UnionEnumItemObject> alloc(Duplicator *duplicator, const UnionEnumItemObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept override;
	};

	class UnionEnumObject : public EnumModuleObject {
	public:
		peff::DynArray<Value> genericArgs;
		peff::HashMap<std::string_view, Value> mappedGenericArgs;

		GenericParamList genericParams;
		peff::HashMap<std::string_view, size_t> mappedGenericParams;

		SLAKE_API UnionEnumObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API UnionEnumObject(Duplicator *duplicator, const UnionEnumObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~UnionEnumObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API virtual Reference getMember(const std::string_view &name) const override;
		[[nodiscard]] SLAKE_API virtual bool addMember(MemberObject *member);
		[[nodiscard]] SLAKE_API virtual bool removeMember(const std::string_view &name);

		SLAKE_API static HostObjectRef<UnionEnumObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<UnionEnumObject> alloc(Duplicator *duplicator, const UnionEnumObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept override;
	};
}

#endif
