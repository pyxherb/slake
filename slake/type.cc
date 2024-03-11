#include "runtime.h"

#include <cassert>

using namespace slake;

Type::Type(RefValue *ref, TypeFlags flags) : typeId(TypeId::OBJECT), flags(flags) {
	exData = (Value*)ref;
}

Type::~Type() {
	switch (typeId) {
		case TypeId::ARRAY: {
			delete std::get<Type *>(exData);
		}
		case TypeId::MAP: {
			auto pair = std::get<std::pair<Type *, Type *>>(exData);
			delete pair.first;
			delete pair.second;
		}
	}
}

bool Type::isLoadingDeferred() const noexcept {
	switch (typeId) {
		case TypeId::CLASS:
		case TypeId::INTERFACE:
		case TypeId::TRAIT:
		case TypeId::OBJECT:
			return getCustomTypeExData()->getType() == TypeId::REF;
		default:
			return false;
	}
}

void Type::loadDeferredType(const Runtime *rt) const {
	if (!isLoadingDeferred())
		return;

	auto ref = (RefValue *)getCustomTypeExData();
	auto typeValue = rt->resolveRef(ref);
	if (!typeValue)
		throw NotFoundError("Value referenced by the type was not found", ref);

	exData = (Value *)typeValue;
}

/// @brief Check if a type can be converted into another type.
/// @param src Source type.
/// @param dest Target type.
/// @return true if convertible, false otherwise.
bool slake::isConvertible(Type src, Type dest) {
	switch (src.typeId) {
		case TypeId::I8:
		case TypeId::I16:
		case TypeId::I32:
		case TypeId::I64:
		case TypeId::U8:
		case TypeId::U16:
		case TypeId::U32:
		case TypeId::U64:
		case TypeId::F32:
		case TypeId::F64:
		case TypeId::BOOL: {
			switch (dest.typeId) {
				case TypeId::I8:
				case TypeId::I16:
				case TypeId::I32:
				case TypeId::I64:
				case TypeId::U8:
				case TypeId::U16:
				case TypeId::U32:
				case TypeId::U64:
				case TypeId::F32:
				case TypeId::F64:
				case TypeId::BOOL:
					return true;
				default:
					return false;
			}
		}
		case TypeId::OBJECT: {
			ClassValue *srcType = (ClassValue *)src.getCustomTypeExData();
			switch (dest.typeId) {
				case TypeId::I8:
					return memberOf(srcType, "operator@i8") ? true : false;
				case TypeId::I16:
					return memberOf(srcType, "operator@i16") ? true : false;
				case TypeId::I32:
					return memberOf(srcType, "operator@i32") ? true : false;
				case TypeId::I64:
					return memberOf(srcType, "operator@i64") ? true : false;
				case TypeId::U8:
					return memberOf(srcType, "operator@u8") ? true : false;
				case TypeId::U16:
					return memberOf(srcType, "operator@u16") ? true : false;
				case TypeId::U32:
					return memberOf(srcType, "operator@u32") ? true : false;
				case TypeId::U64:
					return memberOf(srcType, "operator@u64") ? true : false;
				case TypeId::F32:
					return memberOf(srcType, "operator@f32") ? true : false;
				case TypeId::F64:
					return memberOf(srcType, "operator@f64") ? true : false;
				case TypeId::BOOL:
					return memberOf(srcType, "operator@bool") ? true : false;
				case TypeId::OBJECT: {
					switch (dest.getCustomTypeExData()->getType().typeId) {
						case TypeId::CLASS: {
							auto destType = (ClassValue *)dest.getCustomTypeExData();
							return memberOf(srcType, "operator@" + srcType->getRuntime()->getFullName(destType)) ? true : false;
						}
						case TypeId::INTERFACE: {
							auto destType = (InterfaceValue *)dest.getCustomTypeExData();
							if (srcType->hasImplemented(destType))
								return true;
							return false;
						}
						case TypeId::TRAIT: {
							auto destType = (TraitValue *)dest.getCustomTypeExData();
							if (srcType->consistsOf(destType))
								return true;
							return false;
						}
						default:
							return false;
					}
				}
				default:
					return false;
			}
		}
		case TypeId::ANY:
		case TypeId::NONE:
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
	switch (a.typeId) {
		case TypeId::I8:
		case TypeId::I16:
		case TypeId::I32:
		case TypeId::I64:
		case TypeId::U8:
		case TypeId::U16:
		case TypeId::U32:
		case TypeId::U64:
		case TypeId::F32:
		case TypeId::F64:
		case TypeId::BOOL:
		case TypeId::STRING:
			return a.typeId == b.typeId;
		case TypeId::OBJECT: {
			switch (a.getCustomTypeExData()->getType().typeId) {
				case TypeId::CLASS: {
					switch (b.typeId) {
						case TypeId::OBJECT:
							for (auto i = ((ClassValue *)b.getCustomTypeExData()); i; i = (ClassValue *)i->getParent()) {
								if (i == b.getCustomTypeExData())
									return true;
							}
							return false;
						case TypeId::NONE:
							return true;
						default:
							return false;
					}
				}
				case TypeId::INTERFACE: {
					switch (b.typeId) {
						case TypeId::OBJECT:
							return ((ClassValue *)b.getCustomTypeExData())->hasImplemented((InterfaceValue *)a.getCustomTypeExData());
						case TypeId::NONE:
							return true;
						default:
							return false;
					}
				}
				case TypeId::TRAIT: {
					switch (b.typeId) {
						case TypeId::OBJECT:
							return ((ClassValue *)b.getCustomTypeExData())->consistsOf((TraitValue *)a.getCustomTypeExData());
						case TypeId::NONE:
							return true;
						default:
							return false;
					}
				}
				default:
					return false;
			}
		}
		case TypeId::FN: {
			switch (b.typeId) {
				case TypeId::FN: {
					// stub
					return false;
				}
				case TypeId::NONE:
					return true;
				default:
					return false;
			}
		}
		case TypeId::ANY:
			return true;
		default:
			return false;
	}
}

std::string std::to_string(const slake::Type &&type, const slake::Runtime *rt) {
	switch (type.typeId) {
		case TypeId::I8:
			return "i8";
		case TypeId::I16:
			return "i16";
		case TypeId::I32:
			return "i32";
		case TypeId::I64:
			return "i64";
		case TypeId::U8:
			return "u8";
		case TypeId::U16:
			return "u16";
		case TypeId::U32:
			return "u32";
		case TypeId::U64:
			return "u64";
		case TypeId::F32:
			return "f32";
		case TypeId::F64:
			return "f64";
		case TypeId::STRING:
			return "string";
		case TypeId::BOOL:
			return "bool";
		case TypeId::ARRAY:
			return to_string(type.getArrayExData(), rt) + "[]";
		case TypeId::OBJECT: {
			if (type.isLoadingDeferred())
				return "@" + std::to_string((RefValue *)type.getCustomTypeExData());
			return "@" + rt->getFullName((MemberValue *)type.getCustomTypeExData());
		}
		case TypeId::ANY:
			return "any";
		case TypeId::NONE:
			return "void";
		default:
			return "any";
	}
}
