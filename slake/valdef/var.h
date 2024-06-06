#ifndef _SLAKE_VALDEF_VAR_H_
#define _SLAKE_VALDEF_VAR_H_

#include "member.h"
#include <slake/except.h>
#include <slake/type.h>

namespace slake {
	class BasicVarObject : public MemberObject {
	public:
		Type type = TypeId::Any;

		BasicVarObject(Runtime *rt, AccessModifier access, Type type);
		virtual ~BasicVarObject();

		virtual inline Type getType() const override { return Type(TypeId::Var, type); }

		virtual Type getVarType() const { return type; }

		virtual Value getData() const = 0;
		virtual void setData(const Value &value) = 0;

		inline BasicVarObject &operator=(const BasicVarObject &x) {
			((MemberObject &)*this) = (MemberObject &)x;
			type = x.type;
			return *this;
		}
		BasicVarObject &operator=(BasicVarObject &&) = delete;
	};

	class VarObject final : public BasicVarObject {
	public:
		mutable Value value;

		VarObject(Runtime *rt, AccessModifier access, Type type);
		virtual ~VarObject();

		virtual Object *duplicate() const override;

		virtual inline Value getData() const override { return value; }
		virtual inline void setData(const Value &value) override {
			switch (value.valueType) {
				case ValueType::I8:
				case ValueType::I16:
				case ValueType::I32:
				case ValueType::I64:
				case ValueType::U8:
				case ValueType::U16:
				case ValueType::U32:
				case ValueType::U64:
				case ValueType::F32:
				case ValueType::F64:
				case ValueType::Bool:
				case ValueType::String:
				case ValueType::Undefined:
					if (!isCompatible(type, value.valueType))
						throw MismatchedTypeError("Mismatched variable type");
					break;
				case ValueType::ObjectRef:
					if (auto p = value.getObjectRef().objectPtr; p) {
						type.loadDeferredType(_rt);

						if (!isCompatible(type, value.getObjectRef().objectPtr->getType()))
							throw MismatchedTypeError("Mismatched variable type");
					}
					break;
				default:
					throw MismatchedTypeError("Mismatched variable type");
			}

			this->value = value;
		}

		inline VarObject &operator=(const VarObject &x) {
			((MemberObject &)*this) = (MemberObject &)x;
			// TODO: Do we actually need to duplicate value of the variable? If so, how do we treat object values (they should not be duplicated)?
			//
			// if (x.value)
			//	value = x.value->duplicate();
			value = x.value;
			return *this;
		}
		VarObject &operator=(VarObject &&) = delete;
	};
}

#endif
