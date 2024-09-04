#include <slake/runtime.h>

using namespace slake;

slake::AliasObject::AliasObject(Runtime *rt, AccessModifier access, Object *src)
	: MemberObject(rt), src(src) {
	scope = src->scope;
	_flags |= VF_ALIAS;
	this->accessModifier = access;
}

AliasObject::~AliasObject() {
}

Object *AliasObject::duplicate() const {
	return (Object *)alloc(this).get();
}

const char *AliasObject::getName() const {
	return name.c_str();
}

void AliasObject::setName(const char *name) {
	this->name = name;
}

Object *AliasObject::getParent() const {
	return parent;
}

void AliasObject::setParent(Object *parent) {
	this->parent = parent;
}

HostObjectRef<AliasObject> AliasObject::alloc(Runtime *rt, Object *src) {
	std::pmr::polymorphic_allocator<AliasObject> allocator(&rt->globalHeapPoolResource);

	AliasObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, ACCESS_PUB, src);

	rt->createdObjects.insert(ptr);

	return ptr;
}

HostObjectRef<AliasObject> AliasObject::alloc(const AliasObject *other) {
	std::pmr::polymorphic_allocator<AliasObject> allocator(&other->_rt->globalHeapPoolResource);

	AliasObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, *other);

	other->_rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::AliasObject::dealloc() {
	std::pmr::polymorphic_allocator<AliasObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
