#include "runtime.h"

using namespace slake;

SLAKE_API CountablePoolAlloc slake::g_countablePoolDefaultAlloc(nullptr);

SLAKE_API CountablePoolAlloc::CountablePoolAlloc(peff::Alloc *upstream) : upstream(upstream) {}

SLAKE_API peff::Alloc *CountablePoolAlloc::getDefaultAlloc() const noexcept {
	return &g_countablePoolDefaultAlloc;
}

SLAKE_API void CountablePoolAlloc::onRefZero() noexcept {
}

SLAKE_API void *CountablePoolAlloc::alloc(size_t size, size_t alignment) noexcept {
	void *p = upstream->alloc(size, alignment);
	if (!p)
		return nullptr;

	szAllocated += size;

	return p;
}

SLAKE_API void CountablePoolAlloc::release(void *p, size_t size, size_t alignment) noexcept {
	assert(size <= szAllocated);
	upstream->release(p, size, alignment);

	szAllocated -= size;
}

SLAKE_API Runtime::Runtime(peff::Alloc *upstream, RuntimeFlags flags)
	: globalHeapPoolAlloc(upstream),
	  _flags(flags | _RT_INITING),
	  _genericCacheLookupTable(&globalHeapPoolAlloc),
	  _genericCacheDir(&globalHeapPoolAlloc),
	  createdObjects(&globalHeapPoolAlloc) {
	_rootObject = RootObject::alloc(this);
	_flags &= ~_RT_INITING;
}

SLAKE_API Runtime::~Runtime() {
	_genericCacheDir.clear();
	_genericCacheLookupTable.clear();

	activeContexts.clear();
	managedThreads.clear();

	_flags |= _RT_DEINITING;

	gc();

	_rootObject = nullptr;

	// No need to delete the root object explicitly.

	assert(!createdObjects.size());
	assert(!globalHeapPoolAlloc.szAllocated);
}
