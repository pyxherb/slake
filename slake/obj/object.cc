#include <slake/runtime.h>

using namespace slake;

SLAKE_API Object::Object(Runtime *rt, peff::Alloc *selfAllocator) : associatedRuntime(rt), selfAllocator(selfAllocator) {
}

SLAKE_API Object::Object(const Object &x, peff::Alloc *allocator) {
	associatedRuntime = x.associatedRuntime;
	selfAllocator = allocator;
	_flags = x._flags & ~VF_WALKED;
}

SLAKE_API Object::~Object() {
	associatedRuntime->invalidateGenericCache(this);
}

SLAKE_API Object *Object::duplicate() const {
	throw std::logic_error("duplicate() method is not supported");
}

SLAKE_API void Object::replaceAllocator(peff::Alloc* allocator) noexcept {
	peff::verifyReplaceable(selfAllocator.get(), allocator);

	selfAllocator = allocator;
}

SLAKE_API EntityRef Object::getMember(const std::string_view &name) const {
	return EntityRef::makeObjectRef(nullptr);
}

SLAKE_API HostRefHolder::HostRefHolder(peff::Alloc *selfAllocator)
	: holdedObjects(selfAllocator) {
}

SLAKE_API HostRefHolder::~HostRefHolder() {
	for (auto i : holdedObjects)
		--i->hostRefCount;
}

SLAKE_API bool HostRefHolder::addObject(Object *object) {
	if (!holdedObjects.contains(object)) {
		if (!holdedObjects.insert(+object))
			return false;
		++object->hostRefCount;
	}
	return true;
}

SLAKE_API void HostRefHolder::removeObject(Object *object) noexcept {
	assert(holdedObjects.contains(object));
	holdedObjects.remove(object);
	--object->hostRefCount;
}
