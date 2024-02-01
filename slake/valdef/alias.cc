#include "alias.h"

using namespace slake;

slake::AliasValue::AliasValue(Runtime *rt, AccessModifier access, Value *src)
	: MemberValue(rt, access), _src(src) {
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(MemberValue));
}

AliasValue::~AliasValue() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(MemberValue));
}

MemberValue *AliasValue::getMember(std::string name) {
	return _src->getMember(name);
}

const MemberValue *AliasValue::getMember(std::string name) const {
	return _src->getMember(name);
}

ValueRef<> AliasValue::call(std::deque<ValueRef<>> args) const {
	return _src->call(args);
}

Value *AliasValue::duplicate() const {
	return (Value *)new AliasValue(_rt, getAccess(), _src.get());
}
