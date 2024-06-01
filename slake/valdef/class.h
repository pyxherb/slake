#ifndef _SLAKE_VALDEF_CLASS_H_
#define _SLAKE_VALDEF_CLASS_H_

#include <cassert>

#include "fn.h"
#include "module.h"
#include "var.h"

namespace slake {
	/// @brief Type for storing class flags.
	using ClassFlags = uint16_t;

	class InterfaceValue;
	class ObjectValue;

	using ClassInstantiator = std::function<ObjectValue *(Runtime *runtime, ClassValue *cls)>;

	class ClassValue : public ModuleValue {
	private:
		mutable ClassFlags _flags = 0;

		friend class Runtime;
		friend bool slake::isConvertible(Type a, Type b);

	public:
		GenericParamList genericParams;

		Type parentClass;
		std::deque<Type> implInterfaces;  // Implemented interfaces

		/// @brief User-defined instantiator.
		ClassInstantiator customInstantiator;

		ClassValue(Runtime *rt, AccessModifier access, Type parentClass = {});
		virtual ~ClassValue();

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
		bool hasImplemented(const InterfaceValue *pInterface) const;

		virtual Value *duplicate() const override;

		inline ClassValue &operator=(const ClassValue &x) {
			((ModuleValue &)*this) = (ModuleValue &)x;

			genericParams = x.genericParams;
			_flags = x._flags;
			implInterfaces = x.implInterfaces;
			customInstantiator = x.customInstantiator;

			return *this;
		}
		ClassValue &operator=(ClassValue &&) = delete;
	};

	class InterfaceValue : public ModuleValue {
	protected:
		friend class Runtime;
		friend class ClassValue;
		friend bool slake::isConvertible(Type a, Type b);

	public:
		GenericParamList genericParams;

		std::deque<Type> parents;

		inline InterfaceValue(Runtime *rt, AccessModifier access, std::deque<Type> parents = {})
			: ModuleValue(rt, access), parents(parents) {
			reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(ModuleValue));
		}
		virtual ~InterfaceValue();

		virtual inline Type getType() const override { return TypeId::Interface; }

		virtual Value *duplicate() const override;

		/// @brief Check if the interface is derived from specified interface
		/// @param pInterface Interface to check.
		/// @return true if the interface is derived from specified interface, false otherwise.
		bool isDerivedFrom(const InterfaceValue *pInterface) const;

		inline InterfaceValue &operator=(const InterfaceValue &x) {
			((ModuleValue &)*this) = (ModuleValue &)x;

			parents = x.parents;

			return *this;
		}
		InterfaceValue &operator=(InterfaceValue &&) = delete;
	};

	class TraitValue : public InterfaceValue {
	protected:
		friend class Runtime;
		friend class ClassValue;

	public:
		GenericParamList genericParams;

		inline TraitValue(Runtime *rt, AccessModifier access, std::deque<Type> parents = {})
			: InterfaceValue(rt, access, parents) {
			reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(InterfaceValue));
		}
		virtual ~TraitValue();

		virtual inline Type getType() const override { return TypeId::Trait; }

		virtual Value *duplicate() const override;

		inline TraitValue &operator=(const TraitValue &x) {
			((InterfaceValue &)*this) = (InterfaceValue &)x;
			return *this;
		}
		TraitValue &operator=(TraitValue &&) = delete;
	};
}

#endif
