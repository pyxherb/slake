#ifndef _SLAKE_VALDEF_VAR_H_
#define _SLAKE_VALDEF_VAR_H_

#include "member.h"
#include <slake/except.h>
#include <slake/type.h>

namespace slake {
	class BasicVarValue : public MemberValue {
	public:
		BasicVarValue(Runtime *rt, AccessModifier access);
		virtual ~BasicVarValue();

		virtual inline Type getType() const override { return TypeId::Var; }

		virtual Type getVarType() const = 0;

		virtual Value *getData() const = 0;
		virtual void setData(Value *value) = 0;

		inline BasicVarValue &operator=(const BasicVarValue &x) {
			((MemberValue &)*this) = (MemberValue &)x;
			return *this;
		}
		BasicVarValue &operator=(BasicVarValue &&) = delete;
	};

	class VarValue final : public BasicVarValue {
	public:
		mutable slake::Value *value = nullptr;
		Type type = TypeId::Any;

		VarValue(Runtime *rt, AccessModifier access, Type type);
		virtual ~VarValue();

		inline Type getVarType() const override { return type; }

		virtual Value *duplicate() const override;

		virtual inline Value *getData() const override { return value; }
		virtual inline void setData(Value *value) override {
			type.loadDeferredType(_rt);

			if (value && !isCompatible(type, value->getType()))
				throw MismatchedTypeError("Mismatched types");
			this->value = value;
		}

		inline VarValue &operator=(const VarValue &x) {
			((MemberValue &)*this) = (MemberValue &)x;
			// TODO: Do we actually need to duplicate value of the variable? If so, how do we treat object values (they should not be duplicated)?
			//
			// if (x.value)
			//	value = x.value->duplicate();
			value = x.value;
			type = x.type;
			return *this;
		}
		VarValue &operator=(VarValue &&) = delete;
	};
}

#endif
