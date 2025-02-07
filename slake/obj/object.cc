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

SLAKE_API ObjectRef Object::getMember(const std::string_view &name) const {
	return ObjectRef::makeInstanceRef(nullptr);
}

SLAKE_API HostRefHolder::HostRefHolder(peff::Alloc *selfAllocator)
	: holdedObjects(selfAllocator) {
}

SLAKE_API HostRefHolder::~HostRefHolder() {
	for (auto i : holdedObjects)
		--i->hostRefCount;
}

SLAKE_API void HostRefHolder::addObject(Object *object) {
	if (!holdedObjects.contains(object)) {
		holdedObjects.insert(+object);
		++object->hostRefCount;
	}
}

SLAKE_API void HostRefHolder::removeObject(Object *object) noexcept {
	assert(holdedObjects.contains(object));
	holdedObjects.remove(object);
	--object->hostRefCount;
}
