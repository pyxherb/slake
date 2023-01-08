#ifndef _SLKC_COMPILER_INIT_HH
#define _SLKC_COMPILER_INIT_HH

#include "scope.hh"

namespace Slake {
	namespace Compiler {
		inline void deinit() {
			currentInterface.reset();
			currentClass.reset();
			currentEnum.reset();
			currentScope.reset();
		}
	}
}

#endif
