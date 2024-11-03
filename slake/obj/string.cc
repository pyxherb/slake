#include <slake/runtime.h>

using namespace slake;

SLAKE_API void slake::StringObject::_setData(const char *str, size_t size) {
	if (!size)
		data.clear();
	else {
		data = std::string(str, size);
	}
}

SLAKE_API slake::StringObject::StringObject(Runtime *rt, const char *s, size_t size) : Object(rt), data(&rt->globalHeapPoolResource) {
	_setData(s, size);
}

SLAKE_API slake::StringObject::StringObject(Runtime *rt, std::string &&s) : Object(rt), data(&rt->globalHeapPoolResource) {
	data = std::move(s);
}

SLAKE_API StringObject::StringObject(const StringObject &x) : Object(x) {
	_setData(x.data.c_str(), x.data.size());
}

SLAKE_API StringObject::~StringObject() {
	_setData(nullptr, 0);
}

SLAKE_API ObjectKind StringObject::getKind() const { return ObjectKind::String; }

SLAKE_API Object *StringObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<StringObject> slake::StringObject::alloc(Runtime *rt, const char *str, size_t size) {
	using Alloc = std::pmr::polymorphic_allocator<StringObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<StringObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, str, size);

	rt->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<StringObject> slake::StringObject::alloc(const StringObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<StringObject>;
	Alloc allocator(&other->associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<StringObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->associatedRuntime->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<StringObject> slake::StringObject::alloc(Runtime *rt, std::string &&s) {
	using Alloc = std::pmr::polymorphic_allocator<StringObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<StringObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, std::move(s));

	rt->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::StringObject::dealloc() {
	std::pmr::polymorphic_allocator<StringObject> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
