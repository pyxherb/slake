#ifndef _SLAKE_OBJ_CLASS_H_
#define _SLAKE_OBJ_CLASS_H_

#include "fn.h"
#include "module.h"
#include "var.h"

namespace slake {
	/// @brief Type for storing class flags.
	using ClassFlags = uint16_t;

	class InterfaceObject;
	class InstanceObject;

	struct ObjectFieldRecord {
		peff::String name;
		size_t offset;
		TypeRef type;

		PEFF_FORCEINLINE ObjectFieldRecord(peff::Alloc *selfAllocator) : name(selfAllocator) {}

		SLAKE_API void replaceAllocator(peff::Alloc *allocator) noexcept;
	};

	struct ObjectLayout {
#if _DEBUG
		ClassObject *clsObject = nullptr;  // For debugging
#endif
		peff::RcObjectPtr<peff::Alloc> selfAllocator;
		size_t totalSize = 0;
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

	class ClassObject : public ModuleObject {
	private:
		mutable ClassFlags _flags = 0;

		friend class Runtime;

	public:
		GenericArgList genericArgs;
		peff::HashMap<peff::String, TypeRef> mappedGenericArgs;

		GenericParamList genericParams;

		TypeRef baseType;
		peff::DynArray<TypeRef> implTypes;	// Implemented interfaces

		MethodTable *cachedInstantiatedMethodTable = nullptr;
		ObjectLayout *cachedObjectLayout = nullptr;

		peff::DynArray<Value> cachedFieldInitValues;

		SLAKE_API ClassObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API ClassObject(Duplicator *duplicator, const ClassObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~ClassObject();

		SLAKE_API virtual const GenericArgList *getGenericArgs() const override;

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

	class InterfaceObject : public ModuleObject {
	private:
		friend class Runtime;
		friend class ClassObject;

	public:
		GenericArgList genericArgs;
		peff::HashMap<peff::String, TypeRef> mappedGenericArgs;

		GenericParamList genericParams;

		peff::DynArray<TypeRef> implTypes;

		peff::Set<InterfaceObject *> implInterfaceIndices;

		SLAKE_API InterfaceObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API InterfaceObject(Duplicator *duplicator, const InterfaceObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~InterfaceObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API virtual const GenericArgList *getGenericArgs() const override;

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

	class StructObject : public ModuleObject {
	private:
		mutable ClassFlags _flags = 0;

		friend class Runtime;

	public:
		GenericArgList genericArgs;
		peff::HashMap<peff::String, TypeRef> mappedGenericArgs;

		GenericParamList genericParams;

		ObjectLayout *cachedObjectLayout = nullptr;

		peff::DynArray<Value> cachedFieldInitValues;

		SLAKE_API StructObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API StructObject(Duplicator *duplicator, const StructObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~StructObject();

		SLAKE_API virtual const GenericArgList *getGenericArgs() const override;

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API InternalExceptionPointer isRecursed(peff::Alloc *allocator) noexcept;

		SLAKE_API static HostObjectRef<StructObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<StructObject> alloc(Duplicator *duplicator, const StructObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept override;
	};
}

#endif
