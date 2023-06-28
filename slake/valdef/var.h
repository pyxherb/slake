#ifndef _SLAKE_VALDEF_VAR_H_
#define _SLAKE_VALDEF_VAR_H_

#include "member.h"
#include <slake/except.h>
#include <slake/type.h>

namespace Slake {
	class VarValue final : public MemberValue {
	protected:
		ValueRef<Slake::Value, false> value;
		const Type type = ValueType::ANY;

	public:
		inline VarValue(Runtime *rt, AccessModifier access, Type type, Value *parent = nullptr, std::string name = "")
			: MemberValue(rt, access, parent, name), type(type) {
			reportSizeToRuntime(sizeof(*this));
		}

		virtual inline ~VarValue() {}
		virtual inline Type getType() const override { return ValueType::VAR; }
		inline Type getVarType() const { return type; }

		virtual inline MemberValue *getMember(std::string name) override {
			return value ? value->getMember(name) : nullptr;
		}
		virtual inline const MemberValue *getMember(std::string name) const override {
			return value ? value->getMember(name) : nullptr;
		}

		ValueRef<> getValue() { return value; }
		void setValue(Value *value) {
			if (!isCompatible(type, value->getType()))
				throw MismatchedTypeError("Mismatched types");
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
