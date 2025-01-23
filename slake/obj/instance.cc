#include "instance.h"

#include <slake/runtime.h>

using namespace slake;

SLAKE_API InstanceMemberAccessorVarObject::InstanceMemberAccessorVarObject(
	Runtime *rt,
	InstanceObject *instanceObject)
	: VarObject(rt, VarKind::InstanceMemberAccessor), instanceObject(instanceObject) {
}

SLAKE_API InstanceMemberAccessorVarObject::~InstanceMemberAccessorVarObject() {
}

SLAKE_API HostObjectRef<InstanceMemberAccessorVarObject> slake::InstanceMemberAccessorVarObject::alloc(Runtime *rt, InstanceObject *instanceObject) {
	std::unique_ptr<InstanceMemberAccessorVarObject, util::DeallocableDeleter<InstanceMemberAccessorVarObject>>
		ptr(peff::allocAndConstruct<InstanceMemberAccessorVarObject>(&rt->globalHeapPoolAlloc, sizeof(std::max_align_t), rt, instanceObject));

	if (!rt->createdObjects.pushBack(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::InstanceMemberAccessorVarObject::dealloc() {
	peff::destroyAndRelease<InstanceMemberAccessorVarObject>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API InstanceObject::InstanceObject(Runtime *rt)
	: Object(rt) {
}

SLAKE_API InstanceObject::InstanceObject(const InstanceObject &x) : Object(x) {
	_class = x._class;
	objectLayout = x.objectLayout;
	methodTable = x.methodTable;
	// TODO: Copy the rawFieldData.
}

SLAKE_API InstanceObject::~InstanceObject() {
	if (rawFieldData)
		delete[] rawFieldData;

	// DO NOT DELETE THE OBJECT LAYOUT AND THE METHOD TABLE!!!
	// They are borrowed from the class.
}

SLAKE_API ObjectKind InstanceObject::getKind() const { return ObjectKind::Instance; }

SLAKE_API Object *InstanceObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API MemberObject *InstanceObject::getMember(
	const peff::String &name,
	VarRefContext *varRefContextOut) const {
	if (auto it = methodTable->methods.find((std::string_view)name);
		it != methodTable->methods.end())
		return it.value();

	if (auto it = objectLayout->fieldNameMap.find(name);
		it != objectLayout->fieldNameMap.end()) {
		if (varRefContextOut) {
			*varRefContextOut = VarRefContext::makeInstanceContext(it.value());
		}

		return memberAccessor;
	}

	return nullptr;
}

SLAKE_API HostObjectRef<InstanceObject> slake::InstanceObject::alloc(Runtime *rt) {
	std::unique_ptr<InstanceObject, util::DeallocableDeleter<InstanceObject>> ptr(
		peff::allocAndConstruct<InstanceObject>(
			&rt->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			rt));

	if (!(ptr->memberAccessor = InstanceMemberAccessorVarObject::alloc(rt, ptr.get()).get())) {
		return nullptr;
	}

	if (!rt->createdObjects.pushBack(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<InstanceObject> slake::InstanceObject::alloc(const InstanceObject *other) {
	std::unique_ptr<InstanceObject, util::DeallocableDeleter<InstanceObject>> ptr(
		peff::allocAndConstruct<InstanceObject>(
			&other->associatedRuntime->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			*other));

	if (!(ptr->memberAccessor = InstanceMemberAccessorVarObject::alloc(other->associatedRuntime, ptr.get()).get())) {
		return nullptr;
	}

	if (!other->associatedRuntime->createdObjects.pushBack(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::InstanceObject::dealloc() {
	peff::destroyAndRelease<InstanceObject>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}
