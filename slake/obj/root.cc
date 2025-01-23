#include <slake/runtime.h>

using namespace slake;

SLAKE_API RootObject::RootObject(Runtime *rt)
	: Object(rt) {
	scope = Scope::alloc(&rt->globalHeapPoolAlloc, this);
}

SLAKE_API RootObject::~RootObject() {
	scope->dealloc();
}

SLAKE_API ObjectKind RootObject::getKind() const { return ObjectKind::RootObject; }

SLAKE_API MemberObject *RootObject::getMember(
	const std::string_view &name,
	VarRefContext* varRefContextOut) const {
	return scope->getMember(name);
}

SLAKE_API RootObject* RootObject::alloc(Runtime* rt) {
	std::unique_ptr<RootObject, util::DeallocableDeleter<RootObject>> ptr(peff::allocAndConstruct<RootObject>(&rt->globalHeapPoolAlloc, sizeof(std::max_align_t), rt));
	if (!ptr)
		return nullptr;

	if (!rt->createdObjects.pushBack(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::RootObject::dealloc() {
	peff::destroyAndRelease<RootObject>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}
