#include <slake/runtime.h>

using namespace slake;

slake::ClassObject::ClassObject(Runtime *rt, AccessModifier access, Type parentClass)
	: ModuleObject(rt, access), parentClass(parentClass) {
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(ModuleObject));
}

ClassObject::~ClassObject() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(ModuleObject));
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
	ClassObject *v = new ClassObject(_rt, 0, {});
	*v = *this;

	return (Object *)v;
}

InterfaceObject::~InterfaceObject() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(ModuleObject));
}

Object *InterfaceObject::duplicate() const {
	InterfaceObject *v = new InterfaceObject(_rt, 0);
	*v = *this;

	return (Object *)v;
}

TraitObject::~TraitObject() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(InterfaceObject));
}

Object *TraitObject::duplicate() const {
	TraitObject *v = new TraitObject(_rt, 0);
	*v = *this;

	return (Object *)v;
}
