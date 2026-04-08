#ifndef _SLAKE_FLIB_BITOP_H_
#define _SLAKE_FLIB_BITOP_H_

#include <slake/basedefs.h>
#include <cstdint>

namespace slake {
	namespace flib {
		SLAKE_FORCEINLINE int8_t shr_signed8(int8_t lhs, uint32_t rhs) {
			if (rhs >= 8)
				return 0;
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
			return lhs >> rhs;
#else
			if (*((uint8_t *)&lhs) & 0x80) {
				uint8_t unsigned_lhs = *((uint8_t *)&lhs);

				unsigned_lhs >>= rhs;

				unsigned_lhs |= 0xff << (8 - rhs);

				return *(int8_t *)&unsigned_lhs;
			} else {
				return (int8_t)(lhs >> rhs);
			}
#endif
		}

		SLAKE_FORCEINLINE int16_t shr_signed16(int16_t lhs, uint32_t rhs) {
			if (rhs >= 16)
				return 0;
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
			return lhs >> rhs;
#else
			if (*((uint16_t *)&lhs) & 0x8000) {
				uint16_t unsigned_lhs = *((uint16_t *)&lhs);

				unsigned_lhs >>= rhs;

				unsigned_lhs |= 0xffff << (16 - rhs);

				return *(int16_t *)&unsigned_lhs;
			} else {
				return (int16_t)(lhs >> rhs);
			}
#endif
		}

		SLAKE_FORCEINLINE int32_t shr_signed32(int32_t lhs, uint32_t rhs) {
			if (rhs >= 32)
				return 0;
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
			return lhs >> rhs;
#else
			if (*((uint32_t *)&lhs) & 0x80000000) {
				uint32_t unsigned_lhs = *((uint32_t *)&lhs);

				unsigned_lhs >>= rhs;

				unsigned_lhs |= 0xffffffff << (32 - rhs);

				return *(int32_t *)&unsigned_lhs;
			} else {
				return (int32_t)(lhs >> rhs);
			}
#endif
		}

		SLAKE_FORCEINLINE int64_t shr_signed64(int64_t lhs, uint32_t rhs) {
			if (rhs >= 64)
				return 0;
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
			return lhs >> rhs;
#else
			if (*((uint64_t *)&lhs) & 0x8000000000000000ull) {
				uint64_t unsigned_lhs = *((uint64_t *)&lhs);

				unsigned_lhs >>= rhs;

				unsigned_lhs |= 0xffffffffffffffffull << (64 - rhs);

				return *(int64_t *)&unsigned_lhs;
			} else {
				return (int64_t)(lhs >> rhs);
			}
#endif
		}

		SLAKE_FORCEINLINE uint8_t shr_unsigned8(uint8_t lhs, uint32_t rhs) {
			if (rhs >= 8)
				return 0;
			return lhs >> rhs;
		}

		SLAKE_FORCEINLINE uint16_t shr_unsigned16(uint16_t lhs, uint32_t rhs) {
			if (rhs >= 16)
				return 0;
			return lhs >> rhs;
		}

		SLAKE_FORCEINLINE uint32_t shr_unsigned32(uint32_t lhs, uint32_t rhs) {
			if (rhs >= 32)
				return 0;
			return lhs >> rhs;
		}

		SLAKE_FORCEINLINE uint64_t shr_unsigned64(uint64_t lhs, uint32_t rhs) {
			if (rhs >= 64)
				return 0;
			return lhs >> rhs;
		}

		SLAKE_FORCEINLINE int8_t shl_signed8(int8_t lhs, uint32_t rhs) {
			if (rhs >= 8)
				return 0;
			return lhs << rhs;
		}

		SLAKE_FORCEINLINE int16_t shl_signed16(int16_t lhs, uint32_t rhs) {
			if (rhs >= 16)
				return 0;
			return lhs << rhs;
		}

		SLAKE_FORCEINLINE int32_t shl_signed32(int32_t lhs, uint32_t rhs) {
			if (rhs >= 32)
				return 0;
			return lhs << rhs;
		}

		SLAKE_FORCEINLINE int64_t shl_signed64(int64_t lhs, uint32_t rhs) {
			if (rhs >= 64)
				return 0;
			return lhs << rhs;
		}

		SLAKE_FORCEINLINE uint8_t shl_unsigned8(uint8_t lhs, uint32_t rhs) {
			if (rhs >= 8)
				return 0;
			return lhs << rhs;
		}

		SLAKE_FORCEINLINE uint16_t shl_unsigned16(uint16_t lhs, uint32_t rhs) {
			if (rhs >= 16)
				return 0;
			return lhs << rhs;
		}

		SLAKE_FORCEINLINE uint32_t shl_unsigned32(uint32_t lhs, uint32_t rhs) {
			if (rhs >= 32)
				return 0;
			return lhs << rhs;
		}

		SLAKE_FORCEINLINE uint64_t shl_unsigned64(uint64_t lhs, uint32_t rhs) {
			if (rhs >= 64)
				return 0;
			return lhs << rhs;
		}

		SLAKE_FORCEINLINE int8_t shl_signed(int8_t lhs, uint32_t rhs) {
			return shl_signed8(lhs, rhs);
		}

		SLAKE_FORCEINLINE int16_t shl_signed(int16_t lhs, uint32_t rhs) {
			return shl_signed16(lhs, rhs);
		}

		SLAKE_FORCEINLINE int32_t shl_signed(int32_t lhs, uint32_t rhs) {
			return shl_signed32(lhs, rhs);
		}

		SLAKE_FORCEINLINE int64_t shl_signed(int64_t lhs, uint32_t rhs) {
			return shl_signed64(lhs, rhs);
		}

		SLAKE_FORCEINLINE int8_t shr_signed(int8_t lhs, uint32_t rhs) {
			return shr_signed8(lhs, rhs);
		}

		SLAKE_FORCEINLINE int16_t shr_signed(int16_t lhs, uint32_t rhs) {
			return shr_signed16(lhs, rhs);
		}

		SLAKE_FORCEINLINE int32_t shr_signed(int32_t lhs, uint32_t rhs) {
			return shr_signed32(lhs, rhs);
		}

		SLAKE_FORCEINLINE int64_t shr_signed(int64_t lhs, uint32_t rhs) {
			return shr_signed64(lhs, rhs);
		}

		SLAKE_FORCEINLINE uint8_t shl_unsigned(uint8_t lhs, uint32_t rhs) {
			return shl_unsigned8(lhs, rhs);
		}

		SLAKE_FORCEINLINE uint16_t shl_unsigned(uint16_t lhs, uint32_t rhs) {
			return shl_unsigned16(lhs, rhs);
		}

		SLAKE_FORCEINLINE uint32_t shl_unsigned(uint32_t lhs, uint32_t rhs) {
			return shl_unsigned32(lhs, rhs);
		}

		SLAKE_FORCEINLINE uint64_t shl_unsigned(uint64_t lhs, uint32_t rhs) {
			return shl_unsigned64(lhs, rhs);
		}

		SLAKE_FORCEINLINE uint8_t shr_unsigned(uint8_t lhs, uint32_t rhs) {
			return shl_unsigned8(lhs, rhs);
		}

		SLAKE_FORCEINLINE uint16_t shr_unsigned(uint16_t lhs, uint32_t rhs) {
			return shl_unsigned16(lhs, rhs);
		}

		SLAKE_FORCEINLINE uint32_t shr_unsigned(uint32_t lhs, uint32_t rhs) {
			return shl_unsigned32(lhs, rhs);
		}

		SLAKE_FORCEINLINE uint64_t shr_unsigned(uint64_t lhs, uint32_t rhs) {
			return shl_unsigned64(lhs, rhs);
		}
	}
}

#endif
