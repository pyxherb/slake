#include "var.h"
#include <slake/runtime.h>

using namespace slake;

SLAKE_API VarObject::VarObject(Runtime *rt, VarKind varKind) : MemberObject(rt), varKind(varKind) {
}

SLAKE_API VarObject::VarObject(const VarObject &x) : MemberObject(x), varKind(x.varKind) {
}

SLAKE_API VarObject::~VarObject() {
}

SLAKE_API ObjectKind VarObject::getKind() const { return ObjectKind::Var; }

SLAKE_API slake::RegularVarObject::RegularVarObject(Runtime *rt, AccessModifier access, const Type &type)
	: VarObject(rt, VarKind::Regular), type(type) {
	this->accessModifier = access;
}

SLAKE_API RegularVarObject::RegularVarObject(const RegularVarObject &other) : VarObject(other) {
	value = other.value;
	type = other.type;

	name = other.name;
	parent = other.parent;
}

SLAKE_API RegularVarObject::~RegularVarObject() {
}

SLAKE_API Object *RegularVarObject::duplicate() const {
	return (Object *)(VarObject *)alloc(this).get();
}

SLAKE_API const char *RegularVarObject::getName() const {
	return name.c_str();
}

SLAKE_API void RegularVarObject::setName(const char *name) {
	this->name = name;
}

SLAKE_API Object *RegularVarObject::getParent() const {
	return parent;
}

SLAKE_API void RegularVarObject::setParent(Object *parent) {
	this->parent = parent;
}

SLAKE_API void slake::RegularVarObject::dealloc() {
	std::pmr::polymorphic_allocator<RegularVarObject> allocator(&VarObject::associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API ObjectKind RegularVarObject::getKind() const { return ObjectKind::Var; }

SLAKE_API HostObjectRef<RegularVarObject> slake::RegularVarObject::alloc(Runtime *rt, AccessModifier access, const Type &type) {
	using Alloc = std::pmr::polymorphic_allocator<RegularVarObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<RegularVarObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, access, type);

	rt->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<RegularVarObject> slake::RegularVarObject::alloc(const RegularVarObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<RegularVarObject>;
	Alloc allocator(&other->associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<RegularVarObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->Object::associatedRuntime->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API LocalVarAccessorVarObject::LocalVarAccessorVarObject(
	Runtime *rt,
	Context *context,
	MajorFrame *majorFrame)
	: VarObject(rt, VarKind::LocalVarAccessor), context(context), majorFrame(majorFrame) {
}

SLAKE_API LocalVarAccessorVarObject::~LocalVarAccessorVarObject() {
}

SLAKE_API HostObjectRef<LocalVarAccessorVarObject> slake::LocalVarAccessorVarObject::alloc(
	Runtime *rt,
	Context *context,
	MajorFrame *majorFrame) {
	using Alloc = std::pmr::polymorphic_allocator<LocalVarAccessorVarObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<LocalVarAccessorVarObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, context, majorFrame);

	rt->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::LocalVarAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<LocalVarAccessorVarObject> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

MismatchedVarTypeError *slake::raiseMismatchedVarTypeError(Runtime *rt) {
	return MismatchedVarTypeError::alloc(rt);
}
