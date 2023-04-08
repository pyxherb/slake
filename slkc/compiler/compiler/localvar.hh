#ifndef _SLKC_COMPILER_LOCALVAR_HH
#define _SLKC_COMPILER_LOCALVAR_HH

#include "../base/scope.hh"

namespace Slake {
	namespace Compiler {
		struct LocalVar {
			std::uint32_t stackPos;
			std::shared_ptr<TypeName> type;

			inline LocalVar& operator=(const LocalVar& x) {
				stackPos = x.stackPos;
				type = x.type;
				return *this;
			}

			inline LocalVar& operator=(const LocalVar&& x) {
				stackPos = x.stackPos;
				type = x.type;
				return *this;
			}

			inline LocalVar(std::uint32_t stackPos, std::shared_ptr<TypeName> type) : stackPos(stackPos), type(type) {}
			inline LocalVar() {}
			inline LocalVar(const LocalVar& x) { *this = x; }
			inline LocalVar(const LocalVar&& x) { *this = x; }
		};
	}
}

#endif
