#ifndef _SLAKE_VALDEF_CLASS_H_
#define _SLAKE_VALDEF_CLASS_H_

#include <cassert>

#include "fn.h"
#include "module.h"
#include "var.h"

namespace slake {
	/// @brief Type for storing class flags.
	using ClassFlags = uint16_t;

	constexpr static ClassFlags
		_CLS_ABSTRACT = 0x4000,			// Set if the class is abstract
		_CLS_ABSTRACT_INITED = 0x8000;	// The class has checked if itself is abstract

	class InterfaceValue;

	class ClassValue : public ModuleValue {
	private:
		mutable ClassFlags _flags = 0;

		friend class Runtime;
		friend bool slake::isConvertible(Type a, Type b);

		/// @brief Actually check if the class is abstract.
		/// @return true if the class is abstract, false otherwise.
		bool _isAbstract() const;

	public:
		GenericParamList genericParams;

		Type parentClass;
		std::deque<Type> implInterfaces;  // Implemented interfaces

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

		/// @brief Check if the class has the trait.
		/// @param[in] t Trait to check.
		///
		/// @return true if the class has the trait, false otherwise.
		bool hasTrait(const TraitValue *t) const;

		virtual Value *duplicate() const override;

		inline ClassValue &operator=(const ClassValue &x) {
			((ModuleValue &)*this) = (ModuleValue &)x;

			genericParams = x.genericParams;
			_flags = x._flags;
			implInterfaces = x.implInterfaces;

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

		InterfaceValue &operator=(const InterfaceValue &x) {
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

		TraitValue &operator=(const TraitValue &x) {
			((InterfaceValue &)*this) = (InterfaceValue &)x;
			return *this;
		}
		TraitValue &operator=(TraitValue &&) = delete;
	};
}

#endif
