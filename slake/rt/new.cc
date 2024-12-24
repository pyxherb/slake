#include <slake/runtime.h>
#include <slake/util/scope_guard.h>

using namespace slake;

SLAKE_API void Runtime::initMethodTableForClass(ClassObject *cls, ClassObject *parentClass) {
	assert(!cls->cachedInstantiatedMethodTable);
	std::unique_ptr<MethodTable, util::DeallocableDeleter<MethodTable>> methodTable;

	if (parentClass && parentClass->cachedInstantiatedMethodTable) {
		methodTable = decltype(methodTable)(parentClass->cachedInstantiatedMethodTable->duplicate());
	} else {
		methodTable = decltype(methodTable)(MethodTable::alloc(&globalHeapPoolResource));
	}

	for (const auto &i : cls->scope->members) {
		switch (i.second->getKind()) {
			case ObjectKind::Fn: {
				std::deque<FnOverloadingObject *> overloadings;

				for (auto j : ((FnObject *)i.second)->overloadings) {
					if ((!(j->access & ACCESS_STATIC)) &&
						(j->overloadingFlags & OL_VIRTUAL))
						overloadings.push_back(j);
				}

				if (i.first == "delete") {
					std::pmr::vector<Type> destructorParamTypes(&globalHeapPoolResource);
					GenericParamList destructorGenericParamList;

					for (auto &j : overloadings) {
						if (isDuplicatedOverloading(j, destructorParamTypes, destructorGenericParamList, false)) {
							methodTable->destructors.push_front(j);
							break;
						}
					}
				} else {
					if (overloadings.size()) {
						// Link the method with method inherited from the parent.
						HostObjectRef<FnObject> fn;
						auto &methodSlot = methodTable->methods[i.first];

						if (auto m = methodTable->getMethod(i.first); m)
							fn = m;
						else {
							fn = FnObject::alloc(this);
							methodSlot = fn.get();
						}

						for (auto &j : overloadings) {
							for (auto &k : methodSlot->overloadings) {
								if (isDuplicatedOverloading(
										k,
										j->paramTypes,
										j->genericParams,
										j->overloadingFlags & OL_VARG)) {
									methodSlot->overloadings.erase(k);
									break;
								}
							}
							methodSlot->overloadings.insert(j);
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
		cls->cachedFieldInitVars = parentClass->cachedFieldInitVars;
	} else {
		objectLayout = decltype(objectLayout)(ObjectLayout::alloc(&globalHeapPoolResource));
		cls->cachedFieldInitVars = std::pmr::vector<VarObject *>(&globalHeapPoolResource);
	}

	for (const auto &i : cls->scope->members) {
		switch (i.second->getKind()) {
			case ObjectKind::Var: {
				ObjectFieldRecord fieldRecord;

				RegularVarObject *var = (RegularVarObject *)i.second;
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

				cls->cachedFieldInitVars.push_back(var);

				objectLayout->fieldNameMap[i.first] = objectLayout->fieldRecords.size();
				objectLayout->fieldRecords.push_back(std::move(fieldRecord));
				break;
			}
		}
	}

	cls->cachedFieldInitVars.shrink_to_fit();
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
		VarObject *initVar = cls->cachedFieldInitVars[i];

		ObjectFieldRecord &fieldRecord = cls->cachedObjectLayout->fieldRecords[i];

		Value data;
		data = readVar(initVar, VarRefContext());
		SLAKE_UNWRAP_EXCEPT(writeVar(instance->memberAccessor, VarRefContext::makeInstanceContext(i), data));
	}

	return instance;
}

SLAKE_API HostObjectRef<ArrayObject> Runtime::newArrayInstance(Runtime *rt, const Type &type, size_t length) {
	switch (type.typeId) {
		case TypeId::Value:
			switch (type.getValueTypeExData()) {
				case ValueType::I8:
					return I8ArrayObject::alloc(rt, length).get();
				case ValueType::I16:
					return I16ArrayObject::alloc(rt, length).get();
				case ValueType::I32:
					return I32ArrayObject::alloc(rt, length).get();
				case ValueType::I64:
					return I64ArrayObject::alloc(rt, length).get();
				case ValueType::U8:
					return U8ArrayObject::alloc(rt, length).get();
				case ValueType::U16:
					return U16ArrayObject::alloc(rt, length).get();
				case ValueType::U32:
					return U32ArrayObject::alloc(rt, length).get();
				case ValueType::U64:
					return U64ArrayObject::alloc(rt, length).get();
				case ValueType::F32:
					return F32ArrayObject::alloc(rt, length).get();
				case ValueType::F64:
					return F64ArrayObject::alloc(rt, length).get();
				case ValueType::Bool:
					return BoolArrayObject::alloc(rt, length).get();
				default:
					throw std::logic_error("Unhandled value type");
			}
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			return ObjectRefArrayObject::alloc(rt, type, length).get();
		case TypeId::Any:
			return AnyArrayObject::alloc(rt, length).get();
		default:
			throw std::logic_error("Unhandled element type");
	}
}
