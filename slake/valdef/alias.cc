#include "alias.h"

using namespace slake;

slake::AliasValue::AliasValue(Runtime *rt, AccessModifier access, Value *src)
	: MemberValue(rt, access), src(src) {
	scope = src->scope;
	_flags |= VF_ALIAS;
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(MemberValue));
}

AliasValue::~AliasValue() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(MemberValue));
}

ValueRef<> AliasValue::call(Value *thisObject, std::deque<Value *> args, std::deque<Type> argTypes) const {
	return src->call(thisObject, args, argTypes);
}

Value *AliasValue::duplicate() const {
	return (Value *)new AliasValue(_rt, getAccess(), src);
}
