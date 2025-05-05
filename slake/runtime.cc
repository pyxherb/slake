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

SLAKE_API Runtime::Runtime(peff::Alloc *selfAllocator, peff::Alloc *upstream, RuntimeFlags flags)
	: selfAllocator(selfAllocator),
	  globalHeapPoolAlloc(upstream),
	  _flags(flags | _RT_INITING),
	  _genericCacheLookupTable(&globalHeapPoolAlloc),
	  _genericCacheDir(&globalHeapPoolAlloc),
	  createdObjects(&globalHeapPoolAlloc) {
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
	// Self allocator should be moved out in the dealloc() method, or the runtime has been destructed prematurely.
	assert(!selfAllocator);
}

SLAKE_API bool Runtime::constructAt(Runtime *dest, peff::Alloc *upstream, RuntimeFlags flags) {
	peff::constructAt<Runtime>(dest, nullptr, upstream, flags);
	peff::ScopeGuard destroyGuard([dest]() noexcept {
		std::destroy_at<Runtime>(dest);
	});
	if (!(dest->_rootObject = ModuleObject::alloc(dest, ACCESS_STATIC).get())) {
		return false;
	}
	destroyGuard.release();
	return true;
}

SLAKE_API Runtime *Runtime::alloc(peff::Alloc *selfAllocator, peff::Alloc *upstream, RuntimeFlags flags) {
	Runtime *runtime = nullptr;

	if (!(runtime = (Runtime*)selfAllocator->alloc(sizeof(Runtime), alignof(Runtime)))) {
		return nullptr;
	}

	peff::ScopeGuard releaseGuard([runtime, selfAllocator]() noexcept {
		selfAllocator->release(runtime, sizeof(Runtime), alignof(Runtime));
	});

	if (!constructAt(runtime, upstream, flags)) {
		return nullptr;
	}
	runtime->selfAllocator = selfAllocator;

	releaseGuard.release();
	return runtime;
}

SLAKE_API void Runtime::dealloc() noexcept {
	peff::RcObjectPtr<peff::Alloc> selfAllocator = std::move(this->selfAllocator);
	std::destroy_at<Runtime>(this);
	if (selfAllocator) {
		selfAllocator->release(this, sizeof(Runtime), alignof(Runtime));
	}
}
