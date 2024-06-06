#include <slake/runtime.h>

using namespace slake;

ModuleObject::ModuleObject(Runtime *rt, AccessModifier access)
	: MemberObject(rt, access) {
	scope = new Scope(this);
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(MemberObject));
}

ModuleObject::~ModuleObject() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(MemberObject));
}

Type ModuleObject::getType() const {
	return TypeId::Module;
}

Object *ModuleObject::duplicate() const {
	ModuleObject* v = new ModuleObject(_rt, getAccess());

	*v = *this;

	return (Object *)v;
}
