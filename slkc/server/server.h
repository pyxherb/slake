#ifndef _SLKC_SERVER_SERVER_H_
#define _SLKC_SERVER_SERVER_H_

#include "../comp/compiler.h"

namespace slkc {
	class ServerAlloc : public peff::Alloc {
	public:
		peff::RcObjectPtr<peff::Alloc> upstream;
		size_t limit = 1024 * 1024 * 1024;
		size_t sz_allocated = 0;
		std::atomic_size_t ref_count = 0;

		SLKC_API ServerAlloc(peff::Alloc *upstream);
		SLKC_API virtual ~ServerAlloc();

		SLKC_API virtual size_t inc_ref(size_t global_rc) noexcept override;
		SLKC_API virtual size_t dec_ref(size_t global_rc) noexcept override;

		SLKC_API void on_ref_zero() noexcept;

		SLKC_API virtual void *alloc(size_t size, size_t alignment) noexcept override;
		SLKC_API virtual void *realloc(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept override;
		SLKC_API virtual void release(void *ptr, size_t size, size_t alignment) noexcept override;

		SLKC_API virtual bool is_replaceable(const Alloc *rhs) const noexcept override;

		SLKC_API virtual peff::UUID type_identity() const noexcept override;
	};
}

#endif
