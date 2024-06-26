#ifndef _SLKC_COMPILER_AST_AST_H_
#define _SLKC_COMPILER_AST_AST_H_

#include "parser.h"
#include "alias.h"

namespace slake {
	namespace slkc {
		bool isMemberNode(std::shared_ptr<AstNode> node);
	}
}

#endif
