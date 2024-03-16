#ifndef _SLAKE_VALDEF_VAR_H_
#define _SLAKE_VALDEF_VAR_H_

#include "member.h"
#include <slake/except.h>
#include <slake/type.h>

namespace slake {
	using VarFlags = uint8_t;
	constexpr static VarFlags
		VAR_REG = 0x01;

	class VarValue final : public MemberValue {
	public:
		mutable slake::Value* value = nullptr;
		Type type = TypeId::Any;

		VarFlags flags;

		VarValue(Runtime *rt, AccessModifier access, Type type, VarFlags flags = 0);
		virtual ~VarValue();

		virtual inline Type getType() const override { return TypeId::Var; }
		inline Type getVarType() const { return type; }

		virtual Value *duplicate() const override;

		Value *getData() const { return value; }
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
		VarValue &operator=(VarValue &&) = delete;
	};
}

#endif
