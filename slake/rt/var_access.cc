#include "../runtime.h"

using namespace slake;

SLAKE_API InternalExceptionPointer Runtime::tryAccessVar(const EntityRef &entityRef) const noexcept {
	switch (entityRef.kind) {
		case ObjectRefKind::FieldRef: {
			FieldRecord &fieldRecord = entityRef.asField.moduleObject->fieldRecords.at(entityRef.asField.index);

			break;
		}
		case ObjectRefKind::LocalVarRef: {
			break;
		}
		case ObjectRefKind::CoroutineLocalVarRef: {
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
			ArgRecord &argRecord = entityRef.asArg.majorFrame->resumable->argStack.at(entityRef.asArg.argIndex);
			break;
		}
		case ObjectRefKind::CoroutineArgRef: {
			if (entityRef.asCoroutineArg.coroutine->curContext) {
				ArgRecord &argRecord = entityRef.asCoroutineArg.coroutine->curMajorFrame->resumable->argStack.at(entityRef.asArg.argIndex);
			} else {
				ArgRecord &argRecord = entityRef.asCoroutineArg.coroutine->resumable->argStack.at(entityRef.asArg.argIndex);
			}
			break;
		}
		default:
			std::terminate();
	}

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::typeofVar(const EntityRef &entityRef, TypeRef &typeOut) const noexcept {
	switch (entityRef.kind) {
		case ObjectRefKind::FieldRef: {
			FieldRecord &fieldRecord = entityRef.asField.moduleObject->fieldRecords.at(entityRef.asField.index);

			typeOut = fieldRecord.type;
			break;
		}
		case ObjectRefKind::LocalVarRef: {
			const char *const rawStackPtr = entityRef.asLocalVar.context->dataStack + SLAKE_STACK_MAX - entityRef.asLocalVar.stackOff;
			memcpy(
				&typeOut,
				rawStackPtr,
				sizeof(TypeRef));
			break;
		}
		case ObjectRefKind::CoroutineLocalVarRef: {
			if (entityRef.asCoroutineArg.coroutine->curContext) {
				const char *const rawStackPtr =
					calcStackAddr(entityRef.asCoroutineArg.coroutine->curContext->dataStack,
						SLAKE_STACK_MAX,
						entityRef.asCoroutineArg.coroutine->curMajorFrame->stackBase + entityRef.asCoroutineLocalVar.stackOff);
				memcpy(
					&typeOut,
					rawStackPtr,
					sizeof(TypeRef));
			} else {
				const char *const rawStackPtr =
					calcStackAddr(entityRef.asCoroutineLocalVar.coroutine->stackData,
						entityRef.asCoroutineLocalVar.coroutine->lenStackData,
						entityRef.asCoroutineLocalVar.stackOff);
				memcpy(
					&typeOut,
					rawStackPtr,
					sizeof(TypeRef));
			}
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
			ArgRecord &argRecord = entityRef.asArg.majorFrame->resumable->argStack.at(entityRef.asArg.argIndex);

			typeOut = argRecord.type;
			break;
		}
		case ObjectRefKind::CoroutineArgRef: {
			if (entityRef.asCoroutineArg.coroutine->curContext) {
				ArgRecord &argRecord = entityRef.asCoroutineArg.coroutine->curMajorFrame->resumable->argStack.at(entityRef.asArg.argIndex);

				typeOut = argRecord.type;
			} else {
				ArgRecord &argRecord = entityRef.asCoroutineArg.coroutine->resumable->argStack.at(entityRef.asArg.argIndex);

				typeOut = argRecord.type;
			}
			break;
		}
		default:
			std::terminate();
	}

	return {};
}

#undef new

SLAKE_API InternalExceptionPointer Runtime::readVar(const EntityRef &entityRef, Value &valueOut) const noexcept {
	SLAKE_RETURN_IF_EXCEPT(tryAccessVar(entityRef));

	new (&valueOut) Value(readVarUnsafe(entityRef));
	return {};
}

SLAKE_API Value Runtime::readVarUnsafe(const EntityRef &entityRef) const noexcept {
	switch (entityRef.kind) {
		case ObjectRefKind::FieldRef: {
			FieldRecord &fieldRecord = entityRef.asField.moduleObject->fieldRecords.at(entityRef.asField.index);

			const char *const rawDataPtr = entityRef.asField.moduleObject->localFieldStorage.data() + fieldRecord.offset;

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
		case ObjectRefKind::LocalVarRef: {
			const char *const rawStackPtr = calcStackAddr(entityRef.asLocalVar.context->dataStack,
				SLAKE_STACK_MAX,
				entityRef.asLocalVar.stackOff);
			const char *const rawDataPtr = rawStackPtr + sizeof(TypeId);

			TypeId typeId = *(TypeId *)rawStackPtr;

			switch (typeId) {
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
		case ObjectRefKind::CoroutineLocalVarRef: {
			char *basePtr;

			if (entityRef.asCoroutineLocalVar.coroutine->curContext) {
				basePtr = calcStackAddr(entityRef.asCoroutineLocalVar.coroutine->curContext->dataStack,
					SLAKE_STACK_MAX,
					entityRef.asCoroutineLocalVar.stackOff + entityRef.asCoroutineLocalVar.coroutine->curMajorFrame->stackBase);
			} else {
				basePtr = calcStackAddr(entityRef.asCoroutineLocalVar.coroutine->stackData,
					entityRef.asCoroutineLocalVar.coroutine->lenStackData,
					entityRef.asCoroutineLocalVar.stackOff);
			}

			const char *const rawDataPtr = basePtr + sizeof(TypeId);

			TypeId typeId = *(TypeId *)basePtr;

			switch (typeId) {
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
				case TypeId::Fn:
					return Value(EntityRef::makeObjectRef(*((Object **)rawFieldPtr)));
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
				case TypeId::Instance:
				case TypeId::String:
				case TypeId::Array:
				case TypeId::Fn:
					return Value(EntityRef::makeObjectRef(((Object **)entityRef.asArray.arrayObject->data)[entityRef.asArray.index]));
				case TypeId::Ref:
					return Value(((EntityRef *)entityRef.asArray.arrayObject->data)[entityRef.asArray.index]);
				case TypeId::Any:
					return Value(((Value *)entityRef.asArray.arrayObject->data)[entityRef.asArray.index]);
				default:
					std::terminate();
			}
			break;
		}
		case ObjectRefKind::ArgRef: {
			const ArgRecord &argRecord = entityRef.asArg.majorFrame->resumable->argStack.at(entityRef.asArg.argIndex);

			return argRecord.value;
		}
		case ObjectRefKind::CoroutineArgRef: {
			if (entityRef.asCoroutineArg.coroutine->curContext) {
				const ArgRecord &argRecord = entityRef.asCoroutineArg.coroutine->resumable->argStack.at(entityRef.asArg.argIndex);

				return argRecord.value;
			} else {
				const ArgRecord &argRecord = entityRef.asCoroutineArg.coroutine->curMajorFrame->resumable->argStack.at(entityRef.asArg.argIndex);

				return argRecord.value;
			}
		}
		default:;
	}
	std::terminate();
}

SLAKE_API InternalExceptionPointer Runtime::writeVar(const EntityRef &entityRef, const Value &value) const noexcept {
	bool result;

	switch (entityRef.kind) {
		case ObjectRefKind::FieldRef: {
			if (entityRef.asField.index >= entityRef.asField.moduleObject->fieldRecords.size())
				// TODO: Use a proper type of exception instead of this.
				return raiseInvalidArrayIndexError(entityRef.asField.moduleObject->associatedRuntime, entityRef.asArray.index);

			const FieldRecord &fieldRecord =
				entityRef.asField.moduleObject->fieldRecords.at(entityRef.asField.index);

			SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), fieldRecord.type, value, result));
			if (!result) {
				return raiseMismatchedVarTypeError(entityRef.asField.moduleObject->associatedRuntime);
			}

			char *const rawDataPtr = entityRef.asField.moduleObject->localFieldStorage.data() + fieldRecord.offset;

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
					std::terminate();
			}

			break;
		}
		case ObjectRefKind::LocalVarRef: {
			if (entityRef.asLocalVar.stackOff >= SLAKE_STACK_MAX)
				// TODO: Use a proper type of exception instead of this.
				return raiseInvalidArrayIndexError((Runtime *)this, entityRef.asArray.index);

			const char *const rawStackPtr = calcStackAddr(entityRef.asLocalVar.context->dataStack,
				SLAKE_STACK_MAX,
				entityRef.asLocalVar.stackOff);
			const char *const rawDataPtr = rawStackPtr + sizeof(TypeId);

			TypeRef t = *(TypeId *)*rawStackPtr;

			switch (t.typeId) {
				case TypeId::I8:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((int8_t *)rawDataPtr) = value.getI8();
					break;
				case TypeId::I16:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((int16_t *)rawDataPtr) = value.getI16();
					break;
				case TypeId::I32:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((int32_t *)rawDataPtr) = value.getI32();
					break;
				case TypeId::I64:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((int64_t *)rawDataPtr) = value.getI64();
					break;
				case TypeId::U8:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((uint8_t *)rawDataPtr) = value.getU8();
					break;
				case TypeId::U16:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((uint16_t *)rawDataPtr) = value.getU16();
					break;
				case TypeId::U32:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((uint32_t *)rawDataPtr) = value.getU32();
					break;
				case TypeId::U64:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((uint64_t *)rawDataPtr) = value.getU64();
					break;
				case TypeId::F32:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((float *)rawDataPtr) = value.getF32();
					break;
				case TypeId::F64:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((double *)rawDataPtr) = value.getF64();
					break;
				case TypeId::Bool:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((bool *)rawDataPtr) = value.getBool();
					break;
				case TypeId::String:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((Object **)rawDataPtr) = value.getEntityRef().asObject.instanceObject;
					break;
				case TypeId::Instance:
				case TypeId::Array:
					memcpy(&t.typeDef, rawStackPtr - sizeof(void *), sizeof(void *));
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((Object **)rawDataPtr) = value.getEntityRef().asObject.instanceObject;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			assert(readVarUnsafe(entityRef) == value);

			break;
		}
		case ObjectRefKind::CoroutineLocalVarRef: {
			char *basePtr;

			if (entityRef.asCoroutineLocalVar.coroutine->curContext) {
				basePtr = calcStackAddr(entityRef.asCoroutineLocalVar.coroutine->curContext->dataStack,
					SLAKE_STACK_MAX,
					entityRef.asCoroutineLocalVar.stackOff + entityRef.asCoroutineLocalVar.coroutine->curMajorFrame->stackBase);
			} else {
				basePtr = calcStackAddr(entityRef.asCoroutineLocalVar.coroutine->stackData,
					entityRef.asCoroutineLocalVar.coroutine->lenStackData,
					entityRef.asCoroutineLocalVar.stackOff);
			}

			const char *const rawDataPtr = basePtr + sizeof(TypeId);

			TypeRef t;

			t.typeId = *(TypeId *)basePtr;

			switch (t.typeId) {
				case TypeId::I8:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((int8_t *)rawDataPtr) = value.getI8();
					break;
				case TypeId::I16:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((int16_t *)rawDataPtr) = value.getI16();
					break;
				case TypeId::I32:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((int32_t *)rawDataPtr) = value.getI32();
					break;
				case TypeId::I64:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((int64_t *)rawDataPtr) = value.getI64();
					break;
				case TypeId::U8:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((uint8_t *)rawDataPtr) = value.getU8();
					break;
				case TypeId::U16:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((uint16_t *)rawDataPtr) = value.getU16();
					break;
				case TypeId::U32:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((uint32_t *)rawDataPtr) = value.getU32();
					break;
				case TypeId::U64:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((uint64_t *)rawDataPtr) = value.getU64();
					break;
				case TypeId::F32:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((float *)rawDataPtr) = value.getF32();
					break;
				case TypeId::F64:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((double *)rawDataPtr) = value.getF64();
					break;
				case TypeId::Bool:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((bool *)rawDataPtr) = value.getBool();
					break;
				case TypeId::String:
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((Object **)rawDataPtr) = value.getEntityRef().asObject.instanceObject;
					break;
				case TypeId::Instance:
				case TypeId::Array:
					memcpy(&t.typeDef, basePtr - sizeof(void *), sizeof(void *));
					SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), t, value, result));
					if (!result) {
						return raiseMismatchedVarTypeError((Runtime *)this);
					}
					*((Object **)rawDataPtr) = value.getEntityRef().asObject.instanceObject;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}
			break;
		}
		case ObjectRefKind::InstanceFieldRef: {
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
					*((Object **)rawFieldPtr) = value.getEntityRef().asObject.instanceObject;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}
			break;
		}
		case ObjectRefKind::ArrayElementRef: {
			if (entityRef.asArray.index > entityRef.asArray.arrayObject->length) {
				return raiseInvalidArrayIndexError((Runtime *)this, entityRef.asArray.index);
			}

			SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), entityRef.asArray.arrayObject->elementType, value, result));
			if (!result) {
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
			ArgRecord &argRecord = entityRef.asArg.majorFrame->resumable->argStack.at(entityRef.asArg.argIndex);

			SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), argRecord.type, value, result));
			if (!result) {
				return raiseMismatchedVarTypeError((Runtime *)this);
			}

			argRecord.value = value;
			break;
		}
		case ObjectRefKind::CoroutineArgRef: {
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
		default:
			std::terminate();
	}

	return {};
}
