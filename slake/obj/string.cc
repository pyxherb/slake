#include <slake/runtime.h>

using namespace slake;

SLAKE_API bool slake::StringObject::_setData(const char *str, size_t size) {
	if (!size)
		data.clear();
	else {
		peff::String s(selfAllocator.get());

		if (!s.resize(size))
			return false;

		memcpy(s.data(), str, size);

		data = std::move(s);
	}
	return true;
}

SLAKE_API slake::StringObject::StringObject(Runtime *rt, peff::Alloc *selfAllocator) : Object(rt, selfAllocator, ObjectKind::String), data(selfAllocator) {
}

SLAKE_API StringObject::StringObject(const StringObject &x, peff::Alloc *allocator, bool &succeededOut) : Object(x, allocator), data(allocator) {
	if (!_setData(x.data.data(), x.data.size())) {
		succeededOut = false;
		return;
	}
}

SLAKE_API StringObject::~StringObject() {
}

SLAKE_API Object *StringObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<StringObject> slake::StringObject::alloc(const StringObject *other) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = other->associatedRuntime->getCurGenAlloc();

	bool succeeded = true;

	std::unique_ptr<StringObject, util::DeallocableDeleter<StringObject>> ptr(
		peff::allocAndConstruct<StringObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			*other, curGenerationAllocator.get(), succeeded));

	if (!succeeded)
		return nullptr;

	if (!other->associatedRuntime->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<StringObject> slake::StringObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<StringObject, util::DeallocableDeleter<StringObject>> ptr(
		peff::allocAndConstruct<StringObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			rt, curGenerationAllocator.get()));

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::StringObject::dealloc() {
	peff::destroyAndRelease<StringObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API void StringObject::replaceAllocator(peff::Alloc *allocator) noexcept {
	this->Object::replaceAllocator(allocator);

	data.replaceAllocator(allocator);
}
