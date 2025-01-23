#include <slake/runtime.h>
#include <slake/type.h>

using namespace slake;

SLAKE_API MemberObject::MemberObject(Runtime *rt)
	: Object(rt), name(&rt->globalHeapPoolAlloc) {
}

SLAKE_API MemberObject::MemberObject(const MemberObject &x, bool &succeededOut)
	: Object(x),
	  name(&x.associatedRuntime->globalHeapPoolAlloc) {
	accessModifier = x.accessModifier;
	if (!peff::copyAssign(name, x.name)) {
		succeededOut = false;
		return;
	}
}

SLAKE_API MemberObject::~MemberObject() {
}

SLAKE_API const char *MemberObject::getName() const {
	return name.data();
}

SLAKE_API bool MemberObject::setName(const char *name) {
	size_t len = strlen(name);
	if (!this->name.resize(len))
		return false;
	memcpy(this->name.data(), name, len);
	return true;
}

SLAKE_API Object *MemberObject::getParent() const {
	return nullptr;
}

SLAKE_API void MemberObject::setParent(Object *parent) {
	throw std::logic_error("The object did not implement setParent()");
}

SLAKE_API const GenericArgList *MemberObject::getGenericArgs() const {
	return nullptr;
}
