#ifndef _SLAKE_ACCESS_H_
#define _SLAKE_ACCESS_H_

#include <cstdint>

namespace slake {
	using AccessModifier = uint16_t;
	constexpr static AccessModifier
		ACCESS_PUB = 0x01,
		ACCESS_STATIC = 0x02,
		ACCESS_NATIVE = 0x04,
		ACCESS_OVERRIDE = 0x08,
		ACCESS_FINAL = 0x10;
}

#endif
