#ifndef _SPKC_SYNTAX_COMMON_HH
#define _SPKC_SYNTAX_COMMON_HH

#include <swampeak/debug.h>

#include <string>

#include "scope.hh"

namespace SpkC {
	namespace Syntax {
		/// @brief Check if an expression can be evaluated in compile time.
		/// @param expr Expression to be checked.
		/// @return true The expression can be evaluated.
		/// @return false The expression cannot be evaluated.
		inline bool isConstExpr(std::shared_ptr<Expr> expr) {
			switch (expr->getType()) {
				case ExprType::INLINE_SW: {
					auto x = std::static_pointer_cast<InlineSwitchExpr>(expr);
					if (!isConstExpr(x->condition))
						return false;
					for (auto &i : *(x->caseList))
						if ((!isConstExpr(i->condition)) ||
							(!isConstExpr(i->x)))
							return false;
					return true;
				}

				case ExprType::UNARY:
					return isConstExpr(std::static_pointer_cast<UnaryOpExpr>(expr)->x);
				case ExprType::BINARY:
					return isConstExpr(std::static_pointer_cast<BinaryOpExpr>(expr)->x) &&
						   isConstExpr(std::static_pointer_cast<BinaryOpExpr>(expr)->y);
				case ExprType::TERNARY:
					return isConstExpr(std::static_pointer_cast<TernaryOpExpr>(expr)->condition) &&
						   isConstExpr(std::static_pointer_cast<TernaryOpExpr>(expr)->x) &&
						   isConstExpr(std::static_pointer_cast<TernaryOpExpr>(expr)->y);

				case ExprType::REF:

				case ExprType::TYPENAME:
				case ExprType::LITERAL:
					return true;
			}

			return false;
		}

		inline std::shared_ptr<Expr> calcConstExpr(std::shared_ptr<Expr> expr) {
			switch (expr->getType()) {
				case ExprType::LITERAL:
					return expr;
			}
			return std::shared_ptr<Expr>();
		}
	}
}

#endif
