#include <slake/runtime.h>

using namespace slake;

SLAKE_API bool slake::StringObject::_setData(const char *str, size_t size) {
	if (!size)
		data.clear();
	else {
		peff::String s(&associatedRuntime->globalHeapPoolAlloc);

		if (!s.resize(size))
			return false;

		memcpy(s.data(), str, size);

		data = std::move(s);
	}
	return true;
}

SLAKE_API slake::StringObject::StringObject(Runtime *rt, peff::String &&s) : Object(rt), data(&rt->globalHeapPoolAlloc) {
	data = std::move(s);
}

SLAKE_API StringObject::StringObject(const StringObject &x, bool &succeededOut) : Object(x), data(&x.associatedRuntime->globalHeapPoolAlloc) {
	if (!_setData(x.data.data(), x.data.size())) {
		succeededOut = false;
		return;
	}
}

SLAKE_API StringObject::~StringObject() {
}

SLAKE_API ObjectKind StringObject::getKind() const { return ObjectKind::String; }

SLAKE_API Object *StringObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<StringObject> slake::StringObject::alloc(const StringObject *other) {
	bool succeeded = true;

	std::unique_ptr<StringObject, util::DeallocableDeleter<StringObject>> ptr(
		peff::allocAndConstruct<StringObject>(
			&other->associatedRuntime->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			*other, succeeded));

	if (!succeeded)
		return nullptr;

	if (!other->associatedRuntime->createdObjects.insert(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<StringObject> slake::StringObject::alloc(Runtime *rt, peff::String &&s) {
	std::unique_ptr<StringObject, util::DeallocableDeleter<StringObject>> ptr(
		peff::allocAndConstruct<StringObject>(
			&rt->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			rt, std::move(s)));

	if (!rt->createdObjects.insert(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::StringObject::dealloc() {
	peff::destroyAndRelease<StringObject>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}
