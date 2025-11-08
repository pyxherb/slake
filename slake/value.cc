#include <slake/runtime.h>

using namespace slake;

SLAKE_API Value::Value(const Value& other) noexcept: valueType(other.valueType) {
	switch (other.valueType) {
		case ValueType::I8:
			data.asI8 = other.data.asI8;
			break;
		case ValueType::I16:
			data.asI16 = other.data.asI16;
			break;
		case ValueType::I32:
			data.asI32 = other.data.asI32;
			break;
		case ValueType::I64:
			data.asI64 = other.data.asI64;
			break;
		case ValueType::ISize:
			data.asISize = other.data.asISize;
			break;
		case ValueType::U8:
			data.asU8 = other.data.asU8;
			break;
		case ValueType::U16:
			data.asU16 = other.data.asU16;
			break;
		case ValueType::U32:
			data.asU32 = other.data.asU32;
			break;
		case ValueType::U64:
			data.asU64 = other.data.asU64;
			break;
		case ValueType::USize:
			data.asUSize = other.data.asUSize;
			break;
		case ValueType::F32:
			data.asF32 = other.data.asF32;
			break;
		case ValueType::F64:
			data.asF64 = other.data.asF64;
			break;
		case ValueType::Bool:
			data.asBool = other.data.asBool;
			break;
		case ValueType::Reference:
			data.asReference = other.data.asReference;
			break;
		case ValueType::RegIndex:
			data.asU32 = other.data.asU32;
			break;
		case ValueType::TypeName:
			data.asType = other.data.asType;
			break;
		case ValueType::Label:
			data.asU32 = other.data.asU32;
			break;
		default:
			break;
	}
}

SLAKE_API bool Value::operator==(const Value &rhs) const {
	if (valueType != rhs.valueType)
		return false;

	switch (valueType) {
		case ValueType::Invalid:
			break;
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
		case ValueType::Reference: {
			return data.asReference == data.asReference;
		}
		case ValueType::RegIndex:
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
		case ValueType::Reference:
			return data.asReference < rhs.data.asReference;
		case ValueType::RegIndex:
			return data.asU32 < rhs.data.asU32;
		case ValueType::TypeName:
			return data.asType < rhs.data.asType;
		case ValueType::Undefined:
			return false;
		default:;
	}
	return false;
}
