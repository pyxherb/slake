#include <slake/runtime.h>

using namespace slake;

ModuleObject::ModuleObject(Runtime *rt, AccessModifier access)
	: MemberObject(rt) {
	scope = Scope::alloc(&rt->globalHeapPoolResource, this);
	this->accessModifier = access;
}

ModuleObject::~ModuleObject() {
	scope->dealloc();
}

Object *ModuleObject::duplicate() const {
	return (Object *)alloc(this).get();
}

MemberObject* ModuleObject::getMember(
	const std::pmr::string& name,
	VarRefContext* varRefContextOut) const {
	return scope->getMember(name);
}

const char* ModuleObject::getName() const {
	return name.c_str();
}

void ModuleObject::setName(const char *name) {
	this->name = name;
}

Object *ModuleObject::getParent() const {
	return parent;
}

void ModuleObject::setParent(Object *parent) {
	this->parent = parent;
}

HostObjectRef<ModuleObject> slake::ModuleObject::alloc(Runtime *rt, AccessModifier access) {
	std::pmr::polymorphic_allocator<ModuleObject> allocator(&rt->globalHeapPoolResource);

	ModuleObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, access);

	rt->createdObjects.insert(ptr);

	return ptr;
}

HostObjectRef<ModuleObject> slake::ModuleObject::alloc(const ModuleObject *other) {
	std::pmr::polymorphic_allocator<ModuleObject> allocator(&other->_rt->globalHeapPoolResource);

	ModuleObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, *other);

	other->_rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::ModuleObject::dealloc() {
	std::pmr::polymorphic_allocator<ModuleObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
