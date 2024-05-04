#include "var.h"
#include <slake/runtime.h>

using namespace slake;

BasicVarValue::BasicVarValue(Runtime *rt, AccessModifier access) : MemberValue(rt, access) {
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(MemberValue));
}

BasicVarValue::~BasicVarValue() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(MemberValue));
}

slake::VarValue::VarValue(Runtime *rt, AccessModifier access, Type type)
	: MemberValue(rt, access), type(type) {
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(MemberValue));
}

VarValue::~VarValue() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(MemberValue));
}

Value *VarValue::duplicate() const {
	VarValue *v = new VarValue(_rt, 0, type);

	*v = *this;

	return (Value *)v;
}
