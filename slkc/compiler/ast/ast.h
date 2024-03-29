#ifndef _SLKC_COMPILER_AST_AST_H_
#define _SLKC_COMPILER_AST_AST_H_

#include "parser.h"
#include "interface.h"
#include "trait.h"
#include "alias.h"

namespace slake {
	namespace slkc {
		bool isMemberNode(shared_ptr<AstNode> node);
	}
}

#endif
