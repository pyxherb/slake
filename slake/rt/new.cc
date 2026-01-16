#include <slake/runtime.h>
#include <peff/base/scope_guard.h>

using namespace slake;

SLAKE_API InternalExceptionPointer Runtime::initMethodTableForClass(ClassObject *cls, ClassObject *parentClass) {
	assert(!cls->cachedInstantiatedMethodTable);
	MethodTable *parentMt = parentClass ? parentClass->cachedInstantiatedMethodTable : nullptr;
	std::unique_ptr<MethodTable, peff::DeallocableDeleter<MethodTable>> methodTable(MethodTable::alloc(cls->selfAllocator.get()));

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
				if (!fnSlot->setName(((FnObject *)it.value())->getName())) {
					return OutOfMemoryError::alloc();
				}

				if (it.key() == "delete") {
					peff::DynArray<TypeRef> destructorParamTypes(getFixedAlloc());

					for (auto j : fn->overloadings) {
						bool result;
						static FnSignatureComparator cmp;
						if (cmp(FnSignature(j.second->paramTypes, j.second->isWithVarArgs(), j.second->genericParams.size(), j.second->overridenType), FnSignature(destructorParamTypes, false, 0, TypeId::Void))) {
							if (!methodTable->destructors.pushFront(+j.second)) {
								return OutOfMemoryError::alloc();
							}
							break;
						}
					}
				} else {
					for (auto j : fn->overloadings) {
						if (!fnSlot->overloadings.insert(FnSignature(j.first), +j.second))
							return OutOfMemoryError::alloc();
					}

					if (parentMt) {
						if (auto m = parentMt->getMethod(fn->getName()); m) {
							if (m->overloadings.size()) {
								// Link the method with method inherited from the parent.
								for (auto k : m->overloadings) {
									// If we found a non-duplicated overloading from the parent, add it.
									if (auto it = fnSlot->overloadings.find(k.first); it == fnSlot->overloadings.end()) {
										if (!fnSlot->overloadings.insert(FnSignature(k.first), +k.second))
											return OutOfMemoryError::alloc();
									}
								}
							}
						}
					}
				}

				if (fnSlot->overloadings.size()) {
					if (!methodTable->methods.insert(fnSlot->getName(), fnSlot.get()))
						return OutOfMemoryError::alloc();
				}

				break;
			}
		}
	}

	cls->cachedInstantiatedMethodTable = methodTable.release();

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::initObjectLayoutForModule(BasicModuleObject *mod, ObjectLayout *objectLayout) {
	size_t cnt = 0;
	for (size_t i = 0; i < mod->fieldRecords.size(); ++i) {
		FieldRecord &clsFieldRecord = mod->fieldRecords.at(i);

		if (clsFieldRecord.accessModifier & ACCESS_STATIC) {
			continue;
		}

		ObjectFieldRecord fieldRecord(mod->selfAllocator.get());

		TypeRef type = clsFieldRecord.type;

		size_t size = sizeofType(type);
		size_t align = alignofType(type);

		if (align > 1) {
			if (size_t diff = objectLayout->totalSize % align; diff) {
				objectLayout->totalSize += align - diff;
			}
		}
		fieldRecord.offset = objectLayout->totalSize;
		fieldRecord.idxInitFieldRecord = i;

		fieldRecord.type = type;
		if (!fieldRecord.name.build(clsFieldRecord.name))
			return OutOfMemoryError::alloc();

		if (!objectLayout->fieldNameMap.insert(fieldRecord.name, objectLayout->fieldRecords.size()))
			return OutOfMemoryError::alloc();
		if (!objectLayout->fieldRecords.pushBack(std::move(fieldRecord)))
			return OutOfMemoryError::alloc();

		objectLayout->totalSize += size;

		++cnt;
	}

	if (!objectLayout->fieldRecordInitModuleFieldsNumber.pushBack({ mod, cnt }))
		return OutOfMemoryError::alloc();

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::initObjectLayoutForClass(ClassObject *cls, ClassObject *parentClass) {
	assert(!cls->cachedObjectLayout);
	std::unique_ptr<ObjectLayout, peff::DeallocableDeleter<ObjectLayout>> objectLayout;

	if (parentClass && parentClass->cachedObjectLayout) {
		objectLayout = decltype(objectLayout)(parentClass->cachedObjectLayout->duplicate(cls->selfAllocator.get()));
	} else {
		objectLayout = decltype(objectLayout)(ObjectLayout::alloc(cls->selfAllocator.get()));
	}

	if (!objectLayout)
		return OutOfMemoryError::alloc();

	SLAKE_RETURN_IF_EXCEPT(initObjectLayoutForModule(cls, objectLayout.get()));

	// cls->cachedFieldInitVars.shrink_to_fit();
	cls->cachedObjectLayout = objectLayout.release();

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::initObjectLayoutForStruct(StructObject *s) {
	assert(!s->cachedObjectLayout);
	std::unique_ptr<ObjectLayout, peff::DeallocableDeleter<ObjectLayout>> objectLayout(ObjectLayout::alloc(s->selfAllocator.get()));

	if (!objectLayout)
		return OutOfMemoryError::alloc();

	SLAKE_RETURN_IF_EXCEPT(initObjectLayoutForModule(s, objectLayout.get()));

	// cls->cachedFieldInitVars.shrink_to_fit();
	s->cachedObjectLayout = objectLayout.release();

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::prepareClassForInstantiation(ClassObject *cls) {
	peff::List<ClassObject *> unpreparedClasses(getFixedAlloc());
	{
		ClassObject *p = cls;

		while (true) {
			if (!unpreparedClasses.pushBack(+p))
				return OutOfMemoryError::alloc();

			if (!p->baseType)
				break;

			if (p->baseType.typeId != TypeId::Instance)
				return allocOutOfMemoryErrorIfAllocFailed(MalformedClassStructureError::alloc(getFixedAlloc(), p));

			Object *parentClass = (p->baseType.getCustomTypeDef())->typeObject;
			if (parentClass->getObjectKind() != ObjectKind::Class)
				return allocOutOfMemoryErrorIfAllocFailed(MalformedClassStructureError::alloc(getFixedAlloc(), p));

			p = (ClassObject *)parentClass;
		}
	}

	ClassObject *c, *p = nullptr;

	while (unpreparedClasses.size()) {
		c = unpreparedClasses.back();

		if (!c->cachedObjectLayout)
			SLAKE_RETURN_IF_EXCEPT(initObjectLayoutForClass(c, p));
		if (!c->cachedInstantiatedMethodTable)
			SLAKE_RETURN_IF_EXCEPT(initMethodTableForClass(c, p));

		p = c;
		unpreparedClasses.popBack();
	}

	return {};
}

struct StructPreparationFrame {
	Object *structObject;
	size_t index;
};

struct StructPreparationContext {
	peff::List<StructPreparationFrame> frames;

	SLAKE_FORCEINLINE StructPreparationContext(peff::Alloc *allocator) : frames(allocator) {}
};

SLAKE_FORCEINLINE InternalExceptionPointer _prepareStructForInstantiation(StructPreparationContext &context) {
	while (context.frames.size()) {
		StructPreparationFrame &curFrame = context.frames.back();

		switch (curFrame.structObject->getObjectKind()) {
			case ObjectKind::Struct: {
				StructObject *structObject = (StructObject *)curFrame.structObject;
				auto &fieldRecords = structObject->getFieldRecords();
				if (curFrame.index >= fieldRecords.size()) {
					if (!structObject->cachedObjectLayout)
						SLAKE_RETURN_IF_EXCEPT(structObject->associatedRuntime->initObjectLayoutForStruct(structObject));
					context.frames.popBack();
					continue;
				}

				auto &curRecord = fieldRecords.at(curFrame.index);

				TypeRef typeRef = curRecord.type;
				switch (curRecord.type.typeId) {
					case TypeId::StructInstance: {
						CustomTypeDefObject *td = typeRef.getCustomTypeDef();
						assert(td->typeObject->getObjectKind() == ObjectKind::Struct);
						if (!context.frames.pushBack({ (StructObject *)td->typeObject, 0 }))
							return OutOfMemoryError::alloc();
						break;
					}
					case TypeId::UnionEnum: {
						CustomTypeDefObject *td = typeRef.getCustomTypeDef();
						assert(td->typeObject->getObjectKind() == ObjectKind::UnionEnum);
						if (!context.frames.pushBack({ (UnionEnumObject *)td->typeObject, 0 }))
							return OutOfMemoryError::alloc();
						break;
					}
				}

				++curFrame.index;
				break;
			}
		}
	}

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::prepareStructForInstantiation(StructObject *cls) {
	StructPreparationContext context(getFixedAlloc());

	if (!context.frames.pushBack({ cls, 0 }))
		return OutOfMemoryError::alloc();

	SLAKE_RETURN_IF_EXCEPT(_prepareStructForInstantiation(context));

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
	size_t index = 0, cnt = 0;
	std::pair<BasicModuleObject *, size_t> p = cls->cachedObjectLayout->fieldRecordInitModuleFieldsNumber.at(0);

	for (size_t i = 0; i < cls->fieldRecords.size(); ++i) {
		const ObjectFieldRecord &fieldRecord = cls->cachedObjectLayout->fieldRecords.at(i);

		Value data;
		readVar(Reference::makeStaticFieldRef(p.first, fieldRecord.idxInitFieldRecord), data);
		SLAKE_UNWRAP_EXCEPT(writeVar(Reference::makeInstanceFieldRef(instance.get(), i), data));

		if (cnt++ >= p.second) {
			cnt = 0;
			p = cls->cachedObjectLayout->fieldRecordInitModuleFieldsNumber.at(++index);
		}
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
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(Reference));
			if (!(obj->data = obj->selfAllocator->alloc(sizeof(Reference) * length, alignof(Reference))))
				return nullptr;
			obj->elementAlignment = alignof(Reference);
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
