#include <slake/runtime.h>
#include <slake/util/scope_guard.h>

using namespace slake;

SLAKE_API void Runtime::initMethodTableForClass(ClassObject *cls, ClassObject *parentClass) {
	assert(!cls->cachedInstantiatedMethodTable);
	std::unique_ptr<MethodTable, util::DeallocableDeleter<MethodTable>> methodTable;

	if (parentClass && parentClass->cachedInstantiatedMethodTable) {
		methodTable = decltype(methodTable)(parentClass->cachedInstantiatedMethodTable->duplicate());
	} else {
		methodTable = decltype(methodTable)(MethodTable::alloc(&globalHeapPoolAlloc));
	}

	for (auto it = cls->scope->members.begin(); it != cls->scope->members.end(); ++it) {
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
					GenericParamList destructorGenericParamList;

					for (auto &j : overloadings) {
						if (isDuplicatedOverloading(j, destructorParamTypes, destructorGenericParamList, false)) {
							auto copiedJ = j;
							methodTable->destructors.pushFront(std::move(copiedJ));
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
							auto copiedJ = j;
							methodSlot->overloadings.insert(std::move(copiedJ));
						}
					}
				}

				break;
			}
		}
	}

	cls->cachedInstantiatedMethodTable = methodTable.release();
}

SLAKE_API void Runtime::initObjectLayoutForClass(ClassObject *cls, ClassObject *parentClass) {
	assert(!cls->cachedObjectLayout);
	std::unique_ptr<ObjectLayout, util::DeallocableDeleter<ObjectLayout>> objectLayout;

	if (parentClass && parentClass->cachedObjectLayout) {
		objectLayout = decltype(objectLayout)(parentClass->cachedObjectLayout->duplicate());
		peff::copyAssign(cls->cachedFieldInitVars, parentClass->cachedFieldInitVars);
	} else {
		objectLayout = decltype(objectLayout)(ObjectLayout::alloc(&globalHeapPoolAlloc));
		cls->cachedFieldInitVars = peff::DynArray<VarObject *>(&globalHeapPoolAlloc);
	}

	for (auto it = cls->scope->members.begin(); it != cls->scope->members.end(); ++it) {
		switch (it.value()->getKind()) {
			case ObjectKind::Var: {
				ObjectFieldRecord fieldRecord(&globalHeapPoolAlloc);

				RegularVarObject *var = (RegularVarObject *)it.value();
				Type type;
				SLAKE_UNWRAP_EXCEPT(typeofVar(var, VarRefContext(), type));

				switch (type.typeId) {
					case TypeId::Value:
						switch (type.getValueTypeExData()) {
							case ValueType::I8:
								fieldRecord.offset = objectLayout->totalSize;
								objectLayout->totalSize += sizeof(int8_t);
								break;
							case ValueType::I16:
								objectLayout->totalSize += (2 - (objectLayout->totalSize & 1));
								fieldRecord.offset = objectLayout->totalSize;
								objectLayout->totalSize += sizeof(int16_t);
								break;
							case ValueType::I32:
								objectLayout->totalSize += (4 - (objectLayout->totalSize & 3));
								fieldRecord.offset = objectLayout->totalSize;
								objectLayout->totalSize += sizeof(int32_t);
								break;
							case ValueType::I64:
								objectLayout->totalSize += (8 - (objectLayout->totalSize & 7));
								fieldRecord.offset = objectLayout->totalSize;
								objectLayout->totalSize += sizeof(int64_t);
								break;
							case ValueType::U8:
								fieldRecord.offset = objectLayout->totalSize;
								objectLayout->totalSize += sizeof(uint8_t);
								break;
							case ValueType::U16:
								objectLayout->totalSize += (2 - (objectLayout->totalSize & 1));
								fieldRecord.offset = objectLayout->totalSize;
								objectLayout->totalSize += sizeof(uint16_t);
								break;
							case ValueType::U32:
								objectLayout->totalSize += (4 - (objectLayout->totalSize & 3));
								fieldRecord.offset = objectLayout->totalSize;
								objectLayout->totalSize += sizeof(uint32_t);
								break;
							case ValueType::U64:
								objectLayout->totalSize += (8 - (objectLayout->totalSize & 7));
								fieldRecord.offset = objectLayout->totalSize;
								objectLayout->totalSize += sizeof(uint64_t);
								break;
							case ValueType::F32:
								objectLayout->totalSize += (4 - (objectLayout->totalSize & 3));
								fieldRecord.offset = objectLayout->totalSize;
								objectLayout->totalSize += sizeof(float);
								break;
							case ValueType::F64:
								objectLayout->totalSize += (8 - (objectLayout->totalSize & 7));
								fieldRecord.offset = objectLayout->totalSize;
								objectLayout->totalSize += sizeof(double);
								break;
							case ValueType::Bool:
								fieldRecord.offset = objectLayout->totalSize;
								objectLayout->totalSize += sizeof(bool);
								break;
							case ValueType::ObjectRef:
								objectLayout->totalSize += sizeof(Object *) - (objectLayout->totalSize & (sizeof(Object *) - 1));
								fieldRecord.offset = objectLayout->totalSize;
								objectLayout->totalSize += sizeof(Object *);
								break;
						}
						break;
					case TypeId::String:
					case TypeId::Instance:
					case TypeId::Array:
						objectLayout->totalSize += sizeof(Object *) - (objectLayout->totalSize & (sizeof(Object *) - 1));
						fieldRecord.offset = objectLayout->totalSize;
						objectLayout->totalSize += sizeof(Object *);
						break;
					default:
						throw std::runtime_error("The variable has an inconstructible type");
				}

				fieldRecord.type = type;
				if (!peff::copy(fieldRecord.name, var->name)) {
					// stub
					terminate();
				}

				cls->cachedFieldInitVars.pushBack(var);

				objectLayout->fieldNameMap.insert(fieldRecord.name, objectLayout->fieldRecords.size());
				objectLayout->fieldRecords.pushBack(std::move(fieldRecord));
				break;
			}
		}
	}

	//cls->cachedFieldInitVars.shrink_to_fit();
	cls->cachedObjectLayout = objectLayout.release();
}

SLAKE_API InternalExceptionPointer Runtime::prepareClassForInstantiation(ClassObject *cls) {
	ClassObject *p = nullptr;
	if (cls->parentClass.typeId != TypeId::None) {
		if (cls->parentClass.typeId != TypeId::Instance)
			return MalformedClassStructureError::alloc(this, cls);

		SLAKE_RETURN_IF_EXCEPT(cls->parentClass.loadDeferredType(this));

		Object *parentClass = (ClassObject *)cls->parentClass.getCustomTypeExData();
		if (parentClass->getKind() != ObjectKind::Class)
			return MalformedClassStructureError::alloc(this, cls);

		SLAKE_RETURN_IF_EXCEPT(prepareClassForInstantiation((ClassObject *)parentClass));
		p = (ClassObject *)parentClass;
	}

	if (!cls->cachedObjectLayout)
		initObjectLayoutForClass(cls, p);
	if (!cls->cachedInstantiatedMethodTable)
		initMethodTableForClass(cls, p);

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

	instance->methodTable = cls->cachedInstantiatedMethodTable;
	instance->objectLayout = cls->cachedObjectLayout;

	if (cls->cachedObjectLayout->totalSize)
		instance->rawFieldData = new char[cls->cachedObjectLayout->totalSize];
	instance->szRawFieldData = cls->cachedObjectLayout->totalSize;

	instance->_class = cls;

	//
	// Initialize the fields.
	//
	for (size_t i = 0; i < cls->cachedFieldInitVars.size(); ++i) {
		VarObject *initVar = cls->cachedFieldInitVars.at(i);

		ObjectFieldRecord &fieldRecord = cls->cachedObjectLayout->fieldRecords.at(i);

		Value data;
		data = readVarUnsafe(initVar, VarRefContext());
		SLAKE_UNWRAP_EXCEPT(writeVar(instance->memberAccessor, VarRefContext::makeInstanceContext(i), data));
	}

	return instance;
}

SLAKE_API HostObjectRef<ArrayObject> Runtime::newArrayInstance(Runtime *rt, const Type &type, size_t length) {
	switch (type.typeId) {
		case TypeId::Value:
			switch (type.getValueTypeExData()) {
				case ValueType::I8: {
					HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(int8_t));
					if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(int8_t) * length, sizeof(int8_t))))
						return nullptr;
					obj->length = length;
					return obj.get();
				}
				case ValueType::I16: {
					HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(int16_t));
					if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(int16_t) * length, sizeof(int16_t))))
						return nullptr;
					obj->length = length;
					return obj.get();
				}
				case ValueType::I32: {
					HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(int32_t));
					if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(int32_t) * length, sizeof(int32_t))))
						return nullptr;
					obj->length = length;
					return obj.get();
				}
				case ValueType::I64: {
					HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(int64_t));
					if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(int64_t) * length, sizeof(int64_t))))
						return nullptr;
					obj->length = length;
					return obj.get();
				}
				case ValueType::U8: {
					HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(uint8_t));
					if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(uint8_t) * length, sizeof(uint8_t))))
						return nullptr;
					obj->length = length;
					return obj.get();
				}
				case ValueType::U16: {
					HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(uint16_t));
					if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(uint16_t) * length, sizeof(uint16_t))))
						return nullptr;
					obj->length = length;
					return obj.get();
				}
				case ValueType::U32: {
					HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(uint32_t));
					if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(uint32_t) * length, sizeof(uint32_t))))
						return nullptr;
					obj->length = length;
					return obj.get();
				}
				case ValueType::U64: {
					HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(uint64_t));
					if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(uint64_t) * length, sizeof(uint64_t))))
						return nullptr;
					obj->length = length;
					return obj.get();
				}
				case ValueType::F32: {
					HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(float));
					if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(float) * length, sizeof(float))))
						return nullptr;
					obj->length = length;
					return obj.get();
				}
				case ValueType::F64: {
					HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(double));
					if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(double) * length, sizeof(double))))
						return nullptr;
					obj->length = length;
					return obj.get();
				}
				case ValueType::Bool: {
					HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(bool));
					if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(bool) * length, sizeof(bool))))
						return nullptr;
					obj->length = length;
					return obj.get();
				}
				default:
					throw std::logic_error("Unhandled value type");
			}
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(Object *));
			if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(Object *) * length, sizeof(Object *))))
				return nullptr;
			obj->length = length;
			return obj.get();
		}
		case TypeId::Any: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(Value));
			if (!(obj->data = globalHeapPoolAlloc.alloc(sizeof(Value) * length, sizeof(Value))))
				return nullptr;
			obj->length = length;
			return obj.get();
		}
		default:
			throw std::logic_error("Unhandled element type");
	}
}
