#include <slake/runtime.h>

using namespace slake;

SLAKE_API ObjectLayout::ObjectLayout(std::pmr::memory_resource *memoryResource)
	: memoryResource(memoryResource),
	  fieldRecords(memoryResource),
	  fieldNameMap(memoryResource) {
}

SLAKE_API ObjectLayout *ObjectLayout::duplicate() const {
	using Alloc = std::pmr::polymorphic_allocator<ObjectLayout>;
	Alloc allocator(memoryResource);

	std::unique_ptr<ObjectLayout, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *this);

	return ptr.release();
}

SLAKE_API ObjectLayout *ObjectLayout::alloc(std::pmr::memory_resource *memoryResource) {
	using Alloc = std::pmr::polymorphic_allocator<ObjectLayout>;
	Alloc allocator(memoryResource);

	std::unique_ptr<ObjectLayout, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), memoryResource);

	return ptr.release();
}

SLAKE_API void ObjectLayout::dealloc() {
	std::pmr::polymorphic_allocator<ObjectLayout> allocator(memoryResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API slake::ClassObject::ClassObject(Runtime *rt, AccessModifier access, const Type &parentClass)
	: ModuleObject(rt, access), parentClass(parentClass) {
}

SLAKE_API ObjectKind ClassObject::getKind() const { return ObjectKind::Class; }

SLAKE_API GenericArgList ClassObject::getGenericArgs() const {
	return genericArgs;
}

SLAKE_API ClassObject::ClassObject(const ClassObject &x) : ModuleObject(x) {
	_flags = x._flags;

	genericArgs = x.genericArgs;
	genericParams = x.genericParams;

	parentClass = x.parentClass;
	implInterfaces = x.implInterfaces;

	// DO NOT copy the cached instantiated method table.
}

SLAKE_API ClassObject::~ClassObject() {
	if (cachedInstantiatedMethodTable)
		cachedInstantiatedMethodTable->dealloc();
	if (cachedObjectLayout)
		cachedObjectLayout->dealloc();
}

SLAKE_API bool ClassObject::hasImplemented(const InterfaceObject *pInterface) const {
	for (auto &i : implInterfaces) {
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

		if (i->parentClass.typeId == TypeId::None)
			break;
		if (auto e = const_cast<Type &>(i->parentClass).loadDeferredType(i->associatedRuntime);
			e) {
			e.reset();
			return false;
		}
		auto parentClassObject = i->parentClass.getCustomTypeExData();
		assert(parentClassObject->getKind() == ObjectKind::Class);
		i = (ClassObject *)parentClassObject;
	}

	return false;
}

SLAKE_API InterfaceObject::InterfaceObject(Runtime *rt, AccessModifier access, const std::vector<Type> &parents)
	: ModuleObject(rt, access), parents(parents) {
}

SLAKE_API InterfaceObject::InterfaceObject(const InterfaceObject &x) : ModuleObject(x) {
	genericArgs = x.genericArgs;

	genericParams = x.genericParams;

	parents = x.parents;
}

SLAKE_API bool InterfaceObject::isDerivedFrom(const InterfaceObject *pInterface) const {
	if (pInterface == this)
		return true;

	for (auto &i : parents) {
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

SLAKE_API Object *ClassObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API GenericArgList InterfaceObject::getGenericArgs() const {
	return genericArgs;
}

SLAKE_API HostObjectRef<ClassObject> slake::ClassObject::alloc(const ClassObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<ClassObject>;
	Alloc allocator(&other->associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<ClassObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->associatedRuntime->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<ClassObject> slake::ClassObject::alloc(Runtime *rt, AccessModifier access, const Type &parentClass) {
	using Alloc = std::pmr::polymorphic_allocator<ClassObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<ClassObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, access, parentClass);

	rt->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::ClassObject::dealloc() {
	std::pmr::polymorphic_allocator<ClassObject> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API InterfaceObject::~InterfaceObject() {
}

SLAKE_API Object *InterfaceObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<InterfaceObject> slake::InterfaceObject::alloc(Runtime *rt, AccessModifier access, const std::vector<Type> &parents) {
	using Alloc = std::pmr::polymorphic_allocator<InterfaceObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<InterfaceObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, access, parents);

	rt->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<InterfaceObject> slake::InterfaceObject::alloc(const InterfaceObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<InterfaceObject>;
	Alloc allocator(&other->associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<InterfaceObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->associatedRuntime->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::InterfaceObject::dealloc() {
	std::pmr::polymorphic_allocator<InterfaceObject> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
