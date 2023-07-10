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
		mutable ClassFlags _flags;

		friend class Runtime;
		friend bool slake::isConvertible(Type a, Type b);

		/// @brief Actually check if the class is abstract.
		/// @return true if the class is abstract, false otherwise.
		bool _isAbstract() const;

	public:
		Type parentClass;
		std::deque<Type> implInterfaces;  // Implemented interfaces

		inline ClassValue(Runtime *rt, AccessModifier access, Type parentClass = Type())
			: ModuleValue(rt, access), parentClass(parentClass) {
			reportSizeToRuntime(sizeof(*this) - sizeof(ModuleValue));
		}
		virtual ~ClassValue() = default;

		virtual inline Type getType() const override { return ValueType::CLASS; }
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

		/// @brief Check if the class is compatible with a trait.
		/// @param[in] t Trait to check.
		///
		/// @return true if compatible, false otherwise.
		bool isCompatibleWith(const TraitValue *t) const;

		ClassValue &operator=(const ClassValue &) = delete;
		ClassValue &operator=(const ClassValue &&) = delete;
	};

	class InterfaceValue : public ModuleValue {
	protected:
		friend class Runtime;
		friend class ClassValue;
		friend bool slake::isConvertible(Type a, Type b);

	public:
		std::deque<Type> parents;

		inline InterfaceValue(Runtime *rt, AccessModifier access, std::deque<Type> parents = {})
			: ModuleValue(rt, access), parents(parents) {
			reportSizeToRuntime(sizeof(*this) - sizeof(ModuleValue));
		}
		virtual ~InterfaceValue() = default;

		virtual inline void addMember(std::string name, MemberValue *value) override {
			switch (value->getType().valueType) {
				case ValueType::FN:
				case ValueType::VAR:
					ModuleValue::addMember(name, value);
					break;
				default:
					throw std::logic_error("Unacceptable member type");
			}
			value->bind(this, name);
		}

		virtual inline Type getType() const override { return ValueType::INTERFACE; }

		/// @brief Check if the interface is derived from specified interface
		/// @param pInterface Interface to check.
		/// @return true if the interface is derived from specified interface, false otherwise.
		virtual bool isDerivedFrom(const InterfaceValue *pInterface) const;

		InterfaceValue &operator=(const InterfaceValue &) = delete;
		InterfaceValue &operator=(const InterfaceValue &&) = delete;
	};

	class TraitValue : public InterfaceValue {
	protected:
		friend class Runtime;
		friend class ClassValue;

	public:
		inline TraitValue(Runtime *rt, AccessModifier access, std::deque<Type> parents = {})
			: InterfaceValue(rt, access, parents) {
			reportSizeToRuntime(sizeof(*this) - sizeof(InterfaceValue));
		}
		virtual ~TraitValue() = default;

		virtual inline Type getType() const override { return ValueType::TRAIT; }

		TraitValue &operator=(const TraitValue &) = delete;
		TraitValue &operator=(const TraitValue &&) = delete;
	};
}

#endif
