#include "../runtime.h"

using namespace slake;

SLAKE_API InternalExceptionPointer Runtime::tryAccessVar(const EntityRef &entityRef) const {
	switch (entityRef.kind) {
	case ObjectRefKind::FieldRef: {
		FieldRecord &fieldRecord = entityRef.asField.moduleObject->fieldRecords.at(entityRef.asField.index);

		break;
	}
	case ObjectRefKind::LocalVarRef: {
		break;
	}
	case ObjectRefKind::InstanceFieldRef: {
		const InstanceObject *v = (const InstanceObject *)entityRef.asArray.arrayObject;

		break;
	}
	case ObjectRefKind::ArrayElementRef: {
		const ArrayObject *v = (const ArrayObject *)entityRef.asArray.arrayObject;

		if (entityRef.asArray.index > v->length) {
			return raiseInvalidArrayIndexError(v->associatedRuntime, entityRef.asArray.index);
		}

		break;
	}
	case ObjectRefKind::ArgRef: {
		ArgRecord &argRecord = entityRef.asArg.majorFrame->argStack.at(entityRef.asArg.argIndex);
		break;
	}
	default:
		assert(false);
	}

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::typeofVar(const EntityRef &entityRef, Type &typeOut) const {
	switch (entityRef.kind) {
	case ObjectRefKind::FieldRef: {
		FieldRecord &fieldRecord = entityRef.asField.moduleObject->fieldRecords.at(entityRef.asField.index);

		typeOut = fieldRecord.type;
		break;
	}
	case ObjectRefKind::LocalVarRef: {
		typeOut = entityRef.asLocalVar.type;
		break;
	}
	case ObjectRefKind::InstanceFieldRef: {
		const InstanceObject *v = (const InstanceObject *)entityRef.asArray.arrayObject;

		typeOut = v->_class->cachedObjectLayout->fieldRecords.at(entityRef.asArray.index).type;
		break;
	}
	case ObjectRefKind::ArrayElementRef: {
		const ArrayObject *v = (const ArrayObject *)entityRef.asArray.arrayObject;

		if (entityRef.asArray.index > v->length) {
			return raiseInvalidArrayIndexError(v->associatedRuntime, entityRef.asArray.index);
		}

		typeOut = v->elementType;
		break;
	}
	case ObjectRefKind::ArgRef: {
		ArgRecord &argRecord = entityRef.asArg.majorFrame->argStack.at(entityRef.asArg.argIndex);

		typeOut = argRecord.type;
		break;
	}
	default:
		assert(false);
	}

	return {};
}

#undef new

SLAKE_API InternalExceptionPointer Runtime::readVar(const EntityRef &entityRef, Value &valueOut) const {
	SLAKE_RETURN_IF_EXCEPT(tryAccessVar(entityRef));

	new (&valueOut) Value(readVarUnsafe(entityRef));
	return {};
}

SLAKE_API Value Runtime::readVarUnsafe(const EntityRef &entityRef) const {
	switch (entityRef.kind) {
	case ObjectRefKind::FieldRef: {
		FieldRecord &fieldRecord = entityRef.asField.moduleObject->fieldRecords.at(entityRef.asField.index);

		const char *const rawDataPtr = entityRef.asField.moduleObject->localFieldStorage + fieldRecord.offset;

		switch (fieldRecord.type.typeId) {
		case TypeId::I8:
			return Value(*((int8_t *)rawDataPtr));
		case TypeId::I16:
			return Value(*((int16_t *)rawDataPtr));
		case TypeId::I32:
			return Value(*((int32_t *)rawDataPtr));
		case TypeId::I64:
			return Value(*((int64_t *)rawDataPtr));
		case TypeId::U8:
			return Value(*((uint8_t *)rawDataPtr));
		case TypeId::U16:
			return Value(*((uint16_t *)rawDataPtr));
		case TypeId::U32:
			return Value(*((uint32_t *)rawDataPtr));
		case TypeId::U64:
			return Value(*((uint64_t *)rawDataPtr));
		case TypeId::F32:
			return Value(*((float *)rawDataPtr));
		case TypeId::F64:
			return Value(*((double *)rawDataPtr));
		case TypeId::Bool:
			return Value(*((bool *)rawDataPtr));
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			return Value(EntityRef::makeObjectRef(*((Object **)rawDataPtr)));
			break;
		default:
			// All fields should be checked during the instantiation.
			throw std::logic_error("Unhandled value type");
		}

		break;
	}
	case ObjectRefKind::LocalVarRef: {
		const char *const rawDataPtr = entityRef.asLocalVar.context->dataStack + SLAKE_STACK_MAX - entityRef.asLocalVar.stackOff;

		switch (entityRef.asLocalVar.type.typeId) {
		case TypeId::I8:
			return Value(*((int8_t *)rawDataPtr));
		case TypeId::I16:
			return Value(*((int16_t *)rawDataPtr));
		case TypeId::I32:
			return Value(*((int32_t *)rawDataPtr));
		case TypeId::I64:
			return Value(*((int64_t *)rawDataPtr));
		case TypeId::U8:
			return Value(*((uint8_t *)rawDataPtr));
		case TypeId::U16:
			return Value(*((uint16_t *)rawDataPtr));
		case TypeId::U32:
			return Value(*((uint32_t *)rawDataPtr));
		case TypeId::U64:
			return Value(*((uint64_t *)rawDataPtr));
		case TypeId::F32:
			return Value(*((float *)rawDataPtr));
		case TypeId::F64:
			return Value(*((double *)rawDataPtr));
		case TypeId::Bool:
			return Value(*((bool *)rawDataPtr));
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			return Value(EntityRef::makeObjectRef(*((Object **)rawDataPtr)));
			break;
		default:
			// All fields should be checked during the instantiation.
			throw std::logic_error("Unhandled value type");
		}

		break;
	}
	case ObjectRefKind::InstanceFieldRef: {
		ObjectFieldRecord &fieldRecord =
			entityRef.asObjectField.instanceObject->_class->cachedObjectLayout->fieldRecords.at(
				entityRef.asObjectField.fieldIndex);

		const char *const rawFieldPtr = entityRef.asObjectField.instanceObject->rawFieldData + fieldRecord.offset;

		switch (fieldRecord.type.typeId) {
		case TypeId::I8:
			return Value(*((int8_t *)rawFieldPtr));
		case TypeId::I16:
			return Value(*((int16_t *)rawFieldPtr));
		case TypeId::I32:
			return Value(*((int32_t *)rawFieldPtr));
		case TypeId::I64:
			return Value(*((int64_t *)rawFieldPtr));
		case TypeId::U8:
			return Value(*((uint8_t *)rawFieldPtr));
		case TypeId::U16:
			return Value(*((uint16_t *)rawFieldPtr));
		case TypeId::U32:
			return Value(*((uint32_t *)rawFieldPtr));
		case TypeId::U64:
			return Value(*((uint64_t *)rawFieldPtr));
		case TypeId::F32:
			return Value(*((float *)rawFieldPtr));
		case TypeId::F64:
			return Value(*((double *)rawFieldPtr));
		case TypeId::Bool:
			return Value(*((bool *)rawFieldPtr));
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			return Value(EntityRef::makeObjectRef(*((Object **)rawFieldPtr)));
		default:
			// All fields should be checked during the instantiation.
			throw std::logic_error("Unhandled value type");
		}
		break;
	}
	case ObjectRefKind::ArrayElementRef: {
		assert(entityRef.asArray.index < entityRef.asArray.arrayObject->length);

		switch (entityRef.asArray.arrayObject->elementType.typeId) {
		case TypeId::I8:
			return Value(((int8_t *)entityRef.asArray.arrayObject->data)[entityRef.asArray.index]);
		case TypeId::I16:
			return Value(((int16_t *)entityRef.asArray.arrayObject->data)[entityRef.asArray.index]);
		case TypeId::I32:
			return Value(((int32_t *)entityRef.asArray.arrayObject->data)[entityRef.asArray.index]);
		case TypeId::I64:
			return Value(((int64_t *)entityRef.asArray.arrayObject->data)[entityRef.asArray.index]);
		case TypeId::U8:
			return Value(((uint8_t *)entityRef.asArray.arrayObject->data)[entityRef.asArray.index]);
		case TypeId::U16:
			return Value(((uint16_t *)entityRef.asArray.arrayObject->data)[entityRef.asArray.index]);
		case TypeId::U32:
			return Value(((uint32_t *)entityRef.asArray.arrayObject->data)[entityRef.asArray.index]);
		case TypeId::U64:
			return Value(((uint64_t *)entityRef.asArray.arrayObject->data)[entityRef.asArray.index]);
		case TypeId::F32:
			return Value(((float *)entityRef.asArray.arrayObject->data)[entityRef.asArray.index]);
		case TypeId::F64:
			return Value(((double *)entityRef.asArray.arrayObject->data)[entityRef.asArray.index]);
		case TypeId::Bool:
			return Value(((bool *)entityRef.asArray.arrayObject->data)[entityRef.asArray.index]);
		case TypeId::Instance: {
			return Value(EntityRef::makeObjectRef(((Object **)entityRef.asArray.arrayObject->data)[entityRef.asArray.index]));
		}
		case TypeId::Any: {
			return Value(((Value *)entityRef.asArray.arrayObject->data)[entityRef.asArray.index]);
		}
		}
		break;
	}
	case ObjectRefKind::ArgRef: {
		const ArgRecord &argRecord = entityRef.asArg.majorFrame->argStack.at(entityRef.asArg.argIndex);

		return argRecord.value;
	}
	default:;
	}
	std::terminate();
}

SLAKE_API InternalExceptionPointer Runtime::writeVar(const EntityRef &entityRef, const Value &value) const {
	switch (entityRef.kind) {
	case ObjectRefKind::FieldRef: {
		if (entityRef.asField.index >= entityRef.asField.moduleObject->fieldRecords.size())
			// TODO: Use a proper type of exception instead of this.
			return raiseInvalidArrayIndexError(entityRef.asField.moduleObject->associatedRuntime, entityRef.asArray.index);

		const FieldRecord &fieldRecord =
			entityRef.asField.moduleObject->fieldRecords.at(entityRef.asField.index);

		if (!isCompatible(fieldRecord.type, value)) {
			return raiseMismatchedVarTypeError(entityRef.asField.moduleObject->associatedRuntime);
		}

		char *const rawDataPtr = entityRef.asField.moduleObject->localFieldStorage + fieldRecord.offset;

		switch (fieldRecord.type.typeId) {
		case TypeId::I8:
			*((int8_t *)rawDataPtr) = value.getI8();
			break;
		case TypeId::I16:
			*((int16_t *)rawDataPtr) = value.getI16();
			break;
		case TypeId::I32:
			*((int32_t *)rawDataPtr) = value.getI32();
			break;
		case TypeId::I64:
			*((int64_t *)rawDataPtr) = value.getI64();
			break;
		case TypeId::U8:
			*((uint8_t *)rawDataPtr) = value.getU8();
			break;
		case TypeId::U16:
			*((uint16_t *)rawDataPtr) = value.getU16();
			break;
		case TypeId::U32:
			*((uint32_t *)rawDataPtr) = value.getU32();
			break;
		case TypeId::U64:
			*((uint64_t *)rawDataPtr) = value.getU64();
			break;
		case TypeId::F32:
			*((float *)rawDataPtr) = value.getF32();
			break;
		case TypeId::F64:
			*((double *)rawDataPtr) = value.getF64();
			break;
		case TypeId::Bool:
			*((bool *)rawDataPtr) = value.getBool();
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			*((Object **)rawDataPtr) = value.getEntityRef().asObject.instanceObject;
			break;
		default:
			// All fields should be checked during the instantiation.
			throw std::logic_error("Unhandled value type");
		}

		break;
	}
	case ObjectRefKind::LocalVarRef: {
		if (entityRef.asLocalVar.stackOff >= SLAKE_STACK_MAX)
			// TODO: Use a proper type of exception instead of this.
			return raiseInvalidArrayIndexError((Runtime *)this, entityRef.asArray.index);

		if (!isCompatible(entityRef.asLocalVar.type, value)) {
			return raiseMismatchedVarTypeError((Runtime *)this);
		}

		char *const rawDataPtr = entityRef.asLocalVar.context->dataStack + SLAKE_STACK_MAX - entityRef.asLocalVar.stackOff;

		switch (entityRef.asLocalVar.type.typeId) {
		case TypeId::I8:
			*((int8_t *)rawDataPtr) = value.getI8();
			break;
		case TypeId::I16:
			*((int16_t *)rawDataPtr) = value.getI16();
			break;
		case TypeId::I32:
			*((int32_t *)rawDataPtr) = value.getI32();
			break;
		case TypeId::I64:
			*((int64_t *)rawDataPtr) = value.getI64();
			break;
		case TypeId::U8:
			*((uint8_t *)rawDataPtr) = value.getU8();
			break;
		case TypeId::U16:
			*((uint16_t *)rawDataPtr) = value.getU16();
			break;
		case TypeId::U32:
			*((uint32_t *)rawDataPtr) = value.getU32();
			break;
		case TypeId::U64:
			*((uint64_t *)rawDataPtr) = value.getU64();
			break;
		case TypeId::F32:
			*((float *)rawDataPtr) = value.getF32();
			break;
		case TypeId::F64:
			*((double *)rawDataPtr) = value.getF64();
			break;
		case TypeId::Bool:
			*((bool *)rawDataPtr) = value.getBool();
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			*((Object **)rawDataPtr) = value.getEntityRef().asObject.instanceObject;
			break;
		default:
			// All fields should be checked during the instantiation.
			throw std::logic_error("Unhandled value type");
		}

		break;
	}
	case ObjectRefKind::InstanceFieldRef: {
		ObjectFieldRecord &fieldRecord =
			entityRef.asObjectField.instanceObject->_class->cachedObjectLayout->fieldRecords.at(
				entityRef.asObjectField.fieldIndex);

		if (!isCompatible(fieldRecord.type, value)) {
			return raiseMismatchedVarTypeError((Runtime *)this);
		}

		char *const rawFieldPtr = entityRef.asObjectField.instanceObject->rawFieldData + fieldRecord.offset;

		switch (fieldRecord.type.typeId) {
		case TypeId::I8:
			*((int8_t *)rawFieldPtr) = value.getI8();
			break;
		case TypeId::I16:
			*((int16_t *)rawFieldPtr) = value.getI16();
			break;
		case TypeId::I32:
			*((int32_t *)rawFieldPtr) = value.getI32();
			break;
		case TypeId::I64:
			*((int64_t *)rawFieldPtr) = value.getI64();
			break;
		case TypeId::U8:
			*((uint8_t *)rawFieldPtr) = value.getU8();
			break;
		case TypeId::U16:
			*((uint16_t *)rawFieldPtr) = value.getU16();
			break;
		case TypeId::U32:
			*((uint32_t *)rawFieldPtr) = value.getU32();
			break;
		case TypeId::U64:
			*((uint64_t *)rawFieldPtr) = value.getU64();
			break;
		case TypeId::F32:
			*((float *)rawFieldPtr) = value.getF32();
			break;
		case TypeId::F64:
			*((double *)rawFieldPtr) = value.getF64();
			break;
		case TypeId::Bool:
			*((bool *)rawFieldPtr) = value.getBool();
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			*((Object **)rawFieldPtr) = value.getEntityRef().asObject.instanceObject;
			break;
		default:
			// All fields should be checked during the instantiation.
			throw std::logic_error("Unhandled value type");
		}
		break;
	}
	case ObjectRefKind::ArrayElementRef: {
		if (entityRef.asArray.index > entityRef.asArray.arrayObject->length) {
			return raiseInvalidArrayIndexError((Runtime *)this, entityRef.asArray.index);
		}

		if (!isCompatible(entityRef.asArray.arrayObject->elementType, value)) {
			return raiseMismatchedVarTypeError((Runtime *)this);
		}

		switch (entityRef.asArray.arrayObject->elementType.typeId) {
		case TypeId::I8:
			((int8_t *)entityRef.asArray.arrayObject->data)[entityRef.asArray.index] = value.getI8();
			break;
		case TypeId::I16:
			((int16_t *)entityRef.asArray.arrayObject->data)[entityRef.asArray.index] = value.getI16();
			break;
		case TypeId::I32:
			((int32_t *)entityRef.asArray.arrayObject->data)[entityRef.asArray.index] = value.getI32();
			break;
		case TypeId::I64:
			((int64_t *)entityRef.asArray.arrayObject->data)[entityRef.asArray.index] = value.getI64();
			break;
		case TypeId::U8:
			((uint8_t *)entityRef.asArray.arrayObject->data)[entityRef.asArray.index] = value.getU8();
			break;
		case TypeId::U16:
			((uint16_t *)entityRef.asArray.arrayObject->data)[entityRef.asArray.index] = value.getU16();
			break;
		case TypeId::U32:
			((uint32_t *)entityRef.asArray.arrayObject->data)[entityRef.asArray.index] = value.getU32();
			break;
		case TypeId::U64:
			((uint64_t *)entityRef.asArray.arrayObject->data)[entityRef.asArray.index] = value.getU64();
			break;
		case TypeId::F32:
			((float *)entityRef.asArray.arrayObject->data)[entityRef.asArray.index] = value.getF32();
			break;
		case TypeId::F64:
			((double *)entityRef.asArray.arrayObject->data)[entityRef.asArray.index] = value.getF64();
			break;
		case TypeId::Bool:
			((bool *)entityRef.asArray.arrayObject->data)[entityRef.asArray.index] = value.getBool();
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array: {
			((Object **)entityRef.asArray.arrayObject->data)[entityRef.asArray.index] = value.getEntityRef().asObject.instanceObject;
			break;
		}
		}
		break;
	}
	case ObjectRefKind::ArgRef: {
		ArgRecord &argRecord = entityRef.asArg.majorFrame->argStack.at(entityRef.asArg.argIndex);

		if (!isCompatible(argRecord.type, value)) {
			return raiseMismatchedVarTypeError((Runtime *)this);
		}

		argRecord.value = value;
		break;
	}
	default:
		assert(false);
	}

	return {};
}
