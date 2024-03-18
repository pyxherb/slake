#include <slake/runtime.h>

using namespace slake;

ModuleValue::ModuleValue(Runtime *rt, AccessModifier access)
	: MemberValue(rt, access) {
	scope = new Scope(this);
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(MemberValue));
}

ModuleValue::~ModuleValue() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(MemberValue));
}

Type ModuleValue::getType() const {
	return TypeId::Module;
}

Value *ModuleValue::duplicate() const {
	ModuleValue* v = new ModuleValue(_rt, getAccess());

	*v = *this;

	return (Value *)v;
}
