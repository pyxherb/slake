#include <slake/runtime.h>
#include <slake/type.h>

using namespace slake;

SLAKE_API MemberObject::MemberObject(Runtime *rt, peff::Alloc *self_allocator, ObjectKind object_kind)
	: Object(rt, self_allocator, object_kind),_name(self_allocator) {
}

SLAKE_API MemberObject::MemberObject(const MemberObject &x, peff::Alloc *allocator, bool &succeeded_out)
	: Object(x, allocator),
	  _name(allocator) {
	_access_modifier = x._access_modifier;
	if (!_name.build(x._name)) {
		succeeded_out = false;
		return;
	}
}

SLAKE_API MemberObject::~MemberObject() {
	associated_runtime->invalidate_generic_cache(this);
}

SLAKE_API const peff::DynArray<Value> *MemberObject::get_generic_args() const {
	return nullptr;
}

SLAKE_API std::string_view MemberObject::get_name() const noexcept {
	return _name;
}

SLAKE_API bool MemberObject::set_name(const std::string_view &name) noexcept {
	if (!_name.build_and_shrink(name)) {
		return false;
	}
	return true;
}

SLAKE_API bool MemberObject::resize_name(size_t size) noexcept {
	return _name.resize_and_shrink(size);
}

SLAKE_API char *MemberObject::get_name_raw_ptr() noexcept {
	return _name.data();
}

SLAKE_API AccessModifier MemberObject::get_access() const noexcept {
	return _access_modifier;
}

SLAKE_API void MemberObject::set_access(AccessModifier access_modifier) noexcept {
	this->_access_modifier = access_modifier;
}

SLAKE_API void MemberObject::replace_allocator(peff::Alloc* allocator) noexcept {
	this->Object::replace_allocator(allocator);

	_name.replace_allocator(allocator);
}
