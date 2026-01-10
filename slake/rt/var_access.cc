#include "../runtime.h"

using namespace slake;

SLAKE_API char *Runtime::calcCoroutineLocalVarRefStackBasePtr(const CoroutineLocalVarRef &localVarRef) {
	if (localVarRef.coroutine->curContext) {
		return calcStackAddr(localVarRef.coroutine->curContext->dataStack,
			localVarRef.coroutine->curContext->stackSize,
			localVarRef.stackOff + localVarRef.coroutine->offStackTop);
	} else {
		return calcStackAddr(localVarRef.coroutine->stackData,
			localVarRef.coroutine->lenStackData,
			localVarRef.stackOff);
	};
}

SLAKE_API char *Runtime::calcLocalVarRefStackBasePtr(const LocalVarRef &localVarRef) {
	return calcStackAddr(localVarRef.context->dataStack,
		localVarRef.context->stackSize,
		localVarRef.stackOff);
}

SLAKE_API char *Runtime::calcLocalVarRefStackRawDataPtr(char *p) {
	return p +
		   sizeof(TypeId) + sizeof(TypeModifier);
}

SLAKE_API void *Runtime::locateValueBasePtr(const Reference &entityRef) const noexcept {
	switch (entityRef.kind) {
		case ReferenceKind::StaticFieldRef: {
			FieldRecord &fieldRecord = entityRef.asStaticField.moduleObject->fieldRecords.at(entityRef.asStaticField.index);

			return entityRef.asStaticField.moduleObject->localFieldStorage.data() + fieldRecord.offset;
		}
		case ReferenceKind::LocalVarRef: {
			char *basePtr = calcLocalVarRefStackBasePtr(entityRef.asLocalVar);
			char *rawDataPtr = calcLocalVarRefStackRawDataPtr(basePtr);

			TypeRef t = *(TypeId *)(rawDataPtr - sizeof(TypeModifier) - sizeof(TypeId));

			switch (t.typeId) {
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
					break;
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					rawDataPtr += sizeof(void *);
					break;
				case TypeId::StructInstance:
					rawDataPtr += sizeof(void *);
					break;
				case TypeId::Ref:
					rawDataPtr += sizeof(void *);
					break;
				case TypeId::Any:
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			return calcLocalVarRefStackRawDataPtr(rawDataPtr);
		}
		case ReferenceKind::CoroutineLocalVarRef: {
			char *basePtr = calcCoroutineLocalVarRefStackBasePtr(entityRef.asCoroutineLocalVar);
			char *rawDataPtr = calcLocalVarRefStackRawDataPtr(basePtr);

			TypeRef t = *(TypeId *)(rawDataPtr - sizeof(TypeModifier) - sizeof(TypeId));

			switch (t.typeId) {
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
					break;
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					rawDataPtr += sizeof(void *);
					break;
				case TypeId::StructInstance:
					rawDataPtr += sizeof(void *);
					break;
				case TypeId::Ref:
					rawDataPtr += sizeof(void *);
					break;
				case TypeId::Any:
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			return calcLocalVarRefStackRawDataPtr(rawDataPtr);
		}
		case ReferenceKind::InstanceFieldRef: {
			ObjectFieldRecord &fieldRecord =
				entityRef.asObjectField.instanceObject->_class->cachedObjectLayout->fieldRecords.at(
					entityRef.asObjectField.fieldIndex);

			return entityRef.asObjectField.instanceObject->rawFieldData + fieldRecord.offset;
		}
		case ReferenceKind::ArrayElementRef: {
			assert(entityRef.asArrayElement.index < entityRef.asArrayElement.arrayObject->length);

			return ((char *)entityRef.asArrayElement.arrayObject->data) + entityRef.asArrayElement.index * entityRef.asArrayElement.arrayObject->elementSize;
		}
		case ReferenceKind::ArgRef:
			std::terminate();
		case ReferenceKind::CoroutineArgRef:
			std::terminate();
		case ReferenceKind::StructRef:
			return locateValueBasePtr(extractStructInnerRef(entityRef.asStruct));
		case ReferenceKind::StructFieldRef: {
			Reference innerRef = extractStructInnerRef(entityRef.asStructField.structRef);
			TypeRef actualType = typeofVar(innerRef);

			Object *const typeObject = ((CustomTypeDefObject *)actualType.typeDef)->typeObject;
			char *basePtr = (char*)locateValueBasePtr(innerRef);

			assert(typeObject->getObjectKind() == ObjectKind::Struct);

			return basePtr + ((StructObject *)typeObject)->fieldRecords.at(entityRef.asStructField.idxField).offset;
		}
		default:
			break;
	}

	std::terminate();
}

SLAKE_API TypeRef Runtime::typeofVar(const Reference &entityRef) const noexcept {
	switch (entityRef.kind) {
		case ReferenceKind::StaticFieldRef: {
			FieldRecord &fieldRecord = entityRef.asStaticField.moduleObject->fieldRecords.at(entityRef.asStaticField.index);

			const char *const rawDataPtr = entityRef.asStaticField.moduleObject->localFieldStorage.data() + fieldRecord.offset;

			return fieldRecord.type;
		}
		case ReferenceKind::LocalVarRef: {
			char *const basePtr = calcLocalVarRefStackBasePtr(entityRef.asLocalVar);
			const char *const rawDataPtr = calcLocalVarRefStackRawDataPtr(basePtr);

			TypeRef t = *(TypeId *)(rawDataPtr - sizeof(TypeModifier) - sizeof(TypeId));
			t.typeModifier = *(TypeModifier *)(rawDataPtr - sizeof(TypeModifier));

			switch (t.typeId) {
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
					break;
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					t.typeDef = *((TypeDefObject **)rawDataPtr);
					break;
				case TypeId::StructInstance:
					t.typeDef = *((TypeDefObject **)rawDataPtr);
					break;
				case TypeId::Ref:
					t.typeDef = *((TypeDefObject **)rawDataPtr);
					break;
				case TypeId::Any:
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			return t;
		}
		case ReferenceKind::CoroutineLocalVarRef: {
			char *basePtr = calcCoroutineLocalVarRefStackBasePtr(entityRef.asCoroutineLocalVar);
			const char *const rawDataPtr = calcLocalVarRefStackRawDataPtr(basePtr);

			TypeRef t = *(TypeId *)(rawDataPtr - sizeof(TypeModifier) - sizeof(TypeId));
			t.typeModifier = *(TypeModifier *)(rawDataPtr - sizeof(TypeModifier));

			switch (t.typeId) {
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
					break;
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					t.typeDef = *((TypeDefObject **)rawDataPtr);
					break;
				case TypeId::StructInstance:
					t.typeDef = *((TypeDefObject **)rawDataPtr);
					break;
				case TypeId::Ref:
					t.typeDef = *((TypeDefObject **)rawDataPtr);
					break;
				case TypeId::Any:
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			return t;
		}
		case ReferenceKind::InstanceFieldRef: {
			ObjectFieldRecord &fieldRecord =
				entityRef.asObjectField.instanceObject->_class->cachedObjectLayout->fieldRecords.at(
					entityRef.asObjectField.fieldIndex);

			return fieldRecord.type;
		}
		case ReferenceKind::ArrayElementRef: {
			assert(entityRef.asArrayElement.index < entityRef.asArrayElement.arrayObject->length);

			return entityRef.asArrayElement.arrayObject->elementType;
		}
		case ReferenceKind::ArgRef: {
			const ArgRecord &argRecord = entityRef.asArg.majorFrame->resumable->argStack.at(entityRef.asArg.argIndex);

			return argRecord.type;
		}
		case ReferenceKind::CoroutineArgRef: {
			if (entityRef.asCoroutineArg.coroutine->curContext) {
				const ArgRecord &argRecord = entityRef.asCoroutineArg.coroutine->resumable->argStack.at(entityRef.asArg.argIndex);

				return argRecord.type;
			} else {
				const ArgRecord &argRecord = entityRef.asCoroutineArg.coroutine->curMajorFrame->resumable->argStack.at(entityRef.asArg.argIndex);

				return argRecord.type;
			}
			break;
		}
		case ReferenceKind::StructRef:
			return typeofVar(extractStructInnerRef(entityRef.asStruct));
		case ReferenceKind::StructFieldRef: {
			TypeRef actualType = typeofVar(extractStructInnerRef(entityRef.asStructField.structRef));

			Object *const typeObject = ((CustomTypeDefObject *)actualType.typeDef)->typeObject;

			assert(typeObject->getObjectKind() == ObjectKind::Struct);

			return ((StructObject *)typeObject)->fieldRecords.at(entityRef.asStructField.idxField).type;
		}
		default:
			break;
	}
	std::terminate();
}

SLAKE_API InternalExceptionPointer Runtime::readVar(const Reference &entityRef, Value &valueOut) const noexcept {
	switch (entityRef.kind) {
		case ReferenceKind::StaticFieldRef: {
			const char *const rawDataPtr = (char*)locateValueBasePtr(entityRef);

			switch (typeofVar(entityRef).typeId) {
				case TypeId::I8:
					valueOut = (*((int8_t *)rawDataPtr));
					break;
				case TypeId::I16:
					valueOut = (*((int16_t *)rawDataPtr));
					break;
				case TypeId::I32:
					valueOut = (*((int32_t *)rawDataPtr));
					break;
				case TypeId::I64:
					valueOut = (*((int64_t *)rawDataPtr));
					break;
				case TypeId::U8:
					valueOut = (*((uint8_t *)rawDataPtr));
					break;
				case TypeId::U16:
					valueOut = (*((uint16_t *)rawDataPtr));
					break;
				case TypeId::U32:
					valueOut = (*((uint32_t *)rawDataPtr));
					break;
				case TypeId::U64:
					valueOut = (*((uint64_t *)rawDataPtr));
					break;
				case TypeId::F32:
					valueOut = (*((float *)rawDataPtr));
					break;
				case TypeId::F64:
					valueOut = (*((double *)rawDataPtr));
					break;
				case TypeId::Bool:
					valueOut = (*((bool *)rawDataPtr));
					break;
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					valueOut = Reference::makeObjectRef(*((Object **)rawDataPtr));
					break;
				case TypeId::StructInstance:
					valueOut = entityRef;
					break;
				case TypeId::Ref:
					valueOut = (*((Reference *)rawDataPtr));
					break;
				case TypeId::Any:
					valueOut = (*((Value *)rawDataPtr));
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case ReferenceKind::LocalVarRef: {
			const char *const rawDataPtr = (char *)locateValueBasePtr(entityRef);

			TypeRef t = typeofVar(entityRef);

			switch (t.typeId) {
				case TypeId::I8:
					valueOut = (*((int8_t *)(rawDataPtr)));
					break;
				case TypeId::I16:
					valueOut = (*((int16_t *)(rawDataPtr)));
					break;
				case TypeId::I32:
					valueOut = (*((int32_t *)(rawDataPtr)));
					break;
				case TypeId::I64:
					valueOut = (*((int64_t *)(rawDataPtr)));
					break;
				case TypeId::U8:
					valueOut = (*((uint8_t *)(rawDataPtr)));
					break;
				case TypeId::U16:
					valueOut = (*((uint16_t *)(rawDataPtr)));
					break;
				case TypeId::U32:
					valueOut = (*((uint32_t *)(rawDataPtr)));
					break;
				case TypeId::U64:
					valueOut = (*((uint64_t *)(rawDataPtr)));
					break;
				case TypeId::F32:
					valueOut = (*((float *)(rawDataPtr)));
					break;
				case TypeId::F64:
					valueOut = (*((double *)(rawDataPtr)));
					break;
				case TypeId::Bool:
					valueOut = (*((bool *)(rawDataPtr)));
					break;
				case TypeId::String:
					valueOut = (Reference::makeObjectRef(*((Object **)(rawDataPtr))));
					break;
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					valueOut = (Reference::makeObjectRef(*((Object **)(rawDataPtr))));
					break;
				case TypeId::StructInstance: {
					StructRef structRef;
					structRef.innerReferenceKind = ReferenceKind::LocalVarRef;
					structRef.innerReference.asLocalVar = entityRef.asLocalVar;

					valueOut = (Reference::makeStructRef(structRef));
					break;
				}
				case TypeId::Ref:
					valueOut = (*((Reference *)(rawDataPtr)));
					break;
				case TypeId::Any:
					valueOut = (*((Value *)(rawDataPtr)));
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case ReferenceKind::CoroutineLocalVarRef: {
			const char *const rawDataPtr = (char *)locateValueBasePtr(entityRef);

			TypeRef t = typeofVar(entityRef);

			switch (t.typeId) {
				case TypeId::I8:
					valueOut = (*((int8_t *)(rawDataPtr)));
					break;
				case TypeId::I16:
					valueOut = (*((int16_t *)(rawDataPtr)));
					break;
				case TypeId::I32:
					valueOut = (*((int32_t *)(rawDataPtr)));
					break;
				case TypeId::I64:
					valueOut = (*((int64_t *)(rawDataPtr)));
					break;
				case TypeId::U8:
					valueOut = (*((uint8_t *)(rawDataPtr)));
					break;
				case TypeId::U16:
					valueOut = (*((uint16_t *)(rawDataPtr)));
					break;
				case TypeId::U32:
					valueOut = (*((uint32_t *)(rawDataPtr)));
					break;
				case TypeId::U64:
					valueOut = (*((uint64_t *)(rawDataPtr)));
					break;
				case TypeId::F32:
					valueOut = (*((float *)(rawDataPtr)));
					break;
				case TypeId::F64:
					valueOut = (*((double *)(rawDataPtr)));
					break;
				case TypeId::Bool:
					valueOut = (*((bool *)(rawDataPtr)));
					break;
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					valueOut = (Reference::makeObjectRef(*((Object **)(rawDataPtr))));
					break;
				case TypeId::StructInstance: {
					StructRef structRef;
					structRef.innerReferenceKind = ReferenceKind::CoroutineLocalVarRef;
					structRef.innerReference.asCoroutineLocalVar = entityRef.asCoroutineLocalVar;

					valueOut = (Reference::makeStructRef(structRef));
					break;
				}
				case TypeId::Ref:
					valueOut = (*((Reference *)(rawDataPtr)));
					break;
				case TypeId::Any:
					valueOut = (*((Value *)(rawDataPtr)));
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case ReferenceKind::InstanceFieldRef: {
			const char *const rawFieldPtr = (char *)locateValueBasePtr(entityRef);

			switch (typeofVar(entityRef).typeId) {
				case TypeId::I8:
					valueOut = (*((int8_t *)rawFieldPtr));
					break;
				case TypeId::I16:
					valueOut = (*((int16_t *)rawFieldPtr));
					break;
				case TypeId::I32:
					valueOut = (*((int32_t *)rawFieldPtr));
					break;
				case TypeId::I64:
					valueOut = (*((int64_t *)rawFieldPtr));
					break;
				case TypeId::U8:
					valueOut = (*((uint8_t *)rawFieldPtr));
					break;
				case TypeId::U16:
					valueOut = (*((uint16_t *)rawFieldPtr));
					break;
				case TypeId::U32:
					valueOut = (*((uint32_t *)rawFieldPtr));
					break;
				case TypeId::U64:
					valueOut = (*((uint64_t *)rawFieldPtr));
					break;
				case TypeId::F32:
					valueOut = (*((float *)rawFieldPtr));
					break;
				case TypeId::F64:
					valueOut = (*((double *)rawFieldPtr));
					break;
				case TypeId::Bool:
					valueOut = (*((bool *)rawFieldPtr));
					break;
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					valueOut = (Reference::makeObjectRef(*((Object **)rawFieldPtr)));
					break;
				case TypeId::StructInstance:
					valueOut = entityRef;
					break;
				case TypeId::Ref:
					valueOut = (*((Reference *)rawFieldPtr));
					break;
				case TypeId::Any:
					valueOut = (*((Value *)rawFieldPtr));
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}
			break;
		}
		case ReferenceKind::ArrayElementRef: {
			assert(entityRef.asArrayElement.index < entityRef.asArrayElement.arrayObject->length);

			switch (entityRef.asArrayElement.arrayObject->elementType.typeId) {
				case TypeId::I8:
					valueOut = (((int8_t *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
					break;
				case TypeId::I16:
					valueOut = (((int16_t *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
					break;
				case TypeId::I32:
					valueOut = (((int32_t *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
					break;
				case TypeId::I64:
					valueOut = (((int64_t *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
					break;
				case TypeId::U8:
					valueOut = (((uint8_t *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
					break;
				case TypeId::U16:
					valueOut = (((uint16_t *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
					break;
				case TypeId::U32:
					valueOut = (((uint32_t *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
					break;
				case TypeId::U64:
					valueOut = (((uint64_t *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
					break;
				case TypeId::F32:
					valueOut = (((float *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
					break;
				case TypeId::F64:
					valueOut = (((double *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
					break;
				case TypeId::Bool:
					valueOut = (((bool *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
					break;
				case TypeId::Instance:
				case TypeId::String:
				case TypeId::Array:
				case TypeId::Fn:
					valueOut = (Reference::makeObjectRef(((Object **)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]));
					break;
				case TypeId::StructInstance:
					valueOut = entityRef;
					break;
				case TypeId::Ref:
					valueOut = (((Reference *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
					break;
				case TypeId::Any:
					valueOut = (((Value *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
					break;
				default:
					std::terminate();
			}
			break;
		}
		case ReferenceKind::ArgRef: {
			const ArgRecord &argRecord = entityRef.asArg.majorFrame->resumable->argStack.at(entityRef.asArg.argIndex);

			valueOut = argRecord.value;
			break;
		}
		case ReferenceKind::CoroutineArgRef: {
			if (entityRef.asCoroutineArg.coroutine->curContext) {
				const ArgRecord &argRecord = entityRef.asCoroutineArg.coroutine->resumable->argStack.at(entityRef.asArg.argIndex);

				valueOut = argRecord.value;
			} else {
				const ArgRecord &argRecord = entityRef.asCoroutineArg.coroutine->curMajorFrame->resumable->argStack.at(entityRef.asArg.argIndex);

				valueOut = argRecord.value;
			}
			break;
		}
		case ReferenceKind::StructFieldRef: {
			const char *rawDataPtr = ((char *)locateValueBasePtr(extractStructInnerRef(entityRef.asStructField.structRef)));
			TypeRef t = typeofVar(entityRef);

			switch (t.typeId) {
				case TypeId::I8:
					valueOut = (*((int8_t *)rawDataPtr));
					break;
				case TypeId::I16:
					valueOut = (*((int16_t *)rawDataPtr));
					break;
				case TypeId::I32:
					valueOut = (*((int32_t *)rawDataPtr));
					break;
				case TypeId::I64:
					valueOut = (*((int64_t *)rawDataPtr));
					break;
				case TypeId::U8:
					valueOut = (*((uint8_t *)rawDataPtr));
					break;
				case TypeId::U16:
					valueOut = (*((uint16_t *)rawDataPtr));
					break;
				case TypeId::U32:
					valueOut = (*((uint32_t *)rawDataPtr));
					break;
				case TypeId::U64:
					valueOut = (*((uint64_t *)rawDataPtr));
					break;
				case TypeId::F32:
					valueOut = (*((float *)rawDataPtr));
					break;
				case TypeId::F64:
					valueOut = (*((double *)rawDataPtr));
					break;
				case TypeId::Bool:
					valueOut = (*((bool *)rawDataPtr));
					break;
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					valueOut = (Reference::makeObjectRef(*((Object **)rawDataPtr)));
					break;
				case TypeId::Ref:
					valueOut = (*((Reference *)rawDataPtr));
					break;
				case TypeId::Any:
					valueOut = (*((Value *)rawDataPtr));
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}
			break;
		}
		default:
			std::terminate();
	}
	return {};
}

SLAKE_API InternalExceptionPointer Runtime::writeVar(const Reference &entityRef, const Value &value) const noexcept {
	bool result;

	switch (entityRef.kind) {
		case ReferenceKind::StaticFieldRef: {
			char *const rawDataPtr = (char *)locateValueBasePtr(entityRef);

			switch (typeofVar(entityRef).typeId) {
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
					*((Object **)rawDataPtr) = value.getReference().asObject;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case ReferenceKind::LocalVarRef: {
			char *const rawDataPtr = (char *)locateValueBasePtr(entityRef);

			TypeRef t = typeofVar(entityRef);

			switch (t.typeId) {
				case TypeId::I8:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((int8_t *)(rawDataPtr)) = value.getI8();
					break;
				case TypeId::I16:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((int16_t *)(rawDataPtr)) = value.getI16();
					break;
				case TypeId::I32:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((int32_t *)(rawDataPtr)) = value.getI32();
					break;
				case TypeId::I64:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((int64_t *)(rawDataPtr)) = value.getI64();
					break;
				case TypeId::U8:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((uint8_t *)(rawDataPtr)) = value.getU8();
					break;
				case TypeId::U16:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((uint16_t *)(rawDataPtr)) = value.getU16();
					break;
				case TypeId::U32:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((uint32_t *)(rawDataPtr)) = value.getU32();
					break;
				case TypeId::U64:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((uint64_t *)(rawDataPtr)) = value.getU64();
					break;
				case TypeId::F32:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((float *)(rawDataPtr)) = value.getF32();
					break;
				case TypeId::F64:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((double *)(rawDataPtr)) = value.getF64();
					break;
				case TypeId::Bool:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((bool *)(rawDataPtr)) = value.getBool();
					break;
				case TypeId::String:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((Object **)(rawDataPtr)) = value.getReference().asObject;
					break;
				case TypeId::Instance:
				case TypeId::Array:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((Object **)(rawDataPtr)) = value.getReference().asObject;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case ReferenceKind::CoroutineLocalVarRef: {
			const char *const rawDataPtr = (char *)locateValueBasePtr(entityRef);

			TypeRef t = typeofVar(entityRef);

			switch (t.typeId) {
				case TypeId::I8:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((int8_t *)(rawDataPtr)) = value.getI8();
					break;
				case TypeId::I16:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((int16_t *)(rawDataPtr)) = value.getI16();
					break;
				case TypeId::I32:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((int32_t *)(rawDataPtr)) = value.getI32();
					break;
				case TypeId::I64:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((int64_t *)(rawDataPtr)) = value.getI64();
					break;
				case TypeId::U8:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((uint8_t *)(rawDataPtr)) = value.getU8();
					break;
				case TypeId::U16:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((uint16_t *)(rawDataPtr)) = value.getU16();
					break;
				case TypeId::U32:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((uint32_t *)(rawDataPtr)) = value.getU32();
					break;
				case TypeId::U64:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((uint64_t *)(rawDataPtr)) = value.getU64();
					break;
				case TypeId::F32:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((float *)(rawDataPtr)) = value.getF32();
					break;
				case TypeId::F64:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((double *)(rawDataPtr)) = value.getF64();
					break;
				case TypeId::Bool:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((bool *)(rawDataPtr)) = value.getBool();
					break;
				case TypeId::String:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((Object **)(rawDataPtr)) = value.getReference().asObject;
					break;
				case TypeId::Instance:
				case TypeId::Array:
					if (!isCompatible(t, value)) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((Object **)(rawDataPtr)) = value.getReference().asObject;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}
			break;
		}
		case ReferenceKind::InstanceFieldRef: {
			char *const rawFieldPtr = (char *)locateValueBasePtr(entityRef);
			TypeRef t = typeofVar(entityRef);

			if (!isCompatible(t, value)) {
				return raiseMismatchedVarTypeError((Runtime *)this);
			}

			switch (t.typeId) {
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
					*((Object **)rawFieldPtr) = value.getReference().asObject;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}
			break;
		}
		case ReferenceKind::ArrayElementRef: {
			TypeRef t = typeofVar(entityRef);
			if (!isCompatible(t, value)) {
				return raiseMismatchedVarTypeError((Runtime *)this);
			}

			switch (t.typeId) {
				case TypeId::I8:
					((int8_t *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index] = value.getI8();
					break;
				case TypeId::I16:
					((int16_t *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index] = value.getI16();
					break;
				case TypeId::I32:
					((int32_t *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index] = value.getI32();
					break;
				case TypeId::I64:
					((int64_t *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index] = value.getI64();
					break;
				case TypeId::U8:
					((uint8_t *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index] = value.getU8();
					break;
				case TypeId::U16:
					((uint16_t *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index] = value.getU16();
					break;
				case TypeId::U32:
					((uint32_t *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index] = value.getU32();
					break;
				case TypeId::U64:
					((uint64_t *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index] = value.getU64();
					break;
				case TypeId::F32:
					((float *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index] = value.getF32();
					break;
				case TypeId::F64:
					((double *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index] = value.getF64();
					break;
				case TypeId::Bool:
					((bool *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index] = value.getBool();
					break;
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array: {
					((Object **)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index] = value.getReference().asObject;
					break;
				}
			}
			break;
		}
		case ReferenceKind::ArgRef: {
			ArgRecord &argRecord = entityRef.asArg.majorFrame->resumable->argStack.at(entityRef.asArg.argIndex);

			if (!isCompatible(argRecord.type, value)) {
				return raiseMismatchedVarTypeError((Runtime *)this);
			}

			argRecord.value = value;
			break;
		}
		case ReferenceKind::CoroutineArgRef: {
			if (entityRef.asCoroutineArg.coroutine->curContext) {
				ArgRecord &argRecord = entityRef.asCoroutineArg.coroutine->curMajorFrame->resumable->argStack.at(entityRef.asArg.argIndex);

				if (!isCompatible(argRecord.type, value)) {
					return raiseMismatchedVarTypeError((Runtime *)this);
				}

				argRecord.value = value;
			} else {
				ArgRecord &argRecord = entityRef.asCoroutineArg.coroutine->resumable->argStack.at(entityRef.asArg.argIndex);

				if (!isCompatible(argRecord.type, value)) {
					return raiseMismatchedVarTypeError((Runtime *)this);
				}

				argRecord.value = value;
			}
			break;
		}
		case ReferenceKind::StructFieldRef: {
			const char *rawDataPtr = (char *)locateValueBasePtr(entityRef);
			TypeRef t = typeofVar(entityRef);

			switch (t.typeId) {
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
					*((Object **)rawDataPtr) = value.getReference().asObject;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		default:
			std::terminate();
	}

	return {};
}
