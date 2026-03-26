#include "server.h"

using namespace slkc;

SLKC_API ServerAlloc::ServerAlloc(peff::Alloc *upstream) : upstream(upstream) {
}

SLKC_API ServerAlloc::~ServerAlloc() {
}

SLKC_API size_t ServerAlloc::inc_ref(size_t global_rc) noexcept {
	SLAKE_REFERENCED_PARAM(global_rc);

	return ++ref_count;
}

SLKC_API size_t ServerAlloc::dec_ref(size_t global_rc) noexcept {
	SLAKE_REFERENCED_PARAM(global_rc);

	if (!--ref_count) {
		on_ref_zero();
		return 0;
	}

	return ref_count;
}

SLKC_API void ServerAlloc::on_ref_zero() noexcept {
}

SLKC_API void *ServerAlloc::alloc(size_t size, size_t alignment) noexcept {
	if (sz_allocated + size > limit)
		return nullptr;
	void *p = upstream->alloc(size, alignment);
	if (!p)
		return nullptr;

	sz_allocated += size;

	return p;
}

SLKC_API void *ServerAlloc::realloc(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept {
	if (sz_allocated - size + new_size > limit)
		return nullptr;
	void *p = upstream->realloc(ptr, size, alignment, new_size, new_alignment);
	if (!p)
		return nullptr;

	sz_allocated -= size;
	sz_allocated += new_size;

	return p;
}

SLKC_API void ServerAlloc::release(void *ptr, size_t size, size_t alignment) noexcept {
	assert(size <= sz_allocated);

	upstream->release(ptr, size, alignment);

	sz_allocated -= size;
}

SLKC_API bool ServerAlloc::is_replaceable(const Alloc *rhs) const noexcept {
	if (type_identity() != rhs->type_identity())
		return false;

	ServerAlloc *r = (ServerAlloc *)rhs;

	if (upstream != r->upstream)
		return false;

	return true;
}

SLKC_API peff::UUID ServerAlloc::type_identity() const noexcept {
	return PEFF_UUID(1a2b3c4d, 5e6f, 7a8b, 9cad, 114514191981);
}
