#include "../compiler.h"
#include <cmath>

using namespace slake::slkc;

template <typename T>
static shared_ptr<ExprNode> _evalConstUnaryOpExpr(
	UnaryOp op,
	shared_ptr<LiteralExprNode<T, getLiteralExprType<T>()>> x) {
	using LT = LiteralExprNode<T, getLiteralExprType<T>()>;

	switch (op) {
		case OP_NOT:
			if constexpr (is_convertible_v<bool, T>) {
				return make_shared<BoolLiteralExprNode>(x->getLocation(), !(x->data));
			} else {
				return {};
			}
		case OP_REV: {
			if constexpr (is_same_v<T, bool>) {
				return make_shared<LT>(x->getLocation(), !x->data);
			} else if constexpr (is_integral_v<T>) {
				return make_shared<LT>(x->getLocation(), ~x->data);
			} else if constexpr (is_same_v<T, float>) {
				return make_shared<LT>(x->getLocation(), ~(*(uint32_t *)&x->data));
			} else if constexpr (is_same_v<T, double>) {
				return make_shared<LT>(x->getLocation(), ~(*(uint64_t *)&x->data));
			} else
				return {};
		}
		default:
			return {};
	}
}

template <typename T>
static shared_ptr<ExprNode> _evalConstBinaryOpExpr(
	BinaryOp op,
	shared_ptr<LiteralExprNode<T, getLiteralExprType<T>()>> x,
	shared_ptr<ExprNode> y) {
	using LT = LiteralExprNode<T, getLiteralExprType<T>()>;

	switch (op) {
		case OP_ADD:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<I32LiteralExprNode>(
					x->getLocation(),
					(int)x->data + static_pointer_cast<LT>(y)->data);
			else
				return make_shared<LT>(
					x->getLocation(),
					x->data + static_pointer_cast<LT>(y)->data);
		case OP_SUB:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<I32LiteralExprNode>(
					x->getLocation(),
					(int)x->data - static_pointer_cast<LT>(y)->data);
			else if constexpr (is_arithmetic_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data - static_pointer_cast<LT>(y)->data);
			else
				return {};
		case OP_MUL:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<I32LiteralExprNode>(
					x->getLocation(),
					(int)x->data * static_pointer_cast<LT>(y)->data);
			else if constexpr (is_arithmetic_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data * static_pointer_cast<LT>(y)->data);
			else
				return {};
		case OP_DIV:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<I32LiteralExprNode>(
					x->getLocation(), ((int)x->data) / static_pointer_cast<LT>(y)->data);
			else if constexpr (is_arithmetic_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data / static_pointer_cast<LT>(y)->data);
			else
				return {};
		case OP_MOD:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<I32LiteralExprNode>(
					x->getLocation(),
					((int)x->data) % static_pointer_cast<LT>(y)->data);
			else if constexpr (is_integral_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data * static_pointer_cast<LT>(y)->data);
			else if constexpr (is_same_v<T, float>)
				return make_shared<LT>(
					x->getLocation(),
					fmodf(x->data, static_pointer_cast<LT>(y)->data));
			else if constexpr (is_same_v<T, double>)
				return make_shared<LT>(
					x->getLocation(),
					fmod(x->data, static_pointer_cast<LT>(y)->data));
			else
				return {};
		case OP_AND:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<BoolLiteralExprNode>(
					x->getLocation(),
					x->data && static_pointer_cast<LT>(y)->data);
			else if constexpr (is_integral_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data & static_pointer_cast<LT>(y)->data);
			else if constexpr (is_same_v<T, float>) {
				uint32_t tmp = (*(uint32_t *)&x->data) & (*(uint32_t *)&static_pointer_cast<LT>(y)->data);
				return make_shared<LT>(x->getLocation(), *((float *)&tmp));
			} else if constexpr (is_same_v<T, double>) {
				uint64_t tmp = (*(uint64_t *)&x->data) & (*(uint64_t *)&static_pointer_cast<LT>(y)->data);
				return make_shared<LT>(x->getLocation(), *((double *)&tmp));
			} else
				return {};
		case OP_OR:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<BoolLiteralExprNode>(
					x->getLocation(),
					x->data || static_pointer_cast<LT>(y)->data);
			else if constexpr (is_integral_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data | static_pointer_cast<LT>(y)->data);
			else if constexpr (is_same_v<T, float>) {
				uint32_t tmp = (*(uint32_t *)&x->data) | (*(uint32_t *)&static_pointer_cast<LT>(y)->data);
				return make_shared<LT>(x->getLocation(), *((float *)&tmp));
			} else if constexpr (is_same_v<T, double>) {
				uint64_t tmp = (*(uint64_t *)&x->data) | (*(uint64_t *)&static_pointer_cast<LT>(y)->data);
				return make_shared<LT>(x->getLocation(), *((double *)&tmp));
			} else
				return {};
		case OP_XOR:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<I32LiteralExprNode>(
					x->getLocation(),
					x->data ^ static_pointer_cast<LT>(y)->data);
			else if constexpr (is_integral_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data ^ static_pointer_cast<LT>(y)->data);
			else if constexpr (is_same_v<T, float>) {
				uint32_t tmp = (*(uint32_t *)&x->data) ^ (*(uint32_t *)&static_pointer_cast<LT>(y)->data);
				return make_shared<LT>(x->getLocation(), *((float *)&tmp));
			} else if constexpr (is_same_v<T, double>) {
				uint64_t tmp = (*(uint64_t *)&x->data) ^ (*(uint64_t *)&static_pointer_cast<LT>(y)->data);
				return make_shared<LT>(x->getLocation(), *((double *)&tmp));
			} else
				return {};
		case OP_LAND:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>) {
				return make_shared<BoolLiteralExprNode>(
					x->getLocation(),
					x->data && static_pointer_cast<LT>(y)->data);
			} else
				return {};
		case OP_LOR:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>) {
				return make_shared<BoolLiteralExprNode>(
					x->getLocation(),
					x->data || static_pointer_cast<LT>(y)->data);
			} else
				return {};
		case OP_LSH:
			if (y->getExprType() != EXPR_I32)
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<I32LiteralExprNode>(
					x->getLocation(),
					(int)x->data << static_pointer_cast<I32LiteralExprNode>(y)->data);
			else if constexpr (is_integral_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data << static_pointer_cast<I32LiteralExprNode>(y)->data);
			else if constexpr (is_same_v<T, float>) {
				uint32_t tmp = (*(uint32_t *)&x->data) << (static_pointer_cast<I32LiteralExprNode>(y)->data);
				return make_shared<LT>(x->getLocation(), *((float *)&tmp));
			} else if constexpr (is_same_v<T, double>) {
				uint64_t tmp = (*(uint64_t *)&x->data) << (static_pointer_cast<I32LiteralExprNode>(y)->data);
				return make_shared<LT>(x->getLocation(), *((double *)&tmp));
			} else
				return {};
		case OP_RSH:
			if (y->getExprType() != EXPR_I32)
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<I32LiteralExprNode>(
					x->getLocation(),
					(int)x->data >> static_pointer_cast<I32LiteralExprNode>(y)->data);
			else if constexpr (is_integral_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data >> static_pointer_cast<I32LiteralExprNode>(y)->data);
			else if constexpr (is_same_v<T, float>) {
				uint32_t tmp = (*(uint32_t *)&x->data) >> (static_pointer_cast<I32LiteralExprNode>(y)->data);
				return make_shared<LT>(x->getLocation(), *((float *)&tmp));
			} else if constexpr (is_same_v<T, double>) {
				uint64_t tmp = (*(uint64_t *)&x->data) >> (static_pointer_cast<I32LiteralExprNode>(y)->data);
				return make_shared<LT>(x->getLocation(), *((double *)&tmp));
			} else
				return {};
		case OP_EQ:
			if (y->getExprType() != x->getExprType())
				return {};
			return make_shared<BoolLiteralExprNode>(
				x->getLocation(),
				x->data == static_pointer_cast<LT>(y)->data);
		case OP_NEQ:
			if (y->getExprType() != x->getExprType())
				return {};
			return make_shared<BoolLiteralExprNode>(
				x->getLocation(),
				x->data != static_pointer_cast<LT>(y)->data);
		case OP_LT:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_arithmetic_v<T>)
				return make_shared<BoolLiteralExprNode>(
					x->getLocation(),
					x->data < static_pointer_cast<LT>(y)->data);
			else
				return {};
		case OP_GT:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_arithmetic_v<T>)
				return make_shared<BoolLiteralExprNode>(
					x->getLocation(),
					x->data > static_pointer_cast<LT>(y)->data);
			else
				return {};
		case OP_LTEQ:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_arithmetic_v<T>)
				return make_shared<BoolLiteralExprNode>(
					x->getLocation(),
					x->data <= static_pointer_cast<LT>(y)->data);
			else
				return {};
		case OP_GTEQ:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_arithmetic_v<T>)
				return make_shared<BoolLiteralExprNode>(
					x->getLocation(),
					x->data >= static_pointer_cast<LT>(y)->data);
			else
				return {};
		case OP_SUBSCRIPT:
			if (y->getExprType() != EXPR_I32)
				return {};
			if (static_pointer_cast<I32LiteralExprNode>(y)->data < 0)
				return {};

			if constexpr (is_same_v<T, string>) {
				return make_shared<I32LiteralExprNode>(
					x->getLocation(),
					x->data[static_pointer_cast<I32LiteralExprNode>(y)->data]);
			} else
				return {};
		default:
			return {};
	}
}

shared_ptr<ExprNode> Compiler::evalConstExpr(shared_ptr<ExprNode> expr) {
	// stub
	return {};

	switch (expr->getExprType()) {
		case EXPR_I8:
		case EXPR_I16:
		case EXPR_I32:
		case EXPR_I64:
		case EXPR_U8:
		case EXPR_U16:
		case EXPR_U32:
		case EXPR_U64:
		case EXPR_F32:
		case EXPR_F64:
		case EXPR_STRING:
		case EXPR_BOOL:
			return expr;
		case EXPR_UNARY: {
			auto e = static_pointer_cast<UnaryOpExprNode>(expr);
			if (!evalConstExpr(e->x))
				return {};

			switch (e->x->getExprType()) {
				case EXPR_I32:
					return _evalConstUnaryOpExpr<int32_t>(e->op, static_pointer_cast<I32LiteralExprNode>(e->x));
				case EXPR_I64:
					return _evalConstUnaryOpExpr<int64_t>(e->op, static_pointer_cast<I64LiteralExprNode>(e->x));
				case EXPR_U32:
					return _evalConstUnaryOpExpr<uint32_t>(e->op, static_pointer_cast<U32LiteralExprNode>(e->x));
				case EXPR_U64:
					return _evalConstUnaryOpExpr<uint64_t>(e->op, static_pointer_cast<U64LiteralExprNode>(e->x));
				case EXPR_F32:
					return _evalConstUnaryOpExpr<float>(e->op, static_pointer_cast<F32LiteralExprNode>(e->x));
				case EXPR_F64:
					return _evalConstUnaryOpExpr<double>(e->op, static_pointer_cast<F64LiteralExprNode>(e->x));
				case EXPR_STRING:
					return _evalConstUnaryOpExpr<string>(e->op, static_pointer_cast<StringLiteralExprNode>(e->x));
				case EXPR_BOOL:
					return _evalConstUnaryOpExpr<bool>(e->op, static_pointer_cast<BoolLiteralExprNode>(e->x));
				default:
					return {};
			}
		}
		case EXPR_BINARY: {
			auto e = static_pointer_cast<BinaryOpExprNode>(expr);

			auto lhs = evalConstExpr(e->lhs), rhs = evalConstExpr(e->rhs);

			if ((!lhs) || (!rhs))
				return {};

			switch (lhs->getExprType()) {
				case EXPR_I32:
					return _evalConstBinaryOpExpr<int32_t>(
						e->op,
						static_pointer_cast<I32LiteralExprNode>(lhs),
						rhs);
				case EXPR_I64:
					return _evalConstBinaryOpExpr<int64_t>(
						e->op,
						static_pointer_cast<I64LiteralExprNode>(lhs),
						rhs);
				case EXPR_U32:
					return _evalConstBinaryOpExpr<uint32_t>(
						e->op,
						static_pointer_cast<U32LiteralExprNode>(lhs),
						rhs);
				case EXPR_U64:
					return _evalConstBinaryOpExpr<uint64_t>(
						e->op,
						static_pointer_cast<U64LiteralExprNode>(lhs),
						rhs);
				case EXPR_F32:
					return _evalConstBinaryOpExpr<float>(
						e->op,
						static_pointer_cast<F32LiteralExprNode>(lhs),
						rhs);
				case EXPR_F64:
					return _evalConstBinaryOpExpr<double>(
						e->op,
						static_pointer_cast<F64LiteralExprNode>(lhs),
						rhs);
				case EXPR_STRING:
					return _evalConstBinaryOpExpr<string>(
						e->op,
						static_pointer_cast<StringLiteralExprNode>(lhs),
						rhs);
				case EXPR_BOOL:
					return _evalConstBinaryOpExpr<bool>(
						e->op,
						static_pointer_cast<BoolLiteralExprNode>(lhs),
						rhs);
				default:
					return {};
			}
		}
		case EXPR_TERNARY: {
			auto e = static_pointer_cast<TernaryOpExprNode>(expr);

			auto condition = evalConstExpr(e->condition);
			if (condition) {
				if (static_pointer_cast<BoolLiteralExprNode>(castLiteralExpr(condition, TYPE_BOOL))->data)
					return e->x;
				return e->y;
			}

			return {};
		}
		case EXPR_ARRAY: {
			auto e = static_pointer_cast<ArrayExprNode>(expr);
			for (auto i : e->elements)
				if (!evalConstExpr(i))
					return {};
			return e;
		}
		case EXPR_MAP: {
			auto e = static_pointer_cast<MapExprNode>(expr);
			for (auto i : e->pairs)
				if ((!evalConstExpr(i.first)) || (!evalConstExpr(i.second)))
					return {};
			return e;
		}
		case EXPR_REF: {
			auto e = static_pointer_cast<RefExprNode>(expr);
		}
		case EXPR_CAST: {
			auto e = static_pointer_cast<CastExprNode>(expr);
			if (!isLiteralType(e->targetType))
				return {};

			switch (e->getExprType()) {
				case EXPR_I32: {
					switch (e->targetType->getTypeId()) {
						case TYPE_I8: {
						}
						case TYPE_I16: {
						}
						case TYPE_I32: {
						}
						case TYPE_I64: {
						}
						case TYPE_U8: {
						}
						case TYPE_U16: {
						}
						case TYPE_U32: {
						}
						case TYPE_U64: {
						}
						case TYPE_F32: {
						}
						case TYPE_F64: {
						}
						case TYPE_CHAR: {
						}
						case TYPE_WCHAR: {
						}
						case TYPE_BOOL: {
						}
					}
				}
				case EXPR_U32: {
				}
				case EXPR_U64: {
				}
				case EXPR_I64: {
				}
				case EXPR_F32: {
				}
				case EXPR_F64: {
				}
				case EXPR_BOOL: {
				}
			}
			return evalConstExpr(e->target);
		}
		case EXPR_MATCH:
			// stub
		default:
			return {};
	}
}

shared_ptr<TypeNameNode> Compiler::evalExprType(shared_ptr<ExprNode> expr) {
	switch (expr->getExprType()) {
		case EXPR_I8:
			return make_shared<I8TypeNameNode>(Location(), true);
		case EXPR_I16:
			return make_shared<I16TypeNameNode>(Location(), true);
		case EXPR_I32:
			return make_shared<I32TypeNameNode>(Location(), true);
		case EXPR_I64:
			return make_shared<I64TypeNameNode>(Location(), true);
		case EXPR_U8:
			return make_shared<U8TypeNameNode>(Location(), true);
		case EXPR_U16:
			return make_shared<U16TypeNameNode>(Location(), true);
		case EXPR_U32:
			return make_shared<U32TypeNameNode>(Location(), true);
		case EXPR_U64:
			return make_shared<U64TypeNameNode>(Location(), true);
		case EXPR_F32:
			return make_shared<F32TypeNameNode>(Location(), true);
		case EXPR_F64:
			return make_shared<F64TypeNameNode>(Location(), true);
		case EXPR_STRING:
			return make_shared<StringTypeNameNode>(Location(), true);
		case EXPR_BOOL:
			return make_shared<BoolTypeNameNode>(Location(), true);
		case EXPR_UNARY: {
			auto e = static_pointer_cast<UnaryOpExprNode>(expr);

			if (isCompoundType(evalExprType(e->x))) {
			} else
				return evalExprType(e->x);

			break;
		}
		case EXPR_BINARY: {
			auto e = static_pointer_cast<BinaryOpExprNode>(expr);

			if (isCompoundType(evalExprType(e->lhs))) {
			} else {
				switch (e->op) {
					case OP_ADD:
					case OP_SUB:
					case OP_MUL:
					case OP_DIV:
					case OP_MOD:
					case OP_AND:
					case OP_OR:
					case OP_XOR:
					case OP_LSH:
					case OP_RSH:
					case OP_ASSIGN:
					case OP_ASSIGN_ADD:
					case OP_ASSIGN_SUB:
					case OP_ASSIGN_MUL:
					case OP_ASSIGN_DIV:
					case OP_ASSIGN_MOD:
					case OP_ASSIGN_AND:
					case OP_ASSIGN_OR:
					case OP_ASSIGN_XOR:
					case OP_ASSIGN_LAND:
					case OP_ASSIGN_LOR:
					case OP_ASSIGN_LSH:
					case OP_ASSIGN_RSH:
						return evalExprType(e->lhs);
					case OP_LAND:
					case OP_LOR:
					case OP_EQ:
					case OP_NEQ:
					case OP_STRICTEQ:
					case OP_STRICTNEQ:
					case OP_LT:
					case OP_GT:
					case OP_LTEQ:
					case OP_GTEQ:
						return make_shared<BoolTypeNameNode>(Location(), true);
					case OP_SWAP:
					default:
						assert(false);
				}
			}

			break;
		}
		case EXPR_REF: {
			auto e = static_pointer_cast<RefExprNode>(expr);
			deque<pair<Ref, shared_ptr<AstNode>>> resolvedParts;

			if (resolveRef(e->ref, resolvedParts)) {
				switch (resolvedParts.back().second->getNodeType()) {
					case AST_VAR:
						return static_pointer_cast<VarNode>(resolvedParts.back().second)->type;
					case AST_LOCAL_VAR:
						return static_pointer_cast<LocalVarNode>(resolvedParts.back().second)->type;
					case AST_ARG_REF:
						return curFn->params.at(static_pointer_cast<ArgRefNode>(resolvedParts.back().second)->index).type;
					case AST_CLASS:
					case AST_INTERFACE:
					case AST_TRAIT:
						throw FatalCompilationError(
							{ e->getLocation(),
								MSG_ERROR,
								"`" + to_string(e->ref) + "' is a type" });
					default:
						assert(false);
				}
			} else {
				throw FatalCompilationError(
					{ e->getLocation(),
						MSG_ERROR,
						"Identifier not found: `" + to_string(e->ref) + "'" });
			}

			break;
		}
		case EXPR_NEW:
			return static_pointer_cast<NewExprNode>(expr)->type;
	}
	assert(false);
}
