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
			ACCESS_NATIVE = 0x08;

		class IAccessModified {
		public:
			AccessModifier accessModifier;
			virtual inline ~IAccessModified(){};
		};

		using StorageModifier = std::uint8_t;
		constexpr StorageModifier
			STORAGE_CONST = 0x01;

		class IStorageModified {
		public:
			StorageModifier accessModifier;
			virtual inline ~IStorageModified(){};
		};
	}
}

#endif
