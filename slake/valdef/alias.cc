#include "alias.h"

using namespace slake;

slake::AliasValue::AliasValue(Runtime *rt, AccessModifier access, Value *src)
	: MemberValue(rt, access), src(src) {
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(MemberValue));
}

AliasValue::~AliasValue() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(MemberValue));
}

ValueRef<> AliasValue::call(std::deque<Value *> args) const {
	return src->call(args);
}

Value *AliasValue::duplicate() const {
	return (Value *)new AliasValue(_rt, getAccess(), src);
}
