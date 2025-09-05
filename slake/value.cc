#include <slake/runtime.h>

using namespace slake;

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
		case ValueType::ISize:
			return data.asISize == rhs.data.asISize;
		case ValueType::U8:
			return data.asU8 == rhs.data.asU8;
		case ValueType::U16:
			return data.asU16 == rhs.data.asU16;
		case ValueType::U32:
			return data.asU32 == rhs.data.asU32;
		case ValueType::U64:
			return data.asU64 == rhs.data.asU64;
		case ValueType::USize:
			return data.asUSize == rhs.data.asUSize;
		case ValueType::Bool:
			return data.asBool == rhs.data.asBool;
		case ValueType::EntityRef: {
			return data.asObjectRef == data.asObjectRef;
		}
		case ValueType::RegRef:
			return data.asU32 == rhs.data.asU32;
		case ValueType::TypeName:
			return data.asType == rhs.data.asType;
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
		case ValueType::ISize:
			return data.asISize < rhs.data.asISize;
		case ValueType::U8:
			return data.asU8 < rhs.data.asU8;
		case ValueType::U16:
			return data.asU16 < rhs.data.asU16;
		case ValueType::U32:
			return data.asU32 < rhs.data.asU32;
		case ValueType::U64:
			return data.asU64 < rhs.data.asU64;
		case ValueType::USize:
			return data.asUSize < rhs.data.asUSize;
		case ValueType::Bool:
			return data.asBool < rhs.data.asBool;
		case ValueType::EntityRef:
			return data.asObjectRef < rhs.data.asObjectRef;
		case ValueType::RegRef:
			return data.asU32 < rhs.data.asU32;
		case ValueType::TypeName:
			return data.asType < rhs.data.asType;
		case ValueType::Undefined:
			return false;
		default:;
	}
	return false;
}
