#include <slake/runtime.h>

using namespace slake;

SLAKE_API bool Value::operator==(const Value &rhs) const noexcept {
	return !compares_to(rhs);
}

SLAKE_API int Value::compares_to(const Value& rhs) const noexcept {
	if (value_flags < rhs.value_flags)
		return -1;
	if (value_flags > rhs.value_flags)
		return 1;
	if (value_type < rhs.value_type)
		return -1;
	if (value_type > rhs.value_type)
		return 1;

	switch (value_type) {
		case ValueType::I8:
			if (as_i8 < rhs.as_i8)
				return -1;
			if (as_i8 > rhs.as_i8)
				return 1;
			break;
		case ValueType::I16:
			if (as_i16 < rhs.as_i16)
				return -1;
			if (as_i16 > rhs.as_i16)
				return 1;
			break;
		case ValueType::I32:
			if (as_i32 < rhs.as_i32)
				return -1;
			if (as_i32 > rhs.as_i32)
				return 1;
			break;
		case ValueType::I64:
			if (as_i64 < rhs.as_i64)
				return -1;
			if (as_i64 > rhs.as_i64)
				return 1;
			break;
		case ValueType::ISize:
			if (as_isize < rhs.as_isize)
				return -1;
			if (as_isize > rhs.as_isize)
				return 1;
			break;
		case ValueType::U8:
			if (as_u8 < rhs.as_u8)
				return -1;
			if (as_u8 > rhs.as_u8)
				return 1;
			break;
		case ValueType::U16:
			if (as_u16 < rhs.as_u16)
				return -1;
			if (as_u16 > rhs.as_u16)
				return 1;
			break;
		case ValueType::U32:
			if (as_u32 < rhs.as_u32)
				return -1;
			if (as_u32 > rhs.as_u32)
				return 1;
			break;
		case ValueType::U64:
			if (as_u64 < rhs.as_u64)
				return -1;
			if (as_u64 > rhs.as_u64)
				return 1;
			break;
		case ValueType::USize:
			if (as_usize < rhs.as_usize)
				return -1;
			if (as_usize > rhs.as_usize)
				return 1;
			break;
		case ValueType::Bool:
			if (as_bool < rhs.as_bool)
				return -1;
			if (as_bool > rhs.as_bool)
				return 1;
			break;
		case ValueType::Reference:
			if (as_reference < rhs.as_reference)
				return -1;
			if (as_reference > rhs.as_reference)
				return 1;
			break;
		case ValueType::RegIndex:
			if (as_u32 < rhs.as_u32)
				return -1;
			if (as_u32 > rhs.as_u32)
				return 1;
			break;
		case ValueType::TypeName:
			if (int result = as_type.compares_to(rhs.as_type); result)
				return result;
			break;
		case ValueType::Undefined:
			std::terminate();
		default:;
	}

	return 0;
}

SLAKE_API bool Value::operator<(const Value &rhs) const noexcept {
	return compares_to(rhs) < 0;
}

SLAKE_API bool Value::operator>(const Value &rhs) const noexcept {
	return compares_to(rhs) > 0;
}
