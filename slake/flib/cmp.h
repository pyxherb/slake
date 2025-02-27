#ifndef _SLAKE_FLIB_CMP_H_
#define _SLAKE_FLIB_CMP_H_

#include <slake/basedefs.h>
#include <cstdint>

namespace slake {
	namespace flib {
		SLAKE_FORCEINLINE int compareU8(uint8_t lhs, uint8_t rhs) {
			if (lhs < rhs)
				return -1;
			if (lhs > rhs)
				return 1;
			return 0;
		}

		SLAKE_FORCEINLINE int compareU16(uint16_t lhs, uint16_t rhs) {
			if (lhs < rhs)
				return -1;
			if (lhs > rhs)
				return 1;
			return 0;
		}

		SLAKE_FORCEINLINE int compareU32(uint32_t lhs, uint32_t rhs) {
			if (lhs < rhs)
				return -1;
			if (lhs > rhs)
				return 1;
			return 0;
		}

		SLAKE_FORCEINLINE int compareU64(uint64_t lhs, uint64_t rhs) {
			if (lhs < rhs)
				return -1;
			if (lhs > rhs)
				return 1;
			return 0;
		}

		SLAKE_FORCEINLINE int compareI8(int8_t lhs, int8_t rhs) {
			if (lhs < rhs)
				return -1;
			if (lhs > rhs)
				return 1;
			return 0;
		}

		SLAKE_FORCEINLINE int compareI16(int16_t lhs, int16_t rhs) {
			if (lhs < rhs)
				return -1;
			if (lhs > rhs)
				return 1;
			return 0;
		}

		SLAKE_FORCEINLINE int compareI32(int32_t lhs, int32_t rhs) {
			if (lhs < rhs)
				return -1;
			if (lhs > rhs)
				return 1;
			return 0;
		}

		SLAKE_FORCEINLINE int compareI64(int64_t lhs, int64_t rhs) {
			if (lhs < rhs)
				return -1;
			if (lhs > rhs)
				return 1;
			return 0;
		}

		SLAKE_FORCEINLINE int compareF32(float lhs, float rhs) {
			if (lhs < rhs)
				return -1;
			if (lhs > rhs)
				return 1;
			return 0;
		}

		SLAKE_FORCEINLINE int compareF64(double lhs, double rhs) {
			if (lhs < rhs)
				return -1;
			if (lhs > rhs)
				return 1;
			return 0;
		}
	}
}

#endif
