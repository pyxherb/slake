#ifndef _SLAKE_VALDEF_CLASS_H_
#define _SLAKE_VALDEF_CLASS_H_

#include <cassert>

#include "fn.h"
#include "module.h"
#include "var.h"

namespace slake {
	/// @brief Type for storing class flags.
	using ClassFlags = uint16_t;

	class InterfaceObject;
	class InstanceObject;

	using ClassInstantiator = std::function<InstanceObject *(Runtime *runtime, ClassObject *cls)>;

	class ClassObject : public ModuleObject {
	private:
		mutable ClassFlags _flags = 0;

		friend class Runtime;

	public:
		ClassObject(Runtime *rt, AccessModifier access, const Type &parentClass);
		inline ClassObject(const ClassObject &x) : ModuleObject(x) {
			parentClass = x.parentClass;
			genericParams = x.genericParams;
			_flags = x._flags;
			implInterfaces = x.implInterfaces;
			customInstantiator = x.customInstantiator;
		}
		virtual ~ClassObject();

		GenericParamList genericParams;

		Type parentClass;
		std::deque<Type> implInterfaces;  // Implemented interfaces

		MethodTable *cachedInstantiatedMethodTable = nullptr;

		/// @brief User-defined instantiator.
		ClassInstantiator customInstantiator;

		virtual inline ObjectKind getKind() const override { return ObjectKind::Class; }
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
		inline InterfaceObject(Runtime *rt, AccessModifier access, const std::deque<Type> &parents)
			: ModuleObject(rt, access), parents(parents) {
		}
		inline InterfaceObject(const InterfaceObject& x) : ModuleObject(x) {
			parents = x.parents;
		}
		virtual ~InterfaceObject();

		GenericParamList genericParams;

		std::deque<Type> parents;

		virtual inline ObjectKind getKind() const override { return ObjectKind::Interface; }

		virtual Object *duplicate() const override;

		static HostObjectRef<InterfaceObject> alloc(Runtime *rt, AccessModifier access, const std::deque<Type> &parents = {});
		static HostObjectRef<InterfaceObject> alloc(const InterfaceObject *other);
		virtual void dealloc() override;

		/// @brief Check if the interface is derived from specified interface
		/// @param pInterface Interface to check.
		/// @return true if the interface is derived from specified interface, false otherwise.
		bool isDerivedFrom(const InterfaceObject *pInterface) const;
	};
}

#endif
