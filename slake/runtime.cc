#include "runtime.h"

using namespace slake;

SLAKE_API CountablePoolResource::CountablePoolResource(std::pmr::memory_resource *upstream) : upstream(upstream) {}

SLAKE_API void *CountablePoolResource::do_allocate(size_t bytes, size_t alignment) {
	void *p = upstream->allocate(bytes, alignment);

	szAllocated += bytes;

	return p;
}

SLAKE_API void CountablePoolResource::do_deallocate(void *p, size_t bytes, size_t alignment) {
	upstream->deallocate(p, bytes, alignment);

	szAllocated -= bytes;
}

SLAKE_API bool CountablePoolResource::do_is_equal(const std::pmr::memory_resource &other) const noexcept {
	return this == &other;
}

SLAKE_API Runtime::Runtime(std::pmr::memory_resource *upstreamMemoryResource, RuntimeFlags flags)
	: globalHeapPoolResource(upstreamMemoryResource),
	  _flags(flags) {
	_flags |= _RT_INITING;
	_rootObject = RootObject::alloc(this).release();
	_flags &= ~_RT_INITING;
}

SLAKE_API Runtime::~Runtime() {
	_genericCacheDir.clear();
	_genericCacheLookupTable.clear();

	_rootObject = nullptr;
	activeContexts.clear();

	gc();

	assert(!createdObjects.size());
	assert(!globalHeapPoolResource.szAllocated);
}
