#ifndef _SLKC_COMPILER_COMPILE_HH
#define _SLKC_COMPILER_COMPILE_HH

#include "state.hh"
#include "expr.hh"

namespace Slake {
	namespace Compiler {
		void compileRightExpr(std::shared_ptr<Expr> expr, std::shared_ptr<State> s, bool isRecursing = false);
		void compileLeftExpr(std::shared_ptr<Expr> expr, std::shared_ptr<State> s, bool isRecursing = false);
		void compileStmt(std::shared_ptr<Compiler::Stmt> src, std::shared_ptr<State> s);
		void compile(std::shared_ptr<Scope> scope, std::fstream& fs, bool isTopLevel = true);
	}
}

#endif
