#include <slake/runtime.h>
#include <slake/type.h>

using namespace slake;

SLAKE_API bool MemberObject::onSetParent(Object* parent) {
	return true;
}

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

SLAKE_API const GenericArgList *MemberObject::getGenericArgs() const {
	return nullptr;
}
