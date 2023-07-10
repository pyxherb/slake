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
		ACCESS_FINAL = 0x10,
		ACCESS_CONST = 0x20;

	class AccessModified {
	private:
		AccessModifier _modifier = 0;

	public:
		AccessModified() = delete;
		AccessModified(const AccessModified &) = delete;
		AccessModified(const AccessModified &&) = delete;

		inline AccessModified(AccessModifier modifier = 0) : _modifier(modifier) {}
		virtual ~AccessModified() = default;
		inline AccessModifier getAccess() noexcept { return _modifier; }
		inline void setAccess(AccessModifier modifier) noexcept { _modifier = modifier; }

		inline bool isPublic() const noexcept { return _modifier & ACCESS_PUB; }
		inline bool isStatic() const noexcept { return _modifier & ACCESS_STATIC; }
		inline bool isNative() const noexcept { return _modifier & ACCESS_NATIVE; }
		inline bool isOverriden() const noexcept { return _modifier & ACCESS_OVERRIDE; }
		inline bool isFinal() const noexcept { return _modifier & ACCESS_FINAL; }
		inline bool isConst() const noexcept { return _modifier & ACCESS_CONST; }
	};
}

#endif
