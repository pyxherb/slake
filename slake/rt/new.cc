#include <slake/runtime.h>
#include <slake/util/scope_guard.h>

using namespace slake;

SLAKE_API InternalExceptionPointer Runtime::initMethodTableForClass(ClassObject *cls, ClassObject *parentClass) {
	assert(!cls->cachedInstantiatedMethodTable);
	MethodTable *parentMt = parentClass ? parentClass->cachedInstantiatedMethodTable : nullptr;
	std::unique_ptr<MethodTable, util::DeallocableDeleter<MethodTable>> methodTable(MethodTable::alloc(cls->selfAllocator.get()));

	if (parentMt) {
		if (!methodTable->destructors.resize(parentMt->destructors.size())) {
			return OutOfMemoryError::alloc();
		}
		memcpy(methodTable->destructors.data(), parentMt->destructors.data(), methodTable->destructors.size() * sizeof(void *));
	}

	for (auto it = cls->members.begin(); it != cls->members.end(); ++it) {
		switch (it.value()->getObjectKind()) {
			case ObjectKind::Fn: {
				FnObject *fn = (FnObject *)it.value();

				HostObjectRef<FnObject> fnSlot;

				fnSlot = FnObject::alloc(this);
				if (!fnSlot) {
					return OutOfMemoryError::alloc();
				}
				if (!fnSlot->name.build(((FnObject *)it.value())->name)) {
					return OutOfMemoryError::alloc();
				}

				if (it.key() == "delete") {
					peff::DynArray<TypeRef> destructorParamTypes(getFixedAlloc());
					GenericParamList destructorGenericParamList(getFixedAlloc());

					for (auto j : fn->overloadings) {
						bool result;
						SLAKE_RETURN_IF_EXCEPT(isDuplicatedOverloading(getFixedAlloc(), j, destructorParamTypes, destructorGenericParamList, false, result));
						if (result) {
							if (!methodTable->destructors.pushFront(+j)) {
								return OutOfMemoryError::alloc();
							}
							break;
						}
					}
				} else {
					for (auto j : fn->overloadings) {
						if (!fnSlot->overloadings.insert(+j))
							return OutOfMemoryError::alloc();
					}

					if (parentMt) {
						if (auto m = parentMt->getMethod(fn->name); m) {
							if (m->overloadings.size()) {
								// Link the method with method inherited from the parent.

								for (auto j : fnSlot->overloadings) {
									for (auto k : m->overloadings) {
										// If we found a non-duplicated overloading from the parent, add it.
										bool result;
										SLAKE_RETURN_IF_EXCEPT(isDuplicatedOverloading(
											getFixedAlloc(),
											k,
											j->paramTypes,
											j->genericParams,
											j->overloadingFlags & OL_VARG,
											result));
										if (!result) {
											if (!fnSlot->overloadings.insert(+k))
												return OutOfMemoryError::alloc();
										}
									}
								}
							}
						}
					}
				}

				if (fnSlot->overloadings.size()) {
					if (!methodTable->methods.insert(fnSlot->name, fnSlot.get()))
						return OutOfMemoryError::alloc();
				}

				break;
			}
		}
	}

	cls->cachedInstantiatedMethodTable = methodTable.release();

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::initObjectLayoutForClass(ClassObject *cls, ClassObject *parentClass) {
	assert(!cls->cachedObjectLayout);
	std::unique_ptr<ObjectLayout, util::DeallocableDeleter<ObjectLayout>> objectLayout;

	if (parentClass && parentClass->cachedObjectLayout) {
		objectLayout = decltype(objectLayout)(parentClass->cachedObjectLayout->duplicate(getCurGenAlloc()));

		if (!cls->cachedFieldInitValues.resize(parentClass->cachedFieldInitValues.size())) {
			return OutOfMemoryError::alloc();
		}

		memcpy(cls->cachedFieldInitValues.data(), parentClass->cachedFieldInitValues.data(), sizeof(Value) * parentClass->cachedFieldInitValues.size());
	} else {
		objectLayout = decltype(objectLayout)(ObjectLayout::alloc(cls->selfAllocator.get()));
		cls->cachedFieldInitValues = peff::DynArray<Value>(cls->selfAllocator.get());
	}

	for (size_t i = 0; i < cls->fieldRecords.size(); ++i) {
		FieldRecord &clsFieldRecord = cls->fieldRecords.at(i);

		if (clsFieldRecord.accessModifier & ACCESS_STATIC) {
			continue;
		}

		ObjectFieldRecord fieldRecord(cls->selfAllocator.get());

		TypeRef type = clsFieldRecord.type;

		size_t size = sizeofType(type);
		size_t align = alignofType(type);

		if (align > 1) {
			if (size_t diff = objectLayout->totalSize % align; diff) {
				objectLayout->totalSize += align - diff;
			}
		}
		fieldRecord.offset = objectLayout->totalSize;

		fieldRecord.type = type;
		if (!fieldRecord.name.build(clsFieldRecord.name)) {
			// stub
			std::terminate();
		}

		if (!cls->cachedFieldInitValues.pushBack(readVarUnsafe(EntityRef::makeFieldRef(cls, i))))
			return OutOfMemoryError::alloc();

		if (!objectLayout->fieldNameMap.insert(fieldRecord.name, objectLayout->fieldRecords.size()))
			return OutOfMemoryError::alloc();
		if (!objectLayout->fieldRecords.pushBack(std::move(fieldRecord)))
			return OutOfMemoryError::alloc();

		objectLayout->totalSize += size;
	}

	// cls->cachedFieldInitVars.shrink_to_fit();
	cls->cachedObjectLayout = objectLayout.release();

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::prepareClassForInstantiation(ClassObject *cls) {
	ClassObject *p = nullptr;

	if (cls->baseType.typeId != TypeId::Void) {
		if (cls->baseType.typeId != TypeId::Instance)
			return allocOutOfMemoryErrorIfAllocFailed(MalformedClassStructureError::alloc(getFixedAlloc(), cls));

		Object *parentClass = ((CustomTypeDefObject*)(ClassObject *)cls->baseType.typeDef)->typeObject;
		if (parentClass->getObjectKind() != ObjectKind::Class)
			return allocOutOfMemoryErrorIfAllocFailed(MalformedClassStructureError::alloc(getFixedAlloc(), cls));

		SLAKE_RETURN_IF_EXCEPT(prepareClassForInstantiation((ClassObject *)parentClass));
		p = (ClassObject *)parentClass;
	}

	if (!cls->cachedObjectLayout)
		SLAKE_RETURN_IF_EXCEPT(initObjectLayoutForClass(cls, p));
	if (!cls->cachedInstantiatedMethodTable)
		SLAKE_RETURN_IF_EXCEPT(initMethodTableForClass(cls, p));

	return {};
}

/// @brief Create a new class instance.
/// @param cls Class for instance creation.
/// @return Created instance of the class.
///
/// @note This function normalizes loading-deferred types.
SLAKE_API HostObjectRef<InstanceObject> slake::Runtime::newClassInstance(ClassObject *cls, NewClassInstanceFlags flags) {
	HostObjectRef<InstanceObject> instance;
	InternalExceptionPointer e;

	if ((e = prepareClassForInstantiation(cls))) {
		e.reset();
		return {};
	}

	instance = InstanceObject::alloc(this);

	if (cls->cachedObjectLayout->totalSize)
		instance->rawFieldData = new char[cls->cachedObjectLayout->totalSize];
	instance->szRawFieldData = cls->cachedObjectLayout->totalSize;

	instance->_class = cls;

	//
	// Initialize the fields.
	//
	for (size_t i = 0; i < cls->cachedFieldInitValues.size(); ++i) {
		ObjectFieldRecord &fieldRecord = cls->cachedObjectLayout->fieldRecords.at(i);

		Value data = cls->cachedFieldInitValues.at(i);
		SLAKE_UNWRAP_EXCEPT(writeVar(EntityRef::makeInstanceFieldRef(instance.get(), i), data));
	}

	return instance;
}

SLAKE_API HostObjectRef<ArrayObject> Runtime::newArrayInstance(Runtime *rt, const TypeRef &type, size_t length) {
	switch (type.typeId) {
		case TypeId::I8: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(int8_t));
			if (!(obj->data = obj->selfAllocator->alloc(sizeof(int8_t) * length, alignof(int8_t))))
				return nullptr;
			obj->elementAlignment = alignof(int8_t);
			obj->length = length;
			return obj.get();
		}
		case TypeId::I16: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(int16_t));
			if (!(obj->data = obj->selfAllocator->alloc(sizeof(int16_t) * length, alignof(int16_t))))
				return nullptr;
			obj->elementAlignment = alignof(int16_t);
			obj->length = length;
			return obj.get();
		}
		case TypeId::I32: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(int32_t));
			if (!(obj->data = obj->selfAllocator->alloc(sizeof(int32_t) * length, alignof(int32_t))))
				return nullptr;
			obj->elementAlignment = alignof(int32_t);
			obj->length = length;
			return obj.get();
		}
		case TypeId::I64: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(int64_t));
			if (!(obj->data = obj->selfAllocator->alloc(sizeof(int64_t) * length, alignof(int64_t))))
				return nullptr;
			obj->elementAlignment = alignof(int64_t);
			obj->length = length;
			return obj.get();
		}
		case TypeId::U8: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(uint8_t));
			if (!(obj->data = obj->selfAllocator->alloc(sizeof(uint8_t) * length, alignof(uint8_t))))
				return nullptr;
			obj->elementAlignment = alignof(uint8_t);
			obj->length = length;
			return obj.get();
		}
		case TypeId::U16: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(uint16_t));
			if (!(obj->data = obj->selfAllocator->alloc(sizeof(uint16_t) * length, alignof(uint16_t))))
				return nullptr;
			obj->elementAlignment = alignof(uint16_t);
			obj->length = length;
			return obj.get();
		}
		case TypeId::U32: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(uint32_t));
			if (!(obj->data = obj->selfAllocator->alloc(sizeof(uint32_t) * length, alignof(uint32_t))))
				return nullptr;
			obj->elementAlignment = alignof(uint32_t);
			obj->length = length;
			return obj.get();
		}
		case TypeId::U64: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(uint64_t));
			if (!(obj->data = obj->selfAllocator->alloc(sizeof(uint64_t) * length, alignof(uint64_t))))
				return nullptr;
			obj->elementAlignment = alignof(uint64_t);
			obj->length = length;
			return obj.get();
		}
		case TypeId::F32: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(float));
			if (!(obj->data = obj->selfAllocator->alloc(sizeof(float) * length, alignof(float))))
				return nullptr;
			obj->elementAlignment = alignof(float);
			obj->length = length;
			return obj.get();
		}
		case TypeId::F64: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(double));
			if (!(obj->data = obj->selfAllocator->alloc(sizeof(double) * length, alignof(double))))
				return nullptr;
			obj->elementAlignment = alignof(double);
			obj->length = length;
			return obj.get();
		}
		case TypeId::Bool: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(bool));
			if (!(obj->data = obj->selfAllocator->alloc(sizeof(bool) * length, alignof(bool))))
				return nullptr;
			obj->elementAlignment = alignof(bool);
			obj->length = length;
			return obj.get();
		}
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(EntityRef));
			if (!(obj->data = obj->selfAllocator->alloc(sizeof(EntityRef) * length, alignof(EntityRef))))
				return nullptr;
			obj->elementAlignment = alignof(EntityRef);
			obj->length = length;
			return obj.get();
		}
		case TypeId::Any: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(Value));
			if (!(obj->data = obj->selfAllocator->alloc(sizeof(Value) * length, alignof(Value))))
				return nullptr;
			obj->elementAlignment = alignof(Value);
			obj->length = length;
			return obj.get();
		}
		default:
			throw std::logic_error("Unhandled element type");
	}
}
