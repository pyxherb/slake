#include "instance.h"

#include <slake/runtime.h>

using namespace slake;

SLAKE_API InstanceMemberAccessorVarObject::InstanceMemberAccessorVarObject(
	Runtime *rt,
	InstanceObject *instanceObject)
	: VarObject(rt), instanceObject(instanceObject) {
}

SLAKE_API InstanceMemberAccessorVarObject::~InstanceMemberAccessorVarObject() {
}

SLAKE_API HostObjectRef<InstanceMemberAccessorVarObject> slake::InstanceMemberAccessorVarObject::alloc(Runtime *rt, InstanceObject *instanceObject) {
	using Alloc = std::pmr::polymorphic_allocator<InstanceMemberAccessorVarObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<InstanceMemberAccessorVarObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, instanceObject);

	rt->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::InstanceMemberAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<InstanceMemberAccessorVarObject> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API InstanceObject::InstanceObject(Runtime *rt)
	: Object(rt) {
	memberAccessor = InstanceMemberAccessorVarObject::alloc(rt, this).get();
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
	const std::pmr::string &name,
	VarRefContext *varRefContextOut) const {
	if (auto it = methodTable->methods.find(name);
		it != methodTable->methods.end())
		return it->second;

	if (auto it = objectLayout->fieldNameMap.find(name);
		it != objectLayout->fieldNameMap.end()) {
		if (varRefContextOut) {
			*varRefContextOut = VarRefContext::makeInstanceContext(it->second);
		}

		return memberAccessor;
	}

	return nullptr;
}

SLAKE_API HostObjectRef<InstanceObject> slake::InstanceObject::alloc(Runtime *rt) {
	using Alloc = std::pmr::polymorphic_allocator<InstanceObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<InstanceObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt);

	rt->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<InstanceObject> slake::InstanceObject::alloc(const InstanceObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<InstanceObject>;
	Alloc allocator(&other->associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<InstanceObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->associatedRuntime->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::InstanceObject::dealloc() {
	std::pmr::polymorphic_allocator<InstanceObject> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
