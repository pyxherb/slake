#include "runtime.h"

#include <cassert>

using namespace slake;

Type::Type(IdRefValue *ref) : typeId(TypeId::Object) {
	exData = (Value *)ref;
}

Type::~Type() {
	switch (typeId) {
		case TypeId::Array:
		case TypeId::Ref:
		case TypeId::Var: {
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
		case TypeId::Var:
			if (a.typeId != b.typeId)
				return false;
			return isCompatible(a.getVarExData(), b.getVarExData());
		case TypeId::Ref:
			if (b.typeId != TypeId::Var)
				return false;
			return isCompatible(a.getRefExData(), b.getVarExData());
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
		case TypeId::Ref:
			return to_string(type.getArrayExData(), rt) + "&";
		case TypeId::Object: {
			if (type.isLoadingDeferred()) {
				return std::to_string((IdRefValue *)type.getCustomTypeExData());
			} else {
				return rt->getFullName((MemberValue *)type.getCustomTypeExData());
			}
		}
		case TypeId::GenericArg:
			return "!" + type.getGenericArgExData();
		case TypeId::Any:
			return "any";
		case TypeId::None:
			return "void";
		default:
			return "<Unknown Type>";
	}
}
