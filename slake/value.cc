#include <slake/runtime.h>

using namespace slake;

SLAKE_API bool Value::operator==(const Value &rhs) const {
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
			if (data.asI8 < rhs.data.asI8)
				return -1;
			if (data.asI8 > rhs.data.asI8)
				return 1;
			break;
		case ValueType::I16:
			if (data.asI16 < rhs.data.asI16)
				return -1;
			if (data.asI16 > rhs.data.asI16)
				return 1;
			break;
		case ValueType::I32:
			if (data.asI32 < rhs.data.asI32)
				return -1;
			if (data.asI32 > rhs.data.asI32)
				return 1;
			break;
		case ValueType::I64:
			if (data.asI64 < rhs.data.asI64)
				return -1;
			if (data.asI64 > rhs.data.asI64)
				return 1;
			break;
		case ValueType::ISize:
			if (data.asISize < rhs.data.asISize)
				return -1;
			if (data.asISize > rhs.data.asISize)
				return 1;
			break;
		case ValueType::U8:
			if (data.asU8 < rhs.data.asU8)
				return -1;
			if (data.asU8 > rhs.data.asU8)
				return 1;
			break;
		case ValueType::U16:
			if (data.asU16 < rhs.data.asU16)
				return -1;
			if (data.asU16 > rhs.data.asU16)
				return 1;
			break;
		case ValueType::U32:
			if (data.asU32 < rhs.data.asU32)
				return -1;
			if (data.asU32 > rhs.data.asU32)
				return 1;
			break;
		case ValueType::U64:
			if (data.asU64 < rhs.data.asU64)
				return -1;
			if (data.asU64 > rhs.data.asU64)
				return 1;
			break;
		case ValueType::USize:
			if (data.asUSize < rhs.data.asUSize)
				return -1;
			if (data.asUSize > rhs.data.asUSize)
				return 1;
			break;
		case ValueType::Bool:
			if (data.asBool < rhs.data.asBool)
				return -1;
			if (data.asBool > rhs.data.asBool)
				return 1;
			break;
		case ValueType::Reference:
			if (data.asReference < rhs.data.asReference)
				return -1;
			if (data.asReference > rhs.data.asReference)
				return 1;
			break;
		case ValueType::RegIndex:
			if (data.asU32 < rhs.data.asU32)
				return -1;
			if (data.asU32 > rhs.data.asU32)
				return 1;
			break;
		case ValueType::TypeName:
			if (int result = data.asType.comparesTo(rhs.data.asType); result)
				return result;
			break;
		case ValueType::Undefined:
			std::terminate();
		default:;
	}

	return 0;
}

SLAKE_API bool Value::operator<(const Value &rhs) const {
	return comparesTo(rhs) < 0;
}

SLAKE_API bool Value::operator>(const Value &rhs) const {
	return comparesTo(rhs) > 0;
}
