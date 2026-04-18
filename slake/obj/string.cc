#include <slake/runtime.h>

using namespace slake;

SLAKE_API bool slake::StringObject::_set_data(const char *str, size_t size) {
	if (!size)
		data.clear_and_shrink();
	else {
		peff::String s(get_allocator());

		if (!s.resize_and_shrink(size))
			return false;

		memcpy(s.data(), str, size);

		data = std::move(s);
	}
	return true;
}

SLAKE_API slake::StringObject::StringObject(Runtime *rt, peff::Alloc *self_allocator) : Object(rt, self_allocator, ObjectKind::String), data(self_allocator) {
}

SLAKE_API StringObject::StringObject(const StringObject &x, peff::Alloc *allocator, bool &succeeded_out) : Object(x, allocator), data(allocator) {
	if (!_set_data(x.data.data(), x.data.size())) {
		succeeded_out = false;
		return;
	}
}

SLAKE_API StringObject::~StringObject() {
}

SLAKE_API Object *StringObject::duplicate(Duplicator *duplicator) const {
	SLAKE_REFERENCED_PARAM(duplicator);

	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<StringObject> slake::StringObject::alloc(const StringObject *other) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = other->associated_runtime->get_cur_gen_alloc();

	bool succeeded = true;

	std::unique_ptr<StringObject, peff::DeallocableDeleter<StringObject>> ptr(
		peff::alloc_and_construct<StringObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			*other, cur_generation_allocator.get(), succeeded));

	if (!succeeded)
		return nullptr;

	if (!other->associated_runtime->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<StringObject> slake::StringObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = rt->get_cur_gen_alloc();

	std::unique_ptr<StringObject, peff::DeallocableDeleter<StringObject>> ptr(
		peff::alloc_and_construct<StringObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			rt, cur_generation_allocator.get()));

	if (!rt->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::StringObject::dealloc() {
	peff::destroy_and_release<StringObject>(get_allocator(), this, alignof(StringObject));
}

SLAKE_API void StringObject::replace_allocator(peff::Alloc *allocator) noexcept {
	this->Object::replace_allocator(allocator);

	data.replace_allocator(allocator);
}
