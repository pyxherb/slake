#ifndef _SLKC_COMPILER_SCOPE_H_
#define _SLKC_COMPILER_SCOPE_H_

#include "node.h"

namespace slake {
	namespace slkc {
		class Scope {
		public:
			weak_ptr<AstNode> parent;
			unordered_map<string, shared_ptr<AstNode>> members;
		};
	}
}

#endif
