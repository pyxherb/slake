#include <slake/runtime.h>

using namespace slake;

slake::ClassValue::ClassValue(Runtime *rt, AccessModifier access, Type parentClass)
	: ModuleValue(rt, access), parentClass(parentClass) {
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(ModuleValue));
}

ClassValue::~ClassValue() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(ModuleValue));
}

bool ClassValue::hasImplemented(const InterfaceValue *pInterface) const {
	for (auto &i : implInterfaces) {
		i.loadDeferredType(_rt);

		if (((InterfaceValue *)i.getCustomTypeExData())->isDerivedFrom(pInterface))
			return true;
	}
	return false;
}

bool InterfaceValue::isDerivedFrom(const InterfaceValue *pInterface) const {
	if (pInterface == this)
		return true;

	for (auto &i : parents) {
		i.loadDeferredType(_rt);

		InterfaceValue *interface = (InterfaceValue *)i.getCustomTypeExData();

		if (interface->getType() != TypeId::Interface)
			throw IncompatibleTypeError("Referenced type value is not an interface");

		if (interface->isDerivedFrom(pInterface))
			return true;
	}

	return false;
}

Value *ClassValue::duplicate() const {
	ClassValue *v = new ClassValue(_rt, 0, {});
	*v = *this;

	return (Value *)v;
}

InterfaceValue::~InterfaceValue() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(ModuleValue));
}

Value *InterfaceValue::duplicate() const {
	InterfaceValue *v = new InterfaceValue(_rt, 0);
	*v = *this;

	return (Value *)v;
}

TraitValue::~TraitValue() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(InterfaceValue));
}

Value *TraitValue::duplicate() const {
	TraitValue *v = new TraitValue(_rt, 0);
	*v = *this;

	return (Value *)v;
}
