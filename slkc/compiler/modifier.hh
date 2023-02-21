#ifndef _SLKC_SYNTAX_MODIFIER_HH
#define _SLKC_SYNTAX_MODIFIER_HH

#include <cstdint>

namespace Slake {
	namespace Compiler {
		using AccessModifier = std::uint8_t;
		constexpr AccessModifier
			ACCESS_PUB = 0x01,
			ACCESS_FINAL = 0x02,
			ACCESS_OVERRIDE = 0x04,
			ACCESS_CONST = 0x08,
			ACCESS_VOLATILE = 0x10,
			ACCESS_STATIC = 0x20;

		class IAccessModified {
		public:
			AccessModifier accessModifier;
			inline IAccessModified(AccessModifier accessModifier) { this->accessModifier = accessModifier; }
			virtual inline ~IAccessModified(){}
		};
	}
}

#endif
