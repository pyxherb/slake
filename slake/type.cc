#include "runtime.h"

#include <cassert>

using namespace Slake;

Type::Type(RefValue *ref) : valueType(ValueType::OBJECT) {
	exData.deferred = ref;
}

Type::~Type() {
	switch (valueType) {
		case ValueType::ARRAY:
			if (exData.array)
				delete exData.array;
			break;
		case ValueType::OBJECT:
			if (exData.deferred)
				break;
			break;
		case ValueType::MAP:
			if (exData.map.k)
				delete exData.map.k;
			if (exData.map.v)
				delete exData.map.v;
			break;
	}
}


bool Type::isLoadingDeferred() noexcept {
	switch (valueType) {
		case ValueType::CLASS:
		case ValueType::INTERFACE:
		case ValueType::TRAIT:
		case ValueType::STRUCT:
			return ((Value *)exData.customType)->getType() == ValueType::REF;
		default:
			return false;
	}
}

/// @brief Check if a type can be converted into another type.
/// @param src Source type.
/// @param dest Target type.
/// @return true if convertible, false otherwise.
bool Slake::isConvertible(Type src, Type dest) {
	switch (src.valueType) {
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
		case ValueType::BOOL: {
			switch (dest.valueType) {
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
				case ValueType::BOOL:
					return true;
				default:
					return false;
			}
		}
		case ValueType::OBJECT: {
			ClassValue *srcType = (ClassValue *)src.exData.customType;
			switch (dest.valueType) {
				case ValueType::I8:
					return srcType->getMember("operator@i8") ? true : false;
				case ValueType::I16:
					return srcType->getMember("operator@i16") ? true : false;
				case ValueType::I32:
					return srcType->getMember("operator@i32") ? true : false;
				case ValueType::I64:
					return srcType->getMember("operator@i64") ? true : false;
				case ValueType::U8:
					return srcType->getMember("operator@u8") ? true : false;
				case ValueType::U16:
					return srcType->getMember("operator@u16") ? true : false;
				case ValueType::U32:
					return srcType->getMember("operator@u32") ? true : false;
				case ValueType::U64:
					return srcType->getMember("operator@u64") ? true : false;
				case ValueType::F32:
					return srcType->getMember("operator@f32") ? true : false;
				case ValueType::F64:
					return srcType->getMember("operator@f64") ? true : false;
				case ValueType::BOOL:
					return srcType->getMember("operator@bool") ? true : false;
				case ValueType::OBJECT: {
					switch (dest.exData.customType->getType().valueType) {
						case ValueType::CLASS: {
							auto destType = (ClassValue *)dest.exData.customType;
							return srcType->getMember("operator@" + srcType->getRuntime()->resolveName(destType)) ? true : false;
						}
						case ValueType::INTERFACE: {
							auto destType = (InterfaceValue *)dest.exData.customType;
							if (srcType->hasImplemented(destType))
								return true;
							return false;
						}
						case ValueType::TRAIT: {
							auto destType = (TraitValue *)dest.exData.customType;
							if (srcType->isCompatibleWith(destType))
								return true;
							return false;
						}
						case ValueType::STRUCT: {
							auto destType = (StructValue *)dest.exData.customType;
							return srcType->getMember("operator@" + srcType->getRuntime()->resolveName(destType)) ? true : false;
						}
						default:
							return false;
					}
				}
				default:
					return false;
			}
		}
		case ValueType::STRUCTOBJ:
			if (dest.valueType != ValueType::STRUCT ||
				dest.exData.customType != src.exData.customType)
				return false;
			return true;
		case ValueType::ANY:
		case ValueType::NONE:
			return true;
		default:
			return false;
	}
}

/// @brief
/// @param a Type of the variable
/// @param b Type of the value
/// @return
bool Slake::isCompatible(Type a, Type b) {
	switch (a.valueType) {
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
		case ValueType::BOOL:
		case ValueType::STRING:
			return a.valueType == b.valueType;
		case ValueType::OBJECT: {
			switch (b.valueType) {
				case ValueType::OBJECT:
					for (auto i = ((ClassValue*)b.exData.customType); i; i = (ClassValue*)i->getParent()) {
						if (i == b.exData.customType)
							return true;
					}
					return false;
				case ValueType::NONE:
					return true;
				default:
					return false;
			}
		}
		case ValueType::STRUCTOBJ: {
			switch (b.valueType) {
				case ValueType::STRUCTOBJ:
					return a.exData.customType == b.exData.customType;
				case ValueType::NONE:
					return true;
				default:
					return false;
			}
		}
		case ValueType::FN: {
			switch (b.valueType) {
				case ValueType::FN: {
					// stub
					return false;
				}
				case ValueType::NONE:
					return true;
				default:
					return false;
			}
		}
		case ValueType::ANY:
			return true;
		default:
			return false;
	}
}
