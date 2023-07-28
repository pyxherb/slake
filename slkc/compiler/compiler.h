#ifndef _SLKC_COMPILER_COMPILER_H_
#define _SLKC_COMPILER_COMPILER_H_

#include "ast/ast.h"

namespace slake {
	namespace slkc {
		struct StateContext {
			shared_ptr<TypeName> desiredType;
		};
	}
}

#endif
