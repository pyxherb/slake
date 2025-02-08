#include "var.h"
#include <slake/runtime.h>

using namespace slake;

SLAKE_API VarObject::VarObject(Runtime *rt, VarKind varKind) : MemberObject(rt), varKind(varKind) {
}

SLAKE_API VarObject::VarObject(const VarObject &x, bool &succeededOut) : MemberObject(x, succeededOut), varKind(x.varKind) {
}

SLAKE_API VarObject::~VarObject() {
}

SLAKE_API ObjectKind VarObject::getKind() const { return ObjectKind::String; }

SLAKE_API slake::RegularVarObject::RegularVarObject(Runtime *rt, AccessModifier access, const Type &type)
	: VarObject(rt, VarKind::Regular), value(ValueType::Undefined), type(type) {
	this->accessModifier = access;
}

SLAKE_API RegularVarObject::RegularVarObject(const RegularVarObject &other, bool &succeededOut) : VarObject(other, succeededOut) {
	if (succeededOut) {
		value = other.value;
		type = other.type;

		parent = other.parent;
	}
}

SLAKE_API RegularVarObject::~RegularVarObject() {
}

SLAKE_API Object *RegularVarObject::duplicate() const {
	return (Object *)(VarObject *)alloc(this).get();
}

SLAKE_API Object *RegularVarObject::getParent() const {
	return parent;
}

SLAKE_API void RegularVarObject::setParent(Object *parent) {
	this->parent = parent;
}

SLAKE_API void slake::RegularVarObject::dealloc() {
	peff::destroyAndRelease<RegularVarObject>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API ObjectKind RegularVarObject::getKind() const { return ObjectKind::String; }

SLAKE_API HostObjectRef<RegularVarObject> slake::RegularVarObject::alloc(Runtime *rt, AccessModifier access, const Type &type) {
	std::unique_ptr<RegularVarObject, util::DeallocableDeleter<RegularVarObject>> ptr(
		peff::allocAndConstruct<RegularVarObject>(
			&rt->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			rt, access, type));
	if (!ptr)
		return nullptr;

	if (!rt->createdObjects.pushBack(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<RegularVarObject> slake::RegularVarObject::alloc(const RegularVarObject *other) {
	bool succeeded = true;

	std::unique_ptr<RegularVarObject, util::DeallocableDeleter<RegularVarObject>> ptr(
		peff::allocAndConstruct<RegularVarObject>(
			&other->associatedRuntime->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			*other, succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associatedRuntime->createdObjects.pushBack(ptr.get()))
		return nullptr;

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
	std::unique_ptr<LocalVarAccessorVarObject, util::DeallocableDeleter<LocalVarAccessorVarObject>> ptr(
		peff::allocAndConstruct<LocalVarAccessorVarObject>(
			&rt->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			rt, context, majorFrame));
	if (!ptr)
		return nullptr;

	if (!rt->createdObjects.pushBack(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::LocalVarAccessorVarObject::dealloc() {
	peff::destroyAndRelease<LocalVarAccessorVarObject>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

MismatchedVarTypeError *slake::raiseMismatchedVarTypeError(Runtime *rt) {
	return MismatchedVarTypeError::alloc(rt);
}
