#include "alias.h"

using namespace slake;

slake::AliasObject::AliasObject(Runtime *rt, AccessModifier access, Object *src)
	: MemberObject(rt, access), src(src) {
	scope = src->scope;
	_flags |= VF_ALIAS;
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(MemberObject));
}

AliasObject::~AliasObject() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(MemberObject));
}

Object *AliasObject::duplicate() const {
	return (Object *)new AliasObject(_rt, getAccess(), src);
}
