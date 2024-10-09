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
		const_cast<Type &>(i).loadDeferredType(_rt);

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
		const_cast<Type &>(i->parentClass).loadDeferredType(i->_rt);
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
		const_cast<Type &>(i).loadDeferredType(_rt);

		InterfaceObject *interface = (InterfaceObject *)i.getCustomTypeExData();

		if (interface->getKind() != ObjectKind::Interface)
			throw IncompatibleTypeError("Referenced type value is not an interface");

		if (interface->isDerivedFrom(pInterface))
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
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<ClassObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<ClassObject> slake::ClassObject::alloc(Runtime *rt, AccessModifier access, const Type &parentClass) {
	using Alloc = std::pmr::polymorphic_allocator<ClassObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<ClassObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, access, parentClass);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::ClassObject::dealloc() {
	std::pmr::polymorphic_allocator<ClassObject> allocator(&_rt->globalHeapPoolResource);

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

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<InterfaceObject> slake::InterfaceObject::alloc(const InterfaceObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<InterfaceObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<InterfaceObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::InterfaceObject::dealloc() {
	std::pmr::polymorphic_allocator<InterfaceObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
