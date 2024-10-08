#include <slake/runtime.h>

using namespace slake;

SLAKE_API Object::Object(Runtime *rt) : _rt(rt) {
}

SLAKE_API Object::Object(const Object &x) {
	_rt = x._rt;
	_flags = x._flags & ~VF_WALKED;
}

SLAKE_API Object::~Object() {
	_rt->invalidateGenericCache(this);
}

SLAKE_API Object *Object::duplicate() const {
	throw std::logic_error("duplicate() method is not supported");
}

SLAKE_API MemberObject *Object::getMember(
	const std::pmr::string &name,
	VarRefContext *varRefContextOut) const {
	return nullptr;
}

SLAKE_API HostRefHolder::HostRefHolder(std::pmr::memory_resource *memoryResource)
	: holdedObjects(memoryResource) {
}

SLAKE_API HostRefHolder::~HostRefHolder() {
	for (auto i : holdedObjects)
		--i->hostRefCount;
}

SLAKE_API void HostRefHolder::addObject(Object *object) {
	holdedObjects.insert(object);
	++object->hostRefCount;
}

SLAKE_API void HostRefHolder::removeObject(Object *object) noexcept {
	holdedObjects.erase(object);
	++object->hostRefCount;
}
