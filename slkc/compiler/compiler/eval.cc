#include <slkparse.hh>

#include "misc.hh"

using namespace Slake;
using namespace Slake::Compiler;

/// @brief Evaluate type of an expression.
/// @param state State for the expression.
/// @param expr Expression to evaluate.
/// @param isRecusring Set to false by default, set if we are recursing. DO NOT use local variables in the state if set.
/// @return Type of the expression, null if unknown.
std::shared_ptr<TypeName> State::evalExprType(std::shared_ptr<Expr> expr, bool isRecursing) {
	auto &fn = fnDefs[currentFn];
	// assert(fn);
	switch (expr->getExprKind()) {
		case ExprKind::LITERAL: {
			auto literalType = std::static_pointer_cast<LiteralExpr>(expr)->getLiteralType();
			if (!_lt2tnKindMap.count(literalType))
				throw parser::syntax_error(expr->getLocation(), "Unevaluatable literal type");
			return std::make_shared<TypeName>(expr->getLocation(), _lt2tnKindMap.at(literalType));
		}
		case ExprKind::REF: {
			auto ref = std::static_pointer_cast<RefExpr>(expr);
			if (!isRecursing) {
				if (context.lvars.count(ref->name)) {
					if (ref->next) {
						std::shared_ptr<Scope> s = scope;

						switch (context.lvars[ref->name].type->kind) {
							case TypeNameKind::CUSTOM: {
								auto t = Scope::getCustomType(std::static_pointer_cast<CustomTypeName>(context.lvars[ref->name].type));
								if (!t)
									throw parser::syntax_error(expr->getLocation(), "Type was not defined");
								if (!t->getScope())
									throw parser::syntax_error(expr->getLocation(), "Accessing member `" + ref->name + "' with unsupported type");
								scope = t->getScope();
								break;
							}
							default:
								throw parser::syntax_error(expr->getLocation(), "Accessing member `" + ref->name + "' with unsupported type");
						}
						auto refType = evalExprType(ref->next, true);
						scope = s;
						return refType;
					}
					return context.lvars[ref->name].type;
				}
			}
			{
				auto v = scope->getVar(ref->name);
				if (v) {
					isLastResolvedVar = true;
					if (ref->next) {
						std::shared_ptr<Scope> s = scope;
						switch (v->typeName->kind) {
							case TypeNameKind::CUSTOM: {
								auto tn = std::static_pointer_cast<CustomTypeName>(v->typeName);
								auto t = Scope::getCustomType(tn);
								if (!t) {
									throw parser::syntax_error(expr->getLocation(), "Type was not defined");
								}
								if (!t->getScope())
									throw parser::syntax_error(expr->getLocation(), "Accessing member `" + ref->name + "' with unsupported type");
								break;
							}
							default:
								throw parser::syntax_error(expr->getLocation(), "Accessing member `" + ref->name + "' with unsupported type");
						}
						auto refType = evalExprType(ref->next, true);
						scope = s;
						return refType;
					}
					return v->typeName;
				}
			}
			{
				auto e = scope->getEnumItem(ref);
				if (e)
					return evalExprType(e, true);
			}
			{
				auto fn = scope->getFn(ref->name);
				if (fn) {
					auto fnType = std::make_shared<FnTypeName>(expr->getLocation(), fn->returnTypeName, fn->accessModifier & ACCESS_STATIC);
					for (auto &i : *(fn->params))
						fnType->argTypes.push_back(i->typeName);
					if (ref->next)
						throw parser::syntax_error(expr->getLocation(), "Accessing member `" + ref->next->name + "' with unsupported type");
					return fnType;
				}
			}
			{
				auto t = scope->getType(ref->name);
				if (t) {
					if (!ref->next)
						throw parser::syntax_error(expr->getLocation(), "Unexpected type name");
					std::shared_ptr<Scope> s = scope;
					scope = t->getScope();
					auto refType = evalExprType(ref->next, true);
					scope = s;
					return refType;
				}
			}
			throw parser::syntax_error(expr->getLocation(), "Undefined identifier: `" + ref->name + "'");
		}
		case ExprKind::CALL: {
			auto e = std::static_pointer_cast<CallExpr>(expr);
			if (e->isAsync)
				return std::make_shared<TypeName>(expr->getLocation(), TypeNameKind::U32);
			auto exprType = evalExprType(e->target);
			if (exprType->kind != TypeNameKind::FN)
				throw parser::syntax_error(e->target->getLocation(), "Expression is not callable");

			if (e->target->getExprKind() == ExprKind::REF) {
				auto i = copyRef(std::static_pointer_cast<RefExpr>(e->target)), j = i;
				while (j->next && j->next->next)
					j = j->next;
				j->next.reset();
				calleeParent = i;
			}
			return std::static_pointer_cast<FnTypeName>(exprType)->resultType;
		}
		case ExprKind::AWAIT:
			return std::make_shared<TypeName>(expr->getLocation(), TypeNameKind::ANY);
		case ExprKind::NEW:
			return std::static_pointer_cast<NewExpr>(expr)->type;
		case ExprKind::TERNARY: {
			auto e = std::static_pointer_cast<TernaryOpExpr>(expr);
			auto xType = evalExprType(e->x), yType = evalExprType(e->y);

			// Check if the condition expression is boolean.
			if (!isConvertible(xType, std::make_shared<TypeName>(expr->getLocation(), TypeNameKind::BOOL)))
				throw parser::syntax_error(e->x->getLocation(), "Expecting a boolean expression");

			// Check if the expressions have the same type.
			if (!isSameType(xType, yType))
				throw parser::syntax_error(e->x->getLocation(), "Operands for ternary operation have different types");
			return xType;
		}
		case ExprKind::UNARY: {
			auto e = std::static_pointer_cast<UnaryOpExpr>(expr);
			auto xType = evalExprType(e->x);
			switch (e->op) {
				case UnaryOp::INC_B:
				case UnaryOp::INC_F:
				case UnaryOp::DEC_B:
				case UnaryOp::DEC_F: {
				}
				case UnaryOp::NEG: {
				}
				case UnaryOp::REV: {
				}
				case UnaryOp::NOT:
					return std::make_shared<TypeName>(expr->getLocation(), TypeNameKind::BOOL);
				default:
					assert(false);
			}
		}
		case ExprKind::BINARY: {
			auto e = std::static_pointer_cast<BinaryOpExpr>(expr);
			auto xType = evalExprType(e->x), yType = evalExprType(e->y);
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
		case ExprKind::ARRAY: {
			auto e = std::static_pointer_cast<ArrayExpr>(expr);

			if (e->elements.empty())
				return std::make_shared<ArrayTypeName>(e->getLocation(), std::make_shared<TypeName>(e->getLocation(), TypeNameKind::ANY));

			std::shared_ptr<ArrayTypeName> type;
			if (desiredType) {
				assert(desiredType->kind == TypeNameKind::ARRAY);
				type = std::static_pointer_cast<ArrayTypeName>(desiredType);
			} else
				type = std::make_shared<ArrayTypeName>(e->getLocation(), std::shared_ptr<TypeName>());

			for (auto i : e->elements) {
				if (!type->type) {
					type->type = evalExprType(i);
					if (type->type->kind == TypeNameKind::NONE)
						type->type.reset();
				}
				if (!isConvertible(evalExprType(i), type->type))
					throw parser::syntax_error(i->getLocation(), "Incompatible member type");
			}
			return type;
		}
		case ExprKind::MAP: {
			auto e = std::static_pointer_cast<MapExpr>(expr);

			if (!e->pairs)
				return std::make_shared<MapTypeName>(
					e->getLocation(),
					std::make_shared<TypeName>(e->getLocation(), TypeNameKind::ANY),
					std::make_shared<TypeName>(e->getLocation(), TypeNameKind::ANY));

			std::shared_ptr<MapTypeName> type;
			if (desiredType) {
				assert(desiredType->kind == TypeNameKind::MAP);
				type = std::static_pointer_cast<MapTypeName>(desiredType);
			}
			type = std::make_shared<MapTypeName>(e->getLocation(), std::shared_ptr<TypeName>(), std::shared_ptr<TypeName>());

			for (auto i : *e->pairs) {
				if (!type->keyType) {
					type->keyType = evalExprType(i->first);
					if (type->kind == TypeNameKind::NONE)
						type->keyType.reset();
				}
				if (!type->valueType) {
					type->valueType = evalExprType(i->second);
					if (type->kind == TypeNameKind::NONE)
						type->valueType.reset();
				}

				if (!isConvertible(evalExprType(i->first), type->keyType))
					throw parser::syntax_error(i->first->getLocation(), "Incompatible key type");
				if (!isConvertible(evalExprType(i->second), type->valueType))
					throw parser::syntax_error(i->second->getLocation(), "Incompatible value type");
			}

			if (!type->keyType)
				type->keyType = std::make_shared<TypeName>(e->getLocation(), TypeNameKind::NONE);
			if (!type->valueType)
				type->valueType = std::make_shared<TypeName>(e->getLocation(), TypeNameKind::NONE);

			return type;
		}
	}
	throw std::logic_error("Unevaluatable expression type");
}

std::shared_ptr<Expr> State::evalConstExpr(std::shared_ptr<Expr> expr) {
	switch (expr->getExprKind()) {
		case ExprKind::LITERAL:
			return expr;
		case ExprKind::UNARY: {
			std::shared_ptr<UnaryOpExpr> opExpr = std::static_pointer_cast<UnaryOpExpr>(expr);
			switch (opExpr->x->getExprKind()) {
				case ExprKind::LITERAL:
					return std::static_pointer_cast<LiteralExpr>(opExpr->x)->execUnaryOp(opExpr->op);
				case ExprKind::REF: {
					auto ref = std::static_pointer_cast<RefExpr>(opExpr->x);
					//
					// Variable and function are both not evaluatable at compile time.
					//
					if ((currentScope->getVar(ref->name)) || (currentScope->getFn(ref->name)))
						return std::shared_ptr<Expr>();
					{
						auto x = currentScope->getEnumItem(ref);
						if (x)
							return evalConstExpr(x);
					}
					break;
				}
			}
			break;
		}
		case ExprKind::BINARY: {
			std::shared_ptr<BinaryOpExpr> opExpr = std::static_pointer_cast<BinaryOpExpr>(expr);
			auto x = evalConstExpr(opExpr->x);
			if ((!x) || (x->getExprKind() != ExprKind::LITERAL))
				return std::shared_ptr<Expr>();
			auto y = evalConstExpr(opExpr->y);
			if ((!y) || (y->getExprKind() != ExprKind::LITERAL))
				return std::shared_ptr<Expr>();
			return std::static_pointer_cast<LiteralExpr>(opExpr->x)->execBinaryOp(opExpr->op, std::static_pointer_cast<LiteralExpr>(opExpr->y));
		}
	}
	return std::shared_ptr<Expr>();
}
