#include <slake/runtime.h>

using namespace slake;

void slake::StringObject::_setData(const char *str, size_t size) {
	if (!size)
		data.clear();
	else {
		data = std::string(str, size);
	}
}

slake::StringObject::StringObject(Runtime *rt, const char *s, size_t size) : Object(rt), data(&rt->globalHeapPoolResource) {
	_setData(s, size);
}

slake::StringObject::StringObject(Runtime *rt, std::string &&s) : Object(rt), data(&rt->globalHeapPoolResource) {
	data = std::move(s);
}

StringObject::StringObject(const StringObject &x) : Object(x) {
	_setData(x.data.c_str(), x.data.size());
}

StringObject::~StringObject() {
	_setData(nullptr, 0);
}

Object *StringObject::duplicate() const {
	return (Object *)alloc(this).get();
}

HostObjectRef<StringObject> slake::StringObject::alloc(Runtime *rt, const char *str, size_t size) {
	using Alloc = std::pmr::polymorphic_allocator<StringObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<StringObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, str, size);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

HostObjectRef<StringObject> slake::StringObject::alloc(const StringObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<StringObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<StringObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

HostObjectRef<StringObject> slake::StringObject::alloc(Runtime *rt, std::string &&s) {
	using Alloc = std::pmr::polymorphic_allocator<StringObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<StringObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, std::move(s));

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

void slake::StringObject::dealloc() {
	std::pmr::polymorphic_allocator<StringObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
