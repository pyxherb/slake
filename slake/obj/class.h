#ifndef _SLAKE_OBJ_CLASS_H_
#define _SLAKE_OBJ_CLASS_H_

#include <cassert>

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
		Type type;

		PEFF_FORCEINLINE ObjectFieldRecord() : name(nullptr) { std::terminate(); }
		PEFF_FORCEINLINE ObjectFieldRecord(peff::Alloc *selfAllocator) : name(selfAllocator) {}

		PEFF_FORCEINLINE bool copy(ObjectFieldRecord &dest) const {
			if (!peff::copy(dest.name, name)) {
				return false;
			}

			dest.offset = offset;
			dest.type = type;

			return true;
		}
	};

	struct ObjectLayout {
		peff::RcObjectPtr<peff::Alloc> selfAllocator;
		size_t totalSize = 0;
		peff::DynArray<ObjectFieldRecord> fieldRecords;
		peff::HashMap<std::string_view, size_t> fieldNameMap;

		SLAKE_API ObjectLayout(peff::Alloc *selfAllocator);

		SLAKE_API ObjectLayout *duplicate() const;

		SLAKE_API static ObjectLayout *alloc(peff::Alloc *selfAllocator);
		SLAKE_API void dealloc();
	};

	class MethodTable {
	public:
		peff::RcObjectPtr<peff::Alloc> selfAllocator;
		peff::HashMap<std::string_view, FnObject *> methods;
		peff::List<FnOverloadingObject *> destructors;

		SLAKE_API MethodTable(peff::Alloc *selfAllocator);

		SLAKE_API FnObject *getMethod(const std::string_view &name);

		SLAKE_API MethodTable *duplicate();

		SLAKE_API static MethodTable *alloc(peff::Alloc *selfAllocator);
		SLAKE_API void dealloc();
	};

	class ClassObject : public ModuleObject {
	private:
		mutable ClassFlags _flags = 0;

		friend class Runtime;

	public:
		GenericArgList genericArgs;
		peff::HashMap<peff::String, Type> mappedGenericArgs;

		GenericParamList genericParams;

		Type baseType;
		peff::DynArray<Type> implTypes;				 // Implemented interfaces

		MethodTable *cachedInstantiatedMethodTable = nullptr;
		ObjectLayout *cachedObjectLayout = nullptr;

		peff::DynArray<Value> cachedFieldInitValues;

		SLAKE_API ClassObject(Runtime *rt);
		SLAKE_API ClassObject(const ClassObject &x, bool &succeededOut);
		SLAKE_API virtual ~ClassObject();

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API virtual const GenericArgList *getGenericArgs() const override;

		/// @brief Check if the class has implemented the interface.
		///
		/// @param[in] pInterface Interface to check.
		///
		/// @return true if implemented, false otherwise.
		SLAKE_API bool hasImplemented(const InterfaceObject *pInterface) const;
		SLAKE_API bool isBaseOf(const ClassObject *pClass) const;

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API static HostObjectRef<ClassObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<ClassObject> alloc(const ClassObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class InterfaceObject : public ModuleObject {
	protected:
		friend class Runtime;
		friend class ClassObject;

	public:
		GenericArgList genericArgs;
		peff::HashMap<peff::String, Type> mappedGenericArgs;

		GenericParamList genericParams;

		peff::DynArray<Type> implTypes;

		SLAKE_API InterfaceObject(Runtime *rt);
		SLAKE_API InterfaceObject(const InterfaceObject &x, bool &succeededOut);
		SLAKE_API virtual ~InterfaceObject();

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API virtual const GenericArgList *getGenericArgs() const override;

		SLAKE_API static HostObjectRef<InterfaceObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<InterfaceObject> alloc(const InterfaceObject *other);
		SLAKE_API virtual void dealloc() override;

		/// @brief Check if the interface is derived from specified interface
		/// @param pInterface Interface to check.
		/// @return true if the interface is derived from specified interface, false otherwise.
		SLAKE_API bool isDerivedFrom(const InterfaceObject *pInterface) const;
	};
}

#endif
