#ifndef _SPKC_SYNTAX_MODIFIER_HH
	#define _SPKC_SYNTAX_MODIFIER_HH

#include <cstdint>

namespace SpkC {
	namespace Syntax {
		using AccessModifier = std::uint8_t;
		constexpr AccessModifier
			ACCESS_PUB = 0x01,
			ACCESS_FINAL = 0x02;

		using StorageModifier = std::uint8_t;
		constexpr StorageModifier
			STORAGE_CONST = 0x01;
	}
}

#endif
