#include "instance.h"

#include <slake/runtime.h>

using namespace slake;

SLAKE_API InstanceObject::InstanceObject(Runtime *rt, peff::Alloc *selfAllocator)
	: Object(rt, selfAllocator, ObjectKind::Instance) {
}

SLAKE_API InstanceObject::InstanceObject(const InstanceObject &x, peff::Alloc *allocator) : Object(x, allocator) {
	_class = x._class;
	// TODO: Copy the rawFieldData.
}

SLAKE_API InstanceObject::~InstanceObject() {
	if (rawFieldData)
		delete[] rawFieldData;

	// DO NOT DELETE THE OBJECT LAYOUT AND THE METHOD TABLE!!!
	// They are borrowed from the class.
}

SLAKE_API Object *InstanceObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(this).get();
}

SLAKE_API Reference InstanceObject::getMember(const std::string_view &name) const {
	if (auto it = _class->cachedInstantiatedMethodTable->methods.find(name);
		it != _class->cachedInstantiatedMethodTable->methods.end())
		return Reference(it.value());

	if (auto it = _class->cachedObjectLayout->fieldNameMap.find(name);
		it != _class->cachedObjectLayout->fieldNameMap.end()) {
		return ObjectFieldRef((InstanceObject *)this, it.value());
	}

	return ReferenceKind::Invalid;
}

SLAKE_API HostObjectRef<InstanceObject> slake::InstanceObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<InstanceObject, peff::DeallocableDeleter<InstanceObject>> ptr(
		peff::allocAndConstruct<InstanceObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			rt, curGenerationAllocator.get()));

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<InstanceObject> slake::InstanceObject::alloc(const InstanceObject *other) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = other->associatedRuntime->getCurGenAlloc();

	std::unique_ptr<InstanceObject, peff::DeallocableDeleter<InstanceObject>> ptr(
		peff::allocAndConstruct<InstanceObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			*other, curGenerationAllocator.get()));

	if (!other->associatedRuntime->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::InstanceObject::dealloc() {
	peff::destroyAndRelease<InstanceObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}
