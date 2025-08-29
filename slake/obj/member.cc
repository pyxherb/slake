#include <slake/runtime.h>
#include <slake/type.h>

using namespace slake;

SLAKE_API MemberObject::MemberObject(Runtime *rt, peff::Alloc *selfAllocator, ObjectKind objectKind)
	: Object(rt, selfAllocator, objectKind), name(selfAllocator) {
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
	associatedRuntime->invalidateGenericCache(this);
}

SLAKE_API const GenericArgList *MemberObject::getGenericArgs() const {
	return nullptr;
}

SLAKE_API void MemberObject::replaceAllocator(peff::Alloc* allocator) noexcept {
	this->Object::replaceAllocator(allocator);

	name.replaceAllocator(allocator);
}
