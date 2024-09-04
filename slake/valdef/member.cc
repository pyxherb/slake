#include <slake/runtime.h>
#include <slake/type.h>

using namespace slake;

MemberObject::MemberObject(Runtime *rt)
	: Object(rt) {
}

MemberObject::~MemberObject() {
}

const char* MemberObject::getName() const {
	return nullptr;
}

void MemberObject::setName(const char* name) {
	throw std::logic_error("The object did not implement setName()");
}

Object* MemberObject::getParent() const {
	return nullptr;
}

void MemberObject::setParent(Object *parent) {
	throw std::logic_error("The object did not implement setParent()");
}

GenericArgList MemberObject::getGenericArgs() const {
	return {};
}
