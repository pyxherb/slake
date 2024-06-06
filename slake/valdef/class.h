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
		GenericParamList genericParams;

		Type parentClass;
		std::deque<Type> implInterfaces;  // Implemented interfaces

		/// @brief User-defined instantiator.
		ClassInstantiator customInstantiator;

		ClassObject(Runtime *rt, AccessModifier access, Type parentClass = {});
		virtual ~ClassObject();

		virtual inline Type getType() const override { return TypeId::Class; }
		virtual inline Type getParentType() const { return parentClass; }
		virtual inline void setParentType(Type parent) { parentClass = parent; }

		/// @brief Check if the class is abstract.
		///
		/// @return true if abstract, false otherwise.
		bool isAbstract() const;

		/// @brief Check if the class has implemented the interface.
		///
		/// @param[in] pInterface Interface to check.
		///
		/// @return true if implemented, false otherwise.
		bool hasImplemented(const InterfaceObject *pInterface) const;

		virtual Object *duplicate() const override;

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
		GenericParamList genericParams;

		std::deque<Type> parents;

		inline InterfaceObject(Runtime *rt, AccessModifier access, std::deque<Type> parents = {})
			: ModuleObject(rt, access), parents(parents) {
			reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(ModuleObject));
		}
		virtual ~InterfaceObject();

		virtual inline Type getType() const override { return TypeId::Interface; }

		virtual Object *duplicate() const override;

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

	class TraitObject : public InterfaceObject {
	protected:
		friend class Runtime;
		friend class ClassObject;

	public:
		GenericParamList genericParams;

		inline TraitObject(Runtime *rt, AccessModifier access, std::deque<Type> parents = {})
			: InterfaceObject(rt, access, parents) {
			reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(InterfaceObject));
		}
		virtual ~TraitObject();

		virtual inline Type getType() const override { return TypeId::Trait; }

		virtual Object *duplicate() const override;

		inline TraitObject &operator=(const TraitObject &x) {
			((InterfaceObject &)*this) = (InterfaceObject &)x;
			return *this;
		}
		TraitObject &operator=(TraitObject &&) = delete;
	};
}

#endif
