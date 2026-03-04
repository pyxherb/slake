#include <slake/runtime.h>

using namespace slake;

SLAKE_API bool Value::operator==(const Value &rhs) const noexcept {
	return !comparesTo(rhs);
}

SLAKE_API int Value::comparesTo(const Value& rhs) const noexcept {
	if (valueFlags < rhs.valueFlags)
		return -1;
	if (valueFlags > rhs.valueFlags)
		return 1;
	if (valueType < rhs.valueType)
		return -1;
	if (valueType > rhs.valueType)
		return 1;

	switch (valueType) {
		case ValueType::I8:
			if (asI8 < rhs.asI8)
				return -1;
			if (asI8 > rhs.asI8)
				return 1;
			break;
		case ValueType::I16:
			if (asI16 < rhs.asI16)
				return -1;
			if (asI16 > rhs.asI16)
				return 1;
			break;
		case ValueType::I32:
			if (asI32 < rhs.asI32)
				return -1;
			if (asI32 > rhs.asI32)
				return 1;
			break;
		case ValueType::I64:
			if (asI64 < rhs.asI64)
				return -1;
			if (asI64 > rhs.asI64)
				return 1;
			break;
		case ValueType::ISize:
			if (asISize < rhs.asISize)
				return -1;
			if (asISize > rhs.asISize)
				return 1;
			break;
		case ValueType::U8:
			if (asU8 < rhs.asU8)
				return -1;
			if (asU8 > rhs.asU8)
				return 1;
			break;
		case ValueType::U16:
			if (asU16 < rhs.asU16)
				return -1;
			if (asU16 > rhs.asU16)
				return 1;
			break;
		case ValueType::U32:
			if (asU32 < rhs.asU32)
				return -1;
			if (asU32 > rhs.asU32)
				return 1;
			break;
		case ValueType::U64:
			if (asU64 < rhs.asU64)
				return -1;
			if (asU64 > rhs.asU64)
				return 1;
			break;
		case ValueType::USize:
			if (asUSize < rhs.asUSize)
				return -1;
			if (asUSize > rhs.asUSize)
				return 1;
			break;
		case ValueType::Bool:
			if (asBool < rhs.asBool)
				return -1;
			if (asBool > rhs.asBool)
				return 1;
			break;
		case ValueType::Reference:
			if (asReference < rhs.asReference)
				return -1;
			if (asReference > rhs.asReference)
				return 1;
			break;
		case ValueType::RegIndex:
			if (asU32 < rhs.asU32)
				return -1;
			if (asU32 > rhs.asU32)
				return 1;
			break;
		case ValueType::TypeName:
			if (int result = asType.comparesTo(rhs.asType); result)
				return result;
			break;
		case ValueType::Undefined:
			std::terminate();
		default:;
	}

	return 0;
}

SLAKE_API bool Value::operator<(const Value &rhs) const noexcept {
	return comparesTo(rhs) < 0;
}

SLAKE_API bool Value::operator>(const Value &rhs) const noexcept {
	return comparesTo(rhs) > 0;
}
