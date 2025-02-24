#ifndef _SLAKE_FLIB_BITOP_H_
#define _SLAKE_FLIB_BITOP_H_

#include <slake/basedefs.h>
#include <cstdint>

namespace slake {
	namespace flib {
		SLAKE_FORCEINLINE int8_t shrSigned8(int8_t lhs, uint32_t rhs) {
			if (rhs >= 8)
				return 0;
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
			return lhs >> rhs;
#else
			if (*((uint8_t *)&lhs) & 0x80) {
				uint8_t unsignedLhs = *((uint8_t *)&lhs);

				unsignedLhs >>= rhs;

				unsignedLhs |= 0xff << (8 - rhs);

				return *(int8_t *)&unsignedRhs;
			} else {
				return (int8_t)(lhs >> rhs);
			}
#endif
		}

		SLAKE_FORCEINLINE int16_t shrSigned16(int16_t lhs, uint32_t rhs) {
			if (rhs >= 16)
				return 0;
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
			return lhs >> rhs;
#else
			if (*((uint16_t *)&lhs) & 0x8000) {
				uint16_t unsignedLhs = *((uint16_t *)&lhs);

				unsignedLhs >>= rhs;

				unsignedLhs |= 0xffff << (16 - rhs);

				return *(int16_t *)&unsignedRhs;
			} else {
				return (int16_t)(lhs >> rhs);
			}
#endif
		}

		SLAKE_FORCEINLINE int32_t shrSigned32(int32_t lhs, uint32_t rhs) {
			if (rhs >= 32)
				return 0;
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
			return lhs >> rhs;
#else
			if (*((uint32_t *)&lhs) & 0x80000000) {
				uint32_t unsignedLhs = *((uint32_t *)&lhs);

				unsignedLhs >>= rhs;

				unsignedLhs |= 0xffffffff << (32 - rhs);

				return *(int32_t *)&unsignedRhs;
			} else {
				return (int32_t)(lhs >> rhs);
			}
#endif
		}

		SLAKE_FORCEINLINE int64_t shrSigned64(int64_t lhs, uint32_t rhs) {
			if (rhs >= 64)
				return 0;
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
			return lhs >> rhs;
#else
			if (*((uint64_t *)&lhs) & 0x8000000000000000ULL) {
				uint64_t unsignedLhs = *((uint64_t *)&lhs);

				unsignedLhs >>= rhs;

				unsignedLhs |= 0xffffffffffffffffULL << (64 - rhs);

				return *(int64_t *)&unsignedRhs;
			} else {
				return (int64_t)(lhs >> rhs);
			}
#endif
		}

		SLAKE_FORCEINLINE uint8_t shrUnsigned8(uint8_t lhs, uint32_t rhs) {
			if (rhs >= 8)
				return 0;
			return lhs >> rhs;
		}

		SLAKE_FORCEINLINE uint16_t shrUnsigned16(uint16_t lhs, uint32_t rhs) {
			if (rhs >= 16)
				return 0;
			return lhs >> rhs;
		}

		SLAKE_FORCEINLINE uint32_t shrUnsigned32(uint32_t lhs, uint32_t rhs) {
			if (rhs >= 32)
				return 0;
			return lhs >> rhs;
		}

		SLAKE_FORCEINLINE uint64_t shrUnsigned64(uint64_t lhs, uint32_t rhs) {
			if (rhs >= 64)
				return 0;
			return lhs >> rhs;
		}

		SLAKE_FORCEINLINE int8_t shlSigned8(int8_t lhs, uint32_t rhs) {
			if (rhs >= 8)
				return 0;
			return lhs << rhs;
		}

		SLAKE_FORCEINLINE int16_t shlSigned16(int16_t lhs, uint32_t rhs) {
			if (rhs >= 16)
				return 0;
			return lhs << rhs;
		}

		SLAKE_FORCEINLINE int32_t shlSigned32(int32_t lhs, uint32_t rhs) {
			if (rhs >= 32)
				return 0;
			return lhs << rhs;
		}

		SLAKE_FORCEINLINE int64_t shlSigned64(int64_t lhs, uint32_t rhs) {
			if (rhs >= 64)
				return 0;
			return lhs << rhs;
		}

		SLAKE_FORCEINLINE uint8_t shlUnsigned8(uint8_t lhs, uint32_t rhs) {
			if (rhs >= 8)
				return 0;
			return lhs << rhs;
		}

		SLAKE_FORCEINLINE uint16_t shlUnsigned16(uint16_t lhs, uint32_t rhs) {
			if (rhs >= 16)
				return 0;
			return lhs << rhs;
		}

		SLAKE_FORCEINLINE uint32_t shlUnsigned32(uint32_t lhs, uint32_t rhs) {
			if (rhs >= 32)
				return 0;
			return lhs << rhs;
		}

		SLAKE_FORCEINLINE uint64_t shlUnsigned64(uint64_t lhs, uint32_t rhs) {
			if (rhs >= 64)
				return 0;
			return lhs << rhs;
		}
	}
}

#endif
