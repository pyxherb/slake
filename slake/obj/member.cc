#include <slake/runtime.h>
#include <slake/type.h>

using namespace slake;

SLAKE_API MemberObject::MemberObject(Runtime *rt, peff::Alloc *selfAllocator, ObjectKind objectKind)
	: Object(rt, selfAllocator, objectKind),_name(selfAllocator) {
}

SLAKE_API MemberObject::MemberObject(const MemberObject &x, peff::Alloc *allocator, bool &succeededOut)
	: Object(x, allocator),
	  _name(allocator) {
	_accessModifier = x._accessModifier;
	if (!_name.build(x._name)) {
		succeededOut = false;
		return;
	}
}

SLAKE_API MemberObject::~MemberObject() {
	associatedRuntime->invalidateGenericCache(this);
}

SLAKE_API const peff::DynArray<Value> *MemberObject::getGenericArgs() const {
	return nullptr;
}

SLAKE_API std::string_view MemberObject::getName() const noexcept {
	return _name;
}

SLAKE_API bool MemberObject::setName(const std::string_view &name) noexcept {
	if (!_name.build(name)) {
		return false;
	}
	return true;
}

SLAKE_API bool MemberObject::resizeName(size_t size) noexcept {
	return _name.resize(size);
}

SLAKE_API char *MemberObject::getNameRawPtr() noexcept {
	return _name.data();
}

SLAKE_API AccessModifier MemberObject::getAccess() const noexcept {
	return _accessModifier;
}

SLAKE_API void MemberObject::setAccess(AccessModifier accessModifier) noexcept {
	this->_accessModifier = accessModifier;
}

SLAKE_API void MemberObject::replaceAllocator(peff::Alloc* allocator) noexcept {
	this->Object::replaceAllocator(allocator);

	_name.replaceAllocator(allocator);
}
