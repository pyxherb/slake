#ifndef _SLAKE_VALDEF_CLASS_H_
#define _SLAKE_VALDEF_CLASS_H_

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

		inline ObjectLayout(std::pmr::memory_resource *memoryResource)
			: memoryResource(memoryResource),
			  fieldRecords(memoryResource),
			  fieldNameMap(memoryResource) {
		}

		ObjectLayout *duplicate() const;

		static ObjectLayout *alloc(std::pmr::memory_resource *memoryResource);
		void dealloc();
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

		ClassObject(Runtime *rt, AccessModifier access, const Type &parentClass);
		inline ClassObject(const ClassObject &x) : ModuleObject(x) {
			_flags = x._flags;

			genericArgs = x.genericArgs;
			genericParams = x.genericParams;

			parentClass = x.parentClass;
			implInterfaces = x.implInterfaces;

			// DO NOT copy the cached instantiated method table.
		}
		virtual ~ClassObject();

		virtual inline ObjectKind getKind() const override { return ObjectKind::Class; }

		virtual inline GenericArgList getGenericArgs() const override {
			return genericArgs;
		}

		virtual inline Type getParentType() const { return parentClass; }
		virtual inline void setParentType(Type parent) { parentClass = parent; }

		/// @brief Check if the class has implemented the interface.
		///
		/// @param[in] pInterface Interface to check.
		///
		/// @return true if implemented, false otherwise.
		bool hasImplemented(const InterfaceObject *pInterface) const;
		bool isBaseOf(const ClassObject *pClass) const;

		virtual Object *duplicate() const override;

		static HostObjectRef<ClassObject> alloc(Runtime *rt, AccessModifier access, const Type &parentClass = {});
		static HostObjectRef<ClassObject> alloc(const ClassObject *other);
		virtual void dealloc() override;
	};

	class InterfaceObject : public ModuleObject {
	protected:
		friend class Runtime;
		friend class ClassObject;

	public:
		GenericArgList genericArgs;

		GenericParamList genericParams;

		std::vector<Type> parents;

		inline InterfaceObject(Runtime *rt, AccessModifier access, const std::vector<Type> &parents)
			: ModuleObject(rt, access), parents(parents) {
		}
		inline InterfaceObject(const InterfaceObject &x) : ModuleObject(x) {
			genericArgs = x.genericArgs;

			genericParams = x.genericParams;

			parents = x.parents;
		}
		virtual ~InterfaceObject();

		virtual inline ObjectKind getKind() const override { return ObjectKind::Interface; }

		virtual Object *duplicate() const override;

		virtual inline GenericArgList getGenericArgs() const override {
			return genericArgs;
		}

		static HostObjectRef<InterfaceObject> alloc(Runtime *rt, AccessModifier access, const std::vector<Type> &parents = {});
		static HostObjectRef<InterfaceObject> alloc(const InterfaceObject *other);
		virtual void dealloc() override;

		/// @brief Check if the interface is derived from specified interface
		/// @param pInterface Interface to check.
		/// @return true if the interface is derived from specified interface, false otherwise.
		bool isDerivedFrom(const InterfaceObject *pInterface) const;
	};
}

#endif
