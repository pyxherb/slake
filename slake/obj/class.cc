#include <slake/runtime.h>
#include <variant>

using namespace slake;

SLAKE_API void ObjectFieldRecord::replaceAllocator(peff::Alloc *allocator) noexcept {
	name.replaceAllocator(allocator);
}

SLAKE_API void ObjectLayout::replaceAllocator(peff::Alloc *allocator) noexcept {
	peff::verifyReplaceable(selfAllocator.get(), allocator);

	selfAllocator = allocator;

	fieldRecords.replaceAllocator(allocator);

	for (auto &i : fieldRecords) {
		i.replaceAllocator(allocator);
	}

	fieldNameMap.replaceAllocator(allocator);

	fieldRecordInitModuleFieldsNumber.replaceAllocator(allocator);
}

SLAKE_API ObjectLayout::ObjectLayout(peff::Alloc *selfAllocator)
	: selfAllocator(selfAllocator),
	  fieldRecords(selfAllocator),
	  fieldNameMap(selfAllocator),
	  fieldRecordInitModuleFieldsNumber(selfAllocator) {
}

SLAKE_API ObjectLayout *ObjectLayout::duplicate(peff::Alloc *allocator) const {
	std::unique_ptr<ObjectLayout, peff::DeallocableDeleter<ObjectLayout>> ptr(alloc(allocator));
	if (!ptr)
		return nullptr;

	if (!ptr->fieldRecords.resizeUninitialized(fieldRecords.size())) {
		return nullptr;
	}
	if (!ptr->fieldRecordInitModuleFieldsNumber.resizeUninitialized(fieldRecordInitModuleFieldsNumber.size())) {
		return nullptr;
	}
	memcpy(ptr->fieldRecordInitModuleFieldsNumber.data(), fieldRecordInitModuleFieldsNumber.data(), fieldRecordInitModuleFieldsNumber.size() * sizeof(std::pair<BasicModuleObject *, size_t>));
	for (size_t i = 0; i < fieldRecords.size(); ++i) {
		peff::constructAt<ObjectFieldRecord>(&ptr->fieldRecords.at(i), selfAllocator.get());
	}
	for (size_t i = 0; i < fieldRecords.size(); ++i) {
		ObjectFieldRecord &fr = ptr->fieldRecords.at(i);

		if (!fr.name.build(fieldRecords.at(i).name)) {
			return nullptr;
		}
		fr.offset = fieldRecords.at(i).offset;
		fr.idxInitFieldRecord = fieldRecords.at(i).idxInitFieldRecord;
		fr.type = fieldRecords.at(i).type;

		if (!ptr->fieldNameMap.insert(fr.name, +i)) {
			return nullptr;
		}
	}
	ptr->totalSize = totalSize;

	return ptr.release();
}

SLAKE_API ObjectLayout *ObjectLayout::alloc(peff::Alloc *selfAllocator) {
	return peff::allocAndConstruct<ObjectLayout>(selfAllocator, sizeof(std::max_align_t), selfAllocator);
}

SLAKE_API void ObjectLayout::dealloc() {
	peff::destroyAndRelease<ObjectLayout>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API MethodTable::MethodTable(peff::Alloc *selfAllocator)
	: selfAllocator(selfAllocator),
	  methods(selfAllocator),
	  destructors(selfAllocator) {
}

SLAKE_API FnObject *MethodTable::getMethod(const std::string_view &name) {
	if (auto it = methods.find(name); it != methods.end())
		return it.value();
	return nullptr;
}

SLAKE_API MethodTable *MethodTable::alloc(peff::Alloc *selfAllocator) {
	return peff::allocAndConstruct<MethodTable>(selfAllocator, sizeof(std::max_align_t), selfAllocator);
}

SLAKE_API void MethodTable::dealloc() {
	peff::destroyAndRelease<MethodTable>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API void MethodTable::replaceAllocator(peff::Alloc *allocator) noexcept {
	peff::verifyReplaceable(selfAllocator.get(), allocator);

	selfAllocator = allocator;

	methods.replaceAllocator(allocator);

	destructors.replaceAllocator(allocator);
}

SLAKE_API MethodTable *MethodTable::duplicate(peff::Alloc *allocator) {
	std::unique_ptr<MethodTable, peff::DeallocableDeleter<MethodTable>> newMethodTable(alloc(allocator));
	if (!newMethodTable)
		return nullptr;

	for (auto [k, v] : methods) {
		if (!newMethodTable->methods.insert(std::string_view(v->getName()), +v))
			return nullptr;
	}

	for (auto i : destructors) {
		if (!newMethodTable->destructors.pushBack(+i))
			return nullptr;
	}

	return newMethodTable.release();
}

SLAKE_API Object *ClassObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API slake::ClassObject::ClassObject(Runtime *rt, peff::Alloc *selfAllocator)
	: BasicModuleObject(rt, selfAllocator, ObjectKind::Class),
	  genericArgs(selfAllocator),
	  mappedGenericArgs(selfAllocator),
	  genericParams(selfAllocator),
	  mappedGenericParams(selfAllocator),
	  implTypes(selfAllocator) {
}

SLAKE_API const peff::DynArray<Value> *ClassObject::getGenericArgs() const {
	return &genericArgs;
}

SLAKE_API ClassObject::ClassObject(Duplicator *duplicator, const ClassObject &x, peff::Alloc *allocator, bool &succeededOut)
	: BasicModuleObject(duplicator, x, allocator, succeededOut),
	  genericArgs(allocator),
	  mappedGenericArgs(allocator),	 // No need to copy
	  genericParams(allocator),
	  mappedGenericParams(allocator),  // No need to copy
	  implTypes(allocator) {
	if (succeededOut) {
		classFlags = x.classFlags;

		if (!genericArgs.resize(x.genericArgs.size())) {
			succeededOut = false;
			return;
		}
		memcpy(genericArgs.data(), x.genericArgs.data(), genericArgs.size() * sizeof(Value));
		for (auto [k, v] : x.mappedGenericArgs) {
			peff::String name(allocator);

			if (!name.build(k)) {
				succeededOut = false;
				return;
			}

			if (!(mappedGenericArgs.insert(std::move(name), Value(v)))) {
				succeededOut = false;
				return;
			}
		}
		if (!genericParams.resizeUninitialized(x.genericParams.size())) {
			succeededOut = false;
			return;
		}
		for (size_t i = 0; i < x.genericParams.size(); ++i) {
			if (!x.genericParams.at(i).copy(genericParams.at(i))) {
				for (size_t j = i; j; --j) {
					peff::destroyAt<GenericParam>(&genericParams.at(j - 1));
				}
				succeededOut = false;
				return;
			}
		}
		if (!implTypes.resize(x.implTypes.size())) {
			succeededOut = false;
			return;
		}
		memcpy(implTypes.data(), x.implTypes.data(), implTypes.size() * sizeof(TypeRef));

		baseType = x.baseType;

		// DO NOT copy the cached instantiated method table.
	}
}

SLAKE_API ClassObject::~ClassObject() {
	if (cachedInstantiatedMethodTable)
		cachedInstantiatedMethodTable->dealloc();
	if (cachedObjectLayout)
		cachedObjectLayout->dealloc();
}

SLAKE_API bool ClassObject::hasImplemented(InterfaceObject *pInterface) const {
	for (auto &i : implTypes) {
		InterfaceObject *interfaceObject = (InterfaceObject *)i.getCustomTypeDef()->typeObject;
		if (interfaceObject->isDerivedFrom(pInterface))
			return true;
	}
	return false;
}

SLAKE_API bool ClassObject::isBaseOf(const ClassObject *pClass) const {
	const ClassObject *i = pClass;
	while (true) {
		if (i == this)
			return true;

		if (i->baseType.typeId == TypeId::Void)
			break;
		auto parentClassObject = (i->baseType.getCustomTypeDef())->typeObject;
		assert(parentClassObject->getObjectKind() == ObjectKind::Class);
		i = (ClassObject *)parentClassObject;
	}

	return false;
}

SLAKE_API HostObjectRef<ClassObject> slake::ClassObject::alloc(Duplicator *duplicator, const ClassObject *other) {
	bool succeeded = true;

	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = other->associatedRuntime->getCurGenAlloc();

	std::unique_ptr<ClassObject, peff::DeallocableDeleter<ClassObject>> ptr(
		peff::allocAndConstruct<ClassObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			duplicator, *other, curGenerationAllocator.get(), succeeded));

	if (!succeeded)
		return nullptr;

	if (!other->associatedRuntime->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<ClassObject> slake::ClassObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<ClassObject, peff::DeallocableDeleter<ClassObject>> ptr(
		peff::allocAndConstruct<ClassObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			rt, curGenerationAllocator.get()));

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::ClassObject::dealloc() {
	peff::destroyAndRelease<ClassObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API void ClassObject::replaceAllocator(peff::Alloc *allocator) noexcept {
	this->BasicModuleObject::replaceAllocator(allocator);

	genericArgs.replaceAllocator(allocator);

	mappedGenericParams.replaceAllocator(allocator);
	mappedGenericArgs.replaceAllocator(allocator);

	genericParams.replaceAllocator(allocator);

	for (auto &i : genericParams) {
		i.replaceAllocator(allocator);
	}

	implTypes.replaceAllocator(allocator);

	if (cachedInstantiatedMethodTable)
		cachedInstantiatedMethodTable->replaceAllocator(allocator);

	if (cachedObjectLayout)
		cachedObjectLayout->replaceAllocator(allocator);
}

SLAKE_API InterfaceObject::InterfaceObject(Runtime *rt, peff::Alloc *selfAllocator)
	: BasicModuleObject(rt, selfAllocator, ObjectKind::Interface),
	  genericArgs(selfAllocator),
	  mappedGenericArgs(selfAllocator),
	  genericParams(selfAllocator),
	  mappedGenericParams(selfAllocator),
	  implTypes(selfAllocator),
	  implInterfaceIndices(selfAllocator) {
}

SLAKE_API InterfaceObject::InterfaceObject(Duplicator *duplicator, const InterfaceObject &x, peff::Alloc *allocator, bool &succeededOut)
	: BasicModuleObject(duplicator, x, allocator, succeededOut),
	  genericArgs(allocator),
	  mappedGenericArgs(allocator),	 // No need to copy
	  genericParams(allocator),
	  mappedGenericParams(allocator),  // No need to copy
	  implTypes(allocator),
	  implInterfaceIndices(allocator) {
	if (succeededOut) {
		if (!genericParams.resizeUninitialized(x.genericParams.size())) {
			succeededOut = false;
			return;
		}
		for (size_t i = 0; i < x.genericParams.size(); ++i) {
			if (!x.genericParams.at(i).copy(genericParams.at(i))) {
				for (size_t j = i; j; --j) {
					peff::destroyAt<GenericParam>(&genericParams.at(j - 1));
				}
				succeededOut = false;
				return;
			}
		}

		if (!implTypes.resize(x.implTypes.size())) {
			succeededOut = false;
			return;
		}
		memcpy(implTypes.data(), x.implTypes.data(), implTypes.size() * sizeof(TypeRef));
	}
}

struct UpdateInterfaceInheritanceRelationshipFrame {
	InterfaceObject *interfaceObject;
	size_t index;
};

struct UpdateInterfaceInheritanceRelationshipContext {
	peff::List<UpdateInterfaceInheritanceRelationshipFrame> frames;

	SLAKE_FORCEINLINE UpdateInterfaceInheritanceRelationshipContext(peff::Alloc *allocator) : frames(allocator) {}
};

SLAKE_FORCEINLINE static InternalExceptionPointer _updateInterfaceInheritanceRelationship(InterfaceObject *interfaceObject, UpdateInterfaceInheritanceRelationshipContext &context) noexcept {
	if (!context.frames.pushBack({ interfaceObject, 0 }))
		return OutOfMemoryError::alloc();

	while (context.frames.size()) {
		UpdateInterfaceInheritanceRelationshipFrame &curFrame = context.frames.back();

		InterfaceObject *interfaceObject = curFrame.interfaceObject;
		// Check if the interface has cyclic inheritance.
		if (!curFrame.index) {
			for (auto &i : context.frames) {
				if ((&i != &curFrame) && (i.interfaceObject == curFrame.interfaceObject))
					std::terminate();
			}
		}
		if (curFrame.index >= interfaceObject->implTypes.size()) {
			if (!interfaceObject->implInterfaceIndices.insert(+interfaceObject))
				return OutOfMemoryError::alloc();
			context.frames.popBack();
			continue;
		}

		TypeRef typeRef = interfaceObject->implTypes.at(curFrame.index);

		// TODO: Return a malformed interface exception.
		if ((!typeRef.typeDef) || (typeRef.typeDef->getTypeDefKind() != TypeDefKind::CustomTypeDef))
			std::terminate();

		CustomTypeDefObject *td = typeRef.getCustomTypeDef();

		// TODO: Return a malformed interface exception.
		if (td->typeObject->getObjectKind() != ObjectKind::Interface)
			std::terminate();

		if (!context.frames.pushBack({ (InterfaceObject *)td->typeObject, 0 }))
			return OutOfMemoryError::alloc();

		++curFrame.index;
	}

	return {};
}

SLAKE_API InternalExceptionPointer InterfaceObject::updateInheritanceRelationship(peff::Alloc *allocator) noexcept {
	invalidateInheritanceRelationshipCache();

	UpdateInterfaceInheritanceRelationshipContext context(allocator);

	return _updateInterfaceInheritanceRelationship(this, context);
}

SLAKE_API bool InterfaceObject::isDerivedFrom(InterfaceObject *pInterface) const {
	if (pInterface == this)
		return true;

	return implInterfaceIndices.contains(pInterface);
}

SLAKE_API const peff::DynArray<Value> *InterfaceObject::getGenericArgs() const {
	return &genericArgs;
}

SLAKE_API InterfaceObject::~InterfaceObject() {
}

SLAKE_API Object *InterfaceObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<InterfaceObject> slake::InterfaceObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<InterfaceObject, peff::DeallocableDeleter<InterfaceObject>> ptr(
		peff::allocAndConstruct<InterfaceObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			rt,
			curGenerationAllocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<InterfaceObject> slake::InterfaceObject::alloc(Duplicator *duplicator, const InterfaceObject *other) {
	bool succeeded = true;

	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = other->associatedRuntime->getCurGenAlloc();

	std::unique_ptr<InterfaceObject, peff::DeallocableDeleter<InterfaceObject>> ptr(
		peff::allocAndConstruct<InterfaceObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			duplicator, *other, curGenerationAllocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associatedRuntime->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::InterfaceObject::dealloc() {
	peff::destroyAndRelease<InterfaceObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API void InterfaceObject::replaceAllocator(peff::Alloc *allocator) noexcept {
	this->BasicModuleObject::replaceAllocator(allocator);

	genericArgs.replaceAllocator(allocator);

	mappedGenericParams.replaceAllocator(allocator);
	mappedGenericArgs.replaceAllocator(allocator);

	genericParams.replaceAllocator(allocator);

	for (auto &i : genericParams) {
		i.replaceAllocator(allocator);
	}

	implTypes.replaceAllocator(allocator);

	implInterfaceIndices.replaceAllocator(allocator);
}

SLAKE_API Object *StructObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

struct IndexedStructRecursionCheckFrameExData {
	size_t index;
};

struct EnumModuleIteratorStructRecursionCheckFrameExData {
	decltype(EnumModuleObject::members)::ConstIterator enumModuleIterator;
};

struct StructRecursionCheckFrame {
	Object *structObject;

	std::variant<IndexedStructRecursionCheckFrameExData,
		EnumModuleIteratorStructRecursionCheckFrameExData>
		exData;
};

struct StructRecursionCheckContext {
	peff::List<StructRecursionCheckFrame> frames;
	peff::Set<Object *> walkedObjects;

	SLAKE_FORCEINLINE StructRecursionCheckContext(peff::Alloc *allocator) : frames(allocator), walkedObjects(allocator) {}
};

static SLAKE_FORCEINLINE InternalExceptionPointer _isStructRecursed(StructRecursionCheckContext &context) {
	auto checkTypeRef = [&context](const TypeRef &typeRef) -> InternalExceptionPointer {
		switch (typeRef.typeId) {
			case TypeId::StructInstance: {
				CustomTypeDefObject *td = typeRef.getCustomTypeDef();

				assert(td->typeObject->getObjectKind() == ObjectKind::Struct);
				if (!context.frames.pushBack({ td->typeObject, IndexedStructRecursionCheckFrameExData{ 0 } }))
					return OutOfMemoryError::alloc();
				break;
			}
			case TypeId::UnionEnum: {
				CustomTypeDefObject *td = typeRef.getCustomTypeDef();

				assert(td->typeObject->getObjectKind() == ObjectKind::UnionEnum);
				if (!context.frames.pushBack({ td->typeObject, EnumModuleIteratorStructRecursionCheckFrameExData{ ((UnionEnumObject *)td->typeObject)->members.beginConst() } }))
					return OutOfMemoryError::alloc();
				break;
			}
			case TypeId::UnionEnumItem: {
				CustomTypeDefObject *td = typeRef.getCustomTypeDef();

				assert(td->typeObject->getObjectKind() == ObjectKind::UnionEnum);
				if (!context.frames.pushBack({ td->typeObject, IndexedStructRecursionCheckFrameExData{ 0 } }))
					return OutOfMemoryError::alloc();
				break;
			}
		}
		return {};
	};
	while (context.frames.size()) {
		StructRecursionCheckFrame &curFrame = context.frames.back();

		switch (curFrame.structObject->getObjectKind()) {
			case ObjectKind::Struct: {
				StructObject *structObject = (StructObject *)curFrame.structObject;
				size_t &index = std::get<IndexedStructRecursionCheckFrameExData>(curFrame.exData).index;
				if (!index) {
					if (context.walkedObjects.contains(curFrame.structObject))
						// Recursed!
						std::terminate();

					if (!context.walkedObjects.insert(+curFrame.structObject))
						return OutOfMemoryError::alloc();
				} else if (index >= structObject->fieldRecords.size()) {
					context.walkedObjects.remove(structObject);
					context.frames.popBack();
					continue;
				}

				auto &curRecord = structObject->fieldRecords.at(index);

				TypeRef typeRef = curRecord.type;
				SLAKE_RETURN_IF_EXCEPT(checkTypeRef(typeRef));

				++index;
				break;
			}
			case ObjectKind::UnionEnum: {
				UnionEnumObject *structObject = (UnionEnumObject *)curFrame.structObject;
				decltype(EnumModuleObject::members)::ConstIterator &iterator = std::get<EnumModuleIteratorStructRecursionCheckFrameExData>(curFrame.exData).enumModuleIterator;
				if (iterator == structObject->members.beginConst()) {
					if (context.walkedObjects.contains(curFrame.structObject))
						// Recursed!
						std::terminate();

					if (!context.walkedObjects.insert(+curFrame.structObject))
						return OutOfMemoryError::alloc();
				} else if (iterator == structObject->members.endConst()) {
					context.walkedObjects.remove(structObject);
					context.frames.popBack();
					continue;
				}

				auto curRecord = *iterator;

				if (!context.frames.pushBack({ curRecord.second, IndexedStructRecursionCheckFrameExData{ 0 } }))
					return OutOfMemoryError::alloc();
				if (!context.walkedObjects.insert(+structObject))
					return OutOfMemoryError::alloc();
				break;

				++iterator;
				break;
			}
			case ObjectKind::UnionEnumItem: {
				UnionEnumItemObject *structObject = (UnionEnumItemObject *)curFrame.structObject;
				size_t &index = std::get<IndexedStructRecursionCheckFrameExData>(curFrame.exData).index;
				if (!index) {
					if (context.walkedObjects.contains(curFrame.structObject))
						// Recursed!
						std::terminate();

					if (!context.walkedObjects.insert(+curFrame.structObject))
						return OutOfMemoryError::alloc();
				} else if (index >= structObject->fieldRecords.size()) {
					context.walkedObjects.remove(structObject);
					context.frames.popBack();
					continue;
				}

				auto &curRecord = structObject->fieldRecords.at(index);

				TypeRef typeRef = curRecord.type;
				SLAKE_RETURN_IF_EXCEPT(checkTypeRef(typeRef));

				++index;
				break;
			}
			default:
				std::terminate();
		}
	}

	return {};
}

SLAKE_API InternalExceptionPointer StructObject::isRecursed(peff::Alloc *allocator) noexcept {
	StructRecursionCheckContext context(allocator);

	if (!context.frames.pushBack({ this, IndexedStructRecursionCheckFrameExData{ 0 } }))
		return OutOfMemoryError::alloc();

	return _isStructRecursed(context);
}

SLAKE_API slake::StructObject::StructObject(Runtime *rt, peff::Alloc *selfAllocator)
	: BasicModuleObject(rt, selfAllocator, ObjectKind::Struct),
	  genericArgs(selfAllocator),
	  mappedGenericArgs(selfAllocator),
	  genericParams(selfAllocator),
	  mappedGenericParams(selfAllocator),
	  implTypes(selfAllocator) {
}

SLAKE_API const peff::DynArray<Value> *StructObject::getGenericArgs() const {
	return &genericArgs;
}

SLAKE_API StructObject::StructObject(Duplicator *duplicator, const StructObject &x, peff::Alloc *allocator, bool &succeededOut)
	: BasicModuleObject(duplicator, x, allocator, succeededOut),
	  genericArgs(allocator),
	  mappedGenericArgs(allocator),	 // No need to copy
	  genericParams(allocator),
	  mappedGenericParams(allocator),  // No need to copy
	  implTypes(allocator) {
	if (succeededOut) {
		structFlags = x.structFlags;

		if (!genericArgs.resize(x.genericArgs.size())) {
			succeededOut = false;
			return;
		}
		memcpy(genericArgs.data(), x.genericArgs.data(), genericArgs.size() * sizeof(Value));
		for (auto [k, v] : x.mappedGenericArgs) {
			peff::String name(allocator);

			if (!name.build(k)) {
				succeededOut = false;
				return;
			}

			if (!(mappedGenericArgs.insert(std::move(name), TypeRef(v)))) {
				succeededOut = false;
				return;
			}
		}
		if (!genericParams.resizeUninitialized(x.genericParams.size())) {
			succeededOut = false;
			return;
		}
		for (size_t i = 0; i < x.genericParams.size(); ++i) {
			if (!x.genericParams.at(i).copy(genericParams.at(i))) {
				for (size_t j = i; j; --j) {
					peff::destroyAt<GenericParam>(&genericParams.at(j - 1));
				}
				succeededOut = false;
				return;
			}
		}
		if (!implTypes.resize(x.implTypes.size())) {
			succeededOut = false;
			return;
		}
		memcpy(implTypes.data(), x.implTypes.data(), implTypes.size() * sizeof(TypeRef));
	}
}

SLAKE_API StructObject::~StructObject() {
	if (cachedObjectLayout)
		cachedObjectLayout->dealloc();
}

SLAKE_API HostObjectRef<StructObject> slake::StructObject::alloc(Duplicator *duplicator, const StructObject *other) {
	bool succeeded = true;

	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = other->associatedRuntime->getCurGenAlloc();

	std::unique_ptr<StructObject, peff::DeallocableDeleter<StructObject>> ptr(
		peff::allocAndConstruct<StructObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			duplicator, *other, curGenerationAllocator.get(), succeeded));

	if (!succeeded)
		return nullptr;

	if (!other->associatedRuntime->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<StructObject> slake::StructObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<StructObject, peff::DeallocableDeleter<StructObject>> ptr(
		peff::allocAndConstruct<StructObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			rt, curGenerationAllocator.get()));

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::StructObject::dealloc() {
	peff::destroyAndRelease<StructObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API void StructObject::replaceAllocator(peff::Alloc *allocator) noexcept {
	this->BasicModuleObject::replaceAllocator(allocator);

	genericArgs.replaceAllocator(allocator);

	mappedGenericParams.replaceAllocator(allocator);
	mappedGenericArgs.replaceAllocator(allocator);

	genericParams.replaceAllocator(allocator);

	for (auto &i : genericParams) {
		i.replaceAllocator(allocator);
	}

	implTypes.replaceAllocator(allocator);

	if (cachedObjectLayout)
		cachedObjectLayout->replaceAllocator(allocator);
}

SLAKE_API EnumModuleObject::EnumModuleObject(Runtime *rt, peff::Alloc *selfAllocator, ObjectKind objectKind)
	: MemberObject(rt, selfAllocator, objectKind),
	  members(selfAllocator) {
}

SLAKE_API EnumModuleObject::EnumModuleObject(Duplicator *duplicator, const EnumModuleObject &x, peff::Alloc *allocator, bool &succeededOut)
	: MemberObject(x, allocator, succeededOut),
	  members(allocator) {
	if (succeededOut) {
	}
}

SLAKE_API EnumModuleObject::~EnumModuleObject() {
}

SLAKE_API Reference EnumModuleObject::getMember(const std::string_view &name) const {
	if (auto it = members.find(name); it != members.end()) {
		return Reference::makeObjectRef(it.value());
	}
	return Reference::makeInvalidRef();
}

SLAKE_API bool EnumModuleObject::addMember(MemberObject *member) {
	if (!members.insert(member->getName(), +member))
		return false;
	member->setParent(this);
	return true;
}

SLAKE_API bool EnumModuleObject::removeMember(const std::string_view &name) {
	return members.remove(name);
}

SLAKE_API void EnumModuleObject::replaceAllocator(peff::Alloc *allocator) noexcept {
	this->MemberObject::replaceAllocator(allocator);

	members.replaceAllocator(allocator);
}

SLAKE_API ScopedEnumObject::ScopedEnumObject(Runtime *rt, peff::Alloc *selfAllocator)
	: EnumModuleObject(rt, selfAllocator, ObjectKind::ScopedEnum) {
}

SLAKE_API ScopedEnumObject::ScopedEnumObject(Duplicator *duplicator, const ScopedEnumObject &x, peff::Alloc *allocator, bool &succeededOut)
	: EnumModuleObject(duplicator, x, allocator, succeededOut) {
	if (succeededOut) {
		baseType = x.baseType;
	}
}

SLAKE_API ScopedEnumObject::~ScopedEnumObject() {
}

SLAKE_API Object *ScopedEnumObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API Reference ScopedEnumObject::getMember(const std::string_view &name) const {
	if (auto it = members.find(name); it != members.end()) {
		return Reference::makeObjectRef(it.value());
	}
	return Reference::makeInvalidRef();
}

SLAKE_API bool ScopedEnumObject::addMember(MemberObject *member) {
	if (!members.insert(member->getName(), +member))
		return false;
	member->setParent(this);
	return true;
}

SLAKE_API bool ScopedEnumObject::removeMember(const std::string_view &name) {
	return members.remove(name);
}

SLAKE_API HostObjectRef<ScopedEnumObject> ScopedEnumObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<ScopedEnumObject, peff::DeallocableDeleter<ScopedEnumObject>> ptr(
		peff::allocAndConstruct<ScopedEnumObject>(curGenerationAllocator.get(), sizeof(std::max_align_t), rt, curGenerationAllocator.get()));

	if (!ptr)
		return nullptr;

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<ScopedEnumObject> ScopedEnumObject::alloc(Duplicator *duplicator, const ScopedEnumObject *other) {
	return (ScopedEnumObject *)other->duplicate(duplicator);
}

SLAKE_API void ScopedEnumObject::dealloc() {
	peff::destroyAndRelease<ScopedEnumObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API void ScopedEnumObject::replaceAllocator(peff::Alloc *allocator) noexcept {
	this->MemberObject::replaceAllocator(allocator);

	members.replaceAllocator(allocator);
}

SLAKE_API UnionEnumItemObject::UnionEnumItemObject(Runtime *rt, peff::Alloc *selfAllocator)
	: BasicModuleObject(rt, selfAllocator, ObjectKind::UnionEnumItem) {
}

SLAKE_API UnionEnumItemObject::UnionEnumItemObject(Duplicator *duplicator, const UnionEnumItemObject &x, peff::Alloc *allocator, bool &succeededOut)
	: BasicModuleObject(duplicator, x, allocator, succeededOut) {
}

SLAKE_API UnionEnumItemObject::~UnionEnumItemObject() {
}

SLAKE_API Object *UnionEnumItemObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<UnionEnumItemObject> slake::UnionEnumItemObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<UnionEnumItemObject, peff::DeallocableDeleter<UnionEnumItemObject>> ptr(
		peff::allocAndConstruct<UnionEnumItemObject>(curGenerationAllocator.get(), sizeof(std::max_align_t), rt, curGenerationAllocator.get()));

	if (!ptr)
		return nullptr;

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<UnionEnumItemObject> slake::UnionEnumItemObject::alloc(Duplicator *duplicator, const UnionEnumItemObject *other) {
	return (UnionEnumItemObject *)other->duplicate(duplicator);
}

SLAKE_API void slake::UnionEnumItemObject::dealloc() {
	peff::destroyAndRelease<UnionEnumItemObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API void UnionEnumItemObject::replaceAllocator(peff::Alloc *allocator) noexcept {
	this->BasicModuleObject::replaceAllocator(allocator);
}

SLAKE_API UnionEnumObject::UnionEnumObject(Runtime *rt, peff::Alloc *selfAllocator)
	: EnumModuleObject(rt, selfAllocator, ObjectKind::UnionEnum) {
}

SLAKE_API UnionEnumObject::UnionEnumObject(Duplicator *duplicator, const UnionEnumObject &x, peff::Alloc *allocator, bool &succeededOut)
	: EnumModuleObject(duplicator, x, allocator, succeededOut) {
	if (succeededOut) {
	}
}

SLAKE_API UnionEnumObject::~UnionEnumObject() {
}

SLAKE_API Object *UnionEnumObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API Reference UnionEnumObject::getMember(const std::string_view &name) const {
	if (auto it = members.find(name); it != members.end()) {
		return Reference::makeObjectRef(it.value());
	}
	return Reference::makeInvalidRef();
}

SLAKE_API bool UnionEnumObject::addMember(MemberObject *member) {
	if (!members.insert(member->getName(), +member))
		return false;
	member->setParent(this);
	return true;
}

SLAKE_API bool UnionEnumObject::removeMember(const std::string_view &name) {
	return members.remove(name);
}

SLAKE_API HostObjectRef<UnionEnumObject> UnionEnumObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<UnionEnumObject, peff::DeallocableDeleter<UnionEnumObject>> ptr(
		peff::allocAndConstruct<UnionEnumObject>(curGenerationAllocator.get(), sizeof(std::max_align_t), rt, curGenerationAllocator.get()));

	if (!ptr)
		return nullptr;

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<UnionEnumObject> UnionEnumObject::alloc(Duplicator *duplicator, const UnionEnumObject *other) {
	return (UnionEnumObject *)other->duplicate(duplicator);
}

SLAKE_API void UnionEnumObject::dealloc() {
	peff::destroyAndRelease<UnionEnumObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API void UnionEnumObject::replaceAllocator(peff::Alloc *allocator) noexcept {
	this->MemberObject::replaceAllocator(allocator);

	members.replaceAllocator(allocator);
}
