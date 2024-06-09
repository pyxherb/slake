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

StringObject::~StringObject() {
	_setData(nullptr, 0);
}

Object *StringObject::duplicate() const {
	HostObjectRef<StringObject> v = StringObject::alloc(_rt, nullptr, 0);
	*(v.get()) = *this;

	return (Object *)v.release();
}

HostObjectRef<StringObject> slake::StringObject::alloc(Runtime *rt, const char *str, size_t size) {
	std::pmr::polymorphic_allocator<StringObject> allocator(&rt->globalHeapPoolResource);

	StringObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, str, size);

	rt->createdObjects.insert(ptr);

	return ptr;
}

HostObjectRef<StringObject> slake::StringObject::alloc(Runtime *rt, std::string &&s) {
	std::pmr::polymorphic_allocator<StringObject> allocator(&rt->globalHeapPoolResource);

	StringObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, std::move(s));

	rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::StringObject::dealloc() {
	std::pmr::polymorphic_allocator<StringObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
