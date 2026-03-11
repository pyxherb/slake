#include "../compiler.h"
#include <slake/flib/bitop.h>
#include <slake/flib/math/fmod.h>

using namespace slkc;

template <typename T, typename E, typename SetData = typename E::SetData>
peff::Option<CompilationError> _doSimpleIntLiteralCast(
	CompileEnv *compileEnv,
	AstNodePtr<ExprNode> src,
	AstNodePtr<E> &exprOut) {
	static SetData _setData;

	switch (src->exprKind) {
		case ExprKind::I8: {
			AstNodePtr<I8LiteralExprNode> l = src.castTo<I8LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::I16: {
			AstNodePtr<I16LiteralExprNode> l = src.castTo<I16LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::I32: {
			AstNodePtr<I32LiteralExprNode> l = src.castTo<I32LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::I64: {
			AstNodePtr<I64LiteralExprNode> l = src.castTo<I64LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::U8: {
			AstNodePtr<U8LiteralExprNode> l = src.castTo<U8LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::U16: {
			AstNodePtr<U16LiteralExprNode> l = src.castTo<U16LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::U32: {
			AstNodePtr<U32LiteralExprNode> l = src.castTo<U32LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::U64: {
			AstNodePtr<U64LiteralExprNode> l = src.castTo<U64LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::F32: {
			AstNodePtr<F32LiteralExprNode> l = src.castTo<F32LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::F64: {
			AstNodePtr<F64LiteralExprNode> l = src.castTo<F64LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::Bool: {
			AstNodePtr<BoolLiteralExprNode> l = src.castTo<BoolLiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		default:
			exprOut = {};
			break;
	}

	return {};
}

peff::Option<CompilationError> _castConstExpr(
	CompileEnv *compileEnv,
	CompilationContext *compilationContext,
	PathEnv *pathEnv,
	AstNodePtr<ExprNode> expr,
	AstNodePtr<TypeNameNode> type,
	AstNodePtr<ExprNode> &exprOut,
	bool *sideEffectAppliedOut,
	EvalConstExprContext &context,
	EvalConstExprResult &resultOut) {
	AstNodePtr<CastExprNode> castExpr;

	if (!(castExpr = makeAstNode<CastExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
		return genOutOfMemoryCompError();
	}

	castExpr->source = expr;
	castExpr->targetType = type;

	bool sideEffectApplied;

	SLKC_RETURN_IF_COMP_ERROR(_doEvalConstExpr(compileEnv, compilationContext, pathEnv, castExpr.castTo<ExprNode>(), exprOut, &sideEffectApplied, context, resultOut));

	*sideEffectAppliedOut |= sideEffectApplied;

	return {};
}

template <typename LT>
static peff::Option<CompilationError> evalIntegralBinaryOpExpr(
	CompileEnv *compileEnv,
	CompilationContext *compilationContext,
	PathEnv *pathEnv,
	AstNodePtr<TypeNameNode> mainOperationType,
	BinaryOp binaryOp,
	AstNodePtr<ExprNode> lhs,
	AstNodePtr<TypeNameNode> lhsType,
	const EvalConstExprResult &lhsResult,
	AstNodePtr<ExprNode> rhs,
	AstNodePtr<TypeNameNode> rhsType,
	const EvalConstExprResult &rhsResult,
	AstNodePtr<ExprNode> &exprOut,
	bool *sideEffectAppliedOut,
	EvalConstExprContext &context,
	EvalConstExprResult &resultOut) {
	switch (binaryOp) {
		case BinaryOp::Add: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, mainOperationType, lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (!(exprOut = makeAstNode<LT>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<LT>()->data + rhs.castTo<LT>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		case BinaryOp::Sub: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, mainOperationType, lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (!(exprOut = makeAstNode<LT>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<LT>()->data - rhs.castTo<LT>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		case BinaryOp::Mul: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, mainOperationType, lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (!(exprOut = makeAstNode<LT>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<LT>()->data * rhs.castTo<LT>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		case BinaryOp::Div: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, mainOperationType, lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (!(exprOut = makeAstNode<LT>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<LT>()->data / rhs.castTo<LT>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		case BinaryOp::Mod: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, mainOperationType, lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (!(exprOut = makeAstNode<LT>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<LT>()->data % rhs.castTo<LT>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		case BinaryOp::And: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, mainOperationType, lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (!(exprOut = makeAstNode<LT>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<LT>()->data & rhs.castTo<LT>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		case BinaryOp::Or: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, mainOperationType, lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (!(exprOut = makeAstNode<LT>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<LT>()->data | rhs.castTo<LT>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		case BinaryOp::Xor: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, mainOperationType, lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (!(exprOut = makeAstNode<LT>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<LT>()->data ^ rhs.castTo<LT>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		case BinaryOp::LAnd: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			AstNodePtr<BoolTypeNameNode> boolType;

			if (!(boolType = makeAstNode<BoolTypeNameNode>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document)))
				return genOutOfMemoryCompError();
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, boolType.castTo<TypeNameNode>(), lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, boolType.castTo<TypeNameNode>(), rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<BoolLiteralExprNode>()->data && rhs.castTo<BoolLiteralExprNode>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		case BinaryOp::LOr: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			AstNodePtr<BoolTypeNameNode> boolType;

			if (!(boolType = makeAstNode<BoolTypeNameNode>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document)))
				return genOutOfMemoryCompError();
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, boolType.castTo<TypeNameNode>(), lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, boolType.castTo<TypeNameNode>(), rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<BoolLiteralExprNode>()->data || rhs.castTo<BoolLiteralExprNode>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		case BinaryOp::Shl: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			AstNodePtr<U32TypeNameNode> u32Type;

			if (!(u32Type = makeAstNode<U32TypeNameNode>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document)))
				return genOutOfMemoryCompError();
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, u32Type.castTo<TypeNameNode>(), rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			// TODO: Use a portable one.
			if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<BoolLiteralExprNode>()->data << rhs.castTo<U32LiteralExprNode>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		case BinaryOp::Shr: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			AstNodePtr<U32TypeNameNode> u32Type;

			if (!(u32Type = makeAstNode<U32TypeNameNode>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document)))
				return genOutOfMemoryCompError();
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, u32Type.castTo<TypeNameNode>(), rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			// TODO: Use a portable one.
			if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<BoolLiteralExprNode>()->data >> rhs.castTo<U32LiteralExprNode>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		case BinaryOp::Assign: {
			if ((!lhsType->isNullable) && (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			// Because the lhs and rhs must be a comptime value expression, we can safely assign them to the overrides.
			if (lhsResult.correspondingMember && (lhsResult.correspondingMember->getAstNodeType() == AstNodeType::Var)) {
				AstNodePtr<VarNode> v = lhsResult.correspondingMember.castTo<VarNode>();
				if (v->isLocalVar())
					if (!context.varValueOverrides.insert(
							std::move(v),
							AstNodePtr<ExprNode>(rhs))) {
						return genOutOfMemoryCompError();
					}
			}

			exprOut = rhs;
			*sideEffectAppliedOut = true;
			break;
		}
		case BinaryOp::Eq:
		case BinaryOp::StrictEq: {
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, mainOperationType, lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (lhsType->isNullable) {
				if (rhs->exprKind == ExprKind::Null) {
					if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
							  compileEnv->allocator.get(),
							  compileEnv->allocator.get(), compileEnv->document,
							  lhs->exprKind == ExprKind::Null)
								.template castTo<ExprNode>()))
						return genOutOfMemoryCompError();
				} else {
					if (lhs->exprKind == ExprKind::Null) {
						if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
								  compileEnv->allocator.get(),
								  compileEnv->allocator.get(), compileEnv->document,
								  rhs->exprKind == ExprKind::Null)
									.template castTo<ExprNode>()))
							return genOutOfMemoryCompError();
					} else {
						if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
								  compileEnv->allocator.get(),
								  compileEnv->allocator.get(), compileEnv->document,
								  lhs.castTo<LT>()->data == rhs.castTo<LT>()->data)
									.template castTo<ExprNode>()))
							return genOutOfMemoryCompError();
					}
				}
			} else {
				if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(), compileEnv->document,
						  lhs.castTo<LT>()->data == rhs.castTo<LT>()->data)
							.template castTo<ExprNode>()))
					return genOutOfMemoryCompError();
			}
			break;
		}
		case BinaryOp::Neq:
		case BinaryOp::StrictNeq: {
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, mainOperationType, lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (lhsType->isNullable) {
				if (rhs->exprKind == ExprKind::Null) {
					if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
							  compileEnv->allocator.get(),
							  compileEnv->allocator.get(), compileEnv->document,
							  lhs->exprKind != ExprKind::Null)
								.template castTo<ExprNode>()))
						return genOutOfMemoryCompError();
				} else {
					if (lhs->exprKind == ExprKind::Null) {
						if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
								  compileEnv->allocator.get(),
								  compileEnv->allocator.get(), compileEnv->document,
								  rhs->exprKind != ExprKind::Null)
									.template castTo<ExprNode>()))
							return genOutOfMemoryCompError();
					} else {
						if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
								  compileEnv->allocator.get(),
								  compileEnv->allocator.get(), compileEnv->document,
								  lhs.castTo<LT>()->data != rhs.castTo<LT>()->data)
									.template castTo<ExprNode>()))
							return genOutOfMemoryCompError();
					}
				}
			} else {
				if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(), compileEnv->document,
						  lhs.castTo<LT>()->data != rhs.castTo<LT>()->data)
							.template castTo<ExprNode>()))
					return genOutOfMemoryCompError();
			}
			break;
		}
		case BinaryOp::Gt: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, mainOperationType, lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<LT>()->data > rhs.castTo<LT>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		case BinaryOp::Lt: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, mainOperationType, lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<LT>()->data < rhs.castTo<LT>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		case BinaryOp::GtEq: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, mainOperationType, lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<LT>()->data >= rhs.castTo<LT>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		case BinaryOp::LtEq: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, mainOperationType, lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<LT>()->data <= rhs.castTo<LT>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		// TODO: Implement comparison operation.
		default:
			exprOut = {};
			break;
	}

	return {};
}

template <typename LT>
static peff::Option<CompilationError> evalFloatingPointBinaryOpExpr(
	CompileEnv *compileEnv,
	CompilationContext *compilationContext,
	PathEnv *pathEnv,
	AstNodePtr<TypeNameNode> mainOperationType,
	BinaryOp binaryOp,
	AstNodePtr<ExprNode> lhs,
	AstNodePtr<TypeNameNode> lhsType,
	const EvalConstExprResult &lhsResult,
	AstNodePtr<ExprNode> rhs,
	AstNodePtr<TypeNameNode> rhsType,
	const EvalConstExprResult &rhsResult,
	AstNodePtr<ExprNode> &exprOut,
	bool *sideEffectAppliedOut,
	EvalConstExprContext &context,
	EvalConstExprResult &resultOut) {
	switch (binaryOp) {
		case BinaryOp::Add: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, mainOperationType, lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (!(exprOut = makeAstNode<LT>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<LT>()->data + rhs.castTo<LT>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		case BinaryOp::Sub: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, mainOperationType, lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (!(exprOut = makeAstNode<LT>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<LT>()->data - rhs.castTo<LT>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		case BinaryOp::Mul: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, mainOperationType, lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (!(exprOut = makeAstNode<LT>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<LT>()->data * rhs.castTo<LT>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		case BinaryOp::Div: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, mainOperationType, lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (!(exprOut = makeAstNode<LT>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<LT>()->data / rhs.castTo<LT>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		// TODO: Implement mod operation.
		case BinaryOp::LAnd: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			AstNodePtr<BoolTypeNameNode> boolType;

			if (!(boolType = makeAstNode<BoolTypeNameNode>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document)))
				return genOutOfMemoryCompError();
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, boolType.castTo<TypeNameNode>(), lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, boolType.castTo<TypeNameNode>(), rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<BoolLiteralExprNode>()->data && rhs.castTo<BoolLiteralExprNode>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		case BinaryOp::LOr: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			AstNodePtr<BoolTypeNameNode> boolType;

			if (!(boolType = makeAstNode<BoolTypeNameNode>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document)))
				return genOutOfMemoryCompError();
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, boolType.castTo<TypeNameNode>(), lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, boolType.castTo<TypeNameNode>(), rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<BoolLiteralExprNode>()->data || rhs.castTo<BoolLiteralExprNode>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		case BinaryOp::Assign: {
			if ((!lhsType->isNullable) && (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if (lhsResult.correspondingMember && (lhsResult.correspondingMember->getAstNodeType() == AstNodeType::Var)) {
				AstNodePtr<VarNode> v = lhsResult.correspondingMember.castTo<VarNode>();
				if (v->isLocalVar())
					if (!context.varValueOverrides.insert(
							std::move(v),
							AstNodePtr<ExprNode>(rhs))) {
						return genOutOfMemoryCompError();
					}
			}

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			exprOut = rhs;
			*sideEffectAppliedOut = true;
			break;
		}

		case BinaryOp::Eq:
		case BinaryOp::StrictEq: {
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, mainOperationType, lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (lhsType->isNullable) {
				if (rhs->exprKind == ExprKind::Null) {
					if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
							  compileEnv->allocator.get(),
							  compileEnv->allocator.get(), compileEnv->document,
							  lhs->exprKind == ExprKind::Null)
								.template castTo<ExprNode>()))
						return genOutOfMemoryCompError();
				} else {
					if (lhs->exprKind == ExprKind::Null) {
						if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
								  compileEnv->allocator.get(),
								  compileEnv->allocator.get(), compileEnv->document,
								  rhs->exprKind == ExprKind::Null)
									.template castTo<ExprNode>()))
							return genOutOfMemoryCompError();
					} else {
						if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
								  compileEnv->allocator.get(),
								  compileEnv->allocator.get(), compileEnv->document,
								  lhs.castTo<LT>()->data == rhs.castTo<LT>()->data)
									.template castTo<ExprNode>()))
							return genOutOfMemoryCompError();
					}
				}
			} else {
				if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(), compileEnv->document,
						  lhs.castTo<LT>()->data == rhs.castTo<LT>()->data)
							.template castTo<ExprNode>()))
					return genOutOfMemoryCompError();
			}
			break;
		}
		case BinaryOp::Neq:
		case BinaryOp::StrictNeq: {
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, mainOperationType, lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (lhsType->isNullable) {
				if (rhs->exprKind == ExprKind::Null) {
					if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
							  compileEnv->allocator.get(),
							  compileEnv->allocator.get(), compileEnv->document,
							  lhs->exprKind != ExprKind::Null)
								.template castTo<ExprNode>()))
						return genOutOfMemoryCompError();
				} else {
					if (lhs->exprKind == ExprKind::Null) {
						if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
								  compileEnv->allocator.get(),
								  compileEnv->allocator.get(), compileEnv->document,
								  rhs->exprKind != ExprKind::Null)
									.template castTo<ExprNode>()))
							return genOutOfMemoryCompError();
					} else {
						if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
								  compileEnv->allocator.get(),
								  compileEnv->allocator.get(), compileEnv->document,
								  lhs.castTo<LT>()->data != rhs.castTo<LT>()->data)
									.template castTo<ExprNode>()))
							return genOutOfMemoryCompError();
					}
				}
			} else {
				if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(), compileEnv->document,
						  lhs.castTo<LT>()->data != rhs.castTo<LT>()->data)
							.template castTo<ExprNode>()))
					return genOutOfMemoryCompError();
			}
			break;
		}
		case BinaryOp::Gt: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, mainOperationType, lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<LT>()->data > rhs.castTo<LT>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		case BinaryOp::Lt: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, mainOperationType, lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<LT>()->data < rhs.castTo<LT>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		case BinaryOp::GtEq: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, mainOperationType, lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<LT>()->data >= rhs.castTo<LT>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		case BinaryOp::LtEq: {
			if ((lhsType->isNullable) || (rhsType->isNullable)) {
				exprOut = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, lhs, mainOperationType, lhs, sideEffectAppliedOut, context, resultOut));
			SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, pathEnv, rhs, mainOperationType, rhs, sideEffectAppliedOut, context, resultOut));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(), compileEnv->document,
					  lhs.castTo<LT>()->data <= rhs.castTo<LT>()->data)
						.template castTo<ExprNode>()))
				return genOutOfMemoryCompError();
			break;
		}
		// TODO: Implement comparison operation.
		default:
			exprOut = {};
			break;
	}

	return {};
}

SLAKE_API peff::Option<CompilationError> slkc::_doEvalConstExpr(
	CompileEnv *compileEnv,
	CompilationContext *compilationContext,
	PathEnv *pathEnv,
	AstNodePtr<ExprNode> expr,
	AstNodePtr<ExprNode> &exprOut,
	bool *sideEffectAppliedOut,
	EvalConstExprContext &context,
	EvalConstExprResult &resultOut) {
	*sideEffectAppliedOut = false;
reeval:

	switch (expr->exprKind) {
		case ExprKind::Unary: {
			// stub
			exprOut = {};
			break;
		}
		case ExprKind::Binary: {
			AstNodePtr<BinaryExprNode> e = expr.castTo<BinaryExprNode>();

			AstNodePtr<ExprNode> lhs, rhs;

			AstNodePtr<TypeNameNode> lhsType, rhsType, decayedLhsType, decayedRhsType;
			SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, pathEnv, e->lhs, lhsType, {}));
			SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, pathEnv, e->rhs, rhsType, {}));

			SLKC_RETURN_IF_COMP_ERROR(
				removeRefOfType(lhsType, decayedLhsType));
			SLKC_RETURN_IF_COMP_ERROR(
				removeRefOfType(rhsType, decayedRhsType));

			AstNodePtr<TypeNameNode> mainOperationType;

			switch (e->binaryOp) {
				case BinaryOp::Assign:
				case BinaryOp::Shl:
				case BinaryOp::Shr:
				case BinaryOp::ShlAssign:
				case BinaryOp::ShrAssign:
				case BinaryOp::Subscript:
					mainOperationType = decayedLhsType;
					break;
				default: {
					// If the both conversions of sides are viable, choose their common type.
					// If only one side is viable, choose the viable side.
					// Or else, choose the left side to generate the error message.
					bool lhsToRhsViability, rhsToLhsViability;
					SLKC_RETURN_IF_COMP_ERROR(isConvertible(decayedLhsType, decayedRhsType, true, lhsToRhsViability));
					SLKC_RETURN_IF_COMP_ERROR(isConvertible(decayedRhsType, decayedLhsType, true, rhsToLhsViability));
					if (lhsToRhsViability && rhsToLhsViability) {
						SLKC_RETURN_IF_COMP_ERROR(deduceCommonType(decayedLhsType, decayedRhsType, mainOperationType));
					}
					if (!mainOperationType) {
						if (lhsToRhsViability)
							mainOperationType = decayedRhsType;
						else
							mainOperationType = decayedLhsType;
					}
				}
			}

			EvalConstExprResult lhsResult;
			EvalConstExprResult rhsResult;

			bool el, er;
			SLKC_RETURN_IF_COMP_ERROR(_doEvalConstExpr(compileEnv, compilationContext, pathEnv, e->lhs, lhs, &el, context, lhsResult));
			SLKC_RETURN_IF_COMP_ERROR(_doEvalConstExpr(compileEnv, compilationContext, pathEnv, e->rhs, rhs, &er, context, rhsResult));

			*sideEffectAppliedOut |= el;
			*sideEffectAppliedOut |= er;

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			switch (mainOperationType->typeNameKind) {
				case TypeNameKind::I8:
					SLKC_RETURN_IF_COMP_ERROR(evalIntegralBinaryOpExpr<I8LiteralExprNode>(
						compileEnv, compilationContext, pathEnv,
						mainOperationType,
						e->binaryOp,
						lhs, decayedLhsType, lhsResult,
						rhs, decayedRhsType, rhsResult,
						exprOut,
						sideEffectAppliedOut, context, resultOut));
					break;
				case TypeNameKind::I16:
					SLKC_RETURN_IF_COMP_ERROR(evalIntegralBinaryOpExpr<I16LiteralExprNode>(
						compileEnv, compilationContext, pathEnv,
						mainOperationType,
						e->binaryOp,
						lhs, decayedLhsType, lhsResult,
						rhs, decayedRhsType, rhsResult,
						exprOut,
						sideEffectAppliedOut, context, resultOut));
					break;
				case TypeNameKind::I32:
					SLKC_RETURN_IF_COMP_ERROR(evalIntegralBinaryOpExpr<I32LiteralExprNode>(
						compileEnv, compilationContext, pathEnv,
						mainOperationType,
						e->binaryOp,
						lhs, decayedLhsType, lhsResult,
						rhs, decayedRhsType, rhsResult,
						exprOut,
						sideEffectAppliedOut, context, resultOut));
					break;
				case TypeNameKind::I64:
					SLKC_RETURN_IF_COMP_ERROR(evalIntegralBinaryOpExpr<I64LiteralExprNode>(
						compileEnv, compilationContext, pathEnv,
						mainOperationType,
						e->binaryOp,
						lhs, decayedLhsType, lhsResult,
						rhs, decayedRhsType, rhsResult,
						exprOut,
						sideEffectAppliedOut, context, resultOut));
					break;
				case TypeNameKind::U8:
					SLKC_RETURN_IF_COMP_ERROR(evalIntegralBinaryOpExpr<U8LiteralExprNode>(
						compileEnv, compilationContext, pathEnv,
						mainOperationType,
						e->binaryOp,
						lhs, decayedLhsType, lhsResult,
						rhs, decayedRhsType, rhsResult,
						exprOut,
						sideEffectAppliedOut, context, resultOut));
					break;
				case TypeNameKind::U16:
					SLKC_RETURN_IF_COMP_ERROR(evalIntegralBinaryOpExpr<U16LiteralExprNode>(
						compileEnv, compilationContext, pathEnv,
						mainOperationType,
						e->binaryOp,
						lhs, decayedLhsType, lhsResult,
						rhs, decayedRhsType, rhsResult,
						exprOut,
						sideEffectAppliedOut, context, resultOut));
					break;
				case TypeNameKind::U32:
					SLKC_RETURN_IF_COMP_ERROR(evalIntegralBinaryOpExpr<U32LiteralExprNode>(
						compileEnv, compilationContext, pathEnv,
						mainOperationType,
						e->binaryOp,
						lhs, decayedLhsType, lhsResult,
						rhs, decayedRhsType, rhsResult,
						exprOut,
						sideEffectAppliedOut, context, resultOut));
					break;
				case TypeNameKind::U64:
					SLKC_RETURN_IF_COMP_ERROR(evalIntegralBinaryOpExpr<U64LiteralExprNode>(
						compileEnv, compilationContext, pathEnv,
						mainOperationType,
						e->binaryOp,
						lhs, decayedLhsType, lhsResult,
						rhs, decayedRhsType, rhsResult,
						exprOut,
						sideEffectAppliedOut, context, resultOut));
					break;
				case TypeNameKind::F32:
					SLKC_RETURN_IF_COMP_ERROR(evalFloatingPointBinaryOpExpr<F32LiteralExprNode>(
						compileEnv, compilationContext, pathEnv,
						mainOperationType,
						e->binaryOp,
						lhs, decayedLhsType, lhsResult,
						rhs, decayedRhsType, rhsResult,
						exprOut,
						sideEffectAppliedOut, context, resultOut));
					break;
				case TypeNameKind::F64:
					SLKC_RETURN_IF_COMP_ERROR(evalFloatingPointBinaryOpExpr<F64LiteralExprNode>(
						compileEnv, compilationContext, pathEnv,
						mainOperationType,
						e->binaryOp,
						lhs, decayedLhsType, lhsResult,
						rhs, decayedRhsType, rhsResult,
						exprOut,
						sideEffectAppliedOut, context, resultOut));
					break;
				default:
					exprOut = {};
			}

			break;
		}
		case ExprKind::Ternary: {
			exprOut = {};
			break;
		}
		case ExprKind::IdRef: {
			CompileExprResult result(compileEnv->allocator.get());

			NormalCompilationContext compCtxt(compileEnv, compilationContext);
			if (auto e = compileExpr(compileEnv, compilationContext, pathEnv, expr, ExprEvalPurpose::RValue, {}, result); e) {
				switch (e->errorKind) {
					case CompilationErrorKind::OutOfMemory:
					case CompilationErrorKind::OutOfRuntimeMemory:
					case CompilationErrorKind::StackOverflow:
						return e;
					default:
						exprOut = {};
				}
			} else {
				if (result.evaluatedFinalMember) {
					if (result.evaluatedFinalMember->getAstNodeType() == AstNodeType::Var) {
						if (auto it = context.varValueOverrides.find(result.evaluatedFinalMember.castTo<VarNode>()); it != context.varValueOverrides.end()) {
							exprOut = it.value();
						} else {
							if (auto overrideType = pathEnv->lookupVarNullOverride(result.evaluatedFinalMember.castTo<VarNode>());
								overrideType.hasValue()) {
								switch (*overrideType) {
									case NullOverrideType::Nullify:
										if (!(exprOut = makeAstNode<NullLiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document).castTo<ExprNode>())) {
											return genOutOfMemoryCompError();
										}
									case NullOverrideType::Denullify:
										exprOut = {};
										*sideEffectAppliedOut = true;
									case NullOverrideType::Uncertain:
										if (!(exprOut = makeAstNode<NullLiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document).castTo<ExprNode>())) {
											return genOutOfMemoryCompError();
										}
										*sideEffectAppliedOut = true;
								}
							} else {
								if (result.evaluatedFinalMember.castTo<VarNode>()->type->isNullable) {
									if (!(exprOut = makeAstNode<NullLiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document).castTo<ExprNode>())) {
										return genOutOfMemoryCompError();
									}
									*sideEffectAppliedOut = true;
								} else {
									exprOut = {};
									*sideEffectAppliedOut = true;
								}
							}
						}
					}
				} else {
					exprOut = {};
					*sideEffectAppliedOut = true;
				}
			}
			break;
		}
		case ExprKind::I8:
		case ExprKind::I16:
		case ExprKind::I32:
		case ExprKind::I64:
		case ExprKind::U8:
		case ExprKind::U16:
		case ExprKind::U32:
		case ExprKind::U64:
		case ExprKind::F32:
		case ExprKind::F64:
		case ExprKind::String:
		case ExprKind::Bool:
		case ExprKind::Null: {
			exprOut = expr;
			break;
		}
		case ExprKind::Cast: {
			AstNodePtr<CastExprNode> e = expr.castTo<CastExprNode>();
			AstNodePtr<ExprNode> src;
			SLKC_RETURN_IF_COMP_ERROR(_doEvalConstExpr(compileEnv, compilationContext, pathEnv, e->source, src, sideEffectAppliedOut, context, resultOut));

			if (!src) {
				exprOut = {};
				break;
			}

			// The type may be nullable, but the value will still be non-nullable.
			switch (e->targetType->typeNameKind) {
				case TypeNameKind::I8: {
					AstNodePtr<I8LiteralExprNode> l;

					if ((e->targetType->isNullable) && (e->source->exprKind == ExprKind::Null)) {
						exprOut = e->source;
					} else {
						if (!(l = makeAstNode<I8LiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, 0))) {
							return genOutOfMemoryCompError();
						}
						SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<int8_t, I8LiteralExprNode>(compileEnv, src, l));
						exprOut = l.castTo<ExprNode>();
					}

					break;
				}
				case TypeNameKind::I16: {
					AstNodePtr<I16LiteralExprNode> l;

					if ((e->targetType->isNullable) && (e->source->exprKind == ExprKind::Null)) {
						exprOut = e->source;
					} else {
						if (!(l = makeAstNode<I16LiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, 0))) {
							return genOutOfMemoryCompError();
						}
						SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<int16_t, I16LiteralExprNode>(compileEnv, src, l));
						exprOut = l.castTo<ExprNode>();
					}

					break;
				}
				case TypeNameKind::I32: {
					AstNodePtr<I32LiteralExprNode> l;

					if ((e->targetType->isNullable) && (e->source->exprKind == ExprKind::Null)) {
						exprOut = e->source;
					} else {
						if (!(l = makeAstNode<I32LiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, 0))) {
							return genOutOfMemoryCompError();
						}
						SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<int32_t, I32LiteralExprNode>(compileEnv, src, l));
						exprOut = l.castTo<ExprNode>();
					}
					break;
				}
				case TypeNameKind::I64: {
					AstNodePtr<I64LiteralExprNode> l;

					if ((e->targetType->isNullable) && (e->source->exprKind == ExprKind::Null)) {
						exprOut = e->source;
					} else {
						if (!(l = makeAstNode<I64LiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, 0))) {
							return genOutOfMemoryCompError();
						}
						SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<int64_t, I64LiteralExprNode>(compileEnv, src, l));
						exprOut = l.castTo<ExprNode>();
					}
					break;
				}
				case TypeNameKind::U8: {
					AstNodePtr<U8LiteralExprNode> l;

					if ((e->targetType->isNullable) && (e->source->exprKind == ExprKind::Null)) {
						exprOut = e->source;
					} else {
						if (!(l = makeAstNode<U8LiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, 0))) {
							return genOutOfMemoryCompError();
						}
						SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<int8_t, U8LiteralExprNode>(compileEnv, src, l));
						exprOut = l.castTo<ExprNode>();
					}
					break;
				}
				case TypeNameKind::U16: {
					AstNodePtr<U16LiteralExprNode> l;

					if ((e->targetType->isNullable) && (e->source->exprKind == ExprKind::Null)) {
						exprOut = e->source;
					} else {
						if (!(l = makeAstNode<U16LiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, 0))) {
							return genOutOfMemoryCompError();
						}
						SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<int16_t, U16LiteralExprNode>(compileEnv, src, l));
						exprOut = l.castTo<ExprNode>();
					}
					break;
				}
				case TypeNameKind::U32: {
					AstNodePtr<U32LiteralExprNode> l;

					if ((e->targetType->isNullable) && (e->source->exprKind == ExprKind::Null)) {
						exprOut = e->source;
					} else {
						if (!(l = makeAstNode<U32LiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, 0))) {
							return genOutOfMemoryCompError();
						}
						SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<int32_t, U32LiteralExprNode>(compileEnv, src, l));
						exprOut = l.castTo<ExprNode>();
					}
					break;
				}
				case TypeNameKind::U64: {
					AstNodePtr<U64LiteralExprNode> l;

					if ((e->targetType->isNullable) && (e->source->exprKind == ExprKind::Null)) {
						exprOut = e->source;
					} else {
						if (!(l = makeAstNode<U64LiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, 0))) {
							return genOutOfMemoryCompError();
						}
						SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<int64_t, U64LiteralExprNode>(compileEnv, src, l));
						exprOut = l.castTo<ExprNode>();
					}
					break;
				}
				case TypeNameKind::F32: {
					AstNodePtr<F32LiteralExprNode> l;

					if ((e->targetType->isNullable) && (e->source->exprKind == ExprKind::Null)) {
						exprOut = e->source;
					} else {
						if (!(l = makeAstNode<F32LiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, 0))) {
							return genOutOfMemoryCompError();
						}
						SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<float, F32LiteralExprNode>(compileEnv, src, l));
						exprOut = l.castTo<ExprNode>();
					}
					break;
				}
				case TypeNameKind::F64: {
					AstNodePtr<F64LiteralExprNode> l;

					if ((e->targetType->isNullable) && (e->source->exprKind == ExprKind::Null)) {
						exprOut = e->source;
					} else {
						if (!(l = makeAstNode<F64LiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, 0))) {
							return genOutOfMemoryCompError();
						}
						SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<double, F64LiteralExprNode>(compileEnv, src, l));
						exprOut = l.castTo<ExprNode>();
					}
					break;
				}
				case TypeNameKind::Bool: {
					AstNodePtr<BoolLiteralExprNode> l;

					if ((e->targetType->isNullable) && (e->source->exprKind == ExprKind::Null)) {
						exprOut = e->source;
					} else {
						if (!(l = makeAstNode<BoolLiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, 0))) {
							return genOutOfMemoryCompError();
						}
						SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<double, BoolLiteralExprNode>(compileEnv, src, l));
						exprOut = l.castTo<ExprNode>();
					}
					break;
				}
				case TypeNameKind::String: {
					switch (expr->exprKind) {
						case ExprKind::String:
							exprOut = expr;
							break;
						default:
							break;
					}
					break;
				}
				case TypeNameKind::Object: {
					switch (expr->exprKind) {
						case ExprKind::Null:
							exprOut = expr;
							break;
						default:
							break;
					}
					break;
				}
				case TypeNameKind::Custom: {
					switch (expr->exprKind) {
						case ExprKind::Null:
							exprOut = expr;
							break;
						default:
							break;
					}
					break;
				}
				case TypeNameKind::Array: {
					switch (expr->exprKind) {
						case ExprKind::InitializerList:
						case ExprKind::Null:
							exprOut = expr;
							break;
						default:
							break;
					}
					break;
				}
				default:
					exprOut = {};
			}

			break;
		}
		case ExprKind::Wrapper:
			expr = expr.castTo<WrapperExprNode>()->target;
			goto reeval;
		case ExprKind::Bad: {
			exprOut = {};
			break;
		}
		default:
			std::terminate();
	}

	return {};
}

SLKC_API EvalConstExprContext::EvalConstExprContext(peff::Alloc *allocator) : varValueOverrides(allocator) {
}

SLKC_API peff::Option<CompilationError> slkc::evalConstExpr(
	CompileEnv *compileEnv,
	CompilationContext *compilationContext,
	PathEnv *pathEnv,
	AstNodePtr<ExprNode> expr,
	AstNodePtr<ExprNode> &exprOut,
	bool *sideEffectsAppliedOut) {
	bool sideEffectsAppliedPlaceholder;

	if (!sideEffectsAppliedOut)
		sideEffectsAppliedOut = &sideEffectsAppliedPlaceholder;

	EvalConstExprContext context(compileEnv->allocator.get());
	EvalConstExprResult result;

	return _doEvalConstExpr(compileEnv, compilationContext, pathEnv, expr, exprOut, sideEffectsAppliedOut, context, result);
}
