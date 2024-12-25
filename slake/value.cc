#include <slake/runtime.h>

using namespace slake;

SLAKE_API bool VarRef::operator<(const VarRef &rhs) const {
	if (varPtr < rhs.varPtr)
		return true;
	if (varPtr > rhs.varPtr)
		return false;

	if (!varPtr)
		return false;

	switch (varPtr->varKind) {
		case VarKind::ArrayElementAccessor:
			if (context.asArray.index < rhs.context.asArray.index)
				return true;
			break;
		case VarKind::Regular:
			break;
	}

	return false;
}

SLAKE_API Value::Value(const Type &type) {
	*((Type *)data.asType) = type;
	valueType = ValueType::TypeName;
}

SLAKE_API Type &Value::getTypeName() {
	return *((Type *)data.asType);
}

SLAKE_API const Type &Value::getTypeName() const {
	return ((Value *)this)->getTypeName();
}

SLAKE_API bool Value::operator==(const Value &rhs) const {
	if (valueType != rhs.valueType)
		return false;

	switch (valueType) {
		case ValueType::I8:
			return data.asI8 == rhs.data.asI8;
		case ValueType::I16:
			return data.asI16 == rhs.data.asI16;
		case ValueType::I32:
			return data.asI32 == rhs.data.asI32;
		case ValueType::I64:
			return data.asI64 == rhs.data.asI64;
		case ValueType::U8:
			return data.asU8 == rhs.data.asU8;
		case ValueType::U16:
			return data.asU16 == rhs.data.asU16;
		case ValueType::U32:
			return data.asU32 == rhs.data.asU32;
		case ValueType::U64:
			return data.asU64 == rhs.data.asU64;
		case ValueType::Bool:
			return data.asBool == rhs.data.asBool;
		case ValueType::ObjectRef: {
			return data.asObjectRef == data.asObjectRef;
		}
		case ValueType::RegRef:
			return data.asU32 == rhs.data.asU32;
		case ValueType::TypeName:
			return getTypeName() == rhs.getTypeName();
		case ValueType::Undefined:
			return true;
		default:;
	}
	return true;
}

SLAKE_API bool Value::operator<(const Value &rhs) const {
	if (valueType < rhs.valueType)
		return true;
	if (valueType > rhs.valueType)
		return false;

	switch (valueType) {
		case ValueType::I8:
			return data.asI8 < rhs.data.asI8;
		case ValueType::I16:
			return data.asI16 < rhs.data.asI16;
		case ValueType::I32:
			return data.asI32 < rhs.data.asI32;
		case ValueType::I64:
			return data.asI64 < rhs.data.asI64;
		case ValueType::U8:
			return data.asU8 < rhs.data.asU8;
		case ValueType::U16:
			return data.asU16 < rhs.data.asU16;
		case ValueType::U32:
			return data.asU32 < rhs.data.asU32;
		case ValueType::U64:
			return data.asU64 < rhs.data.asU64;
		case ValueType::Bool:
			return data.asBool < rhs.data.asBool;
		case ValueType::ObjectRef:
			return data.asObjectRef < rhs.data.asObjectRef;
		case ValueType::RegRef:
			return data.asU32 < rhs.data.asU32;
		case ValueType::TypeName:
			return getTypeName() < rhs.getTypeName();
		case ValueType::VarRef:
			return data.asVarRef < rhs.data.asVarRef;
		case ValueType::Undefined:
			return false;
		default:;
	}
	return false;
}
