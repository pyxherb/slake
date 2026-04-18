#ifndef _SLAKE_ACCESS_H_
#define _SLAKE_ACCESS_H_

#include <cstdint>
#include "basedefs.h"

namespace slake {
	///
	/// @brief The access modifier type, with default access mode `private`.
	///
	using AccessModifier = uint8_t;

	enum class AccessMode : uint8_t {
		Private = 0b00,
		Public = 0b01,
		Protected = 0b10,
	};

	constexpr static uint8_t
		_ACCESS_MODE_MASK = 0b11;

	constexpr static AccessModifier
		ACCESS_STATIC = 0x04,
		ACCESS_NATIVE = 0x08;

	///
	/// @brief Extract access mode from an access modifier.
	///
	/// @param access_modifier Access modifier that contains the access mode to be extracted.
	/// @return Extracted access mode.
	///
	constexpr SLAKE_FORCEINLINE AccessMode access_mode_of(AccessModifier access_modifier) noexcept {
		return (AccessMode)(access_modifier & (uint8_t)_ACCESS_MODE_MASK);
	}

	constexpr SLAKE_FORCEINLINE AccessModifier make_access_modifier(AccessMode mode) noexcept {
		return (uint8_t)mode;
	}

	constexpr SLAKE_FORCEINLINE AccessModifier make_access_modifier(AccessMode mode, AccessModifier access_modifier) noexcept {
		return (access_modifier & ~_ACCESS_MODE_MASK) | (uint8_t)mode;
	}

	///
	/// @brief Check if an access modifier is valid.
	///
	/// @param access_modifier Access modifier to be checked.
	/// @return Whether the access modifier is valid.
	///
	constexpr SLAKE_FORCEINLINE bool is_valid_access_modifier(AccessModifier access_modifier) noexcept {
		return !(access_modifier & ~(((uint8_t)_ACCESS_MODE_MASK) | ACCESS_STATIC | ACCESS_NATIVE));
	}
}

#endif
