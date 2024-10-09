#include <slake/runtime.h>

using namespace slake;

ObjectLayout::ObjectLayout(std::pmr::memory_resource *memoryResource)
	: memoryResource(memoryResource),
	  fieldRecords(memoryResource),
	  fieldNameMap(memoryResource) {
}

ObjectLayout *ObjectLayout::duplicate() const {
	using Alloc = std::pmr::polymorphic_allocator<ObjectLayout>;
	Alloc allocator(memoryResource);

	std::unique_ptr<ObjectLayout, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *this);

	return ptr.release();
}

ObjectLayout *ObjectLayout::alloc(std::pmr::memory_resource *memoryResource) {
	using Alloc = std::pmr::polymorphic_allocator<ObjectLayout>;
	Alloc allocator(memoryResource);

	std::unique_ptr<ObjectLayout, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), memoryResource);

	return ptr.release();
}

void ObjectLayout::dealloc() {
	std::pmr::polymorphic_allocator<ObjectLayout> allocator(memoryResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

slake::ClassObject::ClassObject(Runtime *rt, AccessModifier access, const Type &parentClass)
	: ModuleObject(rt, access), parentClass(parentClass) {
}

ObjectKind ClassObject::getKind() const { return ObjectKind::Class; }

GenericArgList ClassObject::getGenericArgs() const {
	return genericArgs;
}

ClassObject::ClassObject(const ClassObject &x) : ModuleObject(x) {
	_flags = x._flags;

	genericArgs = x.genericArgs;
	genericParams = x.genericParams;

	parentClass = x.parentClass;
	implInterfaces = x.implInterfaces;

	// DO NOT copy the cached instantiated method table.
}

ClassObject::~ClassObject() {
	if (cachedInstantiatedMethodTable)
		cachedInstantiatedMethodTable->dealloc();
	if (cachedObjectLayout)
		cachedObjectLayout->dealloc();
}

bool ClassObject::hasImplemented(const InterfaceObject *pInterface) const {
	for (auto &i : implInterfaces) {
		const_cast<Type &>(i).loadDeferredType(_rt);

		if (((InterfaceObject *)i.getCustomTypeExData())->isDerivedFrom(pInterface))
			return true;
	}
	return false;
}

bool ClassObject::isBaseOf(const ClassObject *pClass) const {
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

InterfaceObject::InterfaceObject(Runtime *rt, AccessModifier access, const std::vector<Type> &parents)
	: ModuleObject(rt, access), parents(parents) {
}

InterfaceObject::InterfaceObject(const InterfaceObject &x) : ModuleObject(x) {
	genericArgs = x.genericArgs;

	genericParams = x.genericParams;

	parents = x.parents;
}

bool InterfaceObject::isDerivedFrom(const InterfaceObject *pInterface) const {
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

ObjectKind InterfaceObject::getKind() const { return ObjectKind::Interface; }

Object *ClassObject::duplicate() const {
	return (Object *)alloc(this).get();
}

GenericArgList InterfaceObject::getGenericArgs() const {
	return genericArgs;
}

HostObjectRef<ClassObject> slake::ClassObject::alloc(const ClassObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<ClassObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<ClassObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

HostObjectRef<ClassObject> slake::ClassObject::alloc(Runtime *rt, AccessModifier access, const Type &parentClass) {
	using Alloc = std::pmr::polymorphic_allocator<ClassObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<ClassObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, access, parentClass);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

void slake::ClassObject::dealloc() {
	std::pmr::polymorphic_allocator<ClassObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

InterfaceObject::~InterfaceObject() {
}

Object *InterfaceObject::duplicate() const {
	return (Object *)alloc(this).get();
}

HostObjectRef<InterfaceObject> slake::InterfaceObject::alloc(Runtime *rt, AccessModifier access, const std::vector<Type> &parents) {
	using Alloc = std::pmr::polymorphic_allocator<InterfaceObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<InterfaceObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, access, parents);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

HostObjectRef<InterfaceObject> slake::InterfaceObject::alloc(const InterfaceObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<InterfaceObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<InterfaceObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

void slake::InterfaceObject::dealloc() {
	std::pmr::polymorphic_allocator<InterfaceObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
