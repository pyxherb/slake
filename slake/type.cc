#include "runtime.h"

#include <cassert>

using namespace slake;

SLAKE_API TypeId slake::valueTypeToTypeId(ValueType valueType) {
	switch (valueType) {
		case ValueType::I8:
			return TypeId::I8;
		case ValueType::I16:
			return TypeId::I16;
		case ValueType::I32:
			return TypeId::I32;
		case ValueType::I64:
			return TypeId::I64;
		case ValueType::U8:
			return TypeId::U8;
		case ValueType::U16:
			return TypeId::U16;
		case ValueType::U32:
			return TypeId::U32;
		case ValueType::U64:
			return TypeId::U64;
		case ValueType::F32:
			return TypeId::F32;
		case ValueType::F64:
			return TypeId::F64;
		case ValueType::Bool:
			return TypeId::Bool;
		default:;
	}
	std::terminate();
}

SLAKE_API bool slake::isValueTypeCompatibleTypeId(TypeId typeId) {
	switch (typeId) {
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
		default:;
	}
	return false;
}

SLAKE_API ValueType slake::typeIdToValueType(TypeId typeId) {
	switch (typeId) {
		case TypeId::I8:
			return ValueType::I8;
		case TypeId::I16:
			return ValueType::I16;
		case TypeId::I32:
			return ValueType::I32;
		case TypeId::I64:
			return ValueType::I64;
		case TypeId::U8:
			return ValueType::U8;
		case TypeId::U16:
			return ValueType::U16;
		case TypeId::U32:
			return ValueType::U32;
		case TypeId::U64:
			return ValueType::U64;
		case TypeId::F32:
			return ValueType::F32;
		case TypeId::F64:
			return ValueType::F64;
		case TypeId::Bool:
			return ValueType::Bool;
		default:;
	}
	std::terminate();
}

SLAKE_API Reference slake::extractStructInnerRef(const StructRefData &structRef, ReferenceKind innerReferenceKind) {
	switch (innerReferenceKind) {
		case ReferenceKind::StaticFieldRef:
			return Reference::makeStaticFieldRef(
				structRef.innerReference.asStaticField.moduleObject,
				structRef.innerReference.asStaticField.index);
			break;
		case ReferenceKind::ArrayElementRef:
			return Reference::makeArrayElementRef(
				structRef.innerReference.asArrayElement.arrayObject,
				structRef.innerReference.asArrayElement.index);
			break;
		case ReferenceKind::InstanceFieldRef:
			return Reference::makeInstanceFieldRef(
				structRef.innerReference.asObjectField.instanceObject,
				structRef.innerReference.asObjectField.fieldIndex);
			break;
		case ReferenceKind::LocalVarRef:
			return Reference::makeLocalVarRef(
				structRef.innerReference.asLocalVar.context,
				structRef.innerReference.asLocalVar.stackOff);
			break;
		case ReferenceKind::CoroutineLocalVarRef:
			return Reference::makeCoroutineLocalVarRef(
				structRef.innerReference.asCoroutineLocalVar.coroutine,
				structRef.innerReference.asCoroutineLocalVar.stackOff);
		case ReferenceKind::ArgRef:
			return Reference::makeArgRef(
				structRef.innerReference.asArg.majorFrame,
				structRef.innerReference.asArg.argIndex);
			break;
		case ReferenceKind::CoroutineArgRef:
			return Reference::makeCoroutineLocalVarRef(
				structRef.innerReference.asCoroutineArg.coroutine,
				structRef.innerReference.asCoroutineArg.argIndex);
			break;
		default:
			std::terminate();
	}
}

SLAKE_API bool Reference::operator==(const Reference &rhs) const {
	if (kind != rhs.kind)
		return false;
	switch (kind) {
		case ReferenceKind::Invalid:
			break;
		case ReferenceKind::StaticFieldRef:
			if (asStaticField.moduleObject != rhs.asStaticField.moduleObject)
				return false;
			return asStaticField.index == rhs.asStaticField.index;
		case ReferenceKind::ArrayElementRef:
			if (asArrayElement.arrayObject != rhs.asArrayElement.arrayObject)
				return false;
			return asStaticField.index == rhs.asStaticField.index;
		case ReferenceKind::ObjectRef:
			return asObject == rhs.asObject;
		case ReferenceKind::InstanceFieldRef:
			if (asObjectField.instanceObject != rhs.asObjectField.instanceObject)
				return false;
			return asObjectField.fieldIndex == rhs.asObjectField.fieldIndex;
		case ReferenceKind::LocalVarRef:
			if (asLocalVar.context != rhs.asLocalVar.context)
				return false;
			return asLocalVar.stackOff == rhs.asLocalVar.stackOff;
		case ReferenceKind::ArgRef:
			if (asArg.majorFrame != rhs.asArg.majorFrame)
				return false;
			return asArg.argIndex == rhs.asArg.argIndex;
		default:
			break;
	}
	std::terminate();
}

SLAKE_API bool Reference::operator<(const Reference &rhs) const {
	if (kind < rhs.kind)
		return true;
	if (kind > rhs.kind)
		return false;
	switch (kind) {
		case ReferenceKind::Invalid:
			break;
		case ReferenceKind::StaticFieldRef:
			if (asStaticField.moduleObject < rhs.asStaticField.moduleObject)
				return true;
			if (asStaticField.moduleObject > rhs.asStaticField.moduleObject)
				return false;
			return asStaticField.index < rhs.asStaticField.index;
		case ReferenceKind::ArrayElementRef:
			if (asArrayElement.arrayObject < rhs.asArrayElement.arrayObject)
				return true;
			if (asArrayElement.arrayObject > rhs.asArrayElement.arrayObject)
				return false;
			return asStaticField.index < rhs.asStaticField.index;
		case ReferenceKind::ObjectRef:
			return asObject < rhs.asObject;
		case ReferenceKind::InstanceFieldRef:
			if (asObjectField.instanceObject < rhs.asObjectField.instanceObject)
				return true;
			if (asObjectField.instanceObject > rhs.asObjectField.instanceObject)
				return false;
			return asObjectField.fieldIndex < rhs.asObjectField.fieldIndex;
		case ReferenceKind::LocalVarRef:
			if (asLocalVar.context < rhs.asLocalVar.context)
				return true;
			if (asLocalVar.context > rhs.asLocalVar.context)
				return false;
			return asLocalVar.stackOff < rhs.asLocalVar.stackOff;
		case ReferenceKind::ArgRef:
			if (asArg.majorFrame < rhs.asArg.majorFrame)
				return true;
			if (asArg.majorFrame > rhs.asArg.majorFrame)
				return false;
			return asArg.argIndex < rhs.asArg.argIndex;
		case ReferenceKind::AotPtrRef:
			return asAotPtr.ptr < rhs.asAotPtr.ptr;
		default:
			break;
	}
	std::terminate();
}

SLAKE_API bool Reference::operator>(const Reference &rhs) const {
	if (kind > rhs.kind)
		return true;
	if (kind < rhs.kind)
		return false;
	switch (kind) {
		case ReferenceKind::Invalid:
			break;
		case ReferenceKind::StaticFieldRef:
			if (asStaticField.moduleObject > rhs.asStaticField.moduleObject)
				return true;
			if (asStaticField.moduleObject < rhs.asStaticField.moduleObject)
				return false;
			return asStaticField.index > rhs.asStaticField.index;
		case ReferenceKind::ArrayElementRef:
			if (asArrayElement.arrayObject > rhs.asArrayElement.arrayObject)
				return true;
			if (asArrayElement.arrayObject < rhs.asArrayElement.arrayObject)
				return false;
			return asStaticField.index > rhs.asStaticField.index;
		case ReferenceKind::ObjectRef:
			return asObject > rhs.asObject;
		case ReferenceKind::InstanceFieldRef:
			if (asObjectField.instanceObject > rhs.asObjectField.instanceObject)
				return true;
			if (asObjectField.instanceObject < rhs.asObjectField.instanceObject)
				return false;
			return asObjectField.fieldIndex > rhs.asObjectField.fieldIndex;
		case ReferenceKind::LocalVarRef:
			if (asLocalVar.context > rhs.asLocalVar.context)
				return true;
			if (asLocalVar.context < rhs.asLocalVar.context)
				return false;
			return asLocalVar.stackOff > rhs.asLocalVar.stackOff;
		case ReferenceKind::ArgRef:
			if (asArg.majorFrame > rhs.asArg.majorFrame)
				return true;
			if (asArg.majorFrame < rhs.asArg.majorFrame)
				return false;
			return asArg.argIndex > rhs.asArg.argIndex;
		case ReferenceKind::AotPtrRef:
			return asAotPtr.ptr > rhs.asAotPtr.ptr;
		default:
			break;
	}
	std::terminate();
}

SLAKE_API int TypeRef::comparesTo(const TypeRef &rhs) const noexcept {
	if (typeId < rhs.typeId)
		return -1;
	if (typeId > rhs.typeId)
		return 1;
	if (typeModifier < rhs.typeModifier)
		return -1;
	if (typeModifier > rhs.typeModifier)
		return 1;
	if (isFundamentalType(typeId))
		return 0;
	if (typeDef < rhs.typeDef)
		return -1;
	if (typeDef > rhs.typeDef)
		return 1;
	return 0;
}

SLAKE_API TypeRef TypeRef::duplicate(bool &succeededOut) const {
	TypeRef newType(*this);

	switch (typeId) {
		case TypeId::Array:
		case TypeId::Ref:
		case TypeId::GenericArg:
			newType.typeDef = (TypeDefObject *)typeDef->duplicate(nullptr);
			if (!succeededOut) {
				return {};
			}
			break;
		default:;
	}

	succeededOut = true;

	return newType;
}

SLAKE_API bool slake::isCompatible(const TypeRef &type, const Value &value) {
	if (value.isLocal() && !type.isLocal())
		return false;

	switch (type.typeId) {
		case TypeId::I8:
		case TypeId::I16:
		case TypeId::I32:
		case TypeId::I64:
		case TypeId::ISize:
		case TypeId::U8:
		case TypeId::U16:
		case TypeId::U32:
		case TypeId::U64:
		case TypeId::USize:
		case TypeId::F32:
		case TypeId::F64:
		case TypeId::Bool:
			if (type.typeId != valueTypeToTypeId(value.valueType))
				return false;
			break;
		case TypeId::String: {
			if (value.valueType != ValueType::Reference)
				return false;
			const Reference &entityRef = value.getReference();
			if (entityRef.kind != ReferenceKind::ObjectRef)
				return false;
			if (!entityRef.asObject)
				return true;
			if (entityRef.asObject->getObjectKind() != ObjectKind::String)
				return false;
			break;
		}
		case TypeId::Instance: {
			if (value.valueType != ValueType::Reference)
				return false;

			const Reference &entityRef = value.getReference();
			if (entityRef.kind != ReferenceKind::ObjectRef)
				return false;
			Object *objectPtr = entityRef.asObject;

			if (!objectPtr)
				return true;

			Object *typeObject = type.getCustomTypeDef()->typeObject;
			switch (typeObject->getObjectKind()) {
				case ObjectKind::Class: {
					ClassObject *thisClass = (ClassObject *)typeObject;

					ClassObject *valueClass = ((InstanceObject *)objectPtr)->_class;

					if (type.isFinal()) {
						if (thisClass != valueClass)
							return false;
					} else {
						if (!thisClass->isBaseOf(valueClass))
							return false;
					}
					break;
				}
				case ObjectKind::Interface: {
					InterfaceObject *thisInterface = (InterfaceObject *)typeObject;

					ClassObject *valueClass = ((InstanceObject *)objectPtr)->_class;

					assert(!type.isFinal());
					if (!valueClass->hasImplemented(thisInterface))
						return false;
					break;
				}
				default:
					break;
			}

			break;
		}
		case TypeId::StructInstance:
			// Cannot pass structure instance as value.
			return false;
		case TypeId::GenericArg:
			return false;
		case TypeId::Array: {
			if (value.valueType != ValueType::Reference) {
				return false;
			}

			const Reference &entityRef = value.getReference();
			if (entityRef.kind != ReferenceKind::ObjectRef) {
				return false;
			}
			Object *objectPtr = entityRef.asObject;
			if (!objectPtr)
				return false;
			if (objectPtr->getObjectKind() != ObjectKind::Array) {
				return false;
			}

			auto arrayObjectPtr = ((ArrayObject *)objectPtr);

			if (arrayObjectPtr->elementType != (type.getArrayTypeDef()->elementType->typeRef)) {
				return false;
			}
			break;
		}
			/*
		case TypeId::Ref: {
			if (value.valueType != ValueType::Reference) {return true;
			}

			const Reference &entityRef = value.getReference();
			Runtime *rt;
			switch (entityRef.kind) {
				case ReferenceKind::FieldRef:
					rt = entityRef.asField.moduleObject->associatedRuntime;
					break;
				case ReferenceKind::InstanceFieldRef:
					rt = entityRef.asObjectField.instanceObject->associatedRuntime;
					break;
				case ReferenceKind::LocalVarRef:
					rt = entityRef.asLocalVar.context->runtime;
					break;
				case ReferenceKind::ArrayElementRef:
					rt = entityRef.asArray.arrayObject->associatedRuntime;
					break;
				case ReferenceKind::ArgRef:
					rt = entityRef.asArg.majorFrame->curFn->associatedRuntime;
					break;
				default:
					resultOut = false;
					return {};
			}
			TypeRef type;

			InternalExceptionPointer e = rt->typeofVar(entityRef, type);
			if (e) {
				resultOut = false;
				return e;
			}

			if (type != type.getRefTypeDef()->referencedType->typeRef) {return true;
			}
			break;
		}*/
		case TypeId::Any:
			return true;
		default:
			return false;
	}
	return true;
}

int TypeRefComparator::operator()(const TypeRef &lhs, const TypeRef &rhs) const noexcept {
	if (lhs.typeId < rhs.typeId)
		return -1;
	if (lhs.typeId > rhs.typeId)
		return 1;

	if (lhs.typeDef < rhs.typeDef)
		return -1;
	if (lhs.typeDef > rhs.typeDef)
		return 1;

	return 0;
}
