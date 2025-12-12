#include "server.h"

using namespace slkc;

SLKC_API ServerAlloc::ServerAlloc(peff::Alloc *upstream) : upstream(upstream) {
}

SLKC_API ServerAlloc::~ServerAlloc() {
}

SLKC_API size_t ServerAlloc::incRef(size_t globalRc) noexcept {
	SLAKE_REFERENCED_PARAM(globalRc);

	return ++refCount;
}

SLKC_API size_t ServerAlloc::decRef(size_t globalRc) noexcept {
	SLAKE_REFERENCED_PARAM(globalRc);

	if (!--refCount) {
		onRefZero();
		return 0;
	}

	return refCount;
}

SLKC_API void ServerAlloc::onRefZero() noexcept {
}

SLKC_API void *ServerAlloc::alloc(size_t size, size_t alignment) noexcept {
	if (szAllocated + size > limit)
		return nullptr;
	void *p = upstream->alloc(size, alignment);
	if (!p)
		return nullptr;

	szAllocated += size;

	return p;
}

SLKC_API void *ServerAlloc::realloc(void *ptr, size_t size, size_t alignment, size_t newSize, size_t newAlignment) noexcept {
	if (szAllocated - size + newSize > limit)
		return nullptr;
	void *p = upstream->realloc(ptr, size, alignment, newSize, newAlignment);
	if (!p)
		return nullptr;

	szAllocated -= size;
	szAllocated += newSize;

	return p;
}

SLKC_API void ServerAlloc::release(void *ptr, size_t size, size_t alignment) noexcept {
	assert(size <= szAllocated);

	upstream->release(ptr, size, alignment);

	szAllocated -= size;
}

SLKC_API bool ServerAlloc::isReplaceable(const Alloc *rhs) const noexcept {
	if (getTypeId() != rhs->getTypeId())
		return false;

	ServerAlloc *r = (ServerAlloc *)rhs;

	if (upstream != r->upstream)
		return false;

	return true;
}

SLKC_API peff::UUID ServerAlloc::getTypeId() const noexcept {
	return PEFF_UUID(1a2b3c4d, 5e6f, 7a8b, 9cad, 114514191981);
}
