#ifndef _SLAKE_VALDEF_CLASS_H_
#define _SLAKE_VALDEF_CLASS_H_

#include <cassert>

#include "fn.h"
#include "module.h"
#include "var.h"

namespace Slake {
	class BasicClassValue : public ModuleValue {
	protected:
		Type _parentClass;

		friend class Runtime;

	public:
		inline BasicClassValue(Runtime *rt, AccessModifier access, Value *parent, std::string name, Type parentClass = Type())
			: ModuleValue(rt, access, parent, name), _parentClass(parentClass) {
			reportSizeToRuntime(sizeof(*this) - sizeof(ModuleValue));
		}
		virtual inline ~BasicClassValue() {}

		virtual inline Type getType() const override { return ValueType::CLASS; }
		virtual inline Type getParentType() const { return _parentClass; }
		virtual inline void setParentType(Type parent) { _parentClass = parent; }

		virtual inline std::string toString() const override {
			std::string s = ModuleValue::toString();

			return s;
		}

		BasicClassValue &operator=(const BasicClassValue &) = delete;
		BasicClassValue &operator=(const BasicClassValue &&) = delete;
	};

	class InterfaceValue;

	class ClassValue : public BasicClassValue {
	public:
		std::vector<Type> implInterfaces;//Implemented interfaces

		friend class Runtime;
		friend bool Slake::isConvertible(Type a, Type b);

		inline ClassValue(Runtime *rt, AccessModifier access, Type parentClass = Type(), Value *parent = nullptr, std::string name = "")
			: BasicClassValue(rt, access, parent, name, parentClass) {
			reportSizeToRuntime(sizeof(*this) - sizeof(BasicClassValue));
		}
		virtual inline ~ClassValue() {}

		virtual inline std::string toString() const override {
			std::string s = BasicClassValue::toString();

			return s;
		}

		bool hasImplemented(const InterfaceValue *pInterface) const;
		bool isCompatibleWith(const TraitValue *t) const;
	};

	class InterfaceValue : public BasicClassValue {
	protected:
		friend class Runtime;
		friend class ClassValue;
		friend bool Slake::isConvertible(Type a, Type b);

	public:
		inline InterfaceValue(Runtime *rt, AccessModifier access, Type parentClass = Type(), Value *parent = nullptr, std::string name = "")
			: BasicClassValue(rt, access, parent, name, parentClass) {
			reportSizeToRuntime(sizeof(*this) - sizeof(BasicClassValue));
		}
		virtual inline ~InterfaceValue() {}

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

		virtual inline std::string toString() const override {
			std::string s = BasicClassValue::toString();

			return s;
		}

		InterfaceValue &operator=(const InterfaceValue &) = delete;
		InterfaceValue &operator=(const InterfaceValue &&) = delete;
	};

	class TraitValue : public InterfaceValue {
	protected:
		friend class Runtime;
		friend class ClassValue;

	public:
		inline TraitValue(Runtime *rt, AccessModifier access, Type parentClass = Type(), Value *parent = nullptr, std::string name = "")
			: InterfaceValue(rt, access, parentClass, parent, name) {
			reportSizeToRuntime(sizeof(*this) - sizeof(InterfaceValue));
		}
		virtual inline ~TraitValue() {}

		virtual inline Type getType() const override { return ValueType::TRAIT; }

		virtual inline std::string toString() const override {
			return BasicClassValue::toString();
		}

		TraitValue &operator=(const TraitValue &) = delete;
		TraitValue &operator=(const TraitValue &&) = delete;
	};
}

#endif
