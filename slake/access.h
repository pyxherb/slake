#ifndef _SLAKE_ACCESS_H_
#define _SLAKE_ACCESS_H_

#include <cstdint>
#include "basedefs.h"

namespace slake {
	using AccessModifier = uint8_t;
	constexpr static AccessModifier
		ACCESS_PUBLIC = 0x01,
		ACCESS_STATIC = 0x02,
		ACCESS_NATIVE = 0x04;

	constexpr SLAKE_FORCEINLINE bool is_valid_access_modifier(AccessModifier access_modifier) {
		return !(access_modifier & ~(ACCESS_PUBLIC | ACCESS_STATIC | ACCESS_NATIVE));
	}
}

#endif
