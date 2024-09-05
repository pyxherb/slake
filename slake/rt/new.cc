#include <slake/runtime.h>

using namespace slake;

/// @brief Create a new class instance.
/// @param cls Class for instance creation.
/// @return Created instance of the class.
///
/// @note This function normalizes loading-deferred types.
InstanceObject *slake::Runtime::newClassInstance(ClassObject *cls, NewClassInstanceFlags flags) {
	InstanceObject *parent = nullptr;

	HostObjectRef<InstanceObject> instance;

	if (cls->parentClass.typeId == TypeId::Instance) {
		cls->parentClass.loadDeferredType(this);

		Object *parentClass = (ClassObject *)cls->parentClass.getCustomTypeExData();
		assert(parentClass->getKind() == ObjectKind::Class);

		parent = this->newClassInstance((ClassObject *)parentClass, _NEWCLSINST_PARENT);
	}

	if (parent) {
		instance = parent;
	} else {
		instance = InstanceObject::alloc(this);
	}

	MethodTable *methodTable = nullptr;
	ObjectLayout *objectLayout = nullptr;
	bool isMethodTablePresent = true, isObjectLayoutPresent = true;

	if (cls->cachedInstantiatedMethodTable) {
		methodTable = cls->cachedInstantiatedMethodTable;
		isMethodTablePresent = true;
	} else {
		if (parent) {
			methodTable =
				(cls->cachedInstantiatedMethodTable = parent->methodTable->duplicate());
		} else
			methodTable =
				(cls->cachedInstantiatedMethodTable = new MethodTable());
		isMethodTablePresent = false;
	}

	if (cls->cachedObjectLayout) {
		objectLayout = cls->cachedObjectLayout;
		isObjectLayoutPresent = true;
	} else {
		if (parent) {
			objectLayout =
				(cls->cachedObjectLayout = parent->objectLayout->duplicate());
		} else
			objectLayout =
				(cls->cachedObjectLayout = new ObjectLayout());
		isObjectLayoutPresent = false;

		if (parent) {
			cls->cachedFieldInitVars = parent->_class->cachedFieldInitVars;
		} else
			cls->cachedFieldInitVars = std::pmr::vector<VarObject *>(&globalHeapPoolResource);
	}

	instance->methodTable = methodTable;
	instance->objectLayout = objectLayout;

	if ((!isMethodTablePresent) || (!isObjectLayoutPresent)) {
		for (const auto &i : cls->scope->members) {
			switch (i.second->getKind()) {
				case ObjectKind::Var: {
					if (!isObjectLayoutPresent) {
						ObjectFieldRecord fieldRecord;

						RegularVarObject *var = (RegularVarObject *)i.second;
						auto type = var->getVarType(VarRefContext());

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
										objectLayout->totalSize += sizeof(Object *) & (sizeof(Object *) - 1);
										fieldRecord.offset = objectLayout->totalSize;
										objectLayout->totalSize += sizeof(Object *);
										break;
								}
								break;
							case TypeId::String:
							case TypeId::Instance:
							case TypeId::Array:
								objectLayout->totalSize += sizeof(Object *) & (sizeof(Object *) - 1);
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
					}
					break;
				}
				case ObjectKind::Fn: {
					if (!isMethodTablePresent) {
						std::deque<FnOverloadingObject *> overloadings;

						for (auto j : ((FnObject *)i.second)->overloadings) {
							if ((!(j->access & ACCESS_STATIC)) &&
								(j->overloadingFlags & OL_VIRTUAL))
								overloadings.push_back(j);
						}

						if (i.first == "delete") {
							std::deque<Type> destructorParamTypes;
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
					}

					break;
				}
			}
		}
	}

	if (!(flags & _NEWCLSINST_PARENT)) {
		instance->rawFieldData = new char[objectLayout->totalSize];
		instance->szRawFieldData = objectLayout->totalSize;
	}

	if (!isObjectLayoutPresent) {
		cls->cachedFieldInitVars.shrink_to_fit();
	}

	instance->_class = cls;

	//
	// Initialize the fields.
	//
	if (!(flags & _NEWCLSINST_PARENT)) {
		for (size_t i = 0; i < cls->cachedFieldInitVars.size(); ++i) {
			VarObject *initVar = cls->cachedFieldInitVars[i];

			ObjectFieldRecord &fieldRecord = objectLayout->fieldRecords[i];

			instance->memberAccessor->setData(
				VarRefContext::makeInstanceContext(i),
				initVar->getData(VarRefContext()));
		}
	}

	return instance.release();
}

HostObjectRef<ArrayObject> Runtime::newArrayInstance(Runtime *rt, const Type &type, size_t length) {
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
			}
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			return ObjectRefArrayObject::alloc(rt, type, length).get();
		case TypeId::Any:
			return AnyArrayObject::alloc(rt, length).get();
		default:
			throw IncompatibleTypeError("Specified type is not constructible");
	}
}
