#ifndef _SLAKE_VALDEF_VAR_H_
#define _SLAKE_VALDEF_VAR_H_

#include "member.h"
#include <slake/except.h>
#include <slake/type.h>

namespace slake {
	class BasicVarObject : public MemberObject {
	public:
		BasicVarObject(Runtime *rt, AccessModifier access, Type type);
		inline BasicVarObject(const BasicVarObject &x) : MemberObject(x) {
			type = x.type;
		}
		virtual ~BasicVarObject();

		Type type = TypeId::Any;

		virtual inline ObjectKind getKind() const override { return ObjectKind::Var; }

		virtual Type getVarType() const { return type; }

		virtual Value getData() const = 0;
		virtual void setData(const Value &value) = 0;
	};

	class VarObject final : public BasicVarObject {
	public:
		Value value;

		VarObject(Runtime *rt, AccessModifier access, const Type &type);
		inline VarObject(const VarObject &other) : BasicVarObject(other) {
			value = other.value;
		}
		virtual ~VarObject();

		virtual Object *duplicate() const override;

		static HostObjectRef<VarObject> alloc(Runtime *rt, AccessModifier access, const Type &type);
		static HostObjectRef<VarObject> alloc(const VarObject *other);
		virtual void dealloc() override;

		virtual inline Value getData() const override { return value; }
		virtual inline void setData(const Value &value) override {
			if (!isCompatible(type, value))
				throw MismatchedTypeError("Mismatched variable type");
			this->value = value;
		}
	};
}

#endif
