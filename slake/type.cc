#include "runtime.h"

#include <cassert>

using namespace slake;

Type::Type(IdRefObject *ref) : typeId(TypeId::Instance) {
	exData.ptr = (Object *)ref;
}

Type Type::makeArrayTypeName(Runtime *runtime, const Type &elementType) {
	assert(elementType.typeId != TypeId::Array);
	assert(elementType.typeId != TypeId::Ref);

	Type type;

	type.typeId = TypeId::Array;
	type.exData.ptr = TypeDefObject::alloc(runtime, elementType).release();

	return type;
}

Type Type::makeRefTypeName(Runtime *runtime, const Type &elementType) {
	assert(elementType.typeId != TypeId::Ref);

	Type type;

	type.typeId = TypeId::Ref;
	type.exData.ptr = TypeDefObject::alloc(runtime, elementType).release();

	return type;
}

Type Type::duplicate() const {
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

Type &Type::getArrayExData() const { return ((TypeDefObject *)exData.ptr)->type; }
Type &Type::getRefExData() const { return ((TypeDefObject *)exData.ptr)->type; }

bool Type::isLoadingDeferred() const noexcept {
	switch (typeId) {
		case TypeId::Instance:
			return getCustomTypeExData()->getKind() == ObjectKind::IdRef;
		default:
			return false;
	}
}

void Type::loadDeferredType(const Runtime *rt) {
	if (!isLoadingDeferred())
		return;

	auto ref = (IdRefObject *)getCustomTypeExData();
	auto typeObject = rt->resolveIdRef(ref);
	if (!typeObject)
		throw NotFoundError("Object referenced by the type was not found", ref);

	exData.ptr = (Object *)typeObject;
}

bool slake::isCompatible(const Type &type, const Value &value) {
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
			if (value.getObjectRef().objectPtr->getKind() != ObjectKind::String)
				return false;
			break;
		case TypeId::Instance: {
			if (value.valueType != ValueType::ObjectRef)
				return false;

			auto objectPtr = value.getObjectRef().objectPtr;
			if (!objectPtr)
				return true;
			if (objectPtr->getKind() != ObjectKind::Instance)
				return false;

			const_cast<Type &>(type).loadDeferredType(objectPtr->_rt);
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
				default:;
			}

			break;
		}
		case TypeId::Array: {
			if (value.valueType != ValueType::ObjectRef)
				return false;

			auto objectPtr = value.getObjectRef().objectPtr;
			if (objectPtr->getKind() != ObjectKind::Array)
				return false;

			auto arrayObjectPtr = ((ArrayObject *)objectPtr);

			if (arrayObjectPtr->type.getArrayExData() != type.getArrayExData())
				return false;
			break;
		}
		case TypeId::Ref: {
			if (value.valueType != ValueType::ObjectRef)
				return false;

			auto objectPtr = value.getObjectRef().objectPtr;
			if (objectPtr->getKind() != ObjectKind::Var)
				return false;

			auto varObjectPtr = ((BasicVarObject *)objectPtr);

			if (varObjectPtr->type != type.getRefExData())
				return false;
			break;
		}
		default:
			return false;
	}

	return true;
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
