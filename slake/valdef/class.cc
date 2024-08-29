#include <slake/runtime.h>

using namespace slake;

slake::ClassObject::ClassObject(Runtime *rt, AccessModifier access, const Type &parentClass)
	: ModuleObject(rt, access), parentClass(parentClass) {
}

ClassObject::~ClassObject() {
	if (cachedInstantiatedMethodTable)
		delete cachedInstantiatedMethodTable;
}

bool ClassObject::hasImplemented(const InterfaceObject *pInterface) const {
	for (auto &i : implInterfaces) {
		i.loadDeferredType(_rt);

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
		i->parentClass.loadDeferredType(i->_rt);
		auto parentClassObject = i->parentClass.getCustomTypeExData();
		assert(parentClassObject->getKind() == ObjectKind::Class);
		i = (ClassObject *)parentClassObject;
	}

	return false;
}

bool InterfaceObject::isDerivedFrom(const InterfaceObject *pInterface) const {
	if (pInterface == this)
		return true;

	for (auto &i : parents) {
		i.loadDeferredType(_rt);

		InterfaceObject *interface = (InterfaceObject *)i.getCustomTypeExData();

		if (interface->getKind() != ObjectKind::Interface)
			throw IncompatibleTypeError("Referenced type value is not an interface");

		if (interface->isDerivedFrom(pInterface))
			return true;
	}

	return false;
}

Object *ClassObject::duplicate() const {
	return (Object *)alloc(this).get();
}

HostObjectRef<ClassObject> slake::ClassObject::alloc(const ClassObject *other) {
	std::pmr::polymorphic_allocator<ClassObject> allocator(&other->_rt->globalHeapPoolResource);

	ClassObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, *other);

	other->_rt->createdObjects.insert(ptr);

	return ptr;
}

HostObjectRef<ClassObject> slake::ClassObject::alloc(Runtime *rt, AccessModifier access, const Type &parentClass) {
	std::pmr::polymorphic_allocator<ClassObject> allocator(&rt->globalHeapPoolResource);

	ClassObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, access, parentClass);

	rt->createdObjects.insert(ptr);

	return ptr;
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

HostObjectRef<InterfaceObject> slake::InterfaceObject::alloc(Runtime *rt, AccessModifier access, const std::deque<Type> &parents) {
	std::pmr::polymorphic_allocator<InterfaceObject> allocator(&rt->globalHeapPoolResource);

	InterfaceObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, access, parents);

	rt->createdObjects.insert(ptr);

	return ptr;
}

HostObjectRef<InterfaceObject> slake::InterfaceObject::alloc(const InterfaceObject *other) {
	std::pmr::polymorphic_allocator<InterfaceObject> allocator(&other->_rt->globalHeapPoolResource);

	InterfaceObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, *other);

	other->_rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::InterfaceObject::dealloc() {
	std::pmr::polymorphic_allocator<InterfaceObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
