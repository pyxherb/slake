#include "../runtime.h"

using namespace slake;

SLAKE_FORCEINLINE static char *calcCoroutineLocalVarRefStackBasePtr(const CoroutineLocalVarRef &localVarRef) noexcept {
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
SLAKE_FORCEINLINE static char *calcLocalVarRefStackBasePtr(const LocalVarRef &localVarRef) noexcept {
	return calcStackAddr(localVarRef.context->dataStack,
		localVarRef.context->stackSize,
		localVarRef.stackOff);
}
SLAKE_FORCEINLINE static char *calcLocalVarRefStackRawDataPtr(char *p) noexcept {
	return p +
		   sizeof(TypeId) + sizeof(TypeModifier);
}
SLAKE_FORCEINLINE static const char *calcLocalVarRefStackRawDataPtr(const char *p) noexcept {
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
			const char *rawDataPtr = calcLocalVarRefStackRawDataPtr(calcLocalVarRefStackBasePtr(entityRef.asLocalVar));

			const TypeId typeId = *(TypeId *)(rawDataPtr - (sizeof(TypeModifier) + sizeof(TypeId)));

			switch (typeId) {
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

			return (void *)rawDataPtr;
		}
		case ReferenceKind::CoroutineLocalVarRef: {
			const char *rawDataPtr = calcLocalVarRefStackRawDataPtr(calcCoroutineLocalVarRefStackBasePtr(entityRef.asCoroutineLocalVar));

			const TypeId typeId = *(TypeId *)(rawDataPtr - (sizeof(TypeModifier) + sizeof(TypeId)));

			switch (typeId) {
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
				default:
					break;
			}

			return (void *)rawDataPtr;
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
			return locateValueBasePtr(extractStructInnerRef(entityRef.asStruct.structRef, entityRef.asStruct.innerReferenceKind));
		case ReferenceKind::StructFieldRef: {
			Reference innerRef = extractStructInnerRef(entityRef.asStructField.structRef, entityRef.asStructField.innerReferenceKind);
			TypeRef actualType = typeofVar(innerRef);

			Object *const typeObject = ((CustomTypeDefObject *)actualType.typeDef)->typeObject;
			char *basePtr = (char *)locateValueBasePtr(innerRef);

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
			const char *const rawDataPtr = calcLocalVarRefStackRawDataPtr(calcLocalVarRefStackBasePtr(entityRef.asLocalVar));

			TypeRef t = TypeRef(*(TypeId *)(rawDataPtr - (sizeof(TypeModifier) + sizeof(TypeId))), *(TypeModifier *)(rawDataPtr - sizeof(TypeModifier)));

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
			const char *const rawDataPtr = calcLocalVarRefStackRawDataPtr(calcCoroutineLocalVarRefStackBasePtr(entityRef.asCoroutineLocalVar));

			TypeRef t = TypeRef(*(TypeId *)(rawDataPtr - (sizeof(TypeModifier) + sizeof(TypeId))), *(TypeModifier *)(rawDataPtr - sizeof(TypeModifier)));

			switch (t.typeId) {
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
				default:
					break;
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
			const ArgRecord &argRecord = entityRef.asArg.majorFrame->resumableContextData->argStack.at(entityRef.asArg.argIndex);

			return argRecord.type;
		}
		case ReferenceKind::CoroutineArgRef: {
			if (entityRef.asCoroutineArg.coroutine->curContext) {
				const ArgRecord &argRecord = entityRef.asCoroutineArg.coroutine->resumable->argStack.at(entityRef.asArg.argIndex);

				return argRecord.type;
			} else {
				const ArgRecord &argRecord = entityRef.asCoroutineArg.coroutine->curMajorFrame->resumableContextData->argStack.at(entityRef.asArg.argIndex);

				return argRecord.type;
			}
			break;
		}
		case ReferenceKind::StructRef:
			return typeofVar(extractStructInnerRef(entityRef.asStruct.structRef, entityRef.asStruct.innerReferenceKind));
		case ReferenceKind::StructFieldRef: {
			TypeRef actualType = typeofVar(extractStructInnerRef(entityRef.asStructField.structRef, entityRef.asStructField.innerReferenceKind));

			Object *const typeObject = ((CustomTypeDefObject *)actualType.typeDef)->typeObject;

			assert(typeObject->getObjectKind() == ObjectKind::Struct);

			return ((StructObject *)typeObject)->fieldRecords.at(entityRef.asStructField.idxField).type;
		}
		default:
			break;
	}
	std::terminate();
}

SLAKE_API void Runtime::readVar(const Reference &entityRef, Value &valueOut) const noexcept {
	switch (entityRef.kind) {
		case ReferenceKind::StaticFieldRef: {
			const char *const rawDataPtr = (char *)locateValueBasePtr(entityRef);

		staticFieldRefRead:
			TypeRef t = typeofVar(entityRef);
			switch (t.typeId) {
				case TypeId::I8:
					valueOut.data.asI8 = (*((int8_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::I8;
					break;
				case TypeId::I16:
					valueOut.data.asI16 = (*((int16_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::I16;
					break;
				case TypeId::I32:
					valueOut.data.asI32 = (*((int32_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::I32;
					break;
				case TypeId::I64:
					valueOut.data.asI64 = (*((int64_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::I64;
					break;
				case TypeId::ISize:
					valueOut.data.asISize = *((ssize_t *)(rawDataPtr));
					valueOut.valueType = ValueType::ISize;
					break;
				case TypeId::U8:
					valueOut.data.asU8 = (*((uint8_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::U8;
					break;
				case TypeId::U16:
					valueOut.data.asU16 = (*((uint16_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::U16;
					break;
				case TypeId::U32:
					valueOut.data.asU32 = (*((uint32_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::U32;
					break;
				case TypeId::U64:
					valueOut.data.asU64 = (*((uint64_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::U64;
					break;
				case TypeId::USize:
					valueOut.data.asUSize = *((size_t *)(rawDataPtr));
					valueOut.valueType = ValueType::USize;
					break;
				case TypeId::F32:
					valueOut.data.asF32 = (*((float *)(rawDataPtr)));
					valueOut.valueType = ValueType::F32;
					break;
				case TypeId::F64:
					valueOut.data.asF64 = (*((double *)(rawDataPtr)));
					valueOut.valueType = ValueType::F64;
					break;
				case TypeId::Bool:
					valueOut.data.asBool = (*((bool *)(rawDataPtr)));
					valueOut.valueType = ValueType::Bool;
					break;
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					valueOut.data.asReference = (Reference::makeObjectRef(*((Object **)(rawDataPtr))));
					valueOut.valueType = ValueType::Reference;
					break;
				case TypeId::StructInstance: {
					StructRefData structRef;
					structRef.innerReference.asStaticField = entityRef.asStaticField;

					valueOut.data.asReference = (Reference::makeStructRef(structRef, ReferenceKind::StaticFieldRef));
					valueOut.valueType = ValueType::Reference;
					break;
				}
				case TypeId::ScopedEnum: {
					CustomTypeDefObject *td = (CustomTypeDefObject *)t.typeDef;
					assert(td->typeObject->getObjectKind() == ObjectKind::ScopedEnum);

					if ((t = ((ScopedEnumObject *)td->typeObject)->baseType))
						goto staticFieldRefRead;
					break;
				}
				case TypeId::TypelessScopedEnum:
					valueOut.data.asTypelessScopedEnum.type = t;
					valueOut.data.asTypelessScopedEnum.value = (*((uint32_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::TypelessScopedEnum;
					break;
				case TypeId::Ref:
					valueOut.data.asReference = (*((Reference *)(rawDataPtr)));
					valueOut.valueType = ValueType::Reference;
					break;
				case TypeId::Any:
					valueOut = (*((Value *)(rawDataPtr)));
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			if (t.isLocal())
				std::terminate();

			break;
		}
		case ReferenceKind::LocalVarRef: {
			const char *const rawDataPtr = (char *)locateValueBasePtr(entityRef);

			const TypeRef t = typeofVar(entityRef);

			switch (t.typeId) {
				case TypeId::I8:
					valueOut.data.asI8 = (*((int8_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::I8;
					break;
				case TypeId::I16:
					valueOut.data.asI16 = (*((int16_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::I16;
					break;
				case TypeId::I32:
					valueOut.data.asI32 = (*((int32_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::I32;
					break;
				case TypeId::I64:
					valueOut.data.asI64 = (*((int64_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::I64;
					break;
				case TypeId::ISize:
					valueOut.data.asISize = *((ssize_t *)(rawDataPtr));
					valueOut.valueType = ValueType::ISize;
					break;
				case TypeId::U8:
					valueOut.data.asU8 = (*((uint8_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::U8;
					break;
				case TypeId::U16:
					valueOut.data.asU16 = (*((uint16_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::U16;
					break;
				case TypeId::U32:
					valueOut.data.asU32 = (*((uint32_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::U32;
					break;
				case TypeId::U64:
					valueOut.data.asU64 = (*((uint64_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::U64;
					break;
				case TypeId::USize:
					valueOut.data.asUSize = *((size_t *)(rawDataPtr));
					valueOut.valueType = ValueType::USize;
					break;
				case TypeId::F32:
					valueOut.data.asF32 = (*((float *)(rawDataPtr)));
					valueOut.valueType = ValueType::F32;
					break;
				case TypeId::F64:
					valueOut.data.asF64 = (*((double *)(rawDataPtr)));
					valueOut.valueType = ValueType::F64;
					break;
				case TypeId::Bool:
					valueOut.data.asBool = (*((bool *)(rawDataPtr)));
					valueOut.valueType = ValueType::Bool;
					break;
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					valueOut.data.asReference = (Reference::makeObjectRef(*((Object **)(rawDataPtr))));
					valueOut.valueType = ValueType::Reference;
					break;
				case TypeId::StructInstance: {
					StructRefData structRef;
					structRef.innerReference.asLocalVar = entityRef.asLocalVar;

					valueOut.data.asReference = (Reference::makeStructRef(structRef, ReferenceKind::LocalVarRef));
					valueOut.valueType = ValueType::Reference;
					break;
				}
				case TypeId::Ref:
					valueOut.data.asReference = (*((Reference *)(rawDataPtr)));
					valueOut.valueType = ValueType::Reference;
					break;
				case TypeId::Any:
					valueOut = (*((Value *)(rawDataPtr)));
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			if (t.isLocal())
				valueOut.setLocal();

			break;
		}
		case ReferenceKind::CoroutineLocalVarRef: {
			const char *const rawDataPtr = (char *)locateValueBasePtr(entityRef);

			const TypeRef t = typeofVar(entityRef);

			switch (t.typeId) {
				case TypeId::I8:
					valueOut.data.asI8 = (*((int8_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::I8;
					break;
				case TypeId::I16:
					valueOut.data.asI16 = (*((int16_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::I16;
					break;
				case TypeId::I32:
					valueOut.data.asI32 = (*((int32_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::I32;
					break;
				case TypeId::I64:
					valueOut.data.asI64 = (*((int64_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::I64;
					break;
				case TypeId::ISize:
					valueOut.data.asISize = *((ssize_t *)(rawDataPtr));
					valueOut.valueType = ValueType::ISize;
					break;
				case TypeId::U8:
					valueOut.data.asU8 = (*((uint8_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::U8;
					break;
				case TypeId::U16:
					valueOut.data.asU16 = (*((uint16_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::U16;
					break;
				case TypeId::U32:
					valueOut.data.asU32 = (*((uint32_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::U32;
					break;
				case TypeId::U64:
					valueOut.data.asU64 = (*((uint64_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::U64;
					break;
				case TypeId::USize:
					valueOut.data.asUSize = *((size_t *)(rawDataPtr));
					valueOut.valueType = ValueType::USize;
					break;
				case TypeId::F32:
					valueOut.data.asF32 = (*((float *)(rawDataPtr)));
					valueOut.valueType = ValueType::F32;
					break;
				case TypeId::F64:
					valueOut.data.asF64 = (*((double *)(rawDataPtr)));
					valueOut.valueType = ValueType::F64;
					break;
				case TypeId::Bool:
					valueOut.data.asBool = (*((bool *)(rawDataPtr)));
					valueOut.valueType = ValueType::Bool;
					break;
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					valueOut.data.asReference = (Reference::makeObjectRef(*((Object **)(rawDataPtr))));
					valueOut.valueType = ValueType::Reference;
					break;
				case TypeId::StructInstance: {
					StructRefData structRef;
					structRef.innerReference.asCoroutineLocalVar = entityRef.asCoroutineLocalVar;

					valueOut.data.asReference = (Reference::makeStructRef(structRef, ReferenceKind::CoroutineLocalVarRef));
					valueOut.valueType = ValueType::Reference;
					break;
				}
				case TypeId::Ref:
					valueOut.data.asReference = (*((Reference *)(rawDataPtr)));
					valueOut.valueType = ValueType::Reference;
					break;
				case TypeId::Any:
					valueOut = (*((Value *)(rawDataPtr)));
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			if (t.isLocal())
				valueOut.setLocal();

			break;
		}
		case ReferenceKind::InstanceFieldRef: {
			const char *const rawDataPtr = (char *)locateValueBasePtr(entityRef);

			TypeRef t = typeofVar(entityRef);
			switch (t.typeId) {
				case TypeId::I8:
					valueOut.data.asI8 = (*((int8_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::I8;
					break;
				case TypeId::I16:
					valueOut.data.asI16 = (*((int16_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::I16;
					break;
				case TypeId::I32:
					valueOut.data.asI32 = (*((int32_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::I32;
					break;
				case TypeId::I64:
					valueOut.data.asI64 = (*((int64_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::I64;
					break;
				case TypeId::ISize:
					valueOut.data.asISize = *((ssize_t *)(rawDataPtr));
					valueOut.valueType = ValueType::ISize;
					break;
				case TypeId::U8:
					valueOut.data.asU8 = (*((uint8_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::U8;
					break;
				case TypeId::U16:
					valueOut.data.asU16 = (*((uint16_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::U16;
					break;
				case TypeId::U32:
					valueOut.data.asU32 = (*((uint32_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::U32;
					break;
				case TypeId::U64:
					valueOut.data.asU64 = (*((uint64_t *)(rawDataPtr)));
					valueOut.valueType = ValueType::U64;
					break;
				case TypeId::USize:
					valueOut.data.asUSize = *((size_t *)(rawDataPtr));
					valueOut.valueType = ValueType::USize;
					break;
				case TypeId::F32:
					valueOut.data.asF32 = (*((float *)(rawDataPtr)));
					valueOut.valueType = ValueType::F32;
					break;
				case TypeId::F64:
					valueOut.data.asF64 = (*((double *)(rawDataPtr)));
					valueOut.valueType = ValueType::F64;
					break;
				case TypeId::Bool:
					valueOut.data.asBool = (*((bool *)(rawDataPtr)));
					valueOut.valueType = ValueType::Bool;
					break;
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					valueOut.data.asReference = (Reference::makeObjectRef(*((Object **)(rawDataPtr))));
					valueOut.valueType = ValueType::Reference;
					break;
				case TypeId::StructInstance: {
					StructRefData structRef;
					structRef.innerReference.asObjectField = entityRef.asObjectField;

					valueOut.data.asReference = (Reference::makeStructRef(structRef, ReferenceKind::InstanceFieldRef));
					valueOut.valueType = ValueType::Reference;
					break;
				}
				case TypeId::Ref:
					valueOut.data.asReference = (*((Reference *)(rawDataPtr)));
					valueOut.valueType = ValueType::Reference;
					break;
				case TypeId::Any:
					valueOut = (*((Value *)(rawDataPtr)));
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			if (t.isLocal())
				std::terminate();

			break;
		}
		case ReferenceKind::ArrayElementRef: {
			TypeRef t = entityRef.asArrayElement.arrayObject->elementType;
			assert(entityRef.asArrayElement.index < entityRef.asArrayElement.arrayObject->length);

			switch (t.typeId) {
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

			if (t.isLocal())
				std::terminate();
			break;
		}
		case ReferenceKind::ArgRef: {
			const ArgRecord &argRecord = entityRef.asArg.majorFrame->resumableContextData->argStack.at(entityRef.asArg.argIndex);

			valueOut = argRecord.value;

			if (argRecord.type.isLocal())
				valueOut.setLocal();
			break;
		}
		case ReferenceKind::CoroutineArgRef: {
			if (entityRef.asCoroutineArg.coroutine->curContext) {
				const ArgRecord &argRecord = entityRef.asCoroutineArg.coroutine->resumable->argStack.at(entityRef.asArg.argIndex);

				valueOut = argRecord.value;

				if (argRecord.type.isLocal())
					valueOut.setLocal();
			} else {
				const ArgRecord &argRecord = entityRef.asCoroutineArg.coroutine->curMajorFrame->resumableContextData->argStack.at(entityRef.asArg.argIndex);

				valueOut = argRecord.value;

				if (argRecord.type.isLocal())
					valueOut.setLocal();
			}
			break;
		}
		case ReferenceKind::StructFieldRef: {
			const char *rawDataPtr = ((char *)locateValueBasePtr(extractStructInnerRef(entityRef.asStructField.structRef, entityRef.asStructField.innerReferenceKind)));
			const TypeRef t = typeofVar(entityRef);

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

			if (t.isLocal())
				std::terminate();
			break;
		}
		default:
			std::terminate();
	}
}

SLAKE_API void Runtime::writeVar(const Reference &entityRef, const Value &value) const noexcept {
	switch (entityRef.kind) {
		case ReferenceKind::StaticFieldRef: {
			char *const rawDataPtr = (char *)locateValueBasePtr(entityRef);

			const TypeRef t = typeofVar(entityRef);

			if (t.isLocal())
				std::terminate();
			if (value.isLocal())
				std::terminate();

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
				case TypeId::ISize:
					*((slake::ssize_t *)rawDataPtr) = value.getISize();
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
				case TypeId::USize:
					*((size_t *)rawDataPtr) = value.getUSize();
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

			const TypeRef t = typeofVar(entityRef);

			if (value.isLocal() && !t.isLocal())
				std::terminate();

			switch (t.typeId) {
				case TypeId::I8:
					*((int8_t *)(rawDataPtr)) = value.getI8();
					break;
				case TypeId::I16:
					*((int16_t *)(rawDataPtr)) = value.getI16();
					break;
				case TypeId::I32:
					*((int32_t *)(rawDataPtr)) = value.getI32();
					break;
				case TypeId::I64:
					*((int64_t *)(rawDataPtr)) = value.getI64();
					break;
				case TypeId::U8:
					*((uint8_t *)(rawDataPtr)) = value.getU8();
					break;
				case TypeId::U16:
					*((uint16_t *)(rawDataPtr)) = value.getU16();
					break;
				case TypeId::U32:
					*((uint32_t *)(rawDataPtr)) = value.getU32();
					break;
				case TypeId::U64:
					*((uint64_t *)(rawDataPtr)) = value.getU64();
					break;
				case TypeId::F32:
					*((float *)(rawDataPtr)) = value.getF32();
					break;
				case TypeId::F64:
					*((double *)(rawDataPtr)) = value.getF64();
					break;
				case TypeId::Bool:
					*((bool *)(rawDataPtr)) = value.getBool();
					break;
				case TypeId::String:
					*((Object **)(rawDataPtr)) = value.getReference().asObject;
					break;
				case TypeId::Instance:
				case TypeId::Array:
					*((Object **)(rawDataPtr)) = value.getReference().asObject;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case ReferenceKind::CoroutineLocalVarRef: {
			char *const rawDataPtr = (char *)locateValueBasePtr(entityRef);

			const TypeRef t = typeofVar(entityRef);

			if (value.isLocal() && !t.isLocal())
				std::terminate();

			switch (t.typeId) {
				case TypeId::I8:
					*((int8_t *)(rawDataPtr)) = value.getI8();
					break;
				case TypeId::I16:
					*((int16_t *)(rawDataPtr)) = value.getI16();
					break;
				case TypeId::I32:
					*((int32_t *)(rawDataPtr)) = value.getI32();
					break;
				case TypeId::I64:
					*((int64_t *)(rawDataPtr)) = value.getI64();
					break;
				case TypeId::U8:
					*((uint8_t *)(rawDataPtr)) = value.getU8();
					break;
				case TypeId::U16:
					*((uint16_t *)(rawDataPtr)) = value.getU16();
					break;
				case TypeId::U32:
					*((uint32_t *)(rawDataPtr)) = value.getU32();
					break;
				case TypeId::U64:
					*((uint64_t *)(rawDataPtr)) = value.getU64();
					break;
				case TypeId::F32:
					*((float *)(rawDataPtr)) = value.getF32();
					break;
				case TypeId::F64:
					*((double *)(rawDataPtr)) = value.getF64();
					break;
				case TypeId::Bool:
					*((bool *)(rawDataPtr)) = value.getBool();
					break;
				case TypeId::String:
					*((Object **)(rawDataPtr)) = value.getReference().asObject;
					break;
				case TypeId::Instance:
				case TypeId::Array:
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
			const TypeRef t = typeofVar(entityRef);

			if (t.isLocal())
				std::terminate();
			if (value.isLocal())
				std::terminate();

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
			const TypeRef t = typeofVar(entityRef);

			if (t.isLocal())
				std::terminate();
			if (value.isLocal())
				std::terminate();

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
			ArgRecord &argRecord = const_cast<MajorFrame *>(entityRef.asArg.majorFrame)->resumableContextData->argStack.at(entityRef.asArg.argIndex);

			if (value.isLocal() && !argRecord.type.isLocal())
				std::terminate();
			argRecord.value = value;
			break;
		}
		case ReferenceKind::CoroutineArgRef: {
			if (entityRef.asCoroutineArg.coroutine->curContext) {
				ArgRecord &argRecord = entityRef.asCoroutineArg.coroutine->curMajorFrame->resumableContextData->argStack.at(entityRef.asArg.argIndex);

				if (value.isLocal() && !argRecord.type.isLocal())
					std::terminate();
				argRecord.value = value;
			} else {
				ArgRecord &argRecord = entityRef.asCoroutineArg.coroutine->resumable->argStack.at(entityRef.asArg.argIndex);

				if (value.isLocal() && !argRecord.type.isLocal())
					std::terminate();
				argRecord.value = value;
			}
			break;
		}
		case ReferenceKind::StructFieldRef: {
			const char *rawDataPtr = (char *)locateValueBasePtr(entityRef);
			const TypeRef t = typeofVar(entityRef);

			if (t.isLocal())
				std::terminate();
			if (value.isLocal())
				std::terminate();

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
}
