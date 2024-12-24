#include "../runtime.h"

using namespace slake;

SLAKE_API InternalExceptionPointer Runtime::tryAccessVar(const VarObject *varObject, const VarRefContext &context) {
	switch (varObject->getVarKind()) {
		case VarKind::Regular: {
			break;
		}
		case VarKind::LocalVarAccessor: {
			const LocalVarAccessorVarObject *v = (const LocalVarAccessorVarObject *)varObject;
			LocalVarRecord &localVarRecord =
				v->majorFrame->localVarRecords[context.asLocalVar.localVarIndex];

			break;
		}
		case VarKind::InstanceMemberAccessor: {
			const InstanceMemberAccessorVarObject *v = (const InstanceMemberAccessorVarObject *)varObject;

			break;
		}
		case VarKind::ArrayElementAccessor: {
			const ArrayAccessorVarObject *v = (const ArrayAccessorVarObject *)varObject;

			if (context.asArray.index > v->arrayObject->length) {
				return raiseInvalidArrayIndexError(v->associatedRuntime, context.asArray.index);
			}

			break;
		}
		default:
			assert(false);
	}

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::typeofVar(const VarObject *varObject, const VarRefContext &context, Type &typeOut) const {
	switch (varObject->getVarKind()) {
		case VarKind::Regular: {
			const RegularVarObject *v = (const RegularVarObject *)varObject;
			typeOut = v->type;
			break;
		}
		case VarKind::LocalVarAccessor: {
			const LocalVarAccessorVarObject *v = (const LocalVarAccessorVarObject *)varObject;
			typeOut = v->majorFrame->localVarRecords[context.asLocalVar.localVarIndex].type;
			break;
		}
		case VarKind::InstanceMemberAccessor: {
			const InstanceMemberAccessorVarObject *v = (const InstanceMemberAccessorVarObject *)varObject;
			typeOut = v->instanceObject->objectLayout->fieldRecords[context.asInstance.fieldIndex].type;
			break;
		}
		case VarKind::ArrayElementAccessor: {
			const ArrayAccessorVarObject *v = (const ArrayAccessorVarObject *)varObject;
			typeOut = Type::makeArrayTypeName(v->associatedRuntime, v->arrayObject->elementType);
			break;
		}
		default:
			assert(false);
	}

	return {};
}

SLAKE_API Value Runtime::readVar(const VarObject *varObject, const VarRefContext &context) const {
	switch (varObject->getVarKind()) {
		case VarKind::Regular: {
			const RegularVarObject *v = (const RegularVarObject *)varObject;
			return v->value;
		}
		case VarKind::LocalVarAccessor: {
			const LocalVarAccessorVarObject *v = (const LocalVarAccessorVarObject *)varObject;
			const LocalVarRecord &localVarRecord =
				v->majorFrame->localVarRecords[context.asLocalVar.localVarIndex];

			const char *const rawDataPtr = v->context->dataStack + localVarRecord.stackOffset;

			switch (localVarRecord.type.typeId) {
				case TypeId::Value:
					switch (localVarRecord.type.getValueTypeExData()) {
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
					}
					break;
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
					return Value(*((Object **)rawDataPtr));
					break;
				default:
					// All fields should be checked during the instantiation.
					throw std::logic_error("Unhandled value type");
			}

			break;
		}
		case VarKind::InstanceMemberAccessor: {
			const InstanceMemberAccessorVarObject *v = (const InstanceMemberAccessorVarObject *)varObject;

			ObjectFieldRecord &fieldRecord =
				v->instanceObject->objectLayout->fieldRecords.at(
					context.asInstance.fieldIndex);

			const char *const rawFieldPtr = v->instanceObject->rawFieldData + fieldRecord.offset;

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
					}
					break;
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
					return Value(*((Object **)rawFieldPtr));
				default:
					// All fields should be checked during the instantiation.
					throw std::logic_error("Unhandled value type");
			}
			break;
		}
		case VarKind::ArrayElementAccessor: {
			const ArrayAccessorVarObject *v = (const ArrayAccessorVarObject *)varObject;

			assert(context.asArray.index < v->arrayObject->length);

			switch (v->arrayObject->elementType.typeId) {
				case TypeId::Value: {
					switch (v->arrayObject->elementType.getValueTypeExData()) {
						case ValueType::I8:
							return Value(((I8ArrayObject *)v->arrayObject)->data[context.asArray.index]);
						case ValueType::I16:
							return Value(((I16ArrayObject *)v->arrayObject)->data[context.asArray.index]);
						case ValueType::I32:
							return Value(((I32ArrayObject *)v->arrayObject)->data[context.asArray.index]);
						case ValueType::I64:
							return Value(((I64ArrayObject *)v->arrayObject)->data[context.asArray.index]);
						case ValueType::U8:
							return Value(((U8ArrayObject *)v->arrayObject)->data[context.asArray.index]);
						case ValueType::U16:
							return Value(((U16ArrayObject *)v->arrayObject)->data[context.asArray.index]);
						case ValueType::U32:
							return Value(((U32ArrayObject *)v->arrayObject)->data[context.asArray.index]);
						case ValueType::U64:
							return Value(((U64ArrayObject *)v->arrayObject)->data[context.asArray.index]);
						case ValueType::F32:
							return Value(((F32ArrayObject *)v->arrayObject)->data[context.asArray.index]);
						case ValueType::F64:
							return Value(((F64ArrayObject *)v->arrayObject)->data[context.asArray.index]);
						case ValueType::Bool:
							return Value(((BoolArrayObject *)v->arrayObject)->data[context.asArray.index]);
						default:
							assert(false);
					}
					break;
				}
				case TypeId::Instance: {
					return Value(((ObjectRefArrayObject *)v->arrayObject)->data[context.asArray.index]);
				}
				case TypeId::Any: {
					return Value(((AnyArrayObject *)v->arrayObject)->data[context.asArray.index]);
				}
			}
			break;
		}
		default:
			assert(false);
	}
}

SLAKE_API InternalExceptionPointer Runtime::writeVar(VarObject *varObject, const VarRefContext &context, const Value &value) const {
	switch (varObject->getVarKind()) {
		case VarKind::Regular: {
			RegularVarObject *v = (RegularVarObject *)varObject;
			if (!isCompatible(v->type, value)) {
				return raiseMismatchedVarTypeError(v->associatedRuntime);
			}
			v->value = value;
			break;
		}
		case VarKind::LocalVarAccessor: {
			LocalVarAccessorVarObject *v = (LocalVarAccessorVarObject *)varObject;
			const LocalVarRecord &localVarRecord =
				v->majorFrame->localVarRecords[context.asLocalVar.localVarIndex];

			if (!isCompatible(localVarRecord.type, value)) {
				return raiseMismatchedVarTypeError(v->associatedRuntime);
			}

			char *const rawDataPtr = v->context->dataStack + localVarRecord.stackOffset;

			switch (localVarRecord.type.typeId) {
				case TypeId::Value:
					switch (localVarRecord.type.getValueTypeExData()) {
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
					*((Object **)rawDataPtr) = value.getObjectRef();
					break;
				default:
					// All fields should be checked during the instantiation.
					throw std::logic_error("Unhandled value type");
			}

			break;
		}
		case VarKind::InstanceMemberAccessor: {
			InstanceMemberAccessorVarObject *v = (InstanceMemberAccessorVarObject *)varObject;

			ObjectFieldRecord &fieldRecord =
				v->instanceObject->objectLayout->fieldRecords.at(
					context.asInstance.fieldIndex);

			if (!isCompatible(fieldRecord.type, value)) {
				return raiseMismatchedVarTypeError(v->associatedRuntime);
			}

			char *const rawFieldPtr = v->instanceObject->rawFieldData + fieldRecord.offset;

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
					}
					break;
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
					*((Object **)rawFieldPtr) = value.getObjectRef();
					break;
				default:
					// All fields should be checked during the instantiation.
					throw std::logic_error("Unhandled value type");
			}
			break;
		}
		case VarKind::ArrayElementAccessor: {
			const ArrayAccessorVarObject *v = (const ArrayAccessorVarObject *)varObject;

			if (context.asArray.index > v->arrayObject->length) {
				return raiseInvalidArrayIndexError(v->associatedRuntime, context.asArray.index);
			}

			if (!isCompatible(v->arrayObject->elementType, value)) {
				return raiseMismatchedVarTypeError(v->associatedRuntime);
			}

			switch (v->arrayObject->elementType.typeId) {
				case TypeId::Value: {
					switch (v->arrayObject->elementType.getValueTypeExData()) {
						case ValueType::I8:
							((I8ArrayObject *)v->arrayObject)->data[context.asArray.index] = value.getI8();
							break;
						case ValueType::I16:
							((I16ArrayObject *)v->arrayObject)->data[context.asArray.index] = value.getI16();
							break;
						case ValueType::I32:
							((I32ArrayObject *)v->arrayObject)->data[context.asArray.index] = value.getI32();
							break;
						case ValueType::I64:
							((I64ArrayObject *)v->arrayObject)->data[context.asArray.index] = value.getI64();
							break;
						case ValueType::U8:
							((U8ArrayObject *)v->arrayObject)->data[context.asArray.index] = value.getU8();
							break;
						case ValueType::U16:
							((U16ArrayObject *)v->arrayObject)->data[context.asArray.index] = value.getU16();
							break;
						case ValueType::U32:
							((U32ArrayObject *)v->arrayObject)->data[context.asArray.index] = value.getU32();
							break;
						case ValueType::U64:
							((U64ArrayObject *)v->arrayObject)->data[context.asArray.index] = value.getU64();
							break;
						case ValueType::F32:
							((F32ArrayObject *)v->arrayObject)->data[context.asArray.index] = value.getF32();
							break;
						case ValueType::F64:
							((F64ArrayObject *)v->arrayObject)->data[context.asArray.index] = value.getF64();
							break;
						case ValueType::Bool:
							((BoolArrayObject *)v->arrayObject)->data[context.asArray.index] = value.getBool();
							break;
						default:
							assert(false);
					}
					break;
				}
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array: {
					((ObjectRefArrayObject *)v->arrayObject)->data[context.asArray.index] = value.getObjectRef();
					break;
				}
			}
			break;
		}
		default:
			assert(false);
	}

	return {};
}
