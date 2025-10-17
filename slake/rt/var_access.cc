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

SLAKE_API InternalExceptionPointer Runtime::typeofVar(const Reference &entityRef, TypeRef &typeOut) const noexcept {
	std::terminate();
}

SLAKE_API InternalExceptionPointer Runtime::readVar(const Reference &entityRef, Value &valueOut) const noexcept {
	new (&valueOut) Value(readVarUnsafe(entityRef));
	return {};
}

SLAKE_API Value Runtime::readVarUnsafe(const Reference &entityRef) const noexcept {
	switch (entityRef.kind) {
		case ReferenceKind::StaticFieldRef: {
			FieldRecord &fieldRecord = entityRef.asStaticField.moduleObject->fieldRecords.at(entityRef.asStaticField.index);

			const char *const rawDataPtr = entityRef.asStaticField.moduleObject->localFieldStorage.data() + fieldRecord.offset;

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
				case TypeId::Fn:
					return Value(Reference::makeObjectRef(*((Object **)rawDataPtr)));
				case TypeId::StructInstance:
					return entityRef;
				case TypeId::Ref:
					return Value(*((Reference *)rawDataPtr));
				case TypeId::Any:
					return Value(*((Value *)rawDataPtr));
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case ReferenceKind::LocalVarRef: {
			char *const basePtr = calcLocalVarRefStackBasePtr(entityRef.asLocalVar);
			const char *const rawDataPtr = calcLocalVarRefStackRawDataPtr(basePtr);

			char *stackTop, *stackBottom;

			stackTop = entityRef.asLocalVar.context->dataStackTopPtr;
			stackBottom = entityRef.asLocalVar.context->dataStack;

			if (!stackBottomCheck(basePtr, stackBottom)) {
				std::terminate();
			}

			if (!stackTopCheck(rawDataPtr, stackTop)) {
				std::terminate();
			}

			TypeRef t = *(TypeId *)(rawDataPtr - sizeof(TypeModifier) - sizeof(TypeId));
			t.typeModifier = *(TypeModifier *)(rawDataPtr - sizeof(TypeModifier));

			switch (t.typeId) {
				case TypeId::I8:
					return Value(*((int8_t *)(rawDataPtr)));
				case TypeId::I16:
					return Value(*((int16_t *)(rawDataPtr)));
				case TypeId::I32:
					return Value(*((int32_t *)(rawDataPtr)));
				case TypeId::I64:
					return Value(*((int64_t *)(rawDataPtr)));
				case TypeId::U8:
					return Value(*((uint8_t *)(rawDataPtr)));
				case TypeId::U16:
					return Value(*((uint16_t *)(rawDataPtr)));
				case TypeId::U32:
					return Value(*((uint32_t *)(rawDataPtr)));
				case TypeId::U64:
					return Value(*((uint64_t *)(rawDataPtr)));
				case TypeId::F32:
					return Value(*((float *)(rawDataPtr)));
				case TypeId::F64:
					return Value(*((double *)(rawDataPtr)));
				case TypeId::Bool:
					return Value(*((bool *)(rawDataPtr)));
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					if (!stackTopCheck(rawDataPtr + sizeof(void *) + sizeof(void *), stackTop)) {
						std::terminate();
					}

					return Value(Reference::makeObjectRef(*((Object **)(rawDataPtr + sizeof(void *)))));
				case TypeId::StructInstance: {
					if (!stackTopCheck(rawDataPtr + sizeof(void *), stackTop)) {
						std::terminate();
					}

					StructRef sr;

					sr.structObject = (StructObject *)((CustomTypeDefObject *)*(TypeDefObject **)(rawDataPtr))->typeObject;
					sr.basePtr = &((TypeDefObject **)rawDataPtr)[1];

					return Value(Reference::makeStructRef(sr));
				}
				case TypeId::Ref:
					return Value(*((Reference *)(rawDataPtr)));
				case TypeId::Any:
					return Value(*((Value *)(rawDataPtr)));
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case ReferenceKind::CoroutineLocalVarRef: {
			char *basePtr = calcCoroutineLocalVarRefStackBasePtr(entityRef.asCoroutineLocalVar);
			const char *const rawDataPtr = calcLocalVarRefStackRawDataPtr(basePtr);

			char *stackTop, *stackBottom;

			if (entityRef.asCoroutineLocalVar.coroutine->curContext) {
				stackTop = entityRef.asCoroutineLocalVar.coroutine->curContext->dataStackTopPtr;
				stackBottom = entityRef.asCoroutineLocalVar.coroutine->curContext->dataStack;
			} else {
				stackTop = entityRef.asCoroutineLocalVar.coroutine->stackData + entityRef.asCoroutineLocalVar.coroutine->lenStackData;
				stackBottom = entityRef.asCoroutineLocalVar.coroutine->stackData;
			};

			if (!stackBottomCheck(basePtr, stackBottom)) {
				std::terminate();
			}

			if (!stackTopCheck(rawDataPtr, stackTop)) {
				std::terminate();
			}

			TypeRef t = *(TypeId *)(rawDataPtr - sizeof(TypeModifier) - sizeof(TypeId));
			t.typeModifier = *(TypeModifier *)(rawDataPtr - sizeof(TypeModifier));

			switch (t.typeId) {
				case TypeId::I8:
					return Value(*((int8_t *)(rawDataPtr)));
				case TypeId::I16:
					return Value(*((int16_t *)(rawDataPtr)));
				case TypeId::I32:
					return Value(*((int32_t *)(rawDataPtr)));
				case TypeId::I64:
					return Value(*((int64_t *)(rawDataPtr)));
				case TypeId::U8:
					return Value(*((uint8_t *)(rawDataPtr)));
				case TypeId::U16:
					return Value(*((uint16_t *)(rawDataPtr)));
				case TypeId::U32:
					return Value(*((uint32_t *)(rawDataPtr)));
				case TypeId::U64:
					return Value(*((uint64_t *)(rawDataPtr)));
				case TypeId::F32:
					return Value(*((float *)(rawDataPtr)));
				case TypeId::F64:
					return Value(*((double *)(rawDataPtr)));
				case TypeId::Bool:
					return Value(*((bool *)(rawDataPtr)));
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					if (!stackTopCheck(rawDataPtr + sizeof(void *) + sizeof(void *), stackTop)) {
						std::terminate();
					}

					return Value(Reference::makeObjectRef(*((Object **)(rawDataPtr + sizeof(void *)))));
				case TypeId::StructInstance: {
					if (!stackTopCheck(rawDataPtr + sizeof(void *), stackTop)) {
						std::terminate();
					}

					StructRef sr;

					TypeDefObject **ptd = (TypeDefObject **)rawDataPtr;	 // For debugging
					TypeDefObject *td;
					memcpy(&td, rawDataPtr, sizeof(void *));
					sr.structObject = (StructObject *)((CustomTypeDefObject *)td)->typeObject;
					sr.basePtr = &ptd[1];

					return Value(Reference::makeStructRef(sr));
				}
				case TypeId::Ref:
					return Value(*((Reference *)(rawDataPtr)));
				case TypeId::Any:
					return Value(*((Value *)(rawDataPtr)));
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case ReferenceKind::InstanceFieldRef: {
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
				case TypeId::Fn:
					return Value(Reference::makeObjectRef(*((Object **)rawFieldPtr)));
				case TypeId::StructInstance:
					return entityRef;
				case TypeId::Ref:
					return Value(*((Reference *)rawFieldPtr));
				case TypeId::Any:
					return Value(*((Value *)rawFieldPtr));
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
					return Value(((int8_t *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
				case TypeId::I16:
					return Value(((int16_t *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
				case TypeId::I32:
					return Value(((int32_t *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
				case TypeId::I64:
					return Value(((int64_t *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
				case TypeId::U8:
					return Value(((uint8_t *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
				case TypeId::U16:
					return Value(((uint16_t *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
				case TypeId::U32:
					return Value(((uint32_t *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
				case TypeId::U64:
					return Value(((uint64_t *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
				case TypeId::F32:
					return Value(((float *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
				case TypeId::F64:
					return Value(((double *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
				case TypeId::Bool:
					return Value(((bool *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
				case TypeId::Instance:
				case TypeId::String:
				case TypeId::Array:
				case TypeId::Fn:
					return Value(Reference::makeObjectRef(((Object **)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]));
				case TypeId::StructInstance:
					return entityRef;
				case TypeId::Ref:
					return Value(((Reference *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
				case TypeId::Any:
					return Value(((Value *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
				default:
					std::terminate();
			}
			break;
		}
		case ReferenceKind::ArgRef: {
			const ArgRecord &argRecord = entityRef.asArg.majorFrame->resumable->argStack.at(entityRef.asArg.argIndex);

			return argRecord.value;
		}
		case ReferenceKind::CoroutineArgRef: {
			if (entityRef.asCoroutineArg.coroutine->curContext) {
				const ArgRecord &argRecord = entityRef.asCoroutineArg.coroutine->resumable->argStack.at(entityRef.asArg.argIndex);

				return argRecord.value;
			} else {
				const ArgRecord &argRecord = entityRef.asCoroutineArg.coroutine->curMajorFrame->resumable->argStack.at(entityRef.asArg.argIndex);

				return argRecord.value;
			}
			break;
		}
		case ReferenceKind::StructFieldRef: {
			const ObjectFieldRecord &fieldRecord = entityRef.asStructField.structRef.structObject->cachedObjectLayout->fieldRecords.at(entityRef.asStructField.idxField);

			const char *rawDataPtr = ((char *)entityRef.asStructField.structRef.basePtr) + fieldRecord.offset;

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
				case TypeId::Fn:
					return Value(Reference::makeObjectRef(*((Object **)rawDataPtr)));
				case TypeId::StructInstance: {
					StructRef sr;

					sr.structObject = (StructObject *)(fieldRecord.type.getCustomTypeDef())->typeObject;
					sr.basePtr = (void *)rawDataPtr;

					return Value(Reference::makeStructRef(sr));
				}
				case TypeId::Ref:
					return Value(*((Reference *)rawDataPtr));
				case TypeId::Any:
					return Value(*((Value *)rawDataPtr));
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}
		}
		default:
			break;
	}
	std::terminate();
}

SLAKE_API InternalExceptionPointer Runtime::writeVar(const Reference &entityRef, const Value &value) const noexcept {
	bool result;

	switch (entityRef.kind) {
		case ReferenceKind::StaticFieldRef: {
			if (entityRef.asStaticField.index >= entityRef.asStaticField.moduleObject->fieldRecords.size())
				// TODO: Use a proper type of exception instead of this.
				return raiseInvalidArrayIndexError(entityRef.asStaticField.moduleObject->associatedRuntime, entityRef.asArrayElement.index);

			const FieldRecord &fieldRecord =
				entityRef.asStaticField.moduleObject->fieldRecords.at(entityRef.asStaticField.index);

			SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), fieldRecord.type, value, result));
			if (!result) {
				return raiseMismatchedVarTypeError(entityRef.asStaticField.moduleObject->associatedRuntime);
			}

			char *const rawDataPtr = entityRef.asStaticField.moduleObject->localFieldStorage.data() + fieldRecord.offset;

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
					*((Object **)rawDataPtr) = value.getReference().asObject;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case ReferenceKind::LocalVarRef: {
			char *const basePtr = calcLocalVarRefStackBasePtr(entityRef.asLocalVar);
			const char *const rawDataPtr = calcLocalVarRefStackRawDataPtr(basePtr);

			char *stackTop, *stackBottom;

			stackTop = entityRef.asLocalVar.context->dataStackTopPtr;
			stackBottom = entityRef.asLocalVar.context->dataStack;

			if (!stackBottomCheck(basePtr, stackBottom)) {
				std::terminate();
			}

			if (!stackTopCheck(rawDataPtr, stackTop)) {
				std::terminate();
			}

			TypeRef t = *(TypeId *)(rawDataPtr - sizeof(TypeModifier) - sizeof(TypeId));
			t.typeModifier = *(TypeModifier *)(rawDataPtr - sizeof(TypeModifier));

			switch (t.typeId) {
				case TypeId::I8:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((int8_t *)(rawDataPtr)) = value.getI8();
					break;
				case TypeId::I16:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((int16_t *)(rawDataPtr)) = value.getI16();
					break;
				case TypeId::I32:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((int32_t *)(rawDataPtr)) = value.getI32();
					break;
				case TypeId::I64:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((int64_t *)(rawDataPtr)) = value.getI64();
					break;
				case TypeId::U8:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((uint8_t *)(rawDataPtr)) = value.getU8();
					break;
				case TypeId::U16:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((uint16_t *)(rawDataPtr)) = value.getU16();
					break;
				case TypeId::U32:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((uint32_t *)(rawDataPtr)) = value.getU32();
					break;
				case TypeId::U64:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((uint64_t *)(rawDataPtr)) = value.getU64();
					break;
				case TypeId::F32:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((float *)(rawDataPtr)) = value.getF32();
					break;
				case TypeId::F64:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((double *)(rawDataPtr)) = value.getF64();
					break;
				case TypeId::Bool:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((bool *)(rawDataPtr)) = value.getBool();
					break;
				case TypeId::String:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((Object **)(rawDataPtr)) = value.getReference().asObject;
					break;
				case TypeId::Instance:
				case TypeId::Array:
					if (!stackTopCheck(rawDataPtr + sizeof(void *) + sizeof(void *), stackTop)) {
						std::terminate();
					}
					t.typeDef = *(TypeDefObject **)rawDataPtr;
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((Object **)(rawDataPtr + sizeof(void *))) = value.getReference().asObject;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case ReferenceKind::CoroutineLocalVarRef: {
			char *basePtr = calcCoroutineLocalVarRefStackBasePtr(entityRef.asCoroutineLocalVar);
			const char *const rawDataPtr = calcLocalVarRefStackRawDataPtr(basePtr);

			char *stackTop, *stackBottom;

			if (entityRef.asCoroutineLocalVar.coroutine->curContext) {
				stackTop = entityRef.asCoroutineLocalVar.coroutine->curContext->dataStackTopPtr;
				stackBottom = entityRef.asCoroutineLocalVar.coroutine->curContext->dataStack;
			} else {
				stackTop = entityRef.asCoroutineLocalVar.coroutine->stackData + entityRef.asCoroutineLocalVar.coroutine->lenStackData;
				stackBottom = entityRef.asCoroutineLocalVar.coroutine->stackData;
			};

			if (!stackBottomCheck(basePtr, stackBottom)) {
				std::terminate();
			}

			if (!stackTopCheck(rawDataPtr, stackTop)) {
				std::terminate();
			}

			TypeRef t = *(TypeId *)(rawDataPtr - sizeof(TypeModifier) - sizeof(TypeId));
			t.typeModifier = *(TypeModifier *)(rawDataPtr - sizeof(TypeModifier));

			switch (t.typeId) {
				case TypeId::I8:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((int8_t *)(rawDataPtr)) = value.getI8();
					break;
				case TypeId::I16:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((int16_t *)(rawDataPtr)) = value.getI16();
					break;
				case TypeId::I32:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((int32_t *)(rawDataPtr)) = value.getI32();
					break;
				case TypeId::I64:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((int64_t *)(rawDataPtr)) = value.getI64();
					break;
				case TypeId::U8:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((uint8_t *)(rawDataPtr)) = value.getU8();
					break;
				case TypeId::U16:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((uint16_t *)(rawDataPtr)) = value.getU16();
					break;
				case TypeId::U32:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((uint32_t *)(rawDataPtr)) = value.getU32();
					break;
				case TypeId::U64:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((uint64_t *)(rawDataPtr)) = value.getU64();
					break;
				case TypeId::F32:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((float *)(rawDataPtr)) = value.getF32();
					break;
				case TypeId::F64:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((double *)(rawDataPtr)) = value.getF64();
					break;
				case TypeId::Bool:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((bool *)(rawDataPtr)) = value.getBool();
					break;
				case TypeId::String:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((Object **)(rawDataPtr)) = value.getReference().asObject;
					break;
				case TypeId::Instance:
				case TypeId::Array:
					if (!stackTopCheck(rawDataPtr + sizeof(void *) + sizeof(void *), stackTop)) {
						std::terminate();
					}

					t.typeDef = *(TypeDefObject **)rawDataPtr;
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((Object **)(rawDataPtr + sizeof(void *))) = value.getReference().asObject;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}
			break;
		}
		case ReferenceKind::InstanceFieldRef: {
			ObjectFieldRecord &fieldRecord =
				entityRef.asObjectField.instanceObject->_class->cachedObjectLayout->fieldRecords.at(
					entityRef.asObjectField.fieldIndex);

			SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), fieldRecord.type, value, result));
			if (!result) {
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
					*((Object **)rawFieldPtr) = value.getReference().asObject;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}
			break;
		}
		case ReferenceKind::ArrayElementRef: {
			if (entityRef.asArrayElement.index > entityRef.asArrayElement.arrayObject->length) {
				return raiseInvalidArrayIndexError((Runtime *)this, entityRef.asArrayElement.index);
			}

			SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), entityRef.asArrayElement.arrayObject->elementType, value, result));
			if (!result) {
				return raiseMismatchedVarTypeError((Runtime *)this);
			}

			switch (entityRef.asArrayElement.arrayObject->elementType.typeId) {
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

			SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), argRecord.type, value, result));
			if (!result) {
				return raiseMismatchedVarTypeError((Runtime *)this);
			}

			argRecord.value = value;
			break;
		}
		case ReferenceKind::CoroutineArgRef: {
			if (entityRef.asCoroutineArg.coroutine->curContext) {
				ArgRecord &argRecord = entityRef.asCoroutineArg.coroutine->curMajorFrame->resumable->argStack.at(entityRef.asArg.argIndex);

				SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), argRecord.type, value, result));
				if (!result) {
					return raiseMismatchedVarTypeError((Runtime *)this);
				}

				argRecord.value = value;
			} else {
				ArgRecord &argRecord = entityRef.asCoroutineArg.coroutine->resumable->argStack.at(entityRef.asArg.argIndex);

				SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), argRecord.type, value, result));
				if (!result) {
					return raiseMismatchedVarTypeError((Runtime *)this);
				}

				argRecord.value = value;
			}
			break;
		}
		case ReferenceKind::StructFieldRef: {
			const ObjectFieldRecord &fieldRecord = entityRef.asStructField.structRef.structObject->cachedObjectLayout->fieldRecords.at(entityRef.asStructField.idxField);

			const char *rawDataPtr = ((char *)entityRef.asStructField.structRef.basePtr) + fieldRecord.offset;

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
