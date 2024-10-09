#include <slake/runtime.h>

using namespace slake;

slake::AliasObject::AliasObject(Runtime *rt, AccessModifier access, Object *src)
	: MemberObject(rt), src(src) {
	_flags |= VF_ALIAS;
	this->accessModifier = access;
}

AliasObject::AliasObject(const AliasObject &other) : MemberObject(other) {
	src = other.src;
	name = other.name;
	parent = other.parent;
}

AliasObject::~AliasObject() {
}

Object *AliasObject::duplicate() const {
	return (Object *)alloc(this).get();
}

MemberObject* AliasObject::getMember(
	const std::pmr::string& name,
	VarRefContext* varRefContextOut) const {
	return src->getMember(name, varRefContextOut);
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

ObjectKind AliasObject::getKind() const { return ObjectKind::Alias; }

HostObjectRef<AliasObject> AliasObject::alloc(Runtime *rt, Object *src) {
	using Alloc = std::pmr::polymorphic_allocator<AliasObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<AliasObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, ACCESS_PUB, src);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

HostObjectRef<AliasObject> AliasObject::alloc(const AliasObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<AliasObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<AliasObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

void slake::AliasObject::dealloc() {
	std::pmr::polymorphic_allocator<AliasObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
