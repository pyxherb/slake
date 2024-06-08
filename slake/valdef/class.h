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
		virtual ~ClassObject();

		GenericParamList genericParams;

		Type parentClass;
		std::deque<Type> implInterfaces;  // Implemented interfaces

		/// @brief User-defined instantiator.
		ClassInstantiator customInstantiator;

		virtual inline Type getType() const override { return TypeId::Class; }
		virtual inline Type getParentType() const { return parentClass; }
		virtual inline void setParentType(Type parent) { parentClass = parent; }

		/// @brief Check if the class has implemented the interface.
		///
		/// @param[in] pInterface Interface to check.
		///
		/// @return true if implemented, false otherwise.
		bool hasImplemented(const InterfaceObject *pInterface) const;

		virtual Object *duplicate() const override;

		static HostObjectRef<ClassObject> alloc(Runtime *rt, AccessModifier access, const Type &parentClass = {});
		virtual void dealloc() override;

		inline ClassObject &operator=(const ClassObject &x) {
			((ModuleObject &)*this) = (ModuleObject &)x;

			parentClass = x.parentClass;
			genericParams = x.genericParams;
			_flags = x._flags;
			implInterfaces = x.implInterfaces;
			customInstantiator = x.customInstantiator;

			return *this;
		}
		ClassObject &operator=(ClassObject &&) = delete;
	};

	class InterfaceObject : public ModuleObject {
	protected:
		friend class Runtime;
		friend class ClassObject;

	public:
		inline InterfaceObject(Runtime *rt, AccessModifier access, const std::deque<Type> &parents)
			: ModuleObject(rt, access), parents(parents) {
		}
		virtual ~InterfaceObject();

		GenericParamList genericParams;

		std::deque<Type> parents;

		virtual inline Type getType() const override { return TypeId::Interface; }

		virtual Object *duplicate() const override;

		static HostObjectRef<InterfaceObject> alloc(Runtime *rt, AccessModifier access, const std::deque<Type> &parents = {});
		virtual void dealloc() override;

		/// @brief Check if the interface is derived from specified interface
		/// @param pInterface Interface to check.
		/// @return true if the interface is derived from specified interface, false otherwise.
		bool isDerivedFrom(const InterfaceObject *pInterface) const;

		inline InterfaceObject &operator=(const InterfaceObject &x) {
			((ModuleObject &)*this) = (ModuleObject &)x;

			parents = x.parents;

			return *this;
		}
		InterfaceObject &operator=(InterfaceObject &&) = delete;
	};
}

#endif
