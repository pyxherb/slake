#include <slake/runtime.h>

using namespace slake;

SLAKE_API slake::AliasObject::AliasObject(Runtime *rt, AccessModifier access, Object *src)
	: MemberObject(rt), src(src) {
	_flags |= VF_ALIAS;
	this->accessModifier = access;
}

SLAKE_API AliasObject::AliasObject(const AliasObject &other) : MemberObject(other) {
	src = other.src;
	name = other.name;
	parent = other.parent;
}

SLAKE_API AliasObject::~AliasObject() {
}

SLAKE_API Object *AliasObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API MemberObject *AliasObject::getMember(
	const std::pmr::string& name,
	VarRefContext* varRefContextOut) const {
	return src->getMember(name, varRefContextOut);
}

SLAKE_API const char *AliasObject::getName() const {
	return name.c_str();
}

SLAKE_API void AliasObject::setName(const char *name) {
	this->name = name;
}

SLAKE_API Object *AliasObject::getParent() const {
	return parent;
}

SLAKE_API void AliasObject::setParent(Object *parent) {
	this->parent = parent;
}

SLAKE_API ObjectKind AliasObject::getKind() const { return ObjectKind::Alias; }

SLAKE_API HostObjectRef<AliasObject> AliasObject::alloc(Runtime *rt, Object *src) {
	using Alloc = std::pmr::polymorphic_allocator<AliasObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<AliasObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, ACCESS_PUB, src);

	rt->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<AliasObject> AliasObject::alloc(const AliasObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<AliasObject>;
	Alloc allocator(&other->associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<AliasObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->associatedRuntime->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::AliasObject::dealloc() {
	std::pmr::polymorphic_allocator<AliasObject> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
