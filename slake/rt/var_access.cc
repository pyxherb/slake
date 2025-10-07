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

SLAKE_API InternalExceptionPointer Runtime::typeofVar(const EntityRef &entityRef, TypeRef &typeOut) const noexcept {
	std::terminate();
}

SLAKE_API InternalExceptionPointer Runtime::readVar(const EntityRef &entityRef, Value &valueOut) const noexcept {
	new (&valueOut) Value(readVarUnsafe(entityRef));
	return {};
}

SLAKE_API Value Runtime::readVarUnsafe(const EntityRef &entityRef) const noexcept {
	switch (entityRef.kind) {
		case EntityRefKind::StaticFieldRef: {
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
					return Value(EntityRef::makeObjectRef(*((Object **)rawDataPtr)));
				case TypeId::StructInstance:
					return entityRef;
				case TypeId::Ref:
					return Value(*((EntityRef *)rawDataPtr));
				case TypeId::Any:
					return Value(*((Value *)rawDataPtr));
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case EntityRefKind::LocalVarRef: {
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

					return Value(EntityRef::makeObjectRef(*((Object **)(rawDataPtr + sizeof(void *)))));
				case TypeId::StructInstance: {
					if (!stackTopCheck(rawDataPtr + sizeof(void *), stackTop)) {
						std::terminate();
					}

					StructRef sr;

					sr.structObject = (StructObject *)((CustomTypeDefObject *)*(TypeDefObject **)(rawDataPtr))->typeObject;
					sr.asLocalVar = entityRef.asLocalVar;
					sr.innerKind = entityRef.kind;

					return Value(EntityRef::makeStructRef(sr));
				}
				case TypeId::Ref:
					return Value(*((EntityRef *)(rawDataPtr)));
				case TypeId::Any:
					return Value(*((Value *)(rawDataPtr)));
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case EntityRefKind::CoroutineLocalVarRef: {
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

					return Value(EntityRef::makeObjectRef(*((Object **)(rawDataPtr + sizeof(void *)))));
				case TypeId::StructInstance: {
					if (!stackTopCheck(rawDataPtr + sizeof(void *), stackTop)) {
						std::terminate();
					}

					StructRef sr;

					TypeDefObject **ptd = (TypeDefObject **)rawDataPtr;	 // For debugging
					TypeDefObject *td;
					memcpy(&td, rawDataPtr, sizeof(void *));
					sr.structObject = (StructObject *)((CustomTypeDefObject *)td)->typeObject;
					sr.asLocalVar = entityRef.asLocalVar;
					sr.innerKind = entityRef.kind;

					return Value(EntityRef::makeStructRef(sr));
				}
				case TypeId::Ref:
					return Value(*((EntityRef *)(rawDataPtr)));
				case TypeId::Any:
					return Value(*((Value *)(rawDataPtr)));
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case EntityRefKind::InstanceFieldRef: {
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
					return Value(EntityRef::makeObjectRef(*((Object **)rawFieldPtr)));
				case TypeId::StructInstance:
					return entityRef;
				case TypeId::Ref:
					return Value(*((EntityRef *)rawFieldPtr));
				case TypeId::Any:
					return Value(*((Value *)rawFieldPtr));
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}
			break;
		}
		case EntityRefKind::ArrayElementRef: {
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
					return Value(EntityRef::makeObjectRef(((Object **)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]));
				case TypeId::StructInstance:
					return entityRef;
				case TypeId::Ref:
					return Value(((EntityRef *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
				case TypeId::Any:
					return Value(((Value *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
				default:
					std::terminate();
			}
			break;
		}
		case EntityRefKind::ArgRef: {
			const ArgRecord &argRecord = entityRef.asArg.majorFrame->resumable->argStack.at(entityRef.asArg.argIndex);

			return argRecord.value;
		}
		case EntityRefKind::CoroutineArgRef: {
			if (entityRef.asCoroutineArg.coroutine->curContext) {
				const ArgRecord &argRecord = entityRef.asCoroutineArg.coroutine->resumable->argStack.at(entityRef.asArg.argIndex);

				return argRecord.value;
			} else {
				const ArgRecord &argRecord = entityRef.asCoroutineArg.coroutine->curMajorFrame->resumable->argStack.at(entityRef.asArg.argIndex);

				return argRecord.value;
			}
			break;
		}
		case EntityRefKind::StructFieldRef:
			return readStructFieldData(entityRef.asStructField);
		default:
			break;
	}
	std::terminate();
}

SLAKE_API void Runtime::readStructData(char *dest, const StructRef &structRef) const noexcept {
	const size_t szStruct = structRef.structObject->cachedObjectLayout->totalSize;
	switch (structRef.innerKind) {
		case EntityRefKind::StaticFieldRef: {
			FieldRecord &fieldRecord = structRef.asStaticField.moduleObject->fieldRecords.at(structRef.asStaticField.index);

			const char *const rawDataPtr = structRef.asStaticField.moduleObject->localFieldStorage.data() + fieldRecord.offset;

			memcpy(dest, rawDataPtr, szStruct);

			break;
		}
		case EntityRefKind::LocalVarRef: {
			char *const basePtr = calcLocalVarRefStackBasePtr(structRef.asLocalVar);
			const char *const rawDataPtr = calcLocalVarRefStackRawDataPtr(basePtr);

			char *stackTop, *stackBottom;

			stackTop = structRef.asLocalVar.context->dataStackTopPtr;
			stackBottom = structRef.asLocalVar.context->dataStack;

			if (!stackBottomCheck(basePtr, stackBottom)) {
				std::terminate();
			}

			if (!stackTopCheck(rawDataPtr + szStruct, stackTop)) {
				std::terminate();
			}

			memcpy(dest, rawDataPtr, szStruct);

			break;
		}
		case EntityRefKind::CoroutineLocalVarRef: {
			char *basePtr = calcCoroutineLocalVarRefStackBasePtr(structRef.asCoroutineLocalVar);
			const char *const rawDataPtr = calcLocalVarRefStackRawDataPtr(basePtr);

			char *stackTop, *stackBottom;

			if (structRef.asCoroutineLocalVar.coroutine->curContext) {
				stackTop = structRef.asCoroutineLocalVar.coroutine->curContext->dataStackTopPtr;
				stackBottom = structRef.asCoroutineLocalVar.coroutine->curContext->dataStack;
			} else {
				stackTop = structRef.asCoroutineLocalVar.coroutine->stackData;
				stackBottom = structRef.asCoroutineLocalVar.coroutine->stackData + structRef.asCoroutineLocalVar.coroutine->lenStackData;
			};

			if (!stackBottomCheck(basePtr, stackBottom)) {
				std::terminate();
			}

			if (!stackTopCheck(rawDataPtr + szStruct, stackTop)) {
				std::terminate();
			}

			memcpy(dest, rawDataPtr, szStruct);

			break;
		}
		case EntityRefKind::InstanceFieldRef: {
			ObjectFieldRecord &fieldRecord =
				structRef.asObjectField.instanceObject->_class->cachedObjectLayout->fieldRecords.at(
					structRef.asObjectField.fieldIndex);

			const char *const rawFieldPtr = structRef.asObjectField.instanceObject->rawFieldData + fieldRecord.offset;

			memcpy(dest, rawFieldPtr, szStruct);
			break;
		}
		case EntityRefKind::ArrayElementRef: {
			assert(structRef.asArrayElement.index < structRef.asArrayElement.arrayObject->length);

			memcpy(dest, ((const char *)structRef.asArrayElement.arrayObject->data) + structRef.asArrayElement.index * szStruct, szStruct);
			break;
		}
		case EntityRefKind::ArgRef: {
			const ArgRecord &argRecord = structRef.asArg.majorFrame->resumable->argStack.at(structRef.asArg.argIndex);

			std::terminate();
		}
		case EntityRefKind::CoroutineArgRef: {
			if (structRef.asCoroutineArg.coroutine->curContext) {
				const ArgRecord &argRecord = structRef.asCoroutineArg.coroutine->resumable->argStack.at(structRef.asArg.argIndex);

				std::terminate();
			} else {
				const ArgRecord &argRecord = structRef.asCoroutineArg.coroutine->curMajorFrame->resumable->argStack.at(structRef.asArg.argIndex);

				std::terminate();
			}
		}
		default:;
	}
}

SLAKE_API Value Runtime::readStructFieldData(const StructFieldRef &structRef) const noexcept {
	const size_t szField = sizeofType(structRef.structRef.structObject->cachedObjectLayout->fieldRecords.at(structRef.idxField).type);
	switch (structRef.structRef.innerKind) {
		case EntityRefKind::StaticFieldRef: {
			// stub
			FieldRecord &fieldRecord = structRef.structRef.asStaticField.moduleObject->fieldRecords.at(structRef.structRef.asStaticField.index);

			const char *const rawDataPtr = structRef.structRef.asStaticField.moduleObject->localFieldStorage.data() + fieldRecord.offset;

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
					return Value(EntityRef::makeObjectRef(*((Object **)rawDataPtr)));
				case TypeId::StructInstance:
					// TODO: Dispose nested structures.
					std::terminate();
				case TypeId::Ref:
					return Value(*((EntityRef *)rawDataPtr));
				case TypeId::Any:
					return Value(*((Value *)rawDataPtr));
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case EntityRefKind::LocalVarRef: {
			char *const basePtr = calcLocalVarRefStackBasePtr(structRef.structRef.asLocalVar);
			const char *const rawDataPtr = calcLocalVarRefStackRawDataPtr(basePtr);

			char *stackTop, *stackBottom;

			stackTop = structRef.structRef.asLocalVar.context->dataStackTopPtr;
			stackBottom = structRef.structRef.asLocalVar.context->dataStack;

			if (!stackBottomCheck(basePtr, stackBottom)) {
				std::terminate();
			}

			if (!stackTopCheck(rawDataPtr + szField, stackTop)) {
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

					return Value(EntityRef::makeObjectRef(*((Object **)(rawDataPtr + sizeof(void *)))));
				case TypeId::StructInstance:
					std::terminate();
				case TypeId::Ref:
					return Value(*((EntityRef *)(rawDataPtr)));
				case TypeId::Any:
					return Value(*((Value *)(rawDataPtr)));
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case EntityRefKind::CoroutineLocalVarRef: {
			char *basePtr = calcCoroutineLocalVarRefStackBasePtr(structRef.structRef.asCoroutineLocalVar);
			const char *const rawDataPtr = calcLocalVarRefStackRawDataPtr(basePtr) + sizeof(void *) + structRef.structRef.structObject->cachedObjectLayout->fieldRecords.at(structRef.idxField).offset;

			char *stackTop, *stackBottom;

			if (structRef.structRef.asCoroutineLocalVar.coroutine->curContext) {
				stackTop = structRef.structRef.asCoroutineLocalVar.coroutine->curContext->dataStackTopPtr;
				stackBottom = structRef.structRef.asCoroutineLocalVar.coroutine->curContext->dataStack;
			} else {
				stackTop = structRef.structRef.asCoroutineLocalVar.coroutine->stackData + structRef.structRef.asCoroutineLocalVar.coroutine->lenStackData;
				stackBottom = structRef.structRef.asCoroutineLocalVar.coroutine->stackData;
			};

			if (!stackBottomCheck(basePtr, stackBottom)) {
				std::terminate();
			}

			if (!stackTopCheck(rawDataPtr + szField, stackTop)) {
				std::terminate();
			}

			TypeRef t = structRef.structRef.structObject->cachedObjectLayout->fieldRecords.at(structRef.idxField).type;

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

					return Value(EntityRef::makeObjectRef(*((Object **)(rawDataPtr + sizeof(void *)))));
				case TypeId::StructInstance:
					std::terminate();
				case TypeId::Ref:
					return Value(*((EntityRef *)(rawDataPtr)));
				case TypeId::Any:
					return Value(*((Value *)(rawDataPtr)));
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case EntityRefKind::InstanceFieldRef: {
			// stub
			ObjectFieldRecord &fieldRecord =
				structRef.structRef.asObjectField.instanceObject->_class->cachedObjectLayout->fieldRecords.at(
					structRef.structRef.asObjectField.fieldIndex);

			const char *const rawFieldPtr = structRef.structRef.asObjectField.instanceObject->rawFieldData + fieldRecord.offset;

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
					return Value(EntityRef::makeObjectRef(*((Object **)rawFieldPtr)));
				case TypeId::StructInstance:
					std::terminate();
				case TypeId::Ref:
					return Value(*((EntityRef *)rawFieldPtr));
				case TypeId::Any:
					return Value(*((Value *)rawFieldPtr));
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}
			break;
		}
		case EntityRefKind::ArrayElementRef: {
			// stub
			assert(structRef.structRef.asArrayElement.index < structRef.structRef.asArrayElement.arrayObject->length);

			switch (structRef.structRef.asArrayElement.arrayObject->elementType.typeId) {
				case TypeId::I8:
					return Value(((int8_t *)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index]);
				case TypeId::I16:
					return Value(((int16_t *)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index]);
				case TypeId::I32:
					return Value(((int32_t *)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index]);
				case TypeId::I64:
					return Value(((int64_t *)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index]);
				case TypeId::U8:
					return Value(((uint8_t *)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index]);
				case TypeId::U16:
					return Value(((uint16_t *)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index]);
				case TypeId::U32:
					return Value(((uint32_t *)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index]);
				case TypeId::U64:
					return Value(((uint64_t *)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index]);
				case TypeId::F32:
					return Value(((float *)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index]);
				case TypeId::F64:
					return Value(((double *)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index]);
				case TypeId::Bool:
					return Value(((bool *)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index]);
				case TypeId::Instance:
				case TypeId::String:
				case TypeId::Array:
				case TypeId::Fn:
					return Value(EntityRef::makeObjectRef(((Object **)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index]));
				case TypeId::StructInstance:
					std::terminate();
				case TypeId::Ref:
					return Value(((EntityRef *)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index]);
				case TypeId::Any:
					return Value(((Value *)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index]);
				default:
					std::terminate();
			}
			break;
		}
		case EntityRefKind::ArgRef: {
			// stub
			const ArgRecord &argRecord = structRef.structRef.asArg.majorFrame->resumable->argStack.at(structRef.structRef.asArg.argIndex);

			std::terminate();
		}
		case EntityRefKind::CoroutineArgRef: {
			// stub
			if (structRef.structRef.asCoroutineArg.coroutine->curContext) {
				const ArgRecord &argRecord = structRef.structRef.asCoroutineArg.coroutine->resumable->argStack.at(structRef.structRef.asArg.argIndex);

				std::terminate();
			} else {
				const ArgRecord &argRecord = structRef.structRef.asCoroutineArg.coroutine->curMajorFrame->resumable->argStack.at(structRef.structRef.asArg.argIndex);

				std::terminate();
			}
		}
		default:;
	}
}

SLAKE_API InternalExceptionPointer Runtime::writeVar(const EntityRef &entityRef, const Value &value) const noexcept {
	bool result;

	switch (entityRef.kind) {
		case EntityRefKind::StaticFieldRef: {
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
					*((Object **)rawDataPtr) = value.getEntityRef().asObject;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case EntityRefKind::LocalVarRef: {
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
					*((Object **)(rawDataPtr)) = value.getEntityRef().asObject;
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
					*((Object **)(rawDataPtr + sizeof(void *))) = value.getEntityRef().asObject;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case EntityRefKind::CoroutineLocalVarRef: {
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
					*((Object **)(rawDataPtr)) = value.getEntityRef().asObject;
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
					*((Object **)(rawDataPtr + sizeof(void *))) = value.getEntityRef().asObject;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}
			break;
		}
		case EntityRefKind::InstanceFieldRef: {
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
					*((Object **)rawFieldPtr) = value.getEntityRef().asObject;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}
			break;
		}
		case EntityRefKind::ArrayElementRef: {
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
					((Object **)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index] = value.getEntityRef().asObject;
					break;
				}
			}
			break;
		}
		case EntityRefKind::ArgRef: {
			ArgRecord &argRecord = entityRef.asArg.majorFrame->resumable->argStack.at(entityRef.asArg.argIndex);

			SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), argRecord.type, value, result));
			if (!result) {
				return raiseMismatchedVarTypeError((Runtime *)this);
			}

			argRecord.value = value;
			break;
		}
		case EntityRefKind::CoroutineArgRef: {
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
		case EntityRefKind::StructFieldRef:
			return writeStructFieldData(entityRef.asStructField, value);
		default:
			std::terminate();
	}

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::writeStructFieldData(const StructFieldRef &structRef, const Value &value) const noexcept {
	bool result;

	switch (structRef.structRef.innerKind) {
		case EntityRefKind::StaticFieldRef: {
			// stub
			if (structRef.structRef.asStaticField.index >= structRef.structRef.asStaticField.moduleObject->fieldRecords.size())
				// TODO: Use a proper type of exception instead of this.
				return raiseInvalidArrayIndexError(structRef.structRef.asStaticField.moduleObject->associatedRuntime, structRef.structRef.asArrayElement.index);

			const FieldRecord &fieldRecord =
				structRef.structRef.asStaticField.moduleObject->fieldRecords.at(structRef.structRef.asStaticField.index);

			SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), fieldRecord.type, value, result));
			if (!result) {
				return raiseMismatchedVarTypeError(structRef.structRef.asStaticField.moduleObject->associatedRuntime);
			}

			char *const rawDataPtr = structRef.structRef.asStaticField.moduleObject->localFieldStorage.data() + fieldRecord.offset;

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
					*((Object **)rawDataPtr) = value.getEntityRef().asObject;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case EntityRefKind::LocalVarRef: {
			char *const basePtr = calcLocalVarRefStackBasePtr(structRef.structRef.asLocalVar);
			const char *const rawDataPtr = calcLocalVarRefStackRawDataPtr(basePtr);

			char *stackTop, *stackBottom;

			stackTop = structRef.structRef.asLocalVar.context->dataStackTopPtr;
			stackBottom = structRef.structRef.asLocalVar.context->dataStack;

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
					*((Object **)(rawDataPtr)) = value.getEntityRef().asObject;
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
					*((Object **)(rawDataPtr + sizeof(void *))) = value.getEntityRef().asObject;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case EntityRefKind::CoroutineLocalVarRef: {
			char *basePtr = calcCoroutineLocalVarRefStackBasePtr(structRef.structRef.asCoroutineLocalVar);
			const char *const rawDataPtr = calcLocalVarRefStackRawDataPtr(basePtr) + sizeof(void *) + structRef.structRef.structObject->cachedObjectLayout->fieldRecords.at(structRef.idxField).offset;

			char *stackTop, *stackBottom;

			if (structRef.structRef.asCoroutineLocalVar.coroutine->curContext) {
				stackTop = structRef.structRef.asCoroutineLocalVar.coroutine->curContext->dataStackTopPtr;
				stackBottom = structRef.structRef.asCoroutineLocalVar.coroutine->curContext->dataStack;
			} else {
				stackTop = structRef.structRef.asCoroutineLocalVar.coroutine->stackData + structRef.structRef.asCoroutineLocalVar.coroutine->lenStackData;
				stackBottom = structRef.structRef.asCoroutineLocalVar.coroutine->stackData;
			};

			if (!stackBottomCheck(basePtr, stackBottom)) {
				std::terminate();
			}

			if (!stackTopCheck(rawDataPtr, stackTop)) {
				std::terminate();
			}

			TypeRef t = structRef.structRef.structObject->cachedObjectLayout->fieldRecords.at(structRef.idxField).type;

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
					*((Object **)(rawDataPtr)) = value.getEntityRef().asObject;
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
					*((Object **)(rawDataPtr + sizeof(void *))) = value.getEntityRef().asObject;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}
			break;
		}
		case EntityRefKind::InstanceFieldRef: {
			// stub
			ObjectFieldRecord &fieldRecord =
				structRef.structRef.asObjectField.instanceObject->_class->cachedObjectLayout->fieldRecords.at(
					structRef.structRef.asObjectField.fieldIndex);

			SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), fieldRecord.type, value, result));
			if (!result) {
				return raiseMismatchedVarTypeError((Runtime *)this);
			}

			char *const rawFieldPtr = structRef.structRef.asObjectField.instanceObject->rawFieldData + fieldRecord.offset;

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
					*((Object **)rawFieldPtr) = value.getEntityRef().asObject;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}
			break;
		}
		case EntityRefKind::ArrayElementRef: {
			// stub
			if (structRef.structRef.asArrayElement.index > structRef.structRef.asArrayElement.arrayObject->length) {
				return raiseInvalidArrayIndexError((Runtime *)this, structRef.structRef.asArrayElement.index);
			}

			SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), structRef.structRef.asArrayElement.arrayObject->elementType, value, result));
			if (!result) {
				return raiseMismatchedVarTypeError((Runtime *)this);
			}

			switch (structRef.structRef.asArrayElement.arrayObject->elementType.typeId) {
				case TypeId::I8:
					((int8_t *)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index] = value.getI8();
					break;
				case TypeId::I16:
					((int16_t *)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index] = value.getI16();
					break;
				case TypeId::I32:
					((int32_t *)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index] = value.getI32();
					break;
				case TypeId::I64:
					((int64_t *)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index] = value.getI64();
					break;
				case TypeId::U8:
					((uint8_t *)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index] = value.getU8();
					break;
				case TypeId::U16:
					((uint16_t *)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index] = value.getU16();
					break;
				case TypeId::U32:
					((uint32_t *)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index] = value.getU32();
					break;
				case TypeId::U64:
					((uint64_t *)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index] = value.getU64();
					break;
				case TypeId::F32:
					((float *)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index] = value.getF32();
					break;
				case TypeId::F64:
					((double *)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index] = value.getF64();
					break;
				case TypeId::Bool:
					((bool *)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index] = value.getBool();
					break;
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array: {
					((Object **)structRef.structRef.asArrayElement.arrayObject->data)[structRef.structRef.asArrayElement.index] = value.getEntityRef().asObject;
					break;
				}
			}
			break;
		}
		case EntityRefKind::ArgRef: {
			// stub
			ArgRecord &argRecord = structRef.structRef.asArg.majorFrame->resumable->argStack.at(structRef.structRef.asArg.argIndex);

			SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), argRecord.type, value, result));
			if (!result) {
				return raiseMismatchedVarTypeError((Runtime *)this);
			}

			argRecord.value = value;
			break;
		}
		case EntityRefKind::CoroutineArgRef: {
			// stub
			if (structRef.structRef.asCoroutineArg.coroutine->curContext) {
				ArgRecord &argRecord = structRef.structRef.asCoroutineArg.coroutine->curMajorFrame->resumable->argStack.at(structRef.structRef.asArg.argIndex);

				SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), argRecord.type, value, result));
				if (!result) {
					return raiseMismatchedVarTypeError((Runtime *)this);
				}

				argRecord.value = value;
			} else {
				ArgRecord &argRecord = structRef.structRef.asCoroutineArg.coroutine->resumable->argStack.at(structRef.structRef.asArg.argIndex);

				SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), argRecord.type, value, result));
				if (!result) {
					return raiseMismatchedVarTypeError((Runtime *)this);
				}

				argRecord.value = value;
			}
			break;
		}
		default:
			std::terminate();
	}

	return {};
}
