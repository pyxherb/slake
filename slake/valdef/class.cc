#include <slake/runtime.h>

using namespace slake;

slake::ClassObject::ClassObject(Runtime *rt, AccessModifier access, const Type &parentClass)
	: ModuleObject(rt, access), parentClass(parentClass) {
}

ClassObject::~ClassObject() {
}

bool ClassObject::hasImplemented(const InterfaceObject *pInterface) const {
	for (auto &i : implInterfaces) {
		i.loadDeferredType(_rt);

		if (((InterfaceObject *)i.getCustomTypeExData())->isDerivedFrom(pInterface))
			return true;
	}
	return false;
}

bool InterfaceObject::isDerivedFrom(const InterfaceObject *pInterface) const {
	if (pInterface == this)
		return true;

	for (auto &i : parents) {
		i.loadDeferredType(_rt);

		InterfaceObject *interface = (InterfaceObject *)i.getCustomTypeExData();

		if (interface->getType() != TypeId::Interface)
			throw IncompatibleTypeError("Referenced type value is not an interface");

		if (interface->isDerivedFrom(pInterface))
			return true;
	}

	return false;
}

Object *ClassObject::duplicate() const {
	HostObjectRef<ClassObject> v = ClassObject::alloc(_rt, 0, {});
	*(v.get()) = *this;

	return (Object *)v.release();
}

HostObjectRef<ClassObject> slake::ClassObject::alloc(Runtime *rt, AccessModifier access, const Type &parentClass) {
	std::pmr::polymorphic_allocator<ClassObject> allocator(&rt->globalHeapPoolResource);

	ClassObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, access, parentClass);

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
	HostObjectRef<InterfaceObject> v = InterfaceObject::alloc(_rt, 0);
	*(v.get()) = *this;

	return (Object *)v.release();
}

HostObjectRef<InterfaceObject> slake::InterfaceObject::alloc(Runtime *rt, AccessModifier access, const std::deque<Type> &parents) {
	std::pmr::polymorphic_allocator<InterfaceObject> allocator(&rt->globalHeapPoolResource);

	InterfaceObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, access, parents);

	return ptr;
}

void slake::InterfaceObject::dealloc() {
	std::pmr::polymorphic_allocator<InterfaceObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
