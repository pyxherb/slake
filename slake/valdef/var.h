#ifndef _SLAKE_VALDEF_VAR_H_
#define _SLAKE_VALDEF_VAR_H_

#include "member.h"
#include <slake/except.h>
#include <slake/type.h>

namespace slake {
	class VarValue final : public MemberValue {
	protected:
		ValueRef<slake::Value, false> value;
		Type type = TypeId::ANY;

		friend class Runtime;

	public:
		inline VarValue(Runtime *rt, AccessModifier access, Type type)
			: MemberValue(rt, access), type(type) {
			reportSizeToRuntime(sizeof(*this) - sizeof(MemberValue));
		}

		virtual ~VarValue() = default;
		virtual inline Type getType() const override { return TypeId::VAR; }
		inline Type getVarType() const { return type; }

		virtual Value *duplicate() const override;

		virtual inline MemberValue *getMember(std::string name) override {
			return value ? value->getMember(name) : nullptr;
		}
		virtual inline const MemberValue *getMember(std::string name) const override {
			return value ? value->getMember(name) : nullptr;
		}

		ValueRef<> getData() { return value; }
		void setData(Value *value) {
			if (value && !isCompatible(type, value->getType()))
				throw MismatchedTypeError("Mismatched types");
			this->value = value;
		}

		VarValue &operator=(const VarValue &x) {
			((MemberValue &)*this) = (MemberValue &)x;
			if (x.value)
				value = x.value->duplicate();
			type = x.type;
			return *this;
		}
		VarValue &operator=(const VarValue &&) = delete;
	};
}

#endif
