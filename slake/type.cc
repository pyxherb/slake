#include "runtime.h"

#include <cassert>

using namespace slake;

Type::Type(IdRefObject *ref) : typeId(TypeId::Instance) {
	exData.basicExData.ptr = (Object *)ref;
}

bool Type::isLoadingDeferred() const noexcept {
	switch (typeId) {
		case TypeId::Class:
		case TypeId::Interface:
		case TypeId::Instance:
			return getCustomTypeExData()->getType() == TypeId::IdRef;
		default:
			return false;
	}
}

void Type::loadDeferredType(const Runtime *rt) const {
	if (!isLoadingDeferred())
		return;

	auto ref = (IdRefObject *)getCustomTypeExData();
	auto typeObject = rt->resolveIdRef(ref);
	if (!typeObject)
		throw NotFoundError("Object referenced by the type was not found", ref);

	exData.basicExData.ptr = (Object *)typeObject;
}

/// @brief
/// @param a Type of the variable
/// @param b Type of the value
/// @return
bool slake::isCompatible(Type a, Type b) {
	switch (a.typeId) {
		case TypeId::Value:
			if (a.typeId != b.typeId)
				return false;
			return a.getValueTypeExData() == b.getValueTypeExData();
		case TypeId::String:
			return a.typeId == b.typeId;
		case TypeId::Array:
			if (a.typeId != b.typeId)
				return false;
			return isCompatible(a.getArrayExData(), b.getArrayExData());
		case TypeId::Var:
			return a.typeId == b.typeId;
		case TypeId::Ref:
			return b.typeId == TypeId::Var;
		case TypeId::Instance: {
			switch (a.getCustomTypeExData()->getType().typeId) {
				case TypeId::Class: {
					switch (b.typeId) {
						case TypeId::Instance:
							for (auto i = ((ClassObject *)b.getCustomTypeExData()); i; i = (ClassObject *)i->getParent()) {
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
						case TypeId::Instance:
							return ((ClassObject *)b.getCustomTypeExData())->hasImplemented((InterfaceObject *)a.getCustomTypeExData());
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
		case TypeId::Value:
			switch (type.getValueTypeExData()) {
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
				case ValueType::Bool:
					return "bool";
				default:
					return "<Unknown value type>";
			}
		case TypeId::String:
			return "string";
		case TypeId::Array:
			return to_string(type.getArrayExData(), rt) + "[]";
		case TypeId::Ref:
			return to_string(type.getArrayExData(), rt) + "&";
		case TypeId::Instance: {
			if (type.isLoadingDeferred()) {
				return std::to_string((IdRefObject *)type.getCustomTypeExData());
			} else {
				return rt->getFullName((MemberObject *)type.getCustomTypeExData());
			}
		}
		case TypeId::GenericArg: {
			StringObject *nameObject = (StringObject *)type.exData.basicExData.ptr;
			return "!" + std::string(nameObject->data);
		}
		case TypeId::Any:
			return "any";
		case TypeId::None:
			return "void";
		default:
			return "<Unknown Type>";
	}
}
