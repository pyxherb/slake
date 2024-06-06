#include "var.h"
#include <slake/runtime.h>

using namespace slake;

BasicVarObject::BasicVarObject(Runtime *rt, AccessModifier access, Type type) : MemberObject(rt, access), type(type) {
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(MemberObject));
}

BasicVarObject::~BasicVarObject() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(MemberObject));
}

slake::VarObject::VarObject(Runtime *rt, AccessModifier access, Type type)
	: BasicVarObject(rt, access, type) {
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(BasicVarObject));
}

VarObject::~VarObject() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(BasicVarObject));
}

Object *VarObject::duplicate() const {
	VarObject *v = new VarObject(_rt, 0, type);

	*v = *this;

	return (Object *)v;
}
