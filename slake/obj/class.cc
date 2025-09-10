#include <slake/runtime.h>

using namespace slake;

SLAKE_API void ObjectFieldRecord::replaceAllocator(peff::Alloc* allocator) noexcept {
	name.replaceAllocator(allocator);
}

SLAKE_API void ObjectLayout::replaceAllocator(peff::Alloc* allocator) noexcept {
	peff::verifyReplaceable(selfAllocator.get(), allocator);

	selfAllocator = allocator;

	fieldRecords.replaceAllocator(allocator);

	for (auto& i : fieldRecords) {
		i.replaceAllocator(allocator);
	}

	fieldNameMap.replaceAllocator(allocator);
}

SLAKE_API ObjectLayout::ObjectLayout(peff::Alloc *selfAllocator)
	: selfAllocator(selfAllocator),
	  fieldRecords(selfAllocator),
	  fieldNameMap(selfAllocator) {
}

SLAKE_API ObjectLayout *ObjectLayout::duplicate(peff::Alloc *allocator) const {
	std::unique_ptr<ObjectLayout, peff::DeallocableDeleter<ObjectLayout>> ptr(alloc(allocator));
	if (!ptr)
		return nullptr;

	if (!ptr->fieldRecords.resizeUninitialized(fieldRecords.size())) {
		return nullptr;
	}
	for (size_t i = 0; i < fieldRecords.size(); ++i) {
		peff::constructAt<ObjectFieldRecord>(&ptr->fieldRecords.at(i), selfAllocator.get());
	}
	for (size_t i = 0; i < fieldRecords.size(); ++i) {
		ObjectFieldRecord &fr = ptr->fieldRecords.at(i);

		if (!fr.name.build(fieldRecords.at(i).name)) {
			return nullptr;
		}
		fr.offset = fieldRecords.at(i).offset;
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

SLAKE_API void MethodTable::replaceAllocator(peff::Alloc* allocator) noexcept {
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
		if(!newMethodTable->methods.insert(std::string_view(v->name), +v))
			return nullptr;
	}

	for (auto i : destructors) {
		if(!newMethodTable->destructors.pushBack(+i))
			return nullptr;
	}

	return newMethodTable.release();
}


SLAKE_API Object *ClassObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API slake::ClassObject::ClassObject(Runtime *rt, peff::Alloc *selfAllocator)
	: ModuleObject(rt, selfAllocator, ObjectKind::Class),
	  baseType(TypeId::Void),
	  genericArgs(selfAllocator),
	  mappedGenericArgs(selfAllocator),
	  genericParams(selfAllocator),
	  implTypes(selfAllocator),
	  cachedFieldInitValues(selfAllocator) {
}

SLAKE_API const GenericArgList *ClassObject::getGenericArgs() const {
	return &genericArgs;
}

SLAKE_API ClassObject::ClassObject(Duplicator *duplicator, const ClassObject &x, peff::Alloc *allocator, bool &succeededOut)
	: ModuleObject(duplicator, x, allocator, succeededOut),
	  genericArgs(allocator),
	  mappedGenericArgs(allocator),
	  genericParams(allocator),
	  implTypes(allocator),
	  cachedFieldInitValues(allocator) {
	if (succeededOut) {
		_flags = x._flags;

		if (!genericArgs.resize(x.genericArgs.size())) {
			succeededOut = false;
			return;
		}
		memcpy(genericArgs.data(), x.genericArgs.data(), genericArgs.size() * sizeof(TypeRef));
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

SLAKE_API bool ClassObject::hasImplemented(const InterfaceObject *pInterface) const {
	for (auto &i : implTypes) {
		if (((InterfaceObject *)((CustomTypeDefObject*)i.typeDef)->typeObject)->isDerivedFrom(pInterface))
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
		auto parentClassObject = ((CustomTypeDefObject *)i->baseType.typeDef)->typeObject;
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

	Runtime::addSameKindObjectToList(&other->associatedRuntime->classObjectList, ptr.get());

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

	Runtime::addSameKindObjectToList(&rt->classObjectList, ptr.get());

	return ptr.release();
}

SLAKE_API void slake::ClassObject::dealloc() {
	Runtime::removeSameKindObjectToList(&associatedRuntime->classObjectList, this);
	peff::destroyAndRelease<ClassObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API void ClassObject::replaceAllocator(peff::Alloc *allocator) noexcept {
	this->ModuleObject::replaceAllocator(allocator);

	genericArgs.replaceAllocator(allocator);

	mappedGenericArgs.replaceAllocator(allocator);

	for (auto i : mappedGenericArgs) {
		i.first.replaceAllocator(allocator);
	}

	genericParams.replaceAllocator(allocator);

	for (auto& i : genericParams) {
		i.replaceAllocator(allocator);
	}

	implTypes.replaceAllocator(allocator);

	if (cachedInstantiatedMethodTable)
		cachedInstantiatedMethodTable->replaceAllocator(allocator);

	if (cachedObjectLayout)
		cachedObjectLayout->replaceAllocator(allocator);

	cachedFieldInitValues.replaceAllocator(allocator);
}

SLAKE_API InterfaceObject::InterfaceObject(Runtime *rt, peff::Alloc *selfAllocator)
	: ModuleObject(rt, selfAllocator, ObjectKind::Interface),
	  genericArgs(selfAllocator),
	  mappedGenericArgs(selfAllocator),
	  genericParams(selfAllocator),
	  implTypes(selfAllocator) {
}

SLAKE_API InterfaceObject::InterfaceObject(Duplicator *duplicator, const InterfaceObject &x, peff::Alloc *allocator, bool &succeededOut)
	: ModuleObject(duplicator, x, allocator, succeededOut),
	  genericArgs(allocator),
	  mappedGenericArgs(allocator),
	  genericParams(allocator),
	  implTypes(allocator) {
	if (succeededOut) {
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

SLAKE_API bool InterfaceObject::isDerivedFrom(const InterfaceObject *pInterface) const {
	if (pInterface == this)
		return true;

	for (auto &i : implTypes) {
		InterfaceObject *interfaceObj = (InterfaceObject *)((CustomTypeDefObject *)i.typeDef)->typeObject;

		if (interfaceObj->getObjectKind() != ObjectKind::Interface) {
			// The parent is not an interface - this situation should not be here,
			// but we have disabled exceptions, so return anyway.
			return false;
		}

		if (interfaceObj->isDerivedFrom(pInterface))
			return true;
	}

	return false;
}

SLAKE_API const GenericArgList *InterfaceObject::getGenericArgs() const {
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
	this->ModuleObject::replaceAllocator(allocator);

	genericArgs.replaceAllocator(allocator);

	mappedGenericArgs.replaceAllocator(allocator);

	for (auto i : mappedGenericArgs) {
		i.first.replaceAllocator(allocator);
	}

	genericParams.replaceAllocator(allocator);

	for (auto& i : genericParams) {
		i.replaceAllocator(allocator);
	}

	implTypes.replaceAllocator(allocator);
}
