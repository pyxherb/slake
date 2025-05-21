#include <slake/runtime.h>
#include <slake/util/scope_guard.h>

using namespace slake;

SLAKE_API InternalExceptionPointer Runtime::initMethodTableForClass(ClassObject *cls, ClassObject *parentClass) {
	assert(!cls->cachedInstantiatedMethodTable);
	std::unique_ptr<MethodTable, util::DeallocableDeleter<MethodTable>> methodTable;

	if (parentClass && parentClass->cachedInstantiatedMethodTable) {
		methodTable = std::unique_ptr<MethodTable, util::DeallocableDeleter<MethodTable>>(
			parentClass->cachedInstantiatedMethodTable->duplicate());
	} else {
		methodTable = std::unique_ptr<MethodTable, util::DeallocableDeleter<MethodTable>>(
			MethodTable::alloc(&globalHeapPoolAlloc));
	}

	for (auto it = cls->members.begin(); it != cls->members.end(); ++it) {
		switch (it.value()->getKind()) {
			case ObjectKind::Fn: {
				std::deque<FnOverloadingObject *> overloadings;

				for (auto j : ((FnObject *)it.value())->overloadings) {
					if ((!(j->access & ACCESS_STATIC)) &&
						(j->overloadingFlags & OL_VIRTUAL))
						overloadings.push_back(j);
				}

				if (it.key() == "delete") {
					peff::DynArray<Type> destructorParamTypes(&globalHeapPoolAlloc);
					GenericParamList destructorGenericParamList(&globalHeapPoolAlloc);

					for (auto j : overloadings) {
						if (isDuplicatedOverloading(j, destructorParamTypes, destructorGenericParamList, false)) {
							if (!methodTable->destructors.pushFront(+j)) {
								return OutOfMemoryError::alloc();
							}
							break;
						}
					}
				} else {
					if (overloadings.size()) {
						// Link the method with method inherited from the parent.
						HostObjectRef<FnObject> fn;
						if (!methodTable->methods.contains(it.key())) {
							methodTable->methods.insert(it.value()->name, nullptr);
						}
						auto &methodSlot = methodTable->methods.at(it.key());

						if (auto m = methodTable->getMethod(it.key()); m)
							fn = m;
						else {
							fn = FnObject::alloc(this);
							methodSlot = fn.get();
						}

						for (auto &j : overloadings) {
							for (auto k : methodSlot->overloadings) {
								if (isDuplicatedOverloading(
										k,
										j->paramTypes,
										j->genericParams,
										j->overloadingFlags & OL_VARG)) {
									methodSlot->overloadings.remove(k);
									break;
								}
							}
							if (!methodSlot->overloadings.insert(+j))
								return OutOfMemoryError::alloc();
						}
					}
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
		objectLayout = decltype(objectLayout)(parentClass->cachedObjectLayout->duplicate());
		peff::copyAssign(cls->cachedFieldInitValues, parentClass->cachedFieldInitValues);
	} else {
		objectLayout = decltype(objectLayout)(ObjectLayout::alloc(&globalHeapPoolAlloc));
		cls->cachedFieldInitValues = peff::DynArray<Value>(&globalHeapPoolAlloc);
	}

	for (size_t i = 0; i < cls->fieldRecords.size(); ++i) {
		FieldRecord &clsFieldRecord = cls->fieldRecords.at(i);

		if (clsFieldRecord.accessModifier & ACCESS_STATIC) {
			continue;
		}

		ObjectFieldRecord fieldRecord(&globalHeapPoolAlloc);

		Type type = clsFieldRecord.type;

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

	if (cls->baseType.typeId != TypeId::None) {
		if (cls->baseType.typeId != TypeId::Instance)
			return allocOutOfMemoryErrorIfAllocFailed(MalformedClassStructureError::alloc(&globalHeapPoolAlloc, cls));

		SLAKE_RETURN_IF_EXCEPT(cls->baseType.loadDeferredType(this));

		Object *parentClass = (ClassObject *)cls->baseType.getCustomTypeExData();
		if (parentClass->getKind() != ObjectKind::Class)
			return allocOutOfMemoryErrorIfAllocFailed(MalformedClassStructureError::alloc(&globalHeapPoolAlloc, cls));

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

SLAKE_API HostObjectRef<ArrayObject> Runtime::newArrayInstance(Runtime *rt, const Type &type, size_t length) {
	switch (type.typeId) {
		case TypeId::I8: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(int8_t));
			if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(int8_t) * length, sizeof(int8_t))))
				return nullptr;
			obj->length = length;
			return obj.get();
		}
		case TypeId::I16: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(int16_t));
			if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(int16_t) * length, sizeof(int16_t))))
				return nullptr;
			obj->length = length;
			return obj.get();
		}
		case TypeId::I32: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(int32_t));
			if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(int32_t) * length, sizeof(int32_t))))
				return nullptr;
			obj->length = length;
			return obj.get();
		}
		case TypeId::I64: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(int64_t));
			if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(int64_t) * length, sizeof(int64_t))))
				return nullptr;
			obj->length = length;
			return obj.get();
		}
		case TypeId::U8: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(uint8_t));
			if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(uint8_t) * length, sizeof(uint8_t))))
				return nullptr;
			obj->length = length;
			return obj.get();
		}
		case TypeId::U16: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(uint16_t));
			if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(uint16_t) * length, sizeof(uint16_t))))
				return nullptr;
			obj->length = length;
			return obj.get();
		}
		case TypeId::U32: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(uint32_t));
			if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(uint32_t) * length, sizeof(uint32_t))))
				return nullptr;
			obj->length = length;
			return obj.get();
		}
		case TypeId::U64: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(uint64_t));
			if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(uint64_t) * length, sizeof(uint64_t))))
				return nullptr;
			obj->length = length;
			return obj.get();
		}
		case TypeId::F32: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(float));
			if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(float) * length, sizeof(float))))
				return nullptr;
			obj->length = length;
			return obj.get();
		}
		case TypeId::F64: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(double));
			if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(double) * length, sizeof(double))))
				return nullptr;
			obj->length = length;
			return obj.get();
		}
		case TypeId::Bool: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(bool));
			if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(bool) * length, sizeof(bool))))
				return nullptr;
			obj->length = length;
			return obj.get();
		}
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(EntityRef));
			if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(EntityRef) * length, sizeof(EntityRef))))
				return nullptr;
			obj->length = length;
			return obj.get();
		}
		case TypeId::Any: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(Value));
			if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(Value) * length, sizeof(std::max_align_t))))
				return nullptr;
			obj->length = length;
			return obj.get();
		}
		default:
			throw std::logic_error("Unhandled element type");
	}
}
