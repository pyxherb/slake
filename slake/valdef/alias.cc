#include <slake/runtime.h>

using namespace slake;

slake::AliasObject::AliasObject(Runtime *rt, AccessModifier access, Object *src)
	: MemberObject(rt, access), src(src) {
	scope = src->scope;
	_flags |= VF_ALIAS;
}

AliasObject::~AliasObject() {
}

Object *AliasObject::duplicate() const {
	return (Object *)new AliasObject(_rt, getAccess(), src);
}

HostObjectRef<AliasObject> AliasObject::alloc(Runtime *rt, Object *src) {
	std::pmr::polymorphic_allocator<AliasObject> allocator(&rt->globalHeapPoolResource);

	AliasObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, ACCESS_PUB, src);

	return ptr;
}

void slake::AliasObject::dealloc() {
	std::pmr::polymorphic_allocator<AliasObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
