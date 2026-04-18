#include "instance.h"

#include <slake/runtime.h>

using namespace slake;

SLAKE_API InstanceObject::InstanceObject(Runtime *rt, peff::Alloc *self_allocator)
	: Object(rt, self_allocator, ObjectKind::Instance) {
}

SLAKE_API InstanceObject::InstanceObject(const InstanceObject &x, peff::Alloc *allocator) : Object(x, allocator) {
	_class = x._class;
	// TODO: Copy the raw_field_data.
}

SLAKE_API InstanceObject::~InstanceObject() {
	if (raw_field_data)
		delete[] raw_field_data;

	// DO NOT DELETE THE OBJECT LAYOUT AND THE METHOD TABLE!!!
	// They are borrowed from the class.
}

SLAKE_API Object *InstanceObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(this).get();
}

SLAKE_API Reference InstanceObject::get_member(const std::string_view &name) const {
	if (auto it = _class->cached_instantiated_method_table->methods.find(name);
		it != _class->cached_instantiated_method_table->methods.end())
		return Reference(it.value());

	if (auto it = _class->cached_object_layout->field_name_map.find(name);
		it != _class->cached_object_layout->field_name_map.end()) {
		return ObjectFieldRef((InstanceObject *)this, it.value());
	}

	return ReferenceKind::Invalid;
}

SLAKE_API HostObjectRef<InstanceObject> slake::InstanceObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = rt->get_cur_gen_alloc();

	std::unique_ptr<InstanceObject, peff::DeallocableDeleter<InstanceObject>> ptr(
		peff::alloc_and_construct<InstanceObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			rt, cur_generation_allocator.get()));

	if (!rt->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<InstanceObject> slake::InstanceObject::alloc(const InstanceObject *other) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = other->associated_runtime->get_cur_gen_alloc();

	std::unique_ptr<InstanceObject, peff::DeallocableDeleter<InstanceObject>> ptr(
		peff::alloc_and_construct<InstanceObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			*other, cur_generation_allocator.get()));

	if (!other->associated_runtime->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::InstanceObject::dealloc() {
	peff::destroy_and_release<InstanceObject>(get_allocator(), this, alignof(InstanceObject));
}
