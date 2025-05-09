#include <slake/runtime.h>

using namespace slake;

SLAKE_API ObjectLayout::ObjectLayout(peff::Alloc *selfAllocator)
	: selfAllocator(selfAllocator),
	  fieldRecords(selfAllocator),
	  fieldNameMap(selfAllocator) {
}

SLAKE_API ObjectLayout *ObjectLayout::duplicate() const {
	std::unique_ptr<ObjectLayout, util::DeallocableDeleter<ObjectLayout>> ptr(alloc(selfAllocator.get()));
	if (!ptr)
		return nullptr;

	if (!peff::copyAssign(ptr->fieldRecords, fieldRecords))
		return nullptr;
	if (!peff::copyAssign(ptr->fieldNameMap, fieldNameMap))
		return nullptr;
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
	  destructors(selfAllocator),
	  nativeDestructors(selfAllocator) {
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

SLAKE_API MethodTable *MethodTable::duplicate() {
	std::unique_ptr<MethodTable, util::DeallocableDeleter<MethodTable>> newMethodTable(alloc(selfAllocator.get()));
	if (!newMethodTable)
		return nullptr;

	if (!peff::copyAssign(newMethodTable->methods, methods)) {
		return nullptr;
	}
	if (!peff::copyAssign(newMethodTable->destructors, destructors)) {
		return nullptr;
	}
	if (!peff::copyAssign(newMethodTable->nativeDestructors, nativeDestructors)) {
		return nullptr;
	}

	return newMethodTable.release();
}


SLAKE_API Object *ClassObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API slake::ClassObject::ClassObject(Runtime *rt)
	: ModuleObject(rt),
	  baseType(TypeId::None),
	  genericArgs(&rt->globalHeapPoolAlloc),
	  mappedGenericArgs(&rt->globalHeapPoolAlloc),
	  genericParams(&rt->globalHeapPoolAlloc),
	  implTypes(&rt->globalHeapPoolAlloc),
	  cachedFieldInitValues(&rt->globalHeapPoolAlloc),
	  nativeDestructors(&rt->globalHeapPoolAlloc) {
}

SLAKE_API ObjectKind ClassObject::getKind() const { return ObjectKind::Class; }

SLAKE_API const GenericArgList *ClassObject::getGenericArgs() const {
	return &genericArgs;
}

SLAKE_API ClassObject::ClassObject(const ClassObject &x, bool &succeededOut)
	: ModuleObject(x, succeededOut),
	  genericArgs(&x.associatedRuntime->globalHeapPoolAlloc),
	  mappedGenericArgs(&x.associatedRuntime->globalHeapPoolAlloc),
	  genericParams(&x.associatedRuntime->globalHeapPoolAlloc),
	  implTypes(&x.associatedRuntime->globalHeapPoolAlloc),
	  cachedFieldInitValues(&x.associatedRuntime->globalHeapPoolAlloc),
	  nativeDestructors(&x.associatedRuntime->globalHeapPoolAlloc) {
	if (succeededOut) {
		_flags = x._flags;

		if (!peff::copyAssign(genericArgs, x.genericArgs)) {
			succeededOut = false;
			return;
		}
		if (!peff::copyAssign(mappedGenericArgs, x.mappedGenericArgs)) {
			succeededOut = false;
			return;
		}
		if (!peff::copyAssign(genericParams, x.genericParams)) {
			succeededOut = false;
			return;
		}
		if (!peff::copyAssign(implTypes, x.implTypes)) {
			succeededOut = false;
			return;
		}

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
		if (auto e = const_cast<Type &>(i).loadDeferredType(associatedRuntime);
			e) {
			e.reset();
			return false;
		}

		if (((InterfaceObject *)i.getCustomTypeExData())->isDerivedFrom(pInterface))
			return true;
	}
	return false;
}

SLAKE_API bool ClassObject::isBaseOf(const ClassObject *pClass) const {
	const ClassObject *i = pClass;
	while (true) {
		if (i == this)
			return true;

		if (i->baseType.typeId == TypeId::None)
			break;
		if (auto e = const_cast<Type &>(i->baseType).loadDeferredType(i->associatedRuntime);
			e) {
			e.reset();
			return false;
		}
		auto parentClassObject = i->baseType.getCustomTypeExData();
		assert(parentClassObject->getKind() == ObjectKind::Class);
		i = (ClassObject *)parentClassObject;
	}

	return false;
}

SLAKE_API HostObjectRef<ClassObject> slake::ClassObject::alloc(const ClassObject *other) {
	bool succeeded = true;

	std::unique_ptr<ClassObject, util::DeallocableDeleter<ClassObject>> ptr(
		peff::allocAndConstruct<ClassObject>(
			&other->associatedRuntime->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			*other, succeeded));

	if (!succeeded)
		return nullptr;

	if (!other->associatedRuntime->createdObjects.insert(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<ClassObject> slake::ClassObject::alloc(Runtime *rt) {
	std::unique_ptr<ClassObject, util::DeallocableDeleter<ClassObject>> ptr(
		peff::allocAndConstruct<ClassObject>(
			&rt->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			rt));

	if (!rt->createdObjects.insert(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::ClassObject::dealloc() {
	peff::destroyAndRelease<ClassObject>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API InterfaceObject::InterfaceObject(Runtime *rt)
	: ModuleObject(rt),
	  genericArgs(&rt->globalHeapPoolAlloc),
	  mappedGenericArgs(&rt->globalHeapPoolAlloc),
	  genericParams(&rt->globalHeapPoolAlloc),
	  implTypes(&rt->globalHeapPoolAlloc) {
}

SLAKE_API InterfaceObject::InterfaceObject(const InterfaceObject &x, bool &succeededOut)
	: ModuleObject(x, succeededOut),
	  genericArgs(&x.associatedRuntime->globalHeapPoolAlloc),
	  mappedGenericArgs(&x.associatedRuntime->globalHeapPoolAlloc),
	  genericParams(&x.associatedRuntime->globalHeapPoolAlloc),
	  implTypes(&x.associatedRuntime->globalHeapPoolAlloc) {
	if (succeededOut) {
		if (!peff::copyAssign(genericArgs, x.genericArgs)) {
			succeededOut = false;
			return;
		}

		if (!peff::copyAssign(genericParams, x.genericParams)) {
			succeededOut = false;
			return;
		}

		if (!peff::copyAssign(implTypes, x.implTypes)) {
			succeededOut = false;
			return;
		}
	}
}

SLAKE_API bool InterfaceObject::isDerivedFrom(const InterfaceObject *pInterface) const {
	if (pInterface == this)
		return true;

	for (auto &i : implTypes) {
		if (auto e = const_cast<Type &>(i).loadDeferredType(associatedRuntime);
			e) {
			e.reset();
			return false;
		}

		InterfaceObject *interfaceObj = (InterfaceObject *)i.getCustomTypeExData();

		if (interfaceObj->getKind() != ObjectKind::Interface) {
			// The parent is not an interface - this situation should not be here,
			// but we have disabled exceptions, so return anyway.
			return false;
		}

		if (interfaceObj->isDerivedFrom(pInterface))
			return true;
	}

	return false;
}

SLAKE_API ObjectKind InterfaceObject::getKind() const { return ObjectKind::Interface; }

SLAKE_API const GenericArgList *InterfaceObject::getGenericArgs() const {
	return &genericArgs;
}

SLAKE_API InterfaceObject::~InterfaceObject() {
}

SLAKE_API Object *InterfaceObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<InterfaceObject> slake::InterfaceObject::alloc(Runtime *rt) {
	std::unique_ptr<InterfaceObject, util::DeallocableDeleter<InterfaceObject>> ptr(
		peff::allocAndConstruct<InterfaceObject>(
			&rt->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			rt));
	if (!ptr)
		return nullptr;

	if (!rt->createdObjects.insert(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<InterfaceObject> slake::InterfaceObject::alloc(const InterfaceObject *other) {
	bool succeeded = true;

	std::unique_ptr<InterfaceObject, util::DeallocableDeleter<InterfaceObject>> ptr(
		peff::allocAndConstruct<InterfaceObject>(
			&other->associatedRuntime->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			*other, succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associatedRuntime->createdObjects.insert(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::InterfaceObject::dealloc() {
	peff::destroyAndRelease<InterfaceObject>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}
