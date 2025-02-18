#include "../runtime.h"

using namespace slake;

SLAKE_API InternalExceptionPointer Runtime::tryAccessVar(const ObjectRef &objectRef) const {
	switch (objectRef.kind) {
	case ObjectRefKind::FieldRef: {
		FieldRecord &fieldRecord = objectRef.asField.moduleObject->fieldRecords.at(objectRef.asField.index);

		break;
	}
	case ObjectRefKind::LocalVarRef: {
		break;
	}
	case ObjectRefKind::InstanceFieldRef: {
		const InstanceObject *v = (const InstanceObject *)objectRef.asArray.arrayObject;

		break;
	}
	case ObjectRefKind::ArrayElementRef: {
		const ArrayObject *v = (const ArrayObject *)objectRef.asArray.arrayObject;

		if (objectRef.asArray.index > v->length) {
			return raiseInvalidArrayIndexError(v->associatedRuntime, objectRef.asArray.index);
		}

		break;
	}
	case ObjectRefKind::ArgRef: {
		ArgRecord &argRecord = objectRef.asArg.majorFrame->argStack.at(objectRef.asArg.argIndex);
		break;
	}
	default:
		assert(false);
	}

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::typeofVar(const ObjectRef &objectRef, Type &typeOut) const {
	switch (objectRef.kind) {
	case ObjectRefKind::FieldRef: {
		FieldRecord &fieldRecord = objectRef.asField.moduleObject->fieldRecords.at(objectRef.asField.index);

		typeOut = fieldRecord.type;
		break;
	}
	case ObjectRefKind::LocalVarRef: {
		typeOut = objectRef.asLocalVar.type;
		break;
	}
	case ObjectRefKind::InstanceFieldRef: {
		const InstanceObject *v = (const InstanceObject *)objectRef.asArray.arrayObject;

		typeOut = v->_class->cachedObjectLayout->fieldRecords.at(objectRef.asArray.index).type;
		break;
	}
	case ObjectRefKind::ArrayElementRef: {
		const ArrayObject *v = (const ArrayObject *)objectRef.asArray.arrayObject;

		if (objectRef.asArray.index > v->length) {
			return raiseInvalidArrayIndexError(v->associatedRuntime, objectRef.asArray.index);
		}

		typeOut = v->elementType;
		break;
	}
	case ObjectRefKind::ArgRef: {
		ArgRecord &argRecord = objectRef.asArg.majorFrame->argStack.at(objectRef.asArg.argIndex);

		typeOut = argRecord.type;
		break;
	}
	default:
		assert(false);
	}

	return {};
}

#undef new

SLAKE_API InternalExceptionPointer Runtime::readVar(const ObjectRef &objectRef, Value &valueOut) const {
	SLAKE_RETURN_IF_EXCEPT(tryAccessVar(objectRef));

	new (&valueOut) Value(readVarUnsafe(objectRef));
	return {};
}

SLAKE_API Value Runtime::readVarUnsafe(const ObjectRef &objectRef) const {
	switch (objectRef.kind) {
	case ObjectRefKind::FieldRef: {
		FieldRecord &fieldRecord = objectRef.asField.moduleObject->fieldRecords.at(objectRef.asField.index);

		const char *const rawDataPtr = objectRef.asField.moduleObject->localFieldStorage + fieldRecord.offset;

		switch (fieldRecord.type.typeId) {
		case TypeId::Value:
			switch (fieldRecord.type.getValueTypeExData()) {
			case ValueType::I8:
				return Value(*((int8_t *)rawDataPtr));
			case ValueType::I16:
				return Value(*((int16_t *)rawDataPtr));
			case ValueType::I32:
				return Value(*((int32_t *)rawDataPtr));
			case ValueType::I64:
				return Value(*((int64_t *)rawDataPtr));
			case ValueType::U8:
				return Value(*((uint8_t *)rawDataPtr));
			case ValueType::U16:
				return Value(*((uint16_t *)rawDataPtr));
			case ValueType::U32:
				return Value(*((uint32_t *)rawDataPtr));
			case ValueType::U64:
				return Value(*((uint64_t *)rawDataPtr));
			case ValueType::F32:
				return Value(*((float *)rawDataPtr));
			case ValueType::F64:
				return Value(*((double *)rawDataPtr));
			case ValueType::Bool:
				return Value(*((bool *)rawDataPtr));
			default:
				std::terminate();
			}
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			return Value(ObjectRef::makeInstanceRef(*((Object **)rawDataPtr)));
			break;
		default:
			// All fields should be checked during the instantiation.
			throw std::logic_error("Unhandled value type");
		}

		break;
	}
	case ObjectRefKind::LocalVarRef: {
		const char *const rawDataPtr = objectRef.asLocalVar.context->dataStack + SLAKE_STACK_MAX - objectRef.asLocalVar.stackOff;

		switch (objectRef.asLocalVar.type.typeId) {
		case TypeId::Value:
			switch (objectRef.asLocalVar.type.getValueTypeExData()) {
			case ValueType::I8:
				return Value(*((int8_t *)rawDataPtr));
			case ValueType::I16:
				return Value(*((int16_t *)rawDataPtr));
			case ValueType::I32:
				return Value(*((int32_t *)rawDataPtr));
			case ValueType::I64:
				return Value(*((int64_t *)rawDataPtr));
			case ValueType::U8:
				return Value(*((uint8_t *)rawDataPtr));
			case ValueType::U16:
				return Value(*((uint16_t *)rawDataPtr));
			case ValueType::U32:
				return Value(*((uint32_t *)rawDataPtr));
			case ValueType::U64:
				return Value(*((uint64_t *)rawDataPtr));
			case ValueType::F32:
				return Value(*((float *)rawDataPtr));
			case ValueType::F64:
				return Value(*((double *)rawDataPtr));
			case ValueType::Bool:
				return Value(*((bool *)rawDataPtr));
			default:
				std::terminate();
			}
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			return Value(ObjectRef::makeInstanceRef(*((Object **)rawDataPtr)));
			break;
		default:
			// All fields should be checked during the instantiation.
			throw std::logic_error("Unhandled value type");
		}

		break;
	}
	case ObjectRefKind::InstanceFieldRef: {
		ObjectFieldRecord &fieldRecord =
			objectRef.asInstanceField.instanceObject->_class->cachedObjectLayout->fieldRecords.at(
				objectRef.asInstanceField.fieldIndex);

		const char *const rawFieldPtr = objectRef.asInstanceField.instanceObject->rawFieldData + fieldRecord.offset;

		switch (fieldRecord.type.typeId) {
		case TypeId::Value:
			switch (fieldRecord.type.getValueTypeExData()) {
			case ValueType::I8:
				return Value(*((int8_t *)rawFieldPtr));
			case ValueType::I16:
				return Value(*((int16_t *)rawFieldPtr));
			case ValueType::I32:
				return Value(*((int32_t *)rawFieldPtr));
			case ValueType::I64:
				return Value(*((int64_t *)rawFieldPtr));
			case ValueType::U8:
				return Value(*((uint8_t *)rawFieldPtr));
			case ValueType::U16:
				return Value(*((uint16_t *)rawFieldPtr));
			case ValueType::U32:
				return Value(*((uint32_t *)rawFieldPtr));
			case ValueType::U64:
				return Value(*((uint64_t *)rawFieldPtr));
			case ValueType::F32:
				return Value(*((float *)rawFieldPtr));
			case ValueType::F64:
				return Value(*((double *)rawFieldPtr));
			case ValueType::Bool:
				return Value(*((bool *)rawFieldPtr));
			default:
				std::terminate();
			}
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			return Value(ObjectRef::makeInstanceRef(*((Object **)rawFieldPtr)));
		default:
			// All fields should be checked during the instantiation.
			throw std::logic_error("Unhandled value type");
		}
		break;
	}
	case ObjectRefKind::ArrayElementRef: {
		assert(objectRef.asArray.index < objectRef.asArray.arrayObject->length);

		switch (objectRef.asArray.arrayObject->elementType.typeId) {
		case TypeId::Value: {
			switch (objectRef.asArray.arrayObject->elementType.getValueTypeExData()) {
			case ValueType::I8:
				return Value(((int8_t *)objectRef.asArray.arrayObject->data)[objectRef.asArray.index]);
			case ValueType::I16:
				return Value(((int16_t *)objectRef.asArray.arrayObject->data)[objectRef.asArray.index]);
			case ValueType::I32:
				return Value(((int32_t *)objectRef.asArray.arrayObject->data)[objectRef.asArray.index]);
			case ValueType::I64:
				return Value(((int64_t *)objectRef.asArray.arrayObject->data)[objectRef.asArray.index]);
			case ValueType::U8:
				return Value(((uint8_t *)objectRef.asArray.arrayObject->data)[objectRef.asArray.index]);
			case ValueType::U16:
				return Value(((uint16_t *)objectRef.asArray.arrayObject->data)[objectRef.asArray.index]);
			case ValueType::U32:
				return Value(((uint32_t *)objectRef.asArray.arrayObject->data)[objectRef.asArray.index]);
			case ValueType::U64:
				return Value(((uint64_t *)objectRef.asArray.arrayObject->data)[objectRef.asArray.index]);
			case ValueType::F32:
				return Value(((float *)objectRef.asArray.arrayObject->data)[objectRef.asArray.index]);
			case ValueType::F64:
				return Value(((double *)objectRef.asArray.arrayObject->data)[objectRef.asArray.index]);
			case ValueType::Bool:
				return Value(((bool *)objectRef.asArray.arrayObject->data)[objectRef.asArray.index]);
			default:
				std::terminate();
			}
			break;
		}
		case TypeId::Instance: {
			return Value(ObjectRef::makeInstanceRef(((Object **)objectRef.asArray.arrayObject->data)[objectRef.asArray.index]));
		}
		case TypeId::Any: {
			return Value(((Value *)objectRef.asArray.arrayObject->data)[objectRef.asArray.index]);
		}
		}
		break;
	}
	case ObjectRefKind::ArgRef: {
		const ArgRecord &argRecord = objectRef.asArg.majorFrame->argStack.at(objectRef.asArg.argIndex);

		return argRecord.value;
	}
	default:;
	}
	std::terminate();
}

SLAKE_API InternalExceptionPointer Runtime::writeVar(const ObjectRef &objectRef, const Value &value) const {
	switch (objectRef.kind) {
	case ObjectRefKind::FieldRef: {
		if (objectRef.asField.index >= objectRef.asField.moduleObject->fieldRecords.size())
			// TODO: Use a proper type of exception instead of this.
			return raiseInvalidArrayIndexError(objectRef.asField.moduleObject->associatedRuntime, objectRef.asArray.index);

		const FieldRecord &fieldRecord =
			objectRef.asField.moduleObject->fieldRecords.at(objectRef.asField.index);

		if (!isCompatible(fieldRecord.type, value)) {
			return raiseMismatchedVarTypeError(objectRef.asField.moduleObject->associatedRuntime);
		}

		char *const rawDataPtr = objectRef.asField.moduleObject->localFieldStorage + fieldRecord.offset;

		switch (fieldRecord.type.typeId) {
		case TypeId::Value:
			switch (fieldRecord.type.getValueTypeExData()) {
			case ValueType::I8:
				*((int8_t *)rawDataPtr) = value.getI8();
				break;
			case ValueType::I16:
				*((int16_t *)rawDataPtr) = value.getI16();
				break;
			case ValueType::I32:
				*((int32_t *)rawDataPtr) = value.getI32();
				break;
			case ValueType::I64:
				*((int64_t *)rawDataPtr) = value.getI64();
				break;
			case ValueType::U8:
				*((uint8_t *)rawDataPtr) = value.getU8();
				break;
			case ValueType::U16:
				*((uint16_t *)rawDataPtr) = value.getU16();
				break;
			case ValueType::U32:
				*((uint32_t *)rawDataPtr) = value.getU32();
				break;
			case ValueType::U64:
				*((uint64_t *)rawDataPtr) = value.getU64();
				break;
			case ValueType::F32:
				*((float *)rawDataPtr) = value.getF32();
				break;
			case ValueType::F64:
				*((double *)rawDataPtr) = value.getF64();
				break;
			case ValueType::Bool:
				*((bool *)rawDataPtr) = value.getBool();
				break;
			default:
				std::terminate();
			}
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			*((Object **)rawDataPtr) = value.getObjectRef().asInstance.instanceObject;
			break;
		default:
			// All fields should be checked during the instantiation.
			throw std::logic_error("Unhandled value type");
		}

		break;
	}
	case ObjectRefKind::LocalVarRef: {
		if (objectRef.asLocalVar.stackOff >= SLAKE_STACK_MAX)
			// TODO: Use a proper type of exception instead of this.
			return raiseInvalidArrayIndexError((Runtime *)this, objectRef.asArray.index);

		if (!isCompatible(objectRef.asLocalVar.type, value)) {
			return raiseMismatchedVarTypeError((Runtime *)this);
		}

		char *const rawDataPtr = objectRef.asLocalVar.context->dataStack + SLAKE_STACK_MAX - objectRef.asLocalVar.stackOff;

		switch (objectRef.asLocalVar.type.typeId) {
		case TypeId::Value:
			switch (objectRef.asLocalVar.type.getValueTypeExData()) {
			case ValueType::I8:
				*((int8_t *)rawDataPtr) = value.getI8();
				break;
			case ValueType::I16:
				*((int16_t *)rawDataPtr) = value.getI16();
				break;
			case ValueType::I32:
				*((int32_t *)rawDataPtr) = value.getI32();
				break;
			case ValueType::I64:
				*((int64_t *)rawDataPtr) = value.getI64();
				break;
			case ValueType::U8:
				*((uint8_t *)rawDataPtr) = value.getU8();
				break;
			case ValueType::U16:
				*((uint16_t *)rawDataPtr) = value.getU16();
				break;
			case ValueType::U32:
				*((uint32_t *)rawDataPtr) = value.getU32();
				break;
			case ValueType::U64:
				*((uint64_t *)rawDataPtr) = value.getU64();
				break;
			case ValueType::F32:
				*((float *)rawDataPtr) = value.getF32();
				break;
			case ValueType::F64:
				*((double *)rawDataPtr) = value.getF64();
				break;
			case ValueType::Bool:
				*((bool *)rawDataPtr) = value.getBool();
				break;
			}
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			*((Object **)rawDataPtr) = value.getObjectRef().asInstance.instanceObject;
			break;
		default:
			// All fields should be checked during the instantiation.
			throw std::logic_error("Unhandled value type");
		}

		break;
	}
	case ObjectRefKind::InstanceFieldRef: {
		ObjectFieldRecord &fieldRecord =
			objectRef.asInstanceField.instanceObject->_class->cachedObjectLayout->fieldRecords.at(
				objectRef.asInstanceField.fieldIndex);

		if (!isCompatible(fieldRecord.type, value)) {
			return raiseMismatchedVarTypeError((Runtime *)this);
		}

		char *const rawFieldPtr = objectRef.asInstanceField.instanceObject->rawFieldData + fieldRecord.offset;

		switch (fieldRecord.type.typeId) {
		case TypeId::Value:
			switch (fieldRecord.type.getValueTypeExData()) {
			case ValueType::I8:
				*((int8_t *)rawFieldPtr) = value.getI8();
				break;
			case ValueType::I16:
				*((int16_t *)rawFieldPtr) = value.getI16();
				break;
			case ValueType::I32:
				*((int32_t *)rawFieldPtr) = value.getI32();
				break;
			case ValueType::I64:
				*((int64_t *)rawFieldPtr) = value.getI64();
				break;
			case ValueType::U8:
				*((uint8_t *)rawFieldPtr) = value.getU8();
				break;
			case ValueType::U16:
				*((uint16_t *)rawFieldPtr) = value.getU16();
				break;
			case ValueType::U32:
				*((uint32_t *)rawFieldPtr) = value.getU32();
				break;
			case ValueType::U64:
				*((uint64_t *)rawFieldPtr) = value.getU64();
				break;
			case ValueType::F32:
				*((float *)rawFieldPtr) = value.getF32();
				break;
			case ValueType::F64:
				*((double *)rawFieldPtr) = value.getF64();
				break;
			case ValueType::Bool:
				*((bool *)rawFieldPtr) = value.getBool();
				break;
			default:
				std::terminate();
			}
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			*((Object **)rawFieldPtr) = value.getObjectRef().asInstance.instanceObject;
			break;
		default:
			// All fields should be checked during the instantiation.
			throw std::logic_error("Unhandled value type");
		}
		break;
	}
	case ObjectRefKind::ArrayElementRef: {
		if (objectRef.asArray.index > objectRef.asArray.arrayObject->length) {
			return raiseInvalidArrayIndexError((Runtime *)this, objectRef.asArray.index);
		}

		if (!isCompatible(objectRef.asArray.arrayObject->elementType, value)) {
			return raiseMismatchedVarTypeError((Runtime *)this);
		}

		switch (objectRef.asArray.arrayObject->elementType.typeId) {
		case TypeId::Value: {
			switch (objectRef.asArray.arrayObject->elementType.getValueTypeExData()) {
			case ValueType::I8:
				((int8_t *)objectRef.asArray.arrayObject->data)[objectRef.asArray.index] = value.getI8();
				break;
			case ValueType::I16:
				((int16_t *)objectRef.asArray.arrayObject->data)[objectRef.asArray.index] = value.getI16();
				break;
			case ValueType::I32:
				((int32_t *)objectRef.asArray.arrayObject->data)[objectRef.asArray.index] = value.getI32();
				break;
			case ValueType::I64:
				((int64_t *)objectRef.asArray.arrayObject->data)[objectRef.asArray.index] = value.getI64();
				break;
			case ValueType::U8:
				((uint8_t *)objectRef.asArray.arrayObject->data)[objectRef.asArray.index] = value.getU8();
				break;
			case ValueType::U16:
				((uint16_t *)objectRef.asArray.arrayObject->data)[objectRef.asArray.index] = value.getU16();
				break;
			case ValueType::U32:
				((uint32_t *)objectRef.asArray.arrayObject->data)[objectRef.asArray.index] = value.getU32();
				break;
			case ValueType::U64:
				((uint64_t *)objectRef.asArray.arrayObject->data)[objectRef.asArray.index] = value.getU64();
				break;
			case ValueType::F32:
				((float *)objectRef.asArray.arrayObject->data)[objectRef.asArray.index] = value.getF32();
				break;
			case ValueType::F64:
				((double *)objectRef.asArray.arrayObject->data)[objectRef.asArray.index] = value.getF64();
				break;
			case ValueType::Bool:
				((bool *)objectRef.asArray.arrayObject->data)[objectRef.asArray.index] = value.getBool();
				break;
			default:
				assert(false);
			}
			break;
		}
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array: {
			((Object **)objectRef.asArray.arrayObject->data)[objectRef.asArray.index] = value.getObjectRef().asInstance.instanceObject;
			break;
		}
		}
		break;
	}
	case ObjectRefKind::ArgRef: {
		ArgRecord &argRecord = objectRef.asArg.majorFrame->argStack.at(objectRef.asArg.argIndex);

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
