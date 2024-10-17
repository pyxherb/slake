#include <slake/runtime.h>

using namespace slake;

SLAKE_API Object::Object(Runtime *rt) : associatedRuntime(rt) {
}

SLAKE_API Object::Object(const Object &x) {
	associatedRuntime = x.associatedRuntime;
	_flags = x._flags & ~VF_WALKED;
}

SLAKE_API Object::~Object() {
	associatedRuntime->invalidateGenericCache(this);
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
	if (!holdedObjects.count(object)) {
		holdedObjects.insert(object);
		++object->hostRefCount;
	}
}

SLAKE_API void HostRefHolder::removeObject(Object *object) noexcept {
	assert(holdedObjects.count(object));
	holdedObjects.erase(object);
	--object->hostRefCount;
}
