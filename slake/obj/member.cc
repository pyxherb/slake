#include <slake/runtime.h>
#include <slake/type.h>

using namespace slake;

SLAKE_API bool MemberObject::onSetParent(Object* parent) {
	return true;
}

SLAKE_API MemberObject::MemberObject(Runtime *rt, peff::Alloc *selfAllocator)
	: Object(rt, selfAllocator), name(selfAllocator) {
}

SLAKE_API MemberObject::MemberObject(const MemberObject &x, peff::Alloc *allocator, bool &succeededOut)
	: Object(x, allocator),
	  name(allocator) {
	accessModifier = x.accessModifier;
	if (!name.build(x.name)) {
		succeededOut = false;
		return;
	}
}

SLAKE_API MemberObject::~MemberObject() {
}

SLAKE_API const GenericArgList *MemberObject::getGenericArgs() const {
	return nullptr;
}
