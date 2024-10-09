#include <slake/runtime.h>

using namespace slake;

Object::Object(Runtime *rt) : _rt(rt) {
}

Object::Object(const Object &x) {
	_rt = x._rt;
	_flags = x._flags & ~VF_WALKED;
}

Object::~Object() {
	_rt->invalidateGenericCache(this);
}

Object *Object::duplicate() const {
	throw std::logic_error("duplicate() method is not supported");
}

MemberObject *Object::getMember(
	const std::pmr::string &name,
	VarRefContext *varRefContextOut) const {
	return nullptr;
}

HostRefHolder::HostRefHolder(std::pmr::memory_resource *memoryResource)
	: holdedObjects(memoryResource) {
}

HostRefHolder::~HostRefHolder() {
	for (auto i : holdedObjects)
		--i->hostRefCount;
}

void HostRefHolder::addObject(Object* object) {
	holdedObjects.insert(object);
	++object->hostRefCount;
}

void HostRefHolder::removeObject(Object *object) noexcept {
	holdedObjects.erase(object);
	++object->hostRefCount;
}
