#include "expr.hh"
#include "utils.hh"
#include <slkparse.hh>

using namespace Slake;
using namespace Slake::Compiler;

/// @brief Evaluate type of an expression.
/// @param state State for the expression.
/// @param expr Expression to evaluate.
/// @param isRecusring Set to false by default, set if we are recursing. DO NOT use local variables in the state if set.
/// @return Type of the expression, null if unknown.
std::shared_ptr<TypeName> Compiler::evalExprType(std::shared_ptr<State> s, std::shared_ptr<Expr> expr, bool isRecursing) {
	auto &fn = s->fnDefs[s->currentFn];
	// assert(fn);
	switch (expr->getType()) {
		case ExprType::LITERAL: {
			auto literalType = std::static_pointer_cast<LiteralExpr>(expr)->getLiteralType();
			if (!_lt2tnKindMap.count(literalType))
				throw parser::syntax_error(expr->getLocation(), "Unevaluatable literal type");
			return std::make_shared<TypeName>(expr->getLocation(), _lt2tnKindMap.at(literalType));
		}
		case ExprType::REF: {
			auto ref = std::static_pointer_cast<RefExpr>(expr);
			if (!isRecursing) {
				if (s->context.lvars.count(ref->name)) {
					if (ref->next) {
						std::shared_ptr<Scope> scope = s->scope;
						switch (s->context.lvars[ref->name].type->kind) {
							case TypeNameKind::CUSTOM: {
								auto t = Scope::getCustomType(std::static_pointer_cast<CustomTypeName>(s->context.lvars[ref->name].type));
								if (!t)
									throw parser::syntax_error(expr->getLocation(), "Type was not defined");
								if (!t->getScope())
									throw parser::syntax_error(expr->getLocation(), "Accessing member `" + ref->name + "' with unsupported type");
								s->scope = t->getScope();
								break;
							}
							default:
								throw parser::syntax_error(expr->getLocation(), "Accessing member `" + ref->name + "' with unsupported type");
						}
						auto refType = evalExprType(s, ref->next, true);
						s->scope = scope;
						return refType;
					}
					return s->context.lvars[ref->name].type;
				}
			}
			{
				auto v = s->scope->getVar(ref->name);
				if (v) {
					if (ref->next) {
						std::shared_ptr<Scope> scope = s->scope;
						switch (v->typeName->kind) {
							case TypeNameKind::CUSTOM: {
								auto tn = std::static_pointer_cast<CustomTypeName>(v->typeName);
								auto t = Scope::getCustomType(tn);
								if (!t) {
									throw parser::syntax_error(expr->getLocation(), "Type was not defined");
								}
								auto scope = t->getScope();
								if (!scope)
									throw parser::syntax_error(expr->getLocation(), "Accessing member `" + ref->name + "' with unsupported type");
								s->scope = scope;
								break;
							}
							default:
								throw parser::syntax_error(expr->getLocation(), "Accessing member `" + ref->name + "' with unsupported type");
						}
						auto refType = evalExprType(s, ref->next, true);
						s->scope = scope;
						return refType;
					}
					return v->typeName;
				}
			}
			{
				auto e = s->scope->getEnumItem(ref);
				if (e)
					return evalExprType(s, e, true);
			}
			{
				auto fn = s->scope->getFn(ref->name);
				if (fn) {
					auto fnType = std::make_shared<FnTypeName>(expr->getLocation(), fn->returnTypeName);
					for (auto &i : *(fn->params))
						fnType->argTypes.push_back(i->typeName);
					if (ref->next)
						throw parser::syntax_error(expr->getLocation(), "Accessing member `" + ref->next->name + "' with unsupported type");
					return fnType;
				}
			}
			{
				auto t = s->scope->getType(ref->name);
				if (t) {
					if (!ref->next)
						throw parser::syntax_error(expr->getLocation(), "Unexpected type name");
					std::shared_ptr<Scope> scope = s->scope;
					s->scope = t->getScope();
					auto refType = evalExprType(s, ref->next, true);
					s->scope = scope;
					return refType;
				}
			}
			throw parser::syntax_error(expr->getLocation(), "Undefined identifier: `" + ref->name + "'");
		}
		case ExprType::CALL: {
			auto e = std::static_pointer_cast<CallExpr>(expr);
			if (e->isAsync)
				return std::make_shared<TypeName>(expr->getLocation(), TypeNameKind::U32);
			auto exprType = evalExprType(s, e->target);
			if (exprType->kind != TypeNameKind::FN)
				throw parser::syntax_error(e->target->getLocation(), "Expression is not callable");
			return std::static_pointer_cast<FnTypeName>(exprType)->resultType;
		}
		case ExprType::AWAIT:
			return std::make_shared<TypeName>(expr->getLocation(), TypeNameKind::ANY);
		case ExprType::NEW:
			return std::static_pointer_cast<NewExpr>(expr)->type;
		case ExprType::TERNARY: {
			auto e = std::static_pointer_cast<TernaryOpExpr>(expr);
			auto xType = evalExprType(s, e->x), yType = evalExprType(s, e->y);

			// Check if the condition expression is boolean.
			if (!isConvertible(xType, std::make_shared<TypeName>(expr->getLocation(), TypeNameKind::BOOL)))
				throw parser::syntax_error(e->x->getLocation(), "Expecting a boolean expression");

			// Check if the expressions have the same type.
			if (!isSameType(xType, yType))
				throw parser::syntax_error(e->x->getLocation(), "Operands for ternary operation have different types");
			return xType;
		}
		case ExprType::BINARY: {
			auto e = std::static_pointer_cast<BinaryOpExpr>(expr);
			auto xType = evalExprType(s, e->x), yType = evalExprType(s, e->y);
			switch (e->op) {
				case BinaryOp::LSH:
				case BinaryOp::LSH_ASSIGN:
				case BinaryOp::RSH:
				case BinaryOp::RSH_ASSIGN:
					if (!isConvertible(std::make_shared<TypeName>(yType->getLocation(), TypeNameKind::U32), yType))
						throw parser::syntax_error(e->y->getLocation(), "Incompatible expression types");
					break;
				default:
					if (!isConvertible(xType, yType))
						throw parser::syntax_error(e->y->getLocation(), "Incompatible expression types");
			}
			return xType;
		}
	}
	return std::shared_ptr<TypeName>();
}

std::shared_ptr<Expr> Slake::Compiler::evalConstExpr(std::shared_ptr<Expr> expr, std::shared_ptr<State> s) {
	switch (expr->getType()) {
		case ExprType::LITERAL:
			return expr;
		case ExprType::UNARY: {
			std::shared_ptr<UnaryOpExpr> opExpr = std::static_pointer_cast<UnaryOpExpr>(expr);
			switch (opExpr->x->getType()) {
				case ExprType::LITERAL:
					return std::static_pointer_cast<LiteralExpr>(opExpr->x)->execUnaryOp(opExpr->op);
				case ExprType::REF: {
					auto ref = std::static_pointer_cast<RefExpr>(opExpr->x);
					//
					// Variable and function are both not evaluatable at compile time.
					//
					if ((currentScope->getVar(ref->name)) || (currentScope->getFn(ref->name)))
						return std::shared_ptr<Expr>();
					{
						auto x = currentScope->getEnumItem(ref);
						if (x)
							return evalConstExpr(x, s);
					}
					break;
				}
			}
			break;
		}
		case ExprType::BINARY: {
			std::shared_ptr<BinaryOpExpr> opExpr = std::static_pointer_cast<BinaryOpExpr>(expr);
			auto x = evalConstExpr(opExpr->x, s);
			if ((!x) || (x->getType() != ExprType::LITERAL))
				return std::shared_ptr<Expr>();
			auto y = evalConstExpr(opExpr->y, s);
			if ((!y) || (y->getType() != ExprType::LITERAL))
				return std::shared_ptr<Expr>();
			return std::static_pointer_cast<LiteralExpr>(opExpr->x)->execBinaryOp(opExpr->op, std::static_pointer_cast<LiteralExpr>(opExpr->y));
		}
		default:
			return std::shared_ptr<Expr>();
	}
}
