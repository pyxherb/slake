#ifndef _SLAKE_VALDEF_VAR_H_
#define _SLAKE_VALDEF_VAR_H_

#include "member.h"
#include <slake/except.h>
#include <slake/type.h>

namespace slake {
	class BasicVarValue final : public MemberValue {
	public:
		BasicVarValue(Runtime *rt, AccessModifier access);
		virtual ~BasicVarValue();

		virtual Value *getData() = 0;
		virtual void setData(Value *value) = 0;

		inline BasicVarValue &operator=(const BasicVarValue &x) {
			((MemberValue &)*this) = (MemberValue &)x;
			return *this;
		}
		BasicVarValue &operator=(BasicVarValue &&) = delete;
	};

	class VarValue final : public MemberValue {
	public:
		mutable slake::Value* value = nullptr;
		Type type = TypeId::Any;

		VarValue(Runtime *rt, AccessModifier access, Type type);
		virtual ~VarValue();

		virtual inline Type getType() const override { return TypeId::Var; }
		inline Type getVarType() const { return type; }

		virtual Value *duplicate() const override;

		inline Value *getData() const { return value; }
		inline void setData(Value *value) {
			type.loadDeferredType(_rt);

			if (value && !isCompatible(type, value->getType()))
				throw MismatchedTypeError("Mismatched types");
			this->value = value;
		}

		inline VarValue &operator=(const VarValue &x) {
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
