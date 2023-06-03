#ifndef _SLAKE_VALDEF_VAR_H_
#define _SLAKE_VALDEF_VAR_H_

#include "member.h"

namespace Slake {
	class VarValue final : public MemberValue {
	protected:
		ValueRef<Slake::Value, false> value;
		const Type type = Type(ValueType::ANY);

	public:
		inline VarValue(Runtime *rt, AccessModifier access, Type type, Value *parent)
			: MemberValue(rt, access, parent), type(type) {
			reportSizeToRuntime(sizeof(*this));
		}

		virtual inline ~VarValue() {}
		virtual inline Type getType() const override { return ValueType::VAR; }
		inline Type getVarType() const { return type; }

		virtual inline Value *getMember(std::string name) override {
			return value ? value->getMember(name) : nullptr;
		}
		virtual inline const Value *getMember(std::string name) const override {
			return value ? value->getMember(name) : nullptr;
		}

		ValueRef<> getValue() { return value; }
		void setValue(Value *value) {
			// if (value->getType() != type)
			//	throw std::runtime_error("Mismatched types");
			this->value = value;
		}

		VarValue &operator=(const VarValue &) = delete;
		VarValue &operator=(const VarValue &&) = delete;

		virtual inline std::string toString() const override {
			return MemberValue::toString() + ",\"value\":" + (value ? "{" + std::to_string((uintptr_t)value) + "}" : "null");
		}
	};
}

#endif
