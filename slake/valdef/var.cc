#include "var.h"
#include <slake/runtime.h>

using namespace slake;

slake::VarValue::VarValue(Runtime *rt, AccessModifier access, Type type, VarFlags flags)
	: MemberValue(rt, access), type(type), flags(flags) {
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(MemberValue));
}

VarValue::~VarValue() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(MemberValue));
}

Value* VarValue::duplicate() const {
	VarValue* v = new VarValue(_rt, 0, type);

	*v = *this;

	return (Value *)v;
}
