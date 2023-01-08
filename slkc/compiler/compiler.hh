#ifndef _SLKC_COMPILER_COMMON_HH
#define _SLKC_COMPILER_COMMON_HH

#include <slake/debug.h>

#include <string>

#include "init.hh"

namespace Slake {
	namespace Compiler {
		inline std::shared_ptr<Expr> calcConstExpr(std::shared_ptr<Expr> expr, bool checkOnly = false) {
			switch (expr->getType()) {
				case ExprType::AWAIT:
				case ExprType::CALL:
					return std::shared_ptr<Expr>();
				case ExprType::LITERAL:
					return expr;
				case ExprType::UNARY: {
					std::shared_ptr<UnaryOpExpr> opExpr = std::static_pointer_cast<UnaryOpExpr>(expr);
					switch (opExpr->x->getType()) {
						case ExprType::LITERAL:
							return std::static_pointer_cast<LiteralExpr>(opExpr->x)->execUnaryOp(opExpr->op);
						case ExprType::REF: {
							auto ref = std::static_pointer_cast<RefExpr>(opExpr->x);
							if (currentScope->getVar(ref))
								return nullptr;
							if (currentScope->getFn(ref))
								return nullptr;
							{
								auto x = currentScope->getEnumItem(ref);
								if (x)
									return calcConstExpr(x, checkOnly);
							}
							break;
						}
						case ExprType::TYPENAME:
							break;
					}
				}
			}
			return std::shared_ptr<Expr>();
		}
	}
}

#endif
