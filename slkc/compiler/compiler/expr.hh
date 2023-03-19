#ifndef _SLKC_COMPILER_EXPR_HH_
#define _SLKC_COMPILER_EXPR_HH_

#include "state.hh"

namespace Slake {
	namespace Compiler {
		std::shared_ptr<TypeName> evalExprType(std::shared_ptr<State> s, std::shared_ptr<Expr> expr, bool isRecursing = false);
		std::shared_ptr<Expr> evalConstExpr(std::shared_ptr<Expr> expr, std::shared_ptr<State> s);
	}
}

#endif
