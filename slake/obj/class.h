#ifndef _SLAKE_OBJ_CLASS_H_
#define _SLAKE_OBJ_CLASS_H_

#include <cassert>
#include <unordered_map>

#include "fn.h"
#include "module.h"
#include "var.h"

namespace slake {
	/// @brief Type for storing class flags.
	using ClassFlags = uint16_t;

	class InterfaceObject;
	class InstanceObject;

	struct ObjectFieldRecord {
		size_t offset;
		Type type;
	};

	struct ObjectLayout {
		std::pmr::memory_resource *memoryResource;
		size_t totalSize = 0;
		std::pmr::vector<ObjectFieldRecord> fieldRecords;
		std::pmr::unordered_map<std::pmr::string, size_t> fieldNameMap;

		SLAKE_API ObjectLayout(std::pmr::memory_resource *memoryResource);

		SLAKE_API ObjectLayout *duplicate() const;

		SLAKE_API static ObjectLayout *alloc(std::pmr::memory_resource *memoryResource);
		SLAKE_API void dealloc();
	};

	class ClassObject : public ModuleObject {
	private:
		mutable ClassFlags _flags = 0;

		friend class Runtime;

	public:
		GenericArgList genericArgs;

		GenericParamList genericParams;

		Type parentClass;
		std::vector<Type> implInterfaces;  // Implemented interfaces

		MethodTable *cachedInstantiatedMethodTable = nullptr;
		ObjectLayout *cachedObjectLayout = nullptr;

		std::pmr::vector<VarObject *> cachedFieldInitVars;

		SLAKE_API ClassObject(Runtime *rt, AccessModifier access, const Type &parentClass);
		SLAKE_API ClassObject(const ClassObject &x);
		SLAKE_API virtual ~ClassObject();

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API virtual GenericArgList getGenericArgs() const override;

		/// @brief Check if the class has implemented the interface.
		///
		/// @param[in] pInterface Interface to check.
		///
		/// @return true if implemented, false otherwise.
		SLAKE_API bool hasImplemented(const InterfaceObject *pInterface) const;
		SLAKE_API bool isBaseOf(const ClassObject *pClass) const;

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API virtual MemberObject *getMember(
			const std::pmr::string &name,
			VarRefContext *varRefContextOut) const;

		SLAKE_API static HostObjectRef<ClassObject> alloc(Runtime *rt, AccessModifier access, const Type &parentClass = {});
		SLAKE_API static HostObjectRef<ClassObject> alloc(const ClassObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class InterfaceObject : public ModuleObject {
	protected:
		friend class Runtime;
		friend class ClassObject;

	public:
		GenericArgList genericArgs;

		GenericParamList genericParams;

		std::vector<Type> parents;

		SLAKE_API InterfaceObject(Runtime *rt, AccessModifier access, const std::vector<Type> &parents);
		SLAKE_API InterfaceObject(const InterfaceObject &x);
		SLAKE_API virtual ~InterfaceObject();

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API virtual MemberObject *getMember(
			const std::pmr::string &name,
			VarRefContext *varRefContextOut) const;

		SLAKE_API virtual GenericArgList getGenericArgs() const override;

		SLAKE_API static HostObjectRef<InterfaceObject> alloc(Runtime *rt, AccessModifier access, const std::vector<Type> &parents = {});
		SLAKE_API static HostObjectRef<InterfaceObject> alloc(const InterfaceObject *other);
		SLAKE_API virtual void dealloc() override;

		/// @brief Check if the interface is derived from specified interface
		/// @param pInterface Interface to check.
		/// @return true if the interface is derived from specified interface, false otherwise.
		SLAKE_API bool isDerivedFrom(const InterfaceObject *pInterface) const;
	};
}

#endif
