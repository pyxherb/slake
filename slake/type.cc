#include "runtime.h"

#include <cassert>

using namespace slake;

Type::Type(RefValue *ref) : valueType(ValueType::OBJECT) {
	exData = ValueRef<>((Value *)ref);
}

Type::~Type() {
	switch(valueType) {
		case ValueType::ARRAY:{
			delete std::get<Type*>(exData);
		}
		case ValueType::MAP: {
			auto pair = std::get<std::pair<Type*, Type*>>(exData);
			delete pair.first;
			delete pair.second;
		}
	}
}

bool Type::isLoadingDeferred() const noexcept {
	switch (valueType) {
		case ValueType::CLASS:
		case ValueType::INTERFACE:
		case ValueType::TRAIT:
		case ValueType::STRUCT:
		case ValueType::OBJECT:
		case ValueType::STRUCTOBJ:
			return getCustomTypeExData()->getType() == ValueType::REF;
		default:
			return false;
	}
}

void Type::loadDeferredType(const Runtime *rt) const {
	if (!isLoadingDeferred())
		return;

	auto ref = (RefValue *)*getCustomTypeExData();
	auto typeValue = rt->resolveRef(ref);
	if (!typeValue)
		throw NotFoundError("Value referenced by the type was not found", ref);

	exData = ValueRef<>((Value *)typeValue);
}

/// @brief Check if a type can be converted into another type.
/// @param src Source type.
/// @param dest Target type.
/// @return true if convertible, false otherwise.
bool slake::isConvertible(Type src, Type dest) {
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
			ClassValue *srcType = (ClassValue *)*src.getCustomTypeExData();
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
					switch (dest.getCustomTypeExData()->getType().valueType) {
						case ValueType::CLASS: {
							auto destType = (ClassValue *)*dest.getCustomTypeExData();
							return srcType->getMember("operator@" + srcType->getRuntime()->resolveName(destType)) ? true : false;
						}
						case ValueType::INTERFACE: {
							auto destType = (InterfaceValue *)*dest.getCustomTypeExData();
							if (srcType->hasImplemented(destType))
								return true;
							return false;
						}
						case ValueType::TRAIT: {
							auto destType = (TraitValue *)*dest.getCustomTypeExData();
							if (srcType->isCompatibleWith(destType))
								return true;
							return false;
						}
						case ValueType::STRUCT: {
							auto destType = (StructValue *)*dest.getCustomTypeExData();
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
				dest.getCustomTypeExData() != src.getCustomTypeExData())
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
bool slake::isCompatible(Type a, Type b) {
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
			switch (a.getCustomTypeExData()->getType().valueType) {
				case ValueType::CLASS: {
					switch (b.valueType) {
						case ValueType::OBJECT:
							for (auto i = ((ClassValue *)*b.getCustomTypeExData()); i; i = (ClassValue *)i->getParent()) {
								if (i == *b.getCustomTypeExData())
									return true;
							}
							return false;
						case ValueType::NONE:
							return true;
						default:
							return false;
					}
				}
				case ValueType::INTERFACE: {
					switch (b.valueType) {
						case ValueType::OBJECT:
							return ((ClassValue *)*b.getCustomTypeExData())->hasImplemented((InterfaceValue *)*a.getCustomTypeExData());
						case ValueType::NONE:
							return true;
						default:
							return false;
					}
				}
				case ValueType::TRAIT: {
					switch (b.valueType) {
						case ValueType::OBJECT:
							return ((ClassValue *)*b.getCustomTypeExData())->isCompatibleWith((TraitValue *)*a.getCustomTypeExData());
						case ValueType::NONE:
							return true;
						default:
							return false;
					}
				}
				default:
					return false;
			}
		}
		case ValueType::STRUCTOBJ: {
			switch (b.valueType) {
				case ValueType::STRUCTOBJ:
					return a.getCustomTypeExData() == b.getCustomTypeExData();
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

std::string std::to_string(slake::Type &&type, slake::Runtime *rt) {
	switch (type.valueType) {
		case ValueType::I8:
			return "i8";
		case ValueType::I16:
			return "i16";
		case ValueType::I32:
			return "i32";
		case ValueType::I64:
			return "i64";
		case ValueType::U8:
			return "u8";
		case ValueType::U16:
			return "u16";
		case ValueType::U32:
			return "u32";
		case ValueType::U64:
			return "u64";
		case ValueType::F32:
			return "f32";
		case ValueType::F64:
			return "f64";
		case ValueType::STRING:
			return "string";
		case ValueType::BOOL:
			return "bool";
		case ValueType::ARRAY:
			return to_string(type.getArrayExData(), rt) + "[]";
		case ValueType::OBJECT: {
			if (type.isLoadingDeferred())
				return std::to_string((RefValue *)*type.getCustomTypeExData());
			return rt->resolveName((MemberValue *)*type.getCustomTypeExData());
		}
		case ValueType::ANY:
			return "any";
		case ValueType::NONE:
			return "void";
		default:
			return "any";
	}
}
