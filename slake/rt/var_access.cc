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
		   (sizeof(TypeId) + sizeof(TypeModifier));
}
SLAKE_FORCEINLINE static const char *calcLocalVarRefStackRawDataPtr(const char *p) noexcept {
	return p +
		   (sizeof(TypeId) + sizeof(TypeModifier));
}

SLAKE_API void *Runtime::locateValueBasePtr(const Reference &entityRef) noexcept {
	switch (entityRef.kind) {
		case ReferenceKind::StaticFieldRef: {
			FieldRecord &fieldRecord = entityRef.asStaticField.moduleObject->fieldRecords.at(entityRef.asStaticField.index);

			return entityRef.asStaticField.moduleObject->localFieldStorage.data() + fieldRecord.offset;
		}
		case ReferenceKind::LocalVarRef: {
			const char *rawDataPtr = calcLocalVarRefStackRawDataPtr(calcLocalVarRefStackBasePtr(entityRef.asLocalVar));

			switch (*reinterpret_cast<const TypeId *>(rawDataPtr - (sizeof(TypeModifier) + sizeof(TypeId)))) {
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

			switch (*reinterpret_cast<const TypeId *>(rawDataPtr - (sizeof(TypeModifier) + sizeof(TypeId)))) {
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
		case ReferenceKind::ObjectFieldRef: {
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
		case ReferenceKind::StaticFieldStructFieldRef:
		case ReferenceKind::LocalVarStructFieldRef:
		case ReferenceKind::CoroutineLocalVarStructFieldRef:
		case ReferenceKind::ObjectFieldStructFieldRef:
		case ReferenceKind::ArrayElementStructFieldRef:
		case ReferenceKind::ArgStructFieldRef:
		case ReferenceKind::CoroutineArgStructFieldRef: {
			Reference innerRef = entityRef;
			((uint8_t &)innerRef.kind) &= ~0x80;
			TypeRef actualType = typeofVar(innerRef);

			Object *const typeObject = ((CustomTypeDefObject *)actualType.typeDef)->typeObject;
			char *basePtr = (char *)locateValueBasePtr(innerRef);

			assert(typeObject->getObjectKind() == ObjectKind::Struct);

			return basePtr + ((StructObject *)typeObject)->fieldRecords.at(entityRef.structFieldIndex).offset;
		}
		default:
			break;
	}

	std::terminate();
}

SLAKE_API TypeRef Runtime::typeofVar(const Reference &entityRef) noexcept {
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
		case ReferenceKind::ObjectFieldRef: {
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
			auto overloading = entityRef.asArg.majorFrame->curFn;

			if (entityRef.asArg.argIndex >= overloading->paramTypes.size()) {
				assert(overloading->overloadingFlags & OL_VARG);
				return TypeId::Any;
			}

			return overloading->paramTypes.at(entityRef.asArg.argIndex);
		}
		case ReferenceKind::CoroutineArgRef: {
			auto coroutine = entityRef.asCoroutineArg.coroutine;
			if (coroutine->curContext) {
				auto overloading = coroutine->boundMajorFrame->curFn;

				if (entityRef.asCoroutineArg.argIndex >= overloading->paramTypes.size()) {
					assert(overloading->overloadingFlags & OL_VARG);
					return TypeId::Any;
				}

				return overloading->paramTypes.at(entityRef.asCoroutineArg.argIndex);
			} else {
				auto overloading = coroutine->overloading;

				if (entityRef.asCoroutineArg.argIndex >= overloading->paramTypes.size()) {
					assert(overloading->overloadingFlags & OL_VARG);
					return TypeId::Any;
				}

				return overloading->paramTypes.at(entityRef.asCoroutineArg.argIndex);
			}
			break;
		}
		case ReferenceKind::StaticFieldStructFieldRef:
		case ReferenceKind::LocalVarStructFieldRef:
		case ReferenceKind::CoroutineLocalVarStructFieldRef:
		case ReferenceKind::ObjectFieldStructFieldRef:
		case ReferenceKind::ArrayElementStructFieldRef:
		case ReferenceKind::ArgStructFieldRef:
		case ReferenceKind::CoroutineArgStructFieldRef: {
			Reference innerRef = entityRef;
			((uint8_t &)innerRef.kind) &= ~0x80;
			TypeRef actualType = typeofVar(innerRef);

			Object *const typeObject = ((CustomTypeDefObject *)actualType.typeDef)->typeObject;

			assert(typeObject->getObjectKind() == ObjectKind::Struct);

			return ((StructObject *)typeObject)->fieldRecords.at(entityRef.structFieldIndex).type;
		}
		default:
			break;
	}
	std::terminate();
}

SLAKE_API void Runtime::readVarWithType(const Reference &entityRef, const TypeRef &t, Value &valueOut) noexcept {
	valueOut.valueFlags = 0;
	switch (entityRef.kind) {
		case ReferenceKind::StaticFieldRef:
		case ReferenceKind::ObjectFieldRef:
		case ReferenceKind::StaticFieldStructFieldRef:
		case ReferenceKind::LocalVarStructFieldRef:
		case ReferenceKind::CoroutineLocalVarStructFieldRef:
		case ReferenceKind::ObjectFieldStructFieldRef:
		case ReferenceKind::ArrayElementStructFieldRef:
		case ReferenceKind::ArgStructFieldRef:
		case ReferenceKind::CoroutineArgStructFieldRef: {
			const char *const rawDataPtr = (char *)locateValueBasePtr(entityRef);

			switch (t.typeId) {
				case TypeId::I8:
					if (t.isNullable())
						if (*(bool *)((rawDataPtr + sizeof(int8_t)))) {
							valueOut = nullptr;
							break;
						}
					valueOut.asI8 = (*(reinterpret_cast<const int8_t *>(rawDataPtr)));
					valueOut.valueType = ValueType::I8;
					break;
				case TypeId::I16:
					if (t.isNullable())
						if (*(bool *)((rawDataPtr + sizeof(int16_t)))) {
							valueOut = nullptr;
							break;
						}
					valueOut.asI16 = (*(reinterpret_cast<const int16_t *>(rawDataPtr)));
					valueOut.valueType = ValueType::I16;
					break;
				case TypeId::I32:
					if (t.isNullable())
						if (*(bool *)((rawDataPtr + sizeof(int32_t)))) {
							valueOut = nullptr;
							break;
						}
					valueOut.asI32 = (*(reinterpret_cast<const int32_t *>(rawDataPtr)));
					valueOut.valueType = ValueType::I32;
					break;
				case TypeId::I64:
					if (t.isNullable())
						if (*(bool *)((rawDataPtr + sizeof(int64_t)))) {
							valueOut = nullptr;
							break;
						}
					valueOut.asI64 = (*(reinterpret_cast<const int64_t *>(rawDataPtr)));
					valueOut.valueType = ValueType::I64;
					break;
				case TypeId::ISize:
					if (t.isNullable())
						if (*(bool *)((rawDataPtr + sizeof(ssize_t)))) {
							valueOut = nullptr;
							break;
						}
					valueOut.asISize = *(reinterpret_cast<const ssize_t *>(rawDataPtr));
					valueOut.valueType = ValueType::ISize;
					break;
				case TypeId::U8:
					if (t.isNullable())
						if (*(bool *)((rawDataPtr + sizeof(uint8_t)))) {
							valueOut = nullptr;
							break;
						}
					valueOut.asU8 = (*(reinterpret_cast<const uint8_t *>(rawDataPtr)));
					valueOut.valueType = ValueType::U8;
					break;
				case TypeId::U16:
					if (t.isNullable())
						if (*(bool *)((rawDataPtr + sizeof(uint16_t)))) {
							valueOut = nullptr;
							break;
						}
					valueOut.asU16 = (*(reinterpret_cast<const uint16_t *>(rawDataPtr)));
					valueOut.valueType = ValueType::U16;
					break;
				case TypeId::U32:
					if (t.isNullable())
						if (*(bool *)((rawDataPtr + sizeof(uint32_t)))) {
							valueOut = nullptr;
							break;
						}
					valueOut.asU32 = (*(reinterpret_cast<const uint32_t *>(rawDataPtr)));
					valueOut.valueType = ValueType::U32;
					break;
				case TypeId::U64:
					if (t.isNullable())
						if (*(bool *)((rawDataPtr + sizeof(uint64_t)))) {
							valueOut = nullptr;
							break;
						}
					valueOut.asU64 = (*(reinterpret_cast<const uint64_t *>(rawDataPtr)));
					valueOut.valueType = ValueType::U64;
					break;
				case TypeId::USize:
					if (t.isNullable())
						if (*(bool *)((rawDataPtr + sizeof(size_t)))) {
							valueOut = nullptr;
							break;
						}
					valueOut.asUSize = *(reinterpret_cast<const size_t *>(rawDataPtr));
					valueOut.valueType = ValueType::USize;
					break;
				case TypeId::F32:
					if (t.isNullable())
						if (*(bool *)((rawDataPtr + sizeof(float)))) {
							valueOut = nullptr;
							break;
						}
					valueOut.asF32 = (*(reinterpret_cast<const float *>(rawDataPtr)));
					valueOut.valueType = ValueType::F32;
					break;
				case TypeId::F64:
					if (t.isNullable())
						if (*(bool *)((rawDataPtr + sizeof(double)))) {
							valueOut = nullptr;
							break;
						}
					valueOut.asF64 = (*(reinterpret_cast<const double *>(rawDataPtr)));
					valueOut.valueType = ValueType::F64;
					break;
				case TypeId::Bool:
					if (t.isNullable())
						if (*(bool *)((rawDataPtr + sizeof(bool)))) {
							valueOut = nullptr;
							break;
						}
					valueOut.asBool = (*(reinterpret_cast<const bool *>(rawDataPtr)));
					valueOut.valueType = ValueType::Bool;
					break;
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					valueOut.asReference = (Reference(*((Object **)(rawDataPtr))));
					valueOut.valueType = ValueType::Reference;
					if (t.isLocal())
						std::terminate();
					break;
				case TypeId::StructInstance: {
					if (t.isNullable())
						if (*(bool *)((rawDataPtr + sizeofType(t)))) {
							valueOut = nullptr;
							break;
						}

					valueOut.asReference = entityRef;
					valueOut.valueType = ValueType::Reference;
					break;
				}
				case TypeId::ScopedEnum: {
					CustomTypeDefObject *td = (CustomTypeDefObject *)t.typeDef;
					assert(td->typeObject->getObjectKind() == ObjectKind::ScopedEnum);

					TypeRef type;
					if ((type = ((ScopedEnumObject *)td->typeObject)->baseType))
						switch (type.typeId) {
							case TypeId::I8:
								if (t.isNullable())
									if (*(bool *)((rawDataPtr + sizeof(int8_t)))) {
										valueOut = nullptr;
										break;
									}
								valueOut.asI8 = (*(reinterpret_cast<const int8_t *>(rawDataPtr)));
								valueOut.valueType = ValueType::I8;
								break;
							case TypeId::I16:
								if (t.isNullable())
									if (*(bool *)((rawDataPtr + sizeof(int16_t)))) {
										valueOut = nullptr;
										break;
									}
								valueOut.asI16 = (*(reinterpret_cast<const int16_t *>(rawDataPtr)));
								valueOut.valueType = ValueType::I16;
								break;
							case TypeId::I32:
								if (t.isNullable())
									if (*(bool *)((rawDataPtr + sizeof(int32_t)))) {
										valueOut = nullptr;
										break;
									}
								valueOut.asI32 = (*(reinterpret_cast<const int32_t *>(rawDataPtr)));
								valueOut.valueType = ValueType::I32;
								break;
							case TypeId::I64:
								if (t.isNullable())
									if (*(bool *)((rawDataPtr + sizeof(int64_t)))) {
										valueOut = nullptr;
										break;
									}
								valueOut.asI64 = (*(reinterpret_cast<const int64_t *>(rawDataPtr)));
								valueOut.valueType = ValueType::I64;
								break;
							case TypeId::ISize:
								if (t.isNullable())
									if (*(bool *)((rawDataPtr + sizeof(ssize_t)))) {
										valueOut = nullptr;
										break;
									}
								valueOut.asISize = *(reinterpret_cast<const ssize_t *>(rawDataPtr));
								valueOut.valueType = ValueType::ISize;
								break;
							case TypeId::U8:
								if (t.isNullable())
									if (*(bool *)((rawDataPtr + sizeof(uint8_t)))) {
										valueOut = nullptr;
										break;
									}
								valueOut.asU8 = (*(reinterpret_cast<const uint8_t *>(rawDataPtr)));
								valueOut.valueType = ValueType::U8;
								break;
							case TypeId::U16:
								if (t.isNullable())
									if (*(bool *)((rawDataPtr + sizeof(uint16_t)))) {
										valueOut = nullptr;
										break;
									}
								valueOut.asU16 = (*(reinterpret_cast<const uint16_t *>(rawDataPtr)));
								valueOut.valueType = ValueType::U16;
								break;
							case TypeId::U32:
								if (t.isNullable())
									if (*(bool *)((rawDataPtr + sizeof(uint32_t)))) {
										valueOut = nullptr;
										break;
									}
								valueOut.asU32 = (*(reinterpret_cast<const uint32_t *>(rawDataPtr)));
								valueOut.valueType = ValueType::U32;
								break;
							case TypeId::U64:
								if (t.isNullable())
									if (*(bool *)((rawDataPtr + sizeof(uint64_t)))) {
										valueOut = nullptr;
										break;
									}
								valueOut.asU64 = (*(reinterpret_cast<const uint64_t *>(rawDataPtr)));
								valueOut.valueType = ValueType::U64;
								break;
							case TypeId::USize:
								if (t.isNullable())
									if (*(bool *)((rawDataPtr + sizeof(size_t)))) {
										valueOut = nullptr;
										break;
									}
								valueOut.asUSize = *(reinterpret_cast<const size_t *>(rawDataPtr));
								valueOut.valueType = ValueType::USize;
								break;
							case TypeId::F32:
								if (t.isNullable())
									if (*(bool *)((rawDataPtr + sizeof(float)))) {
										valueOut = nullptr;
										break;
									}
								valueOut.asF32 = (*(reinterpret_cast<const float *>(rawDataPtr)));
								valueOut.valueType = ValueType::F32;
								break;
							case TypeId::F64:
								if (t.isNullable())
									if (*(bool *)((rawDataPtr + sizeof(double)))) {
										valueOut = nullptr;
										break;
									}
								valueOut.asF64 = (*(reinterpret_cast<const double *>(rawDataPtr)));
								valueOut.valueType = ValueType::F64;
								break;
							case TypeId::Bool:
								if (t.isNullable())
									if (*(bool *)((rawDataPtr + sizeof(bool)))) {
										valueOut = nullptr;
										break;
									}
								valueOut.asBool = (*(reinterpret_cast<const bool *>(rawDataPtr)));
								valueOut.valueType = ValueType::Bool;
								break;
							default:
								std::terminate();
						}
					break;
				}
				case TypeId::TypelessScopedEnum:
					if (t.isNullable())
						if (*(bool *)((rawDataPtr + sizeof(uint32_t)))) {
							valueOut = nullptr;
							break;
						}
					valueOut.asTypelessScopedEnum.type = t;
					valueOut.asTypelessScopedEnum.value = (*(reinterpret_cast<const uint32_t *>(rawDataPtr)));
					valueOut.valueType = ValueType::TypelessScopedEnum;
					break;
				case TypeId::Ref:
					valueOut.asReference = (*(reinterpret_cast<const Reference *>(rawDataPtr)));
					valueOut.valueType = ValueType::Reference;
					if (t.isLocal())
						std::terminate();
					break;
				case TypeId::Any:
					valueOut = (*(reinterpret_cast<const Value *>(rawDataPtr)));
					if (t.isLocal())
						std::terminate();
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case ReferenceKind::LocalVarRef: {
			const char *rawDataPtr = (char *)locateValueBasePtr(entityRef);

			switch (t.typeId) {
				case TypeId::I8:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asI8 = (*(reinterpret_cast<const int8_t *>(rawDataPtr)));
					valueOut.valueType = ValueType::I8;
					break;
				case TypeId::I16:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asI16 = (*(reinterpret_cast<const int16_t *>(rawDataPtr)));
					valueOut.valueType = ValueType::I16;
					break;
				case TypeId::I32:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asI32 = (*(reinterpret_cast<const int32_t *>(rawDataPtr)));
					valueOut.valueType = ValueType::I32;
					break;
				case TypeId::I64:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asI64 = (*(reinterpret_cast<const int64_t *>(rawDataPtr)));
					valueOut.valueType = ValueType::I64;
					break;
				case TypeId::ISize:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asISize = *(reinterpret_cast<const ssize_t *>(rawDataPtr));
					valueOut.valueType = ValueType::ISize;
					break;
				case TypeId::U8:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asU8 = (*(reinterpret_cast<const uint8_t *>(rawDataPtr)));
					valueOut.valueType = ValueType::U8;
					break;
				case TypeId::U16:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asU16 = (*(reinterpret_cast<const uint16_t *>(rawDataPtr)));
					valueOut.valueType = ValueType::U16;
					break;
				case TypeId::U32:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asU32 = (*(reinterpret_cast<const uint32_t *>(rawDataPtr)));
					valueOut.valueType = ValueType::U32;
					break;
				case TypeId::U64:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asU64 = (*(reinterpret_cast<const uint64_t *>(rawDataPtr)));
					valueOut.valueType = ValueType::U64;
					break;
				case TypeId::USize:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asUSize = *(reinterpret_cast<const size_t *>(rawDataPtr));
					valueOut.valueType = ValueType::USize;
					break;
				case TypeId::F32:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asF32 = (*(reinterpret_cast<const float *>(rawDataPtr)));
					valueOut.valueType = ValueType::F32;
					break;
				case TypeId::F64:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asF64 = (*(reinterpret_cast<const double *>(rawDataPtr)));
					valueOut.valueType = ValueType::F64;
					break;
				case TypeId::Bool:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asBool = (*(reinterpret_cast<const bool *>(rawDataPtr)));
					valueOut.valueType = ValueType::Bool;
					break;
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					valueOut.asReference = (Reference(*((Object **)(rawDataPtr))));
					valueOut.valueType = ValueType::Reference;
					if (t.isLocal())
						valueOut.setLocal();
					break;
				case TypeId::StructInstance: {
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}

					valueOut.asReference = entityRef;
					valueOut.valueType = ValueType::Reference;
					if (t.isLocal())
						valueOut.setLocal();
					break;
				}
				case TypeId::Ref:
					valueOut.asReference = (*(reinterpret_cast<const Reference *>(rawDataPtr)));
					valueOut.valueType = ValueType::Reference;
					if (t.isLocal())
						valueOut.setLocal();
					break;
				case TypeId::Any:
					valueOut = (*(reinterpret_cast<const Value *>(rawDataPtr)));
					if (t.isLocal())
						valueOut.setLocal();
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case ReferenceKind::CoroutineLocalVarRef: {
			const char *rawDataPtr = (char *)locateValueBasePtr(entityRef);

			switch (t.typeId) {
				case TypeId::I8:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asI8 = (*(reinterpret_cast<const int8_t *>(rawDataPtr)));
					valueOut.valueType = ValueType::I8;
					break;
				case TypeId::I16:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asI16 = (*(reinterpret_cast<const int16_t *>(rawDataPtr)));
					valueOut.valueType = ValueType::I16;
					break;
				case TypeId::I32:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asI32 = (*(reinterpret_cast<const int32_t *>(rawDataPtr)));
					valueOut.valueType = ValueType::I32;
					break;
				case TypeId::I64:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asI64 = (*(reinterpret_cast<const int64_t *>(rawDataPtr)));
					valueOut.valueType = ValueType::I64;
					break;
				case TypeId::ISize:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asISize = *(reinterpret_cast<const ssize_t *>(rawDataPtr));
					valueOut.valueType = ValueType::ISize;
					break;
				case TypeId::U8:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asU8 = (*(reinterpret_cast<const uint8_t *>(rawDataPtr)));
					valueOut.valueType = ValueType::U8;
					break;
				case TypeId::U16:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asU16 = (*(reinterpret_cast<const uint16_t *>(rawDataPtr)));
					valueOut.valueType = ValueType::U16;
					break;
				case TypeId::U32:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asU32 = (*(reinterpret_cast<const uint32_t *>(rawDataPtr)));
					valueOut.valueType = ValueType::U32;
					break;
				case TypeId::U64:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asU64 = (*(reinterpret_cast<const uint64_t *>(rawDataPtr)));
					valueOut.valueType = ValueType::U64;
					break;
				case TypeId::USize:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asUSize = *(reinterpret_cast<const size_t *>(rawDataPtr));
					valueOut.valueType = ValueType::USize;
					break;
				case TypeId::F32:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asF32 = (*(reinterpret_cast<const float *>(rawDataPtr)));
					valueOut.valueType = ValueType::F32;
					break;
				case TypeId::F64:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asF64 = (*(reinterpret_cast<const double *>(rawDataPtr)));
					valueOut.valueType = ValueType::F64;
					break;
				case TypeId::Bool:
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}
					valueOut.asBool = (*(reinterpret_cast<const bool *>(rawDataPtr)));
					valueOut.valueType = ValueType::Bool;
					break;
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					valueOut.asReference = (Reference(*((Object **)(rawDataPtr))));
					valueOut.valueType = ValueType::Reference;
					if (t.isLocal())
						valueOut.setLocal();
					break;
				case TypeId::StructInstance: {
					if (t.isNullable()) {
						if (*((bool *)rawDataPtr)) {
							valueOut = nullptr;
							break;
						}
						rawDataPtr += sizeof(bool);
					}

					valueOut.asReference = entityRef;
					valueOut.valueType = ValueType::Reference;
					if (t.isLocal())
						valueOut.setLocal();
					break;
				}
				case TypeId::Ref:
					valueOut.asReference = (*(reinterpret_cast<const Reference *>(rawDataPtr)));
					valueOut.valueType = ValueType::Reference;
					if (t.isLocal())
						valueOut.setLocal();
					break;
				case TypeId::Any:
					valueOut = (*(reinterpret_cast<const Value *>(rawDataPtr)));
					if (t.isLocal())
						valueOut.setLocal();
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case ReferenceKind::ArrayElementRef: {
			assert(entityRef.asArrayElement.index < entityRef.asArrayElement.arrayObject->length);

			if (t.isNullable())
				// TODO: Handle it.
				std::terminate();
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
					valueOut = (Reference(((Object **)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]));
					if (t.isLocal())
						std::terminate();
					break;
				case TypeId::StructInstance:
					valueOut = entityRef;
					if (t.isLocal())
						std::terminate();
					break;
				case TypeId::Ref:
					valueOut = (((Reference *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
					if (t.isLocal())
						std::terminate();
					break;
				case TypeId::Any:
					valueOut = (((Value *)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index]);
					if (t.isLocal())
						std::terminate();
					break;
				default:
					std::terminate();
			}
			break;
		}
		case ReferenceKind::ArgRef: {
			valueOut = _fetchArgStack(
				entityRef.asArg.majorFrame->curContext->getContext().dataStack,
				entityRef.asArg.majorFrame->curContext->getContext().stackSize,
				entityRef.asArg.majorFrame,
				entityRef.asArg.majorFrame->resumableContextData.offArgs)[entityRef.asArg.argIndex];

			if (t.isLocal())
				valueOut.setLocal();
			break;
		}
		case ReferenceKind::CoroutineArgRef: {
			if (entityRef.asCoroutineArg.coroutine->curContext) {
				MajorFrame *mf = _fetchMajorFrame(entityRef.asCoroutineArg.coroutine->curContext, entityRef.asCoroutineArg.coroutine->curContext->offCurMajorFrame);
				valueOut = _fetchArgStack(
					entityRef.asCoroutineArg.coroutine->curContext->dataStack,
					entityRef.asCoroutineArg.coroutine->curContext->stackSize,
					mf,
					mf->resumableContextData.offArgs)[entityRef.asCoroutineArg.argIndex];

				if (t.isLocal())
					valueOut.setLocal();
			} else {
				// TODO: Implement it.
				std::terminate();

				if (t.isLocal())
					valueOut.setLocal();
			}
			break;
		}
		default:
			std::terminate();
	}
}

SLAKE_API void Runtime::writeVarWithType(const Reference &entityRef, const TypeRef &t, const Value &value) noexcept {
	switch (entityRef.kind) {
		case ReferenceKind::StaticFieldRef:
		case ReferenceKind::ObjectFieldRef:
		case ReferenceKind::StaticFieldStructFieldRef:
		case ReferenceKind::LocalVarStructFieldRef:
		case ReferenceKind::CoroutineLocalVarStructFieldRef:
		case ReferenceKind::ObjectFieldStructFieldRef:
		case ReferenceKind::ArrayElementStructFieldRef:
		case ReferenceKind::ArgStructFieldRef:
		case ReferenceKind::CoroutineArgStructFieldRef: {
			char *const rawDataPtr = (char *)locateValueBasePtr(entityRef);

			switch (t.typeId) {
				case TypeId::I8:
					if (t.isNullable()) {
						if ((*((bool *)(rawDataPtr + sizeof(int8_t))) = value.isNull()))
							break;
					}
					*((int8_t *)rawDataPtr) = value.getI8();
					break;
				case TypeId::I16:
					if (t.isNullable()) {
						if ((*((bool *)(rawDataPtr + sizeof(int16_t))) = value.isNull()))
							break;
					}
					*((int16_t *)rawDataPtr) = value.getI16();
					break;
				case TypeId::I32:
					if (t.isNullable()) {
						if ((*((bool *)(rawDataPtr + sizeof(int32_t))) = value.isNull()))
							break;
					}
					*((int32_t *)rawDataPtr) = value.getI32();
					break;
				case TypeId::I64:
					if (t.isNullable()) {
						if ((*((bool *)(rawDataPtr + sizeof(int64_t))) = value.isNull()))
							break;
					}
					*((int64_t *)rawDataPtr) = value.getI64();
					break;
				case TypeId::U8:
					if (t.isNullable()) {
						if ((*((bool *)(rawDataPtr + sizeof(uint8_t))) = value.isNull()))
							break;
					}
					*((uint8_t *)rawDataPtr) = value.getU8();
					break;
				case TypeId::U16:
					if (t.isNullable()) {
						if ((*((bool *)(rawDataPtr + sizeof(uint16_t))) = value.isNull()))
							break;
					}
					*((uint16_t *)rawDataPtr) = value.getU16();
					break;
				case TypeId::U32:
					if (t.isNullable()) {
						if ((*((bool *)(rawDataPtr + sizeof(uint32_t))) = value.isNull()))
							break;
					}
					*((uint32_t *)rawDataPtr) = value.getU32();
					break;
				case TypeId::U64:
					if (t.isNullable()) {
						if ((*((bool *)(rawDataPtr + sizeof(uint64_t))) = value.isNull()))
							break;
					}
					*((uint64_t *)rawDataPtr) = value.getU64();
					break;
				case TypeId::F32:
					if (t.isNullable()) {
						if ((*((bool *)(rawDataPtr + sizeof(float))) = value.isNull()))
							break;
					}
					*((float *)rawDataPtr) = value.getF32();
					break;
				case TypeId::F64:
					if (t.isNullable()) {
						if ((*((bool *)(rawDataPtr + sizeof(double))) = value.isNull()))
							break;
					}
					*((double *)rawDataPtr) = value.getF64();
					break;
				case TypeId::Bool:
					if (t.isNullable()) {
						if ((*((bool *)(rawDataPtr + sizeof(bool))) = value.isNull()))
							break;
					}
					*((bool *)rawDataPtr) = value.getBool();
					break;
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
					if (t.isLocal())
						std::terminate();
					if (value.isLocal())
						std::terminate();
					*((Object **)rawDataPtr) = value.getReference().asObject;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case ReferenceKind::LocalVarRef: {
			char *rawDataPtr = (char *)locateValueBasePtr(entityRef);

			if (t.isNullable()) {
				if (value.isNull()) {
					*(bool *)((rawDataPtr + sizeofType(t))) = false;
					break;
				}
			}
			switch (t.typeId) {
				case TypeId::I8:
					if (t.isNullable()) {
						if ((*((bool *)rawDataPtr) = value.isNull()))
							break;
						rawDataPtr += sizeof(bool);
					}
					*(reinterpret_cast<int8_t *>(rawDataPtr)) = value.getI8();
					break;
				case TypeId::I16:
					if (t.isNullable()) {
						if ((*((bool *)rawDataPtr) = value.isNull()))
							break;
						rawDataPtr += sizeof(bool);
					}
					*(reinterpret_cast<int16_t *>(rawDataPtr)) = value.getI16();
					break;
				case TypeId::I32:
					if (t.isNullable()) {
						if ((*((bool *)rawDataPtr) = value.isNull()))
							break;
						rawDataPtr += sizeof(bool);
					}
					*(reinterpret_cast<int32_t *>(rawDataPtr)) = value.getI32();
					break;
				case TypeId::I64:
					if (t.isNullable()) {
						if ((*((bool *)rawDataPtr) = value.isNull()))
							break;
						rawDataPtr += sizeof(bool);
					}
					*(reinterpret_cast<int64_t *>(rawDataPtr)) = value.getI64();
					break;
				case TypeId::U8:
					if (t.isNullable()) {
						if ((*((bool *)rawDataPtr) = value.isNull()))
							break;
						rawDataPtr += sizeof(bool);
					}
					*(reinterpret_cast<uint8_t *>(rawDataPtr)) = value.getU8();
					break;
				case TypeId::U16:
					if (t.isNullable()) {
						if ((*((bool *)rawDataPtr) = value.isNull()))
							break;
						rawDataPtr += sizeof(bool);
					}
					*(reinterpret_cast<uint16_t *>(rawDataPtr)) = value.getU16();
					break;
				case TypeId::U32:
					if (t.isNullable()) {
						if ((*((bool *)rawDataPtr) = value.isNull()))
							break;
						rawDataPtr += sizeof(bool);
					}
					*(reinterpret_cast<uint32_t *>(rawDataPtr)) = value.getU32();
					break;
				case TypeId::U64:
					if (t.isNullable()) {
						if ((*((bool *)rawDataPtr) = value.isNull()))
							break;
						rawDataPtr += sizeof(bool);
					}
					*(reinterpret_cast<uint64_t *>(rawDataPtr)) = value.getU64();
					break;
				case TypeId::F32:
					if (t.isNullable()) {
						if ((*((bool *)rawDataPtr) = value.isNull()))
							break;
						rawDataPtr += sizeof(bool);
					}
					*(reinterpret_cast<float *>(rawDataPtr)) = value.getF32();
					break;
				case TypeId::F64:
					if (t.isNullable()) {
						if ((*((bool *)rawDataPtr) = value.isNull()))
							break;
						rawDataPtr += sizeof(bool);
					}
					*(reinterpret_cast<double *>(rawDataPtr)) = value.getF64();
					break;
				case TypeId::Bool:
					if (t.isNullable()) {
						if ((*((bool *)rawDataPtr) = value.isNull()))
							break;
						rawDataPtr += sizeof(bool);
					}
					*(reinterpret_cast<bool *>(rawDataPtr)) = value.getBool();
					break;
				case TypeId::String:
					if (value.isLocal() && !t.isLocal())
						std::terminate();
					*((Object **)(rawDataPtr)) = value.getReference().asObject;
					break;
				case TypeId::Instance:
				case TypeId::Array:
					if (value.isLocal() && !t.isLocal())
						std::terminate();
					*((Object **)(rawDataPtr)) = value.getReference().asObject;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case ReferenceKind::CoroutineLocalVarRef: {
			char *rawDataPtr = (char *)locateValueBasePtr(entityRef);

			switch (t.typeId) {
				case TypeId::I8:
					if (t.isNullable()) {
						if ((*((bool *)rawDataPtr) = value.isNull()))
							break;
						rawDataPtr += sizeof(bool);
					}
					*(reinterpret_cast<int8_t *>(rawDataPtr)) = value.getI8();
					break;
				case TypeId::I16:
					if (t.isNullable()) {
						if ((*((bool *)rawDataPtr) = value.isNull()))
							break;
						rawDataPtr += sizeof(bool);
					}
					*(reinterpret_cast<int16_t *>(rawDataPtr)) = value.getI16();
					break;
				case TypeId::I32:
					if (t.isNullable()) {
						if ((*((bool *)rawDataPtr) = value.isNull()))
							break;
						rawDataPtr += sizeof(bool);
					}
					*(reinterpret_cast<int32_t *>(rawDataPtr)) = value.getI32();
					break;
				case TypeId::I64:
					if (t.isNullable()) {
						if ((*((bool *)rawDataPtr) = value.isNull()))
							break;
						rawDataPtr += sizeof(bool);
					}
					*(reinterpret_cast<int64_t *>(rawDataPtr)) = value.getI64();
					break;
				case TypeId::U8:
					if (t.isNullable()) {
						if ((*((bool *)rawDataPtr) = value.isNull()))
							break;
						rawDataPtr += sizeof(bool);
					}
					*(reinterpret_cast<uint8_t *>(rawDataPtr)) = value.getU8();
					break;
				case TypeId::U16:
					if (t.isNullable()) {
						if ((*((bool *)rawDataPtr) = value.isNull()))
							break;
						rawDataPtr += sizeof(bool);
					}
					*(reinterpret_cast<uint16_t *>(rawDataPtr)) = value.getU16();
					break;
				case TypeId::U32:
					if (t.isNullable()) {
						if ((*((bool *)rawDataPtr) = value.isNull()))
							break;
						rawDataPtr += sizeof(bool);
					}
					*(reinterpret_cast<uint32_t *>(rawDataPtr)) = value.getU32();
					break;
				case TypeId::U64:
					if (t.isNullable()) {
						if ((*((bool *)rawDataPtr) = value.isNull()))
							break;
						rawDataPtr += sizeof(bool);
					}
					*(reinterpret_cast<uint64_t *>(rawDataPtr)) = value.getU64();
					break;
				case TypeId::F32:
					if (t.isNullable()) {
						if ((*((bool *)rawDataPtr) = value.isNull()))
							break;
						rawDataPtr += sizeof(bool);
					}
					*(reinterpret_cast<float *>(rawDataPtr)) = value.getF32();
					break;
				case TypeId::F64:
					if (t.isNullable()) {
						if ((*((bool *)rawDataPtr) = value.isNull()))
							break;
						rawDataPtr += sizeof(bool);
					}
					*(reinterpret_cast<double *>(rawDataPtr)) = value.getF64();
					break;
				case TypeId::Bool:
					if (t.isNullable()) {
						if ((*((bool *)rawDataPtr) = value.isNull()))
							break;
						rawDataPtr += sizeof(bool);
					}
					*(reinterpret_cast<bool *>(rawDataPtr)) = value.getBool();
					break;
				case TypeId::String:
					if (value.isLocal() && !t.isLocal())
						std::terminate();
					*((Object **)(rawDataPtr)) = value.getReference().asObject;
					break;
				case TypeId::Instance:
				case TypeId::Array:
					if (value.isLocal() && !t.isLocal())
						std::terminate();
					*((Object **)(rawDataPtr)) = value.getReference().asObject;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}
			break;
		}
		case ReferenceKind::ArrayElementRef: {
			if (t.isNullable()) {
				if (value.isNull()) {
					// TODO: Handle this.
					std::terminate();
				}
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
					if (t.isLocal())
						std::terminate();
					if (value.isLocal())
						std::terminate();
					((Object **)entityRef.asArrayElement.arrayObject->data)[entityRef.asArrayElement.index] = value.getReference().asObject;
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case ReferenceKind::ArgRef: {
			if (value.isLocal() && !t.isLocal())
				std::terminate();
			_fetchArgStack(
				entityRef.asArg.majorFrame->curContext->getContext().dataStack,
				entityRef.asArg.majorFrame->curContext->getContext().stackSize,
				entityRef.asArg.majorFrame,
				entityRef.asArg.majorFrame->resumableContextData.offArgs)[entityRef.asArg.argIndex] = value;
			break;
		}
		case ReferenceKind::CoroutineArgRef: {
			if (value.isLocal() && !t.isLocal())
				std::terminate();
			if (entityRef.asCoroutineArg.coroutine->curContext) {
				MajorFrame *mf = _fetchMajorFrame(entityRef.asCoroutineArg.coroutine->curContext, entityRef.asCoroutineArg.coroutine->curContext->offCurMajorFrame);
				_fetchArgStack(
					entityRef.asCoroutineArg.coroutine->curContext->dataStack,
					entityRef.asCoroutineArg.coroutine->curContext->stackSize,
					mf,
					mf->resumableContextData.offArgs)[entityRef.asCoroutineArg.argIndex] = value;
			} else {
				// TODO: Implement it.
				std::terminate();
			}
			break;
		}
		default:
			std::terminate();
	}
}
