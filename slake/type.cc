#include "runtime.h"

#include <cassert>

using namespace slake;

SLAKE_API Type::Type(IdRefObject *ref) : typeId(TypeId::Instance) {
	exData.ptr = (Object *)ref;
}

SLAKE_API Type Type::makeArrayTypeName(Runtime *runtime, const Type &elementType) {
	Type type;

	type.typeId = TypeId::Array;
	type.exData.ptr = TypeDefObject::alloc(runtime, elementType).get();

	return type;
}

SLAKE_API Type Type::makeRefTypeName(Runtime *runtime, const Type &elementType) {
	Type type;

	type.typeId = TypeId::Ref;
	type.exData.ptr = TypeDefObject::alloc(runtime, elementType).get();

	return type;
}

SLAKE_API Type Type::duplicate() const {
	Type newType(*this);

	switch (typeId) {
		case TypeId::Array:
		case TypeId::Ref:
			newType.exData.ptr = newType.exData.ptr->duplicate();
			break;
		default:;
	}

	return newType;
}

SLAKE_API Type &Type::getArrayExData() const { return ((TypeDefObject *)exData.ptr)->type; }
SLAKE_API Type &Type::getRefExData() const { return ((TypeDefObject *)exData.ptr)->type; }

SLAKE_API bool Type::isLoadingDeferred() const noexcept {
	switch (typeId) {
		case TypeId::Instance:
			return getCustomTypeExData()->getKind() == ObjectKind::IdRef;
		default:
			return false;
	}
}

SLAKE_API InternalExceptionPointer Type::loadDeferredType(Runtime *rt) {
	if (!isLoadingDeferred())
		return {};

	auto ref = (IdRefObject *)getCustomTypeExData();
	SLAKE_RETURN_IF_EXCEPT(rt->resolveIdRef(ref, nullptr, exData.ptr));

	return {};
}

SLAKE_API bool Type::operator<(const Type &rhs) const {
	if (typeId < rhs.typeId)
		return true;
	else if (typeId > rhs.typeId)
		return false;
	else {
		switch (rhs.typeId) {
			case TypeId::Instance: {
				auto lhsType = getCustomTypeExData(), rhsType = rhs.getCustomTypeExData();

				assert(lhsType->getKind() == rhsType->getKind());
				switch (lhsType->getKind()) {
					case ObjectKind::IdRef: {
						IdRefObject *lhsRef = (IdRefObject *)lhsType,
									*rhsRef = (IdRefObject *)rhsType;

						if (lhsRef->entries.size() < rhsRef->entries.size())
							return true;
						if (lhsRef->entries.size() > rhsRef->entries.size())
							return false;

						GenericArgListComparator genericArgListComparator;

						for (size_t i = 0; i < lhsRef->entries.size(); ++i) {
							auto &curLhsRefEntry = lhsRef->entries[i],
								 &curRhsRefEntry = rhsRef->entries[i];

							if (curLhsRefEntry.name < curRhsRefEntry.name)
								return true;
							if (curLhsRefEntry.name > curRhsRefEntry.name)
								return false;

							if (genericArgListComparator(
									curLhsRefEntry.genericArgs,
									curRhsRefEntry.genericArgs))
								return true;
						}

						return false;
					}
				}

				return lhsType < rhsType;
			}
			case TypeId::Array:
				return getArrayExData() < rhs.getArrayExData();
			case TypeId::Ref:
				return getRefExData() < rhs.getRefExData();
		}
	}

	return false;
}

SLAKE_API bool Type::operator==(const Type &rhs) const {
	if (rhs.typeId != typeId)
		return false;

	switch (rhs.typeId) {
		case TypeId::Value:
			return getValueTypeExData() == rhs.getValueTypeExData();
		case TypeId::Instance: {
			auto lhsType = getCustomTypeExData(), rhsType = rhs.getCustomTypeExData();

			assert(lhsType->getKind() == rhsType->getKind());
			switch (lhsType->getKind()) {
				case ObjectKind::IdRef: {
					IdRefObject *lhsRef = (IdRefObject *)lhsType,
								*rhsRef = (IdRefObject *)rhsType;

					if (lhsRef->entries.size() != rhsRef->entries.size())
						return false;

					GenericArgListEqComparator genericArgListComparator;

					for (size_t i = 0; i < lhsRef->entries.size(); ++i) {
						auto &curLhsRefEntry = lhsRef->entries[i],
							 &curRhsRefEntry = rhsRef->entries[i];

						if (curLhsRefEntry.name != curRhsRefEntry.name)
							return false;

						if (!genericArgListComparator(
								curLhsRefEntry.genericArgs,
								curRhsRefEntry.genericArgs))
							return false;
					}

					return false;
				}
			}

			return lhsType == rhsType;
		}
		case TypeId::GenericArg: {
			auto lhsOwnerObject = exData.genericArg.ownerObject,
				 rhsOwnerObject = rhs.exData.genericArg.ownerObject;

			auto lhsObjectKind = lhsOwnerObject->getKind();

			auto lhsName = exData.genericArg.nameObject,
				 rhsName = rhs.exData.genericArg.nameObject;

			Object *lhsOwnerOut, *rhsOwnerOut;

			GenericParam *lhsGenericParam = getGenericParam(lhsOwnerObject, lhsName->data, &lhsOwnerOut),
						 *rhsGenericParam = getGenericParam(rhsOwnerObject, rhsName->data, &rhsOwnerOut);

			if ((!lhsGenericParam) ||
				(!rhsGenericParam))
				return false;

			switch (lhsOwnerOut->getKind()) {
				case ObjectKind::Class:
				case ObjectKind::Interface:
					return lhsGenericParam == rhsGenericParam;
				case ObjectKind::FnOverloading: {
					auto l = (FnOverloadingObject *)lhsOwnerOut;

					switch (rhsOwnerOut->getKind()) {
						case ObjectKind::Class:
						case ObjectKind::Interface:
							return false;
						case ObjectKind::FnOverloading: {
							auto r = (FnOverloadingObject *)rhsOwnerOut;
							return getGenericParamIndex(l->genericParams, lhsName->data) == getGenericParamIndex(r->genericParams, rhsName->data);
						}
					}

					break;
				}
			}

			return false;
		}
		case TypeId::Array:
			return getArrayExData() == rhs.getArrayExData();
		case TypeId::Ref:
			return getRefExData() == rhs.getRefExData();
	}
	return true;
}

SLAKE_API bool slake::isCompatible(const Type &type, const Value &value) {
	if (type.typeId == TypeId::Any)
		return true;

	switch (type.typeId) {
		case TypeId::Value: {
			if (type.exData.valueType != value.valueType)
				return false;
			break;
		}
		case TypeId::String:
			if (value.valueType != ValueType::ObjectRef)
				return false;
			if (value.getObjectRef()->getKind() != ObjectKind::String)
				return false;
			break;
		case TypeId::Instance: {
			if (value.valueType != ValueType::ObjectRef)
				return false;

			auto objectPtr = value.getObjectRef();
			if (!objectPtr)
				return true;
			if (objectPtr->getKind() != ObjectKind::Instance)
				return false;

			if (auto e = const_cast<Type &>(type).loadDeferredType(objectPtr->associatedRuntime);
				e) {
				e.reset();
				return false;
			}
			switch (type.getCustomTypeExData()->getKind()) {
				case ObjectKind::Class: {
					ClassObject *thisClass = (ClassObject *)type.getCustomTypeExData();

					ClassObject *valueClass = ((InstanceObject *)objectPtr)->_class;

					if (!thisClass->isBaseOf(valueClass))
						return false;
					break;
				}
				case ObjectKind::Interface: {
					InterfaceObject *thisInterface = (InterfaceObject *)type.getCustomTypeExData();

					ClassObject *valueClass = ((InstanceObject *)objectPtr)->_class;

					if (!valueClass->hasImplemented(thisInterface))
						return false;
					break;
				}
				default:
					break;
			}

			break;
		}
		case TypeId::Array: {
			if (value.valueType != ValueType::ObjectRef)
				return false;

			auto objectPtr = value.getObjectRef();
			if (objectPtr->getKind() != ObjectKind::Array)
				return false;

			auto arrayObjectPtr = ((ArrayObject *)objectPtr);

			if (arrayObjectPtr->elementType != type.getArrayExData())
				return false;
			break;
		}
		case TypeId::Ref: {
			if (value.valueType != ValueType::VarRef)
				return false;

			auto varRef = value.getVarRef();

			if (varRef.varPtr->getVarType(varRef.context) != type.getRefExData())
				return false;
			break;
		}
		default:
			return false;
	}

	return true;
}

SLAKE_API std::string std::to_string(const slake::Type &type, const slake::Runtime *rt) {
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
			StringObject *nameObject = (StringObject *)type.exData.ptr;
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
