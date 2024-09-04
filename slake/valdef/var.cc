#include "var.h"
#include <slake/runtime.h>

using namespace slake;

VarObject::VarObject(Runtime *rt) : MemberObject(rt) {
}

VarObject::~VarObject() {
}

slake::RegularVarObject::RegularVarObject(Runtime *rt, AccessModifier access, const Type &type)
	: VarObject(rt), type(type) {
	this->accessModifier = access;
}

RegularVarObject::~RegularVarObject() {
}

Object *RegularVarObject::duplicate() const {
	return (Object *)(VarObject *)alloc(this).get();
}

const char* RegularVarObject::getName() const {
	return name.c_str();
}

void RegularVarObject::setName(const char* name) {
	this->name = name;
}

Object* RegularVarObject::getParent() const {
	return parent;
}

void RegularVarObject::setParent(Object* parent) {
	this->parent = parent;
}

void slake::RegularVarObject::dealloc() {
	std::pmr::polymorphic_allocator<RegularVarObject> allocator(&VarObject::_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

HostObjectRef<RegularVarObject> slake::RegularVarObject::alloc(Runtime *rt, AccessModifier access, const Type &type) {
	std::pmr::polymorphic_allocator<RegularVarObject> allocator(&rt->globalHeapPoolResource);

	RegularVarObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, access, type);

	rt->createdObjects.insert((VarObject*)ptr);

	return ptr;
}

HostObjectRef<RegularVarObject> slake::RegularVarObject::alloc(const RegularVarObject *other) {
	std::pmr::polymorphic_allocator<RegularVarObject> allocator(&other->Object::_rt->globalHeapPoolResource);

	RegularVarObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, *other);

	other->Object::_rt->createdObjects.insert((VarObject *)ptr);

	return ptr;
}
