#ifndef _SLKC_COMPILER_AST_HH
#define _SLKC_COMPILER_AST_HH

#include <slake/util/debug.h>

#include <cstdint>
#include <location.hh>

#pragma clang diagnostic ignored "-Wc++17-extensions"

namespace Slake {
	namespace Compiler {
		class ILocated {
		public:
			virtual inline ~ILocated() {}
			virtual location getLocation() const = 0;
		};
		class BasicLocated : public ILocated {
		protected:
			location _loc;

		public:
			inline BasicLocated(location loc) { _loc = loc; }
			virtual inline ~BasicLocated() {}
			virtual inline location getLocation() const override { return _loc; }
		};

		class IStringifiable {
		public:
			virtual inline ~IStringifiable() {}

			virtual std::string toString() const = 0;
		};

		extern int indentLevel;
		inline std::string genIndentStr() {
			return std::string(indentLevel, '\t');
		}

		using AccessModifier = std::uint8_t;
		constexpr AccessModifier
			ACCESS_PUB = 0x01,
			ACCESS_FINAL = 0x02,
			ACCESS_OVERRIDE = 0x04,
			ACCESS_CONST = 0x08,
			ACCESS_VOLATILE = 0x10,
			ACCESS_STATIC = 0x20,
			ACCESS_NATIVE = 0x40;

		class IAccessModified {
		public:
			AccessModifier accessModifier;
			inline IAccessModified(AccessModifier accessModifier) { this->accessModifier = accessModifier; }
			virtual inline ~IAccessModified() {}

			inline bool isPublic() noexcept { return accessModifier & ACCESS_PUB; }
			inline bool isFinal() noexcept { return accessModifier & ACCESS_FINAL; }
			inline bool isOverride() noexcept { return accessModifier & ACCESS_OVERRIDE; }
			inline bool isConst() noexcept { return accessModifier & ACCESS_CONST; }
			inline bool isVolatile() noexcept { return accessModifier & ACCESS_VOLATILE; }
			inline bool isStatic() noexcept { return accessModifier & ACCESS_STATIC; }
			inline bool isNative() noexcept { return accessModifier & ACCESS_NATIVE; }
		};
	}
}

namespace std {
	inline std::string to_string(const Slake::Compiler::IStringifiable &s) {
		return s.toString();
	}

	inline std::string to_string(const Slake::Compiler::IStringifiable &&s) {
		return s.toString();
	}
}

#endif
