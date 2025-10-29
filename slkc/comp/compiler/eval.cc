#include "../compiler.h"
#include <slake/flib/bitop.h>
#include <slake/flib/math/fmod.h>

using namespace slkc;

template <typename T, typename E, typename SetData = typename E::SetData>
peff::Option<CompilationError> _doSimpleIntLiteralCast(
	CompileEnvironment *compileEnv,
	AstNodePtr<ExprNode> src,
	peff::SharedPtr<E> &exprOut) {
	static SetData _setData;

	switch (src->exprKind) {
		case ExprKind::I8: {
			peff::SharedPtr<I8LiteralExprNode> l = src.template castTo<I8LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::I16: {
			peff::SharedPtr<I16LiteralExprNode> l = src.template castTo<I16LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::I32: {
			peff::SharedPtr<I32LiteralExprNode> l = src.template castTo<I32LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::I64: {
			peff::SharedPtr<I64LiteralExprNode> l = src.template castTo<I64LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::U8: {
			peff::SharedPtr<U8LiteralExprNode> l = src.template castTo<U8LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::U16: {
			peff::SharedPtr<U16LiteralExprNode> l = src.template castTo<U16LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::U32: {
			peff::SharedPtr<U32LiteralExprNode> l = src.template castTo<U32LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::U64: {
			peff::SharedPtr<U64LiteralExprNode> l = src.template castTo<U64LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::F32: {
			peff::SharedPtr<F32LiteralExprNode> l = src.template castTo<F32LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::F64: {
			peff::SharedPtr<F64LiteralExprNode> l = src.template castTo<F64LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::Bool: {
			AstNodePtr<BoolLiteralExprNode> l = src.template castTo<BoolLiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		default:
			exprOut = {};
			break;
	}

	return {};
}

template <typename E, typename R, typename GetData = typename E::GetData, typename SetData = typename R::SetData>
peff::Option<CompilationError> _doSimpleArithmeticBinaryOp(
	CompileEnvironment *compileEnv,
	BinaryOp binaryOp,
	peff::SharedPtr<E> lhs,
	peff::SharedPtr<E> rhs,
	peff::SharedPtr<R> exprOut) {
	GetData _getData;
	SetData _setData;

	switch (binaryOp) {
		case BinaryOp::Add:
			_setData(exprOut, _getData(lhs) + _getData(rhs));
			break;
		case BinaryOp::Sub:
			_setData(exprOut, _getData(lhs) - _getData(rhs));
			break;
		case BinaryOp::Mul:
			_setData(exprOut, _getData(lhs) * _getData(rhs));
			break;
		case BinaryOp::Div:
			_setData(exprOut, _getData(lhs) / _getData(rhs));
			break;
		case BinaryOp::Mod:
			_setData(exprOut, _getData(lhs) % _getData(rhs));
			break;
		default:
			std::terminate();
	}

	return {};
}

template <typename E, typename R, typename GetData = typename E::GetData, typename SetData = typename R::SetData>
peff::Option<CompilationError> _doSimpleFloatingPointArithmeticBinaryOp(
	CompileEnvironment *compileEnv,
	BinaryOp binaryOp,
	peff::SharedPtr<E> lhs,
	peff::SharedPtr<E> rhs,
	peff::SharedPtr<R> exprOut) {
	GetData _getData;
	SetData _setData;

	switch (binaryOp) {
		case BinaryOp::Add:
			_setData(exprOut, _getData(lhs) + _getData(rhs));
			break;
		case BinaryOp::Sub:
			_setData(exprOut, _getData(lhs) - _getData(rhs));
			break;
		case BinaryOp::Mul:
			_setData(exprOut, _getData(lhs) * _getData(rhs));
			break;
		case BinaryOp::Div:
			_setData(exprOut, _getData(lhs) / _getData(rhs));
			break;
		default:
			std::terminate();
	}

	return {};
}

template <typename T, typename E, typename R, typename GetData = typename E::GetData, typename SetData = typename R::SetData>
peff::Option<CompilationError> _doSimpleBitwiseBinaryOp(
	CompileEnvironment *compileEnv,
	BinaryOp binaryOp,
	peff::SharedPtr<E> lhs,
	peff::SharedPtr<E> rhs,
	peff::SharedPtr<R> exprOut) {
	GetData _getData;
	SetData _setData;

	switch (binaryOp) {
		case BinaryOp::And:
			_setData(exprOut, _getData(lhs) & _getData(rhs));
			break;
		case BinaryOp::Or:
			_setData(exprOut, _getData(lhs) | _getData(rhs));
			break;
		case BinaryOp::Xor:
			_setData(exprOut, _getData(lhs) + _getData(rhs));
			break;
		default:
			std::terminate();
	}

	return {};
}

template <typename E, typename GetData = typename E::GetData>
peff::Option<CompilationError> _doSimpleComparisonBinaryOp(
	CompileEnvironment *compileEnv,
	BinaryOp binaryOp,
	peff::SharedPtr<E> lhs,
	peff::SharedPtr<E> rhs,
	AstNodePtr<ExprNode> &exprOut) {
	GetData _getData;

	switch (binaryOp) {
		case BinaryOp::Eq: {
			AstNodePtr<BoolLiteralExprNode> result;

			if (!(result = makeAstNode<BoolLiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, _getData(lhs) == _getData(rhs))))
				return genOutOfMemoryCompError();

			exprOut = result.template castTo<ExprNode>();
			break;
		}
		case BinaryOp::Neq: {
			AstNodePtr<BoolLiteralExprNode> result;

			if (!(result = makeAstNode<BoolLiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, _getData(lhs) != _getData(rhs))))
				return genOutOfMemoryCompError();

			exprOut = result.template castTo<ExprNode>();
			break;
		}
		case BinaryOp::StrictEq: {
			AstNodePtr<BoolLiteralExprNode> result;

			if (!(result = makeAstNode<BoolLiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, _getData(lhs) == _getData(rhs))))
				return genOutOfMemoryCompError();

			exprOut = result.template castTo<ExprNode>();
			break;
		}
		case BinaryOp::StrictNeq: {
			AstNodePtr<BoolLiteralExprNode> result;

			if (!(result = makeAstNode<BoolLiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, _getData(lhs) != _getData(rhs))))
				return genOutOfMemoryCompError();

			exprOut = result.template castTo<ExprNode>();
			break;
		}
		case BinaryOp::Lt: {
			AstNodePtr<BoolLiteralExprNode> result;

			if (!(result = makeAstNode<BoolLiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, _getData(lhs) < _getData(rhs))))
				return genOutOfMemoryCompError();

			exprOut = result.template castTo<ExprNode>();
			break;
		}
		case BinaryOp::Gt: {
			AstNodePtr<BoolLiteralExprNode> result;

			if (!(result = makeAstNode<BoolLiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, _getData(lhs) > _getData(rhs))))
				return genOutOfMemoryCompError();

			exprOut = result.template castTo<ExprNode>();
			break;
		}
		case BinaryOp::LtEq: {
			AstNodePtr<BoolLiteralExprNode> result;

			if (!(result = makeAstNode<BoolLiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, _getData(lhs) <= _getData(rhs))))
				return genOutOfMemoryCompError();

			exprOut = result.template castTo<ExprNode>();
			break;
		}
		case BinaryOp::GtEq: {
			AstNodePtr<BoolLiteralExprNode> result;

			if (!(result = makeAstNode<BoolLiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, _getData(lhs) >= _getData(rhs))))
				return genOutOfMemoryCompError();

			exprOut = result.template castTo<ExprNode>();
			break;
		}
		case BinaryOp::Cmp: {
			peff::SharedPtr<I32LiteralExprNode> result;

			auto l = _getData(lhs), r = _getData(rhs);

			int cmp;

			if (l < r)
				cmp = -1;
			else if (l > r)
				cmp = 1;
			else
				cmp = 0;

			if (!(result = makeAstNode<I32LiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, cmp)))
				return genOutOfMemoryCompError();

			exprOut = result.template castTo<ExprNode>();
			break;
		}
		default:
			std::terminate();
	}

	return {};
}

peff::Option<CompilationError> _castConstExpr(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	AstNodePtr<ExprNode> expr,
	AstNodePtr<TypeNameNode> type,
	AstNodePtr<ExprNode> &exprOut) {
	AstNodePtr<CastExprNode> castExpr;

	if (!(castExpr = makeAstNode<CastExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
		return genOutOfMemoryCompError();
	}

	castExpr->source = expr;
	castExpr->targetType = type;

	SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, compilationContext, castExpr.template castTo<ExprNode>(), exprOut));

	return {};
}

template <typename LT, ExprKind exprKind, typename TN>
static peff::Option<CompilationError> _evalSignedIntegralBinaryOp(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	AstNodePtr<BinaryExprNode> e,
	AstNodePtr<ExprNode> lhs,
	AstNodePtr<ExprNode> rhs,
	AstNodePtr<ExprNode> &exprOut) {
	peff::SharedPtr<LT> ll = lhs.template castTo<LT>();
	AstNodePtr<ExprNode> rl;

	switch (e->binaryOp) {
		case BinaryOp::Add:
		case BinaryOp::Sub:
		case BinaryOp::Mul:
		case BinaryOp::Div:
		case BinaryOp::Mod: {
			if (exprKind != rhs->exprKind) {
				peff::SharedPtr<TN> tn;

				if (!(tn = makeAstNode<TN>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
					return genOutOfMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
			} else
				rl = rhs;

			peff::SharedPtr<LT> result;

			if (!(result = makeAstNode<LT>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document->sharedFromThis(), 0))) {
				return genOutOfMemoryCompError();
			}

			SLKC_RETURN_IF_COMP_ERROR(_doSimpleArithmeticBinaryOp<LT, LT>(compileEnv, e->binaryOp, ll, rl.template castTo<LT>(), result));

			exprOut = result.template castTo<ExprNode>();
			break;
		}
		case BinaryOp::And:
		case BinaryOp::Or:
		case BinaryOp::Xor: {
			if (exprKind != rhs->exprKind) {
				peff::SharedPtr<TN> tn;

				if (!(tn = makeAstNode<TN>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
					return genOutOfMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
			} else
				rl = rhs;

			peff::SharedPtr<LT> result;

			if (!(result = makeAstNode<LT>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document->sharedFromThis(), 0))) {
				return genOutOfMemoryCompError();
			}

			SLKC_RETURN_IF_COMP_ERROR(_doSimpleBitwiseBinaryOp<LT, LT>(compileEnv, e->binaryOp, ll, rl.template castTo<LT>(), result));

			exprOut = result.template castTo<ExprNode>();
			break;
		}
		case BinaryOp::LAnd: {
			if (exprKind != rhs->exprKind) {
				peff::SharedPtr<TN> tn;

				if (!(tn = makeAstNode<TN>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
					return genOutOfMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
			} else
				rl = rhs;

			AstNodePtr<BoolLiteralExprNode> result;

			if (!(result = makeAstNode<BoolLiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, ll->data && rl.template castTo<I16LiteralExprNode>()->data))) {
				return genOutOfMemoryCompError();
			}

			exprOut = result.template castTo<ExprNode>();
			break;
		}
		case BinaryOp::LOr: {
			if (exprKind != rhs->exprKind) {
				peff::SharedPtr<TN> tn;

				if (!(tn = makeAstNode<TN>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
					return genOutOfMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
			} else
				rl = rhs;

			AstNodePtr<BoolLiteralExprNode> result;

			if (!(result = makeAstNode<BoolLiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, ll->data || rl.template castTo<I16LiteralExprNode>()->data))) {
				return genOutOfMemoryCompError();
			}

			exprOut = result.template castTo<ExprNode>();
			break;
		}
		case BinaryOp::Shl: {
			if (ExprKind::U32 != rhs->exprKind) {
				peff::SharedPtr<U32TypeNameNode> tn;

				if (!(tn = makeAstNode<U32TypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
					return genOutOfMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
			} else
				rl = rhs;

			AstNodePtr<LT> result;

			typedef decltype(ll->data) (*ShlFnType)(decltype(ll->data), uint32_t);

			uint32_t nBits = rl.template castTo<U32LiteralExprNode>()->data;
			if (!(result = makeAstNode<LT>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, (static_cast<ShlFnType>(slake::flib::shlSigned))(ll->data, nBits)))) {
				return genOutOfMemoryCompError();
			}

			exprOut = result.template castTo<ExprNode>();
			break;
		}
		case BinaryOp::Shr: {
			if (ExprKind::U32 != rhs->exprKind) {
				peff::SharedPtr<U32TypeNameNode> tn;

				if (!(tn = makeAstNode<U32TypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
					return genOutOfMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
			} else
				rl = rhs;

			AstNodePtr<LT> result;

			typedef decltype(ll->data) (*ShrFnType)(decltype(ll->data), uint32_t);

			uint32_t nBits = rl.template castTo<U32LiteralExprNode>()->data;
			if (!(result = makeAstNode<LT>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, (static_cast<ShrFnType>(slake::flib::shrSigned))(ll->data, nBits)))) {
				return genOutOfMemoryCompError();
			}

			exprOut = result.template castTo<ExprNode>();
			break;
		}
		case BinaryOp::Eq:
		case BinaryOp::Neq:
		case BinaryOp::StrictEq:
		case BinaryOp::StrictNeq:
		case BinaryOp::Lt:
		case BinaryOp::Gt:
		case BinaryOp::LtEq:
		case BinaryOp::GtEq:
		case BinaryOp::Cmp:
			if (exprKind != rhs->exprKind) {
				peff::SharedPtr<TN> tn;

				if (!(tn = makeAstNode<TN>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
					return genOutOfMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
			} else
				rl = rhs;

			SLKC_RETURN_IF_COMP_ERROR(_doSimpleComparisonBinaryOp<LT>(compileEnv, e->binaryOp, ll, rl.template castTo<LT>(), exprOut));
			break;
	}

	return {};
}

template <typename LT, ExprKind exprKind, typename TN>
static peff::Option<CompilationError> _evalUnsignedIntegralBinaryOp(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	AstNodePtr<BinaryExprNode> e,
	AstNodePtr<ExprNode> lhs,
	AstNodePtr<ExprNode> rhs,
	AstNodePtr<ExprNode> &exprOut) {
	peff::SharedPtr<LT> ll = lhs.template castTo<LT>();
	AstNodePtr<ExprNode> rl;

	switch (e->binaryOp) {
		case BinaryOp::Add:
		case BinaryOp::Sub:
		case BinaryOp::Mul:
		case BinaryOp::Div:
		case BinaryOp::Mod: {
			if (exprKind != rhs->exprKind) {
				peff::SharedPtr<TN> tn;

				if (!(tn = makeAstNode<TN>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
					return genOutOfMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
			} else
				rl = rhs;

			peff::SharedPtr<LT> result;

			if (!(result = makeAstNode<LT>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document->sharedFromThis(), 0))) {
				return genOutOfMemoryCompError();
			}

			SLKC_RETURN_IF_COMP_ERROR(_doSimpleArithmeticBinaryOp<LT, LT>(compileEnv, e->binaryOp, ll, rl.template castTo<LT>(), result));

			exprOut = result.template castTo<ExprNode>();
			break;
		}
		case BinaryOp::And:
		case BinaryOp::Or:
		case BinaryOp::Xor: {
			if (exprKind != rhs->exprKind) {
				peff::SharedPtr<TN> tn;

				if (!(tn = makeAstNode<TN>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
					return genOutOfMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
			} else
				rl = rhs;

			peff::SharedPtr<LT> result;

			if (!(result = makeAstNode<LT>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document->sharedFromThis(), 0))) {
				return genOutOfMemoryCompError();
			}

			SLKC_RETURN_IF_COMP_ERROR(_doSimpleBitwiseBinaryOp<LT, LT>(compileEnv, e->binaryOp, ll, rl.template castTo<LT>(), result));

			exprOut = result.template castTo<ExprNode>();
			break;
		}
		case BinaryOp::LAnd: {
			if (exprKind != rhs->exprKind) {
				peff::SharedPtr<TN> tn;

				if (!(tn = makeAstNode<TN>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
					return genOutOfMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
			} else
				rl = rhs;

			AstNodePtr<BoolLiteralExprNode> result;

			if (!(result = makeAstNode<BoolLiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, ll->data && rl.template castTo<I16LiteralExprNode>()->data))) {
				return genOutOfMemoryCompError();
			}

			exprOut = result.template castTo<ExprNode>();
			break;
		}
		case BinaryOp::LOr: {
			if (exprKind != rhs->exprKind) {
				peff::SharedPtr<TN> tn;

				if (!(tn = makeAstNode<TN>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
					return genOutOfMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
			} else
				rl = rhs;

			AstNodePtr<BoolLiteralExprNode> result;

			if (!(result = makeAstNode<BoolLiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, ll->data || rl.template castTo<I16LiteralExprNode>()->data))) {
				return genOutOfMemoryCompError();
			}

			exprOut = result.template castTo<ExprNode>();
			break;
		}
		case BinaryOp::Shl: {
			if (ExprKind::U32 != rhs->exprKind) {
				peff::SharedPtr<U32TypeNameNode> tn;

				if (!(tn = makeAstNode<U32TypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
					return genOutOfMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
			} else
				rl = rhs;

			AstNodePtr<LT> result;

			uint32_t nBits = rl.template castTo<U32LiteralExprNode>()->data;
			if (!(result = makeAstNode<LT>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, slake::flib::shlUnsigned(ll->data, nBits)))) {
				return genOutOfMemoryCompError();
			}

			exprOut = result.template castTo<ExprNode>();
			break;
		}
		case BinaryOp::Shr: {
			if (ExprKind::U32 != rhs->exprKind) {
				peff::SharedPtr<U32TypeNameNode> tn;

				if (!(tn = makeAstNode<U32TypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
					return genOutOfMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
			} else
				rl = rhs;

			AstNodePtr<LT> result;

			uint32_t nBits = rl.template castTo<U32LiteralExprNode>()->data;
			if (!(result = makeAstNode<LT>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, slake::flib::shrUnsigned(ll->data, nBits)))) {
				return genOutOfMemoryCompError();
			}

			exprOut = result.template castTo<ExprNode>();
			break;
		}
		case BinaryOp::Eq:
		case BinaryOp::Neq:
		case BinaryOp::StrictEq:
		case BinaryOp::StrictNeq:
		case BinaryOp::Lt:
		case BinaryOp::Gt:
		case BinaryOp::LtEq:
		case BinaryOp::GtEq:
		case BinaryOp::Cmp:
			if (exprKind != rhs->exprKind) {
				peff::SharedPtr<TN> tn;

				if (!(tn = makeAstNode<TN>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
					return genOutOfMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
			} else
				rl = rhs;

			SLKC_RETURN_IF_COMP_ERROR(_doSimpleComparisonBinaryOp<LT>(compileEnv, e->binaryOp, ll, rl.template castTo<LT>(), exprOut));
			break;
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::evalConstExpr(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	AstNodePtr<ExprNode> expr,
	AstNodePtr<ExprNode> &exprOut) {
	switch (expr->exprKind) {
		case ExprKind::Unary: {
			// stub
			exprOut = {};
			break;
		}
		case ExprKind::Binary: {
			AstNodePtr<BinaryExprNode> e = expr.template castTo<BinaryExprNode>();

			AstNodePtr<ExprNode> lhs, rhs;

			SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, compilationContext, e->lhs, lhs));
			SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, compilationContext, e->rhs, rhs));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			switch (lhs->exprKind) {
				case ExprKind::I8: {
					SLKC_RETURN_IF_COMP_ERROR(_evalSignedIntegralBinaryOp<I8LiteralExprNode, ExprKind::I8, I8TypeNameNode>(compileEnv, compilationContext, e, lhs, rhs, exprOut));
					break;
				}
				case ExprKind::I16: {
					SLKC_RETURN_IF_COMP_ERROR(_evalSignedIntegralBinaryOp<I16LiteralExprNode, ExprKind::I16, I16TypeNameNode>(compileEnv, compilationContext, e, lhs, rhs, exprOut));
					break;
				}
				case ExprKind::I32: {
					SLKC_RETURN_IF_COMP_ERROR(_evalSignedIntegralBinaryOp<I32LiteralExprNode, ExprKind::I32, I32TypeNameNode>(compileEnv, compilationContext, e, lhs, rhs, exprOut));
					break;
				}
				case ExprKind::I64: {
					SLKC_RETURN_IF_COMP_ERROR(_evalSignedIntegralBinaryOp<I64LiteralExprNode, ExprKind::I64, I64TypeNameNode>(compileEnv, compilationContext, e, lhs, rhs, exprOut));
					break;
				}
				case ExprKind::U8: {
					SLKC_RETURN_IF_COMP_ERROR(_evalUnsignedIntegralBinaryOp<U8LiteralExprNode, ExprKind::U8, U8TypeNameNode>(compileEnv, compilationContext, e, lhs, rhs, exprOut));
					break;
				}
				case ExprKind::U16: {
					SLKC_RETURN_IF_COMP_ERROR(_evalUnsignedIntegralBinaryOp<U16LiteralExprNode, ExprKind::U16, U16TypeNameNode>(compileEnv, compilationContext, e, lhs, rhs, exprOut));
					break;
				}
				case ExprKind::U32: {
					SLKC_RETURN_IF_COMP_ERROR(_evalUnsignedIntegralBinaryOp<U32LiteralExprNode, ExprKind::U32, U32TypeNameNode>(compileEnv, compilationContext, e, lhs, rhs, exprOut));
					break;
				}
				case ExprKind::U64: {
					SLKC_RETURN_IF_COMP_ERROR(_evalUnsignedIntegralBinaryOp<U64LiteralExprNode, ExprKind::U64, U64TypeNameNode>(compileEnv, compilationContext, e, lhs, rhs, exprOut));
					break;
				}
				case ExprKind::F32: {
					peff::SharedPtr<F32LiteralExprNode> ll = lhs.template castTo<F32LiteralExprNode>();
					AstNodePtr<ExprNode> rl;

					switch (e->binaryOp) {
						case BinaryOp::Add:
						case BinaryOp::Sub:
						case BinaryOp::Mul:
						case BinaryOp::Div: {
							if (ExprKind::F32 != rhs->exprKind) {
								peff::SharedPtr<F32TypeNameNode> tn;

								if (!(tn = makeAstNode<F32TypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<F32LiteralExprNode> result;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleFloatingPointArithmeticBinaryOp<F32LiteralExprNode, F32LiteralExprNode>(compileEnv, e->binaryOp, ll, rl.template castTo<F32LiteralExprNode>(), result));

							exprOut = result.template castTo<ExprNode>();
							break;
						}
						case BinaryOp::Mod: {
							if (ExprKind::F64 != rhs->exprKind) {
								peff::SharedPtr<F32TypeNameNode> tn;

								if (!(tn = makeAstNode<F32TypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<F32LiteralExprNode> result;

							if (!(result = makeAstNode<F32LiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, slake::flib::fmodf(ll->data, rl.template castTo<F64LiteralExprNode>()->data)))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.template castTo<ExprNode>();

							break;
						}
						case BinaryOp::LAnd: {
							if (ExprKind::F32 != rhs->exprKind) {
								peff::SharedPtr<F32TypeNameNode> tn;

								if (!(tn = makeAstNode<F32TypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							AstNodePtr<BoolLiteralExprNode> result;

							if (!(result = makeAstNode<BoolLiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, ll->data && rl.template castTo<F32LiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.template castTo<ExprNode>();
							break;
						}
						case BinaryOp::LOr: {
							if (ExprKind::F32 != rhs->exprKind) {
								peff::SharedPtr<F32TypeNameNode> tn;

								if (!(tn = makeAstNode<F32TypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							AstNodePtr<BoolLiteralExprNode> result;

							if (!(result = makeAstNode<BoolLiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, ll->data || rl.template castTo<F32LiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.template castTo<ExprNode>();
							break;
						}
						case BinaryOp::Eq:
						case BinaryOp::Neq:
						case BinaryOp::StrictEq:
						case BinaryOp::StrictNeq:
						case BinaryOp::Lt:
						case BinaryOp::Gt:
						case BinaryOp::LtEq:
						case BinaryOp::GtEq:
						case BinaryOp::Cmp:
							if (ExprKind::F32 != rhs->exprKind) {
								peff::SharedPtr<F32TypeNameNode> tn;

								if (!(tn = makeAstNode<F32TypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleComparisonBinaryOp<F32LiteralExprNode>(compileEnv, e->binaryOp, ll, rl.template castTo<F32LiteralExprNode>(), exprOut));
							break;
					}

					break;
				}
				case ExprKind::F64: {
					peff::SharedPtr<F64LiteralExprNode> ll = lhs.template castTo<F64LiteralExprNode>();
					AstNodePtr<ExprNode> rl;

					switch (e->binaryOp) {
						case BinaryOp::Add:
						case BinaryOp::Sub:
						case BinaryOp::Mul:
						case BinaryOp::Div: {
							if (ExprKind::F64 != rhs->exprKind) {
								peff::SharedPtr<F64TypeNameNode> tn;

								if (!(tn = makeAstNode<F64TypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<F64LiteralExprNode> result;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleFloatingPointArithmeticBinaryOp<F64LiteralExprNode, F64LiteralExprNode>(compileEnv, e->binaryOp, ll, rl.template castTo<F64LiteralExprNode>(), result));

							exprOut = result.template castTo<ExprNode>();
							break;
						}
						case BinaryOp::Mod: {
							if (ExprKind::F64 != rhs->exprKind) {
								peff::SharedPtr<F64TypeNameNode> tn;

								if (!(tn = makeAstNode<F64TypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<F64LiteralExprNode> result;

							if (!(result = makeAstNode<F64LiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, slake::flib::fmod(ll->data, rl.template castTo<F64LiteralExprNode>()->data)))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.template castTo<ExprNode>();

							break;
						}
						case BinaryOp::LAnd: {
							if (ExprKind::F64 != rhs->exprKind) {
								peff::SharedPtr<F64TypeNameNode> tn;

								if (!(tn = makeAstNode<F64TypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							AstNodePtr<BoolLiteralExprNode> result;

							if (!(result = makeAstNode<BoolLiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, ll->data && rl.template castTo<F64LiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.template castTo<ExprNode>();
							break;
						}
						case BinaryOp::LOr: {
							if (ExprKind::F64 != rhs->exprKind) {
								peff::SharedPtr<F64TypeNameNode> tn;

								if (!(tn = makeAstNode<F64TypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							AstNodePtr<BoolLiteralExprNode> result;

							if (!(result = makeAstNode<BoolLiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, ll->data || rl.template castTo<F64LiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.template castTo<ExprNode>();
							break;
						}
						case BinaryOp::Eq:
						case BinaryOp::Neq:
						case BinaryOp::StrictEq:
						case BinaryOp::StrictNeq:
						case BinaryOp::Lt:
						case BinaryOp::Gt:
						case BinaryOp::LtEq:
						case BinaryOp::GtEq:
						case BinaryOp::Cmp:
							if (ExprKind::F64 != rhs->exprKind) {
								peff::SharedPtr<F64TypeNameNode> tn;

								if (!(tn = makeAstNode<F64TypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleComparisonBinaryOp<F64LiteralExprNode>(compileEnv, e->binaryOp, ll, rl.template castTo<F64LiteralExprNode>(), exprOut));
							break;
					}

					break;
				}
				case ExprKind::Bool: {
					AstNodePtr<BoolLiteralExprNode> ll = lhs.template castTo<BoolLiteralExprNode>();
					AstNodePtr<ExprNode> rl;

					switch (e->binaryOp) {
						case BinaryOp::And:
						case BinaryOp::Or:
						case BinaryOp::Xor: {
							if (ExprKind::Bool != rhs->exprKind) {
								AstNodePtr<BoolTypeNameNode> tn;

								if (!(tn = makeAstNode<BoolTypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							AstNodePtr<BoolLiteralExprNode> result;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleBitwiseBinaryOp<BoolLiteralExprNode, BoolLiteralExprNode>(compileEnv, e->binaryOp, ll, rl.template castTo<BoolLiteralExprNode>(), result));

							exprOut = result.template castTo<ExprNode>();
							break;
						}
						case BinaryOp::LAnd: {
							if (ExprKind::Bool != rhs->exprKind) {
								AstNodePtr<BoolTypeNameNode> tn;

								if (!(tn = makeAstNode<BoolTypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							AstNodePtr<BoolLiteralExprNode> result;

							if (!(result = makeAstNode<BoolLiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, ll->data && rl.template castTo<BoolLiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.template castTo<ExprNode>();
							break;
						}
						case BinaryOp::LOr: {
							if (ExprKind::Bool != rhs->exprKind) {
								AstNodePtr<BoolTypeNameNode> tn;

								if (!(tn = makeAstNode<BoolTypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							AstNodePtr<BoolLiteralExprNode> result;

							if (!(result = makeAstNode<BoolLiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, ll->data || rl.template castTo<BoolLiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.template castTo<ExprNode>();
							break;
						}
						case BinaryOp::Eq:
						case BinaryOp::Neq:
						case BinaryOp::StrictEq:
						case BinaryOp::StrictNeq:
						case BinaryOp::Lt:
						case BinaryOp::Gt:
						case BinaryOp::LtEq:
						case BinaryOp::GtEq:
						case BinaryOp::Cmp:
							if (ExprKind::Bool != rhs->exprKind) {
								AstNodePtr<BoolTypeNameNode> tn;

								if (!(tn = makeAstNode<BoolTypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileEnv, compilationContext, rhs, tn.template castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleComparisonBinaryOp<BoolLiteralExprNode>(compileEnv, e->binaryOp, ll, rl.template castTo<BoolLiteralExprNode>(), exprOut));
							break;
					}
				}
				case ExprKind::Null: {
					// stub
					exprOut = {};
					return {};
				}
				default:
					exprOut = {};
					return {};
			}

			break;
		}
		case ExprKind::Ternary: {
			exprOut = {};
			break;
		}
		case ExprKind::IdRef: {
			exprOut = {};
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
			AstNodePtr<CastExprNode> e = expr.template castTo<CastExprNode>();
			AstNodePtr<ExprNode> src;
			SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, compilationContext, e->source, src));

			switch (e->targetType->typeNameKind) {
				case TypeNameKind::I8: {
					peff::SharedPtr<I8LiteralExprNode> l;

					if (!(l = makeAstNode<I8LiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, 0))) {
						return genOutOfMemoryCompError();
					}

					SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<int8_t, I8LiteralExprNode>(compileEnv, src, l));

					exprOut = l.template castTo<ExprNode>();
					break;
				}
				case TypeNameKind::I16: {
					peff::SharedPtr<I16LiteralExprNode> l;

					if (!(l = makeAstNode<I16LiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, 0))) {
						return genOutOfMemoryCompError();
					}

					SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<int16_t, I16LiteralExprNode>(compileEnv, src, l));

					exprOut = l.template castTo<ExprNode>();
					break;
				}
				case TypeNameKind::I32: {
					peff::SharedPtr<I32LiteralExprNode> l;

					if (!(l = makeAstNode<I32LiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, 0))) {
						return genOutOfMemoryCompError();
					}

					SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<int32_t, I32LiteralExprNode>(compileEnv, src, l));

					exprOut = l.template castTo<ExprNode>();
					break;
				}
				case TypeNameKind::I64: {
					peff::SharedPtr<I64LiteralExprNode> l;

					if (!(l = makeAstNode<I64LiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, 0))) {
						return genOutOfMemoryCompError();
					}

					SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<int64_t, I64LiteralExprNode>(compileEnv, src, l));

					exprOut = l.template castTo<ExprNode>();
					break;
				}
				case TypeNameKind::U8: {
					peff::SharedPtr<U8LiteralExprNode> l;

					if (!(l = makeAstNode<U8LiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, 0))) {
						return genOutOfMemoryCompError();
					}

					SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<int8_t, U8LiteralExprNode>(compileEnv, src, l));

					exprOut = l.template castTo<ExprNode>();
					break;
				}
				case TypeNameKind::U16: {
					peff::SharedPtr<U16LiteralExprNode> l;

					if (!(l = makeAstNode<U16LiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, 0))) {
						return genOutOfMemoryCompError();
					}

					SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<int16_t, U16LiteralExprNode>(compileEnv, src, l));

					exprOut = l.template castTo<ExprNode>();
					break;
				}
				case TypeNameKind::U32: {
					peff::SharedPtr<U32LiteralExprNode> l;

					if (!(l = makeAstNode<U32LiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, 0))) {
						return genOutOfMemoryCompError();
					}

					SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<int32_t, U32LiteralExprNode>(compileEnv, src, l));

					exprOut = l.template castTo<ExprNode>();
					break;
				}
				case TypeNameKind::U64: {
					peff::SharedPtr<U64LiteralExprNode> l;

					if (!(l = makeAstNode<U64LiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, 0))) {
						return genOutOfMemoryCompError();
					}

					SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<int64_t, U64LiteralExprNode>(compileEnv, src, l));

					exprOut = l.template castTo<ExprNode>();
					break;
				}
				case TypeNameKind::F32: {
					peff::SharedPtr<F32LiteralExprNode> l;

					if (!(l = makeAstNode<F32LiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, 0))) {
						return genOutOfMemoryCompError();
					}

					SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<float, F32LiteralExprNode>(compileEnv, src, l));

					exprOut = l.template castTo<ExprNode>();
					break;
				}
				case TypeNameKind::F64: {
					peff::SharedPtr<F64LiteralExprNode> l;

					if (!(l = makeAstNode<F64LiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, 0))) {
						return genOutOfMemoryCompError();
					}

					SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<double, F64LiteralExprNode>(compileEnv, src, l));

					exprOut = l.template castTo<ExprNode>();
					break;
				}
				case TypeNameKind::Bool: {
					AstNodePtr<BoolLiteralExprNode> l;

					if (!(l = makeAstNode<BoolLiteralExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, 0))) {
						return genOutOfMemoryCompError();
					}

					SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<double, BoolLiteralExprNode>(compileEnv, src, l));

					exprOut = l.template castTo<ExprNode>();
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
		case ExprKind::Wrapper: {
			SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, compilationContext, expr.template castTo<WrapperExprNode>()->target, exprOut));
			break;
		}
		case ExprKind::Bad: {
			exprOut = {};
			break;
		}
		default:
			std::terminate();
	}

	return {};
}
