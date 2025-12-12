#ifndef _SLKC_SERVER_SERVER_H_
#define _SLKC_SERVER_SERVER_H_

#include "../comp/compiler.h"

namespace slkc {
	class ServerAlloc : public peff::Alloc {
	public:
		peff::RcObjectPtr<peff::Alloc> upstream;
		size_t limit = 1024 * 1024 * 1024;
		size_t szAllocated = 0;
		std::atomic_size_t refCount = 0;

		SLKC_API ServerAlloc(peff::Alloc *upstream);
		SLKC_API virtual ~ServerAlloc();

		SLKC_API virtual size_t incRef(size_t globalRc) noexcept override;
		SLKC_API virtual size_t decRef(size_t globalRc) noexcept override;

		SLKC_API void onRefZero() noexcept;

		SLKC_API virtual void *alloc(size_t size, size_t alignment) noexcept override;
		SLKC_API virtual void *realloc(void *ptr, size_t size, size_t alignment, size_t newSize, size_t newAlignment) noexcept override;
		SLKC_API virtual void release(void *ptr, size_t size, size_t alignment) noexcept override;

		SLKC_API virtual bool isReplaceable(const Alloc *rhs) const noexcept override;

		SLKC_API virtual peff::UUID getTypeId() const noexcept override;
	};
}

#endif
