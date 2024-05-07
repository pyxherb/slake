#include "runtime.h"

#include <cassert>

using namespace slake;

Type::Type(IdRefValue *ref, TypeFlags flags) : typeId(TypeId::Object), flags(flags) {
	exData = (Value *)ref;
}

Type::~Type() {
	switch (typeId) {
		case TypeId::Array: {
			if (auto t = std::get<Type *>(exData); t)
				delete t;
			break;
		}
	}
}

bool Type::isLoadingDeferred() const noexcept {
	switch (typeId) {
		case TypeId::Class:
		case TypeId::Interface:
		case TypeId::Trait:
		case TypeId::Object:
			return getCustomTypeExData()->getType() == TypeId::IdRef;
		default:
			return false;
	}
}

void Type::loadDeferredType(const Runtime *rt) const {
	if (!isLoadingDeferred())
		return;

	auto ref = (IdRefValue *)getCustomTypeExData();
	auto typeValue = rt->resolveIdRef(ref);
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
		case TypeId::Bool: {
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
				case TypeId::Bool:
					return true;
				default:
					return false;
			}
		}
		case TypeId::Object: {
			ClassValue *srcType = (ClassValue *)src.getCustomTypeExData();
			switch (dest.typeId) {
				case TypeId::I8:
					return srcType->getMember("operator@i8") ? true : false;
				case TypeId::I16:
					return srcType->getMember("operator@i16") ? true : false;
				case TypeId::I32:
					return srcType->getMember("operator@i32") ? true : false;
				case TypeId::I64:
					return srcType->getMember("operator@i64") ? true : false;
				case TypeId::U8:
					return srcType->getMember("operator@u8") ? true : false;
				case TypeId::U16:
					return srcType->getMember("operator@u16") ? true : false;
				case TypeId::U32:
					return srcType->getMember("operator@u32") ? true : false;
				case TypeId::U64:
					return srcType->getMember("operator@u64") ? true : false;
				case TypeId::F32:
					return srcType->getMember("operator@f32") ? true : false;
				case TypeId::F64:
					return srcType->getMember("operator@f64") ? true : false;
				case TypeId::Bool:
					return srcType->getMember("operator@bool") ? true : false;
				case TypeId::Object: {
					switch (dest.getCustomTypeExData()->getType().typeId) {
						case TypeId::Class: {
							auto destType = (ClassValue *)dest.getCustomTypeExData();
							return srcType->getMember("operator@" + srcType->getRuntime()->getFullName(destType)) ? true : false;
						}
						case TypeId::Interface: {
							auto destType = (InterfaceValue *)dest.getCustomTypeExData();
							if (srcType->hasImplemented(destType))
								return true;
							return false;
						}
						case TypeId::Trait: {
							auto destType = (TraitValue *)dest.getCustomTypeExData();
							if (srcType->hasTrait(destType))
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
		case TypeId::Any:
		case TypeId::None:
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
		case TypeId::Bool:
		case TypeId::String:
			return a.typeId == b.typeId;
		case TypeId::Array:
			if (a.typeId != b.typeId)
				return false;
			return isCompatible(a.getArrayExData(), b.getArrayExData());
		case TypeId::Object: {
			switch (a.getCustomTypeExData()->getType().typeId) {
				case TypeId::Class: {
					switch (b.typeId) {
						case TypeId::Object:
							for (auto i = ((ClassValue *)b.getCustomTypeExData()); i; i = (ClassValue *)i->getParent()) {
								if (i == b.getCustomTypeExData())
									return true;
							}
							return false;
						case TypeId::None:
							return true;
						default:
							return false;
					}
				}
				case TypeId::Interface: {
					switch (b.typeId) {
						case TypeId::Object:
							return ((ClassValue *)b.getCustomTypeExData())->hasImplemented((InterfaceValue *)a.getCustomTypeExData());
						case TypeId::None:
							return true;
						default:
							return false;
					}
				}
				case TypeId::Trait: {
					switch (b.typeId) {
						case TypeId::Object:
							return ((ClassValue *)b.getCustomTypeExData())->hasTrait((TraitValue *)a.getCustomTypeExData());
						case TypeId::None:
							return true;
						default:
							return false;
					}
				}
				default:
					return false;
			}
		}
		case TypeId::Fn: {
			switch (b.typeId) {
				case TypeId::Fn: {
					// stub
					return false;
				}
				case TypeId::None:
					return true;
				default:
					return false;
			}
		}
		case TypeId::Any:
			return true;
		default:
			return false;
	}
}

std::string std::to_string(const slake::Type &type, const slake::Runtime *rt) {
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
		case TypeId::String:
			return "string";
		case TypeId::Bool:
			return "bool";
		case TypeId::Array:
			return to_string(type.getArrayExData(), rt) + "[]";
		case TypeId::Object: {
			string s = "@";

			if (type.isLoadingDeferred()) {
				return "@" + std::to_string((IdRefValue *)type.getCustomTypeExData());
			} else {
				return "@" + rt->getFullName((MemberValue *)type.getCustomTypeExData());
			}
		}
		case TypeId::GenericArg:
			return "@!" + type.getGenericArgExData();
		case TypeId::Any:
			return "any";
		case TypeId::None:
			return "void";
		default:
			return "<Unknown Type>";
	}
}
