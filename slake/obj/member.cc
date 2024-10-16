#include <slake/runtime.h>
#include <slake/type.h>

using namespace slake;

SLAKE_API MemberObject::MemberObject(Runtime *rt)
	: Object(rt) {
}

SLAKE_API MemberObject::MemberObject(const MemberObject &x) : Object(x) {
	accessModifier = x.accessModifier;
}

SLAKE_API MemberObject::~MemberObject() {
}

SLAKE_API const char *MemberObject::getName() const {
	return nullptr;
}

SLAKE_API void MemberObject::setName(const char *name) {
	throw std::logic_error("The object did not implement setName()");
}

SLAKE_API Object *MemberObject::getParent() const {
	return nullptr;
}

SLAKE_API void MemberObject::setParent(Object *parent) {
	throw std::logic_error("The object did not implement setParent()");
}

SLAKE_API GenericArgList MemberObject::getGenericArgs() const {
	return {};
}
