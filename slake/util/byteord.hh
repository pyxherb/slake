#ifndef _SLAKE_UTIL_BYTEORD_HH_
#define _SLAKE_UTIL_BYTEORD_HH_

#include <cstdint>

namespace slake {
	namespace util {
		/// @brief Get byte order on target machine.
		/// @return Whether the byte order is big-endian.
		/// @retval true The machine is big-endian.
		/// @retval false The machine is little-endian.
		inline bool getByteOrder() {
#ifdef __BYTE_ORDER__
			return __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__;
#else
			uint16_t value = 0xff00;
			return *((uint8_t*)&value);
#endif
		}

		constexpr inline uint16_t swapByteOrder(uint16_t n) {
			return (n & 0xff << 8) | (n >> 8);
		}
		constexpr inline uint16_t swapByteOrder(std::int16_t n) {
			return (n & 0xff << 8) | (n >> 8);
		}
		constexpr inline uint32_t swapByteOrder(uint32_t n) {
			return (n & 0xff << 24) | (n & 0xff00 << 8) | (n & 0xff0000 >> 8) | (n >> 24);
		}
		constexpr inline uint32_t swapByteOrder(std::int32_t n) {
			return (n & 0xff << 24) | (n & 0xff00 << 8) | (n & 0xff0000 >> 8) | (n >> 24);
		}
		constexpr inline uint64_t swapByteOrder(uint64_t n) {
			return ((n & 0xffull) << 56) |
				   ((n & 0xff00ull) << 40) |
				   ((n & 0xff0000ull) << 24) |
				   ((n & 0xff000000ull) << 8) |
				   ((n & 0xff00000000ull) >> 8) |
				   ((n & 0xff0000000000ull) >> 24) |
				   ((n & 0xff000000000000ull) >> 40) |
				   ((n & 0xff00000000000000ull) >> 56);
		}
		constexpr inline uint64_t swapByteOrder(std::int64_t n) {
			return ((n & 0xffull) << 56) |
				   ((n & 0xff00ull) << 40) |
				   ((n & 0xff0000ull) << 24) |
				   ((n & 0xff000000ull) << 8) |
				   ((n & 0xff00000000ull) >> 8) |
				   ((n & 0xff0000000000ull) >> 24) |
				   ((n & 0xff000000000000ull) >> 40) |
				   ((n & 0xff00000000000000ull) >> 56);
		}
	}
}

#endif
