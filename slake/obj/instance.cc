#include "instance.h"

#include <slake/runtime.h>

using namespace slake;

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
	for (auto i : methodTable->nativeDestructors)
		i(this);

	if (rawFieldData)
		delete[] rawFieldData;

	// DO NOT DELETE THE OBJECT LAYOUT AND THE METHOD TABLE!!!
	// They are borrowed from the class.
}

SLAKE_API ObjectKind InstanceObject::getKind() const { return ObjectKind::Instance; }

SLAKE_API Object *InstanceObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API ObjectRef InstanceObject::getMember(const std::string_view &name) const {
	if (auto it = methodTable->methods.find(name);
		it != methodTable->methods.end())
		return ObjectRef::makeInstanceRef(it.value());

	if (auto it = objectLayout->fieldNameMap.find(name);
		it != objectLayout->fieldNameMap.end()) {
		return ObjectRef::makeInstanceFieldRef((InstanceObject *)this, it.value());
	}

	return ObjectRef::makeInstanceRef(nullptr);
}

SLAKE_API HostObjectRef<InstanceObject> slake::InstanceObject::alloc(Runtime *rt) {
	std::unique_ptr<InstanceObject, util::DeallocableDeleter<InstanceObject>> ptr(
		peff::allocAndConstruct<InstanceObject>(
			&rt->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			rt));

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

	if (!other->associatedRuntime->createdObjects.pushBack(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::InstanceObject::dealloc() {
	peff::destroyAndRelease<InstanceObject>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}
