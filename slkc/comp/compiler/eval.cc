#include "../compiler.h"
#include <slake/flib/bitop.h>

using namespace slkc;

template <typename T, typename E, typename SetData = typename E::SetData>
std::optional<CompilationError> _doSimpleIntLiteralCast(
	CompileContext *compileContext,
	peff::SharedPtr<ExprNode> src,
	peff::SharedPtr<E> &exprOut) {
	static SetData _setData;

	switch (src->exprKind) {
		case ExprKind::I8: {
			peff::SharedPtr<I8LiteralExprNode> l = src.castTo<I8LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::I16: {
			peff::SharedPtr<I16LiteralExprNode> l = src.castTo<I16LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::I32: {
			peff::SharedPtr<I32LiteralExprNode> l = src.castTo<I32LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::I64: {
			peff::SharedPtr<I64LiteralExprNode> l = src.castTo<I64LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::U8: {
			peff::SharedPtr<U8LiteralExprNode> l = src.castTo<U8LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::U16: {
			peff::SharedPtr<U16LiteralExprNode> l = src.castTo<U16LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::U32: {
			peff::SharedPtr<U32LiteralExprNode> l = src.castTo<U32LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::U64: {
			peff::SharedPtr<U64LiteralExprNode> l = src.castTo<U64LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::F32: {
			peff::SharedPtr<F32LiteralExprNode> l = src.castTo<F32LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::F64: {
			peff::SharedPtr<F64LiteralExprNode> l = src.castTo<F64LiteralExprNode>();

			_setData(exprOut, (T)l->data);
			break;
		}
		case ExprKind::Bool: {
			peff::SharedPtr<BoolLiteralExprNode> l = src.castTo<BoolLiteralExprNode>();

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
std::optional<CompilationError> _doSimpleArithmeticBinaryOp(
	CompileContext *compileContext,
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
std::optional<CompilationError> _doSimpleFloatingPointArithmeticBinaryOp(
	CompileContext *compileContext,
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
std::optional<CompilationError> _doSimpleBitwiseBinaryOp(
	CompileContext *compileContext,
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
std::optional<CompilationError> _doSimpleComparisonBinaryOp(
	CompileContext *compileContext,
	BinaryOp binaryOp,
	peff::SharedPtr<E> lhs,
	peff::SharedPtr<E> rhs,
	peff::SharedPtr<ExprNode> &exprOut) {
	GetData _getData;

	switch (binaryOp) {
		case BinaryOp::Eq: {
			peff::SharedPtr<BoolLiteralExprNode> result;

			if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, _getData(lhs) == _getData(rhs))))
				return genOutOfMemoryCompError();

			exprOut = result.castTo<ExprNode>();
			break;
		}
		case BinaryOp::Neq: {
			peff::SharedPtr<BoolLiteralExprNode> result;

			if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, _getData(lhs) != _getData(rhs))))
				return genOutOfMemoryCompError();

			exprOut = result.castTo<ExprNode>();
			break;
		}
		case BinaryOp::StrictEq: {
			peff::SharedPtr<BoolLiteralExprNode> result;

			if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, _getData(lhs) == _getData(rhs))))
				return genOutOfMemoryCompError();

			exprOut = result.castTo<ExprNode>();
			break;
		}
		case BinaryOp::StrictNeq: {
			peff::SharedPtr<BoolLiteralExprNode> result;

			if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, _getData(lhs) != _getData(rhs))))
				return genOutOfMemoryCompError();

			exprOut = result.castTo<ExprNode>();
			break;
		}
		case BinaryOp::Lt: {
			peff::SharedPtr<BoolLiteralExprNode> result;

			if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, _getData(lhs) < _getData(rhs))))
				return genOutOfMemoryCompError();

			exprOut = result.castTo<ExprNode>();
			break;
		}
		case BinaryOp::Gt: {
			peff::SharedPtr<BoolLiteralExprNode> result;

			if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, _getData(lhs) > _getData(rhs))))
				return genOutOfMemoryCompError();

			exprOut = result.castTo<ExprNode>();
			break;
		}
		case BinaryOp::LtEq: {
			peff::SharedPtr<BoolLiteralExprNode> result;

			if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, _getData(lhs) <= _getData(rhs))))
				return genOutOfMemoryCompError();

			exprOut = result.castTo<ExprNode>();
			break;
		}
		case BinaryOp::GtEq: {
			peff::SharedPtr<BoolLiteralExprNode> result;

			if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, _getData(lhs) >= _getData(rhs))))
				return genOutOfMemoryCompError();

			exprOut = result.castTo<ExprNode>();
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

			if (!(result = peff::makeShared<I32LiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, cmp)))
				return genOutOfMemoryCompError();

			exprOut = result.castTo<ExprNode>();
			break;
		}
		default:
			std::terminate();
	}

	return {};
}

std::optional<CompilationError> _castConstExpr(
	CompileContext *compileContext,
	CompilationContext *compilationContext,
	peff::SharedPtr<ExprNode> expr,
	peff::SharedPtr<TypeNameNode> type,
	peff::SharedPtr<ExprNode> &exprOut) {
	peff::SharedPtr<CastExprNode> castExpr;

	if (!(castExpr = peff::makeShared<CastExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
		return genOutOfMemoryCompError();
	}

	castExpr->source = expr;
	castExpr->targetType = type;

	SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileContext, compilationContext, castExpr.castTo<ExprNode>(), exprOut));

	return {};
}

SLKC_API std::optional<CompilationError> slkc::evalConstExpr(
	CompileContext *compileContext,
	CompilationContext *compilationContext,
	peff::SharedPtr<ExprNode> expr,
	peff::SharedPtr<ExprNode> &exprOut) {
	switch (expr->exprKind) {
		case ExprKind::Unary: {
			// stub
			exprOut = {};
			break;
		}
		case ExprKind::Binary: {
			peff::SharedPtr<BinaryExprNode> e = expr.castTo<BinaryExprNode>();

			peff::SharedPtr<ExprNode> lhs, rhs;

			SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileContext, compilationContext, e->lhs, lhs));
			SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileContext, compilationContext, e->rhs, rhs));

			if ((!lhs) || (!rhs)) {
				exprOut = {};
				return {};
			}

			switch (lhs->exprKind) {
				case ExprKind::I8: {
					peff::SharedPtr<I8LiteralExprNode> ll = lhs.castTo<I8LiteralExprNode>();
					peff::SharedPtr<ExprNode> rl;

					switch (e->binaryOp) {
						case BinaryOp::Add:
						case BinaryOp::Sub:
						case BinaryOp::Mul:
						case BinaryOp::Div:
						case BinaryOp::Mod: {
							if (ExprKind::I8 != rhs->exprKind) {
								peff::SharedPtr<I8TypeNameNode> tn;

								if (!(tn = peff::makeShared<I8TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<I8LiteralExprNode> result;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleArithmeticBinaryOp<I8LiteralExprNode, I8LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<I8LiteralExprNode>(), result));

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::And:
						case BinaryOp::Or:
						case BinaryOp::Xor: {
							if (ExprKind::I8 != rhs->exprKind) {
								peff::SharedPtr<I8TypeNameNode> tn;

								if (!(tn = peff::makeShared<I8TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<I8LiteralExprNode> result;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleBitwiseBinaryOp<I8LiteralExprNode, I8LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<I8LiteralExprNode>(), result));

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::LAnd: {
							if (ExprKind::I8 != rhs->exprKind) {
								peff::SharedPtr<I8TypeNameNode> tn;

								if (!(tn = peff::makeShared<I8TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, ll->data && rl.castTo<I8LiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::LOr: {
							if (ExprKind::I8 != rhs->exprKind) {
								peff::SharedPtr<I8TypeNameNode> tn;

								if (!(tn = peff::makeShared<I8TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, ll->data || rl.castTo<I8LiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::Shl: {
							if (ExprKind::U32 != rhs->exprKind) {
								peff::SharedPtr<U32TypeNameNode> tn;

								if (!(tn = peff::makeShared<U32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							uint32_t nBits = rl.castTo<U32LiteralExprNode>()->data;
							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, slake::flib::shrSigned8(ll->data, nBits)))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::Shr: {
							if (ExprKind::U32 != rhs->exprKind) {
								peff::SharedPtr<U32TypeNameNode> tn;

								if (!(tn = peff::makeShared<U32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							uint32_t nBits = rl.castTo<U32LiteralExprNode>()->data;
							if (nBits >= 8) {
								if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, 0))) {
									return genOutOfMemoryCompError();
								}
							} else {
								if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, slake::flib::shrSigned8(ll->data, nBits)))) {
									return genOutOfMemoryCompError();
								}
							}

							exprOut = result.castTo<ExprNode>();
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
							if (ExprKind::I8 != rhs->exprKind) {
								peff::SharedPtr<I8TypeNameNode> tn;

								if (!(tn = peff::makeShared<I8TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleComparisonBinaryOp<I8LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<I8LiteralExprNode>(), exprOut));
							break;
					}

					break;
				}
				case ExprKind::I16: {
					peff::SharedPtr<I16LiteralExprNode> ll = lhs.castTo<I16LiteralExprNode>();
					peff::SharedPtr<ExprNode> rl;

					switch (e->binaryOp) {
						case BinaryOp::Add:
						case BinaryOp::Sub:
						case BinaryOp::Mul:
						case BinaryOp::Div:
						case BinaryOp::Mod: {
							if (ExprKind::I16 != rhs->exprKind) {
								peff::SharedPtr<I16TypeNameNode> tn;

								if (!(tn = peff::makeShared<I16TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<I16LiteralExprNode> result;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleArithmeticBinaryOp<I16LiteralExprNode, I16LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<I16LiteralExprNode>(), result));

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::And:
						case BinaryOp::Or:
						case BinaryOp::Xor: {
							if (ExprKind::I16 != rhs->exprKind) {
								peff::SharedPtr<I16TypeNameNode> tn;

								if (!(tn = peff::makeShared<I16TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<I16LiteralExprNode> result;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleBitwiseBinaryOp<I16LiteralExprNode, I16LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<I16LiteralExprNode>(), result));

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::LAnd: {
							if (ExprKind::I16 != rhs->exprKind) {
								peff::SharedPtr<I16TypeNameNode> tn;

								if (!(tn = peff::makeShared<I16TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, ll->data && rl.castTo<I16LiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::LOr: {
							if (ExprKind::I16 != rhs->exprKind) {
								peff::SharedPtr<I16TypeNameNode> tn;

								if (!(tn = peff::makeShared<I16TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, ll->data || rl.castTo<I16LiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::Shl: {
							if (ExprKind::U32 != rhs->exprKind) {
								peff::SharedPtr<U32TypeNameNode> tn;

								if (!(tn = peff::makeShared<U32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							uint32_t nBits = rl.castTo<U32LiteralExprNode>()->data;
							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, slake::flib::shrSigned8(ll->data, nBits)))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::Shr: {
							if (ExprKind::U32 != rhs->exprKind) {
								peff::SharedPtr<U32TypeNameNode> tn;

								if (!(tn = peff::makeShared<U32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							uint32_t nBits = rl.castTo<U32LiteralExprNode>()->data;
							if (nBits >= 8) {
								if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, 0))) {
									return genOutOfMemoryCompError();
								}
							} else {
								if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, slake::flib::shrSigned8(ll->data, nBits)))) {
									return genOutOfMemoryCompError();
								}
							}

							exprOut = result.castTo<ExprNode>();
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
							if (ExprKind::I16 != rhs->exprKind) {
								peff::SharedPtr<I16TypeNameNode> tn;

								if (!(tn = peff::makeShared<I16TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleComparisonBinaryOp<I16LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<I16LiteralExprNode>(), exprOut));
							break;
					}

					break;
				}
				case ExprKind::I32: {
					peff::SharedPtr<I32LiteralExprNode> ll = lhs.castTo<I32LiteralExprNode>();
					peff::SharedPtr<ExprNode> rl;

					switch (e->binaryOp) {
						case BinaryOp::Add:
						case BinaryOp::Sub:
						case BinaryOp::Mul:
						case BinaryOp::Div:
						case BinaryOp::Mod: {
							if (ExprKind::I32 != rhs->exprKind) {
								peff::SharedPtr<I32TypeNameNode> tn;

								if (!(tn = peff::makeShared<I32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<I32LiteralExprNode> result;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleArithmeticBinaryOp<I32LiteralExprNode, I32LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<I32LiteralExprNode>(), result));

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::And:
						case BinaryOp::Or:
						case BinaryOp::Xor: {
							if (ExprKind::I32 != rhs->exprKind) {
								peff::SharedPtr<I32TypeNameNode> tn;

								if (!(tn = peff::makeShared<I32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<I32LiteralExprNode> result;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleBitwiseBinaryOp<I32LiteralExprNode, I32LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<I32LiteralExprNode>(), result));

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::LAnd: {
							if (ExprKind::I32 != rhs->exprKind) {
								peff::SharedPtr<I32TypeNameNode> tn;

								if (!(tn = peff::makeShared<I32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, ll->data && rl.castTo<I32LiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::LOr: {
							if (ExprKind::I32 != rhs->exprKind) {
								peff::SharedPtr<I32TypeNameNode> tn;

								if (!(tn = peff::makeShared<I32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, ll->data || rl.castTo<I32LiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::Shl: {
							if (ExprKind::U32 != rhs->exprKind) {
								peff::SharedPtr<U32TypeNameNode> tn;

								if (!(tn = peff::makeShared<U32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							uint32_t nBits = rl.castTo<U32LiteralExprNode>()->data;
							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, slake::flib::shrSigned8(ll->data, nBits)))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::Shr: {
							if (ExprKind::U32 != rhs->exprKind) {
								peff::SharedPtr<U32TypeNameNode> tn;

								if (!(tn = peff::makeShared<U32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							uint32_t nBits = rl.castTo<U32LiteralExprNode>()->data;
							if (nBits >= 8) {
								if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, 0))) {
									return genOutOfMemoryCompError();
								}
							} else {
								if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, slake::flib::shrSigned8(ll->data, nBits)))) {
									return genOutOfMemoryCompError();
								}
							}

							exprOut = result.castTo<ExprNode>();
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
							if (ExprKind::I32 != rhs->exprKind) {
								peff::SharedPtr<I32TypeNameNode> tn;

								if (!(tn = peff::makeShared<I32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleComparisonBinaryOp<I32LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<I32LiteralExprNode>(), exprOut));
							break;
					}

					break;
				}
				case ExprKind::I64: {
					peff::SharedPtr<I64LiteralExprNode> ll = lhs.castTo<I64LiteralExprNode>();
					peff::SharedPtr<ExprNode> rl;

					switch (e->binaryOp) {
						case BinaryOp::Add:
						case BinaryOp::Sub:
						case BinaryOp::Mul:
						case BinaryOp::Div:
						case BinaryOp::Mod: {
							if (ExprKind::I64 != rhs->exprKind) {
								peff::SharedPtr<I64TypeNameNode> tn;

								if (!(tn = peff::makeShared<I64TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<I64LiteralExprNode> result;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleArithmeticBinaryOp<I64LiteralExprNode, I64LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<I64LiteralExprNode>(), result));

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::And:
						case BinaryOp::Or:
						case BinaryOp::Xor: {
							if (ExprKind::I64 != rhs->exprKind) {
								peff::SharedPtr<I64TypeNameNode> tn;

								if (!(tn = peff::makeShared<I64TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<I64LiteralExprNode> result;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleBitwiseBinaryOp<I64LiteralExprNode, I64LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<I64LiteralExprNode>(), result));

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::LAnd: {
							if (ExprKind::I64 != rhs->exprKind) {
								peff::SharedPtr<I64TypeNameNode> tn;

								if (!(tn = peff::makeShared<I64TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, ll->data && rl.castTo<I64LiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::LOr: {
							if (ExprKind::I64 != rhs->exprKind) {
								peff::SharedPtr<I64TypeNameNode> tn;

								if (!(tn = peff::makeShared<I64TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, ll->data || rl.castTo<I64LiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::Shl: {
							if (ExprKind::U32 != rhs->exprKind) {
								peff::SharedPtr<U32TypeNameNode> tn;

								if (!(tn = peff::makeShared<U32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							uint32_t nBits = rl.castTo<U32LiteralExprNode>()->data;
							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, slake::flib::shrSigned8(ll->data, nBits)))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::Shr: {
							if (ExprKind::U32 != rhs->exprKind) {
								peff::SharedPtr<U32TypeNameNode> tn;

								if (!(tn = peff::makeShared<U32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							uint32_t nBits = rl.castTo<U32LiteralExprNode>()->data;
							if (nBits >= 8) {
								if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, 0))) {
									return genOutOfMemoryCompError();
								}
							} else {
								if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, slake::flib::shrSigned8(ll->data, nBits)))) {
									return genOutOfMemoryCompError();
								}
							}

							exprOut = result.castTo<ExprNode>();
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
							if (ExprKind::I64 != rhs->exprKind) {
								peff::SharedPtr<I64TypeNameNode> tn;

								if (!(tn = peff::makeShared<I64TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleComparisonBinaryOp<I64LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<I64LiteralExprNode>(), exprOut));
							break;
					}

					break;
				}
				case ExprKind::U8: {
					peff::SharedPtr<U8LiteralExprNode> ll = lhs.castTo<U8LiteralExprNode>();
					peff::SharedPtr<ExprNode> rl;

					switch (e->binaryOp) {
						case BinaryOp::Add:
						case BinaryOp::Sub:
						case BinaryOp::Mul:
						case BinaryOp::Div:
						case BinaryOp::Mod: {
							if (ExprKind::U8 != rhs->exprKind) {
								peff::SharedPtr<U8TypeNameNode> tn;

								if (!(tn = peff::makeShared<U8TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<U8LiteralExprNode> result;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleArithmeticBinaryOp<U8LiteralExprNode, U8LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<U8LiteralExprNode>(), result));

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::And:
						case BinaryOp::Or:
						case BinaryOp::Xor: {
							if (ExprKind::U8 != rhs->exprKind) {
								peff::SharedPtr<U8TypeNameNode> tn;

								if (!(tn = peff::makeShared<U8TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<U8LiteralExprNode> result;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleBitwiseBinaryOp<U8LiteralExprNode, U8LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<U8LiteralExprNode>(), result));

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::LAnd: {
							if (ExprKind::U8 != rhs->exprKind) {
								peff::SharedPtr<U8TypeNameNode> tn;

								if (!(tn = peff::makeShared<U8TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, ll->data && rl.castTo<U8LiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::LOr: {
							if (ExprKind::U8 != rhs->exprKind) {
								peff::SharedPtr<U8TypeNameNode> tn;

								if (!(tn = peff::makeShared<U8TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, ll->data || rl.castTo<U8LiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::Shl: {
							if (ExprKind::U32 != rhs->exprKind) {
								peff::SharedPtr<U32TypeNameNode> tn;

								if (!(tn = peff::makeShared<U32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							uint32_t nBits = rl.castTo<U32LiteralExprNode>()->data;
							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, slake::flib::shrUnsigned8(ll->data, nBits)))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::Shr: {
							if (ExprKind::U32 != rhs->exprKind) {
								peff::SharedPtr<U32TypeNameNode> tn;

								if (!(tn = peff::makeShared<U32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							uint32_t nBits = rl.castTo<U32LiteralExprNode>()->data;
							if (nBits >= 8) {
								if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, 0))) {
									return genOutOfMemoryCompError();
								}
							} else {
								if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, slake::flib::shrUnsigned8(ll->data, nBits)))) {
									return genOutOfMemoryCompError();
								}
							}

							exprOut = result.castTo<ExprNode>();
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
							if (ExprKind::U8 != rhs->exprKind) {
								peff::SharedPtr<U8TypeNameNode> tn;

								if (!(tn = peff::makeShared<U8TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleComparisonBinaryOp<U8LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<U8LiteralExprNode>(), exprOut));
							break;
					}

					break;
				}
				case ExprKind::U16: {
					peff::SharedPtr<U16LiteralExprNode> ll = lhs.castTo<U16LiteralExprNode>();
					peff::SharedPtr<ExprNode> rl;

					switch (e->binaryOp) {
						case BinaryOp::Add:
						case BinaryOp::Sub:
						case BinaryOp::Mul:
						case BinaryOp::Div:
						case BinaryOp::Mod: {
							if (ExprKind::U16 != rhs->exprKind) {
								peff::SharedPtr<U16TypeNameNode> tn;

								if (!(tn = peff::makeShared<U16TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<U16LiteralExprNode> result;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleArithmeticBinaryOp<U16LiteralExprNode, U16LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<U16LiteralExprNode>(), result));

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::And:
						case BinaryOp::Or:
						case BinaryOp::Xor: {
							if (ExprKind::U16 != rhs->exprKind) {
								peff::SharedPtr<U16TypeNameNode> tn;

								if (!(tn = peff::makeShared<U16TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<U16LiteralExprNode> result;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleBitwiseBinaryOp<U16LiteralExprNode, U16LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<U16LiteralExprNode>(), result));

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::LAnd: {
							if (ExprKind::U16 != rhs->exprKind) {
								peff::SharedPtr<U16TypeNameNode> tn;

								if (!(tn = peff::makeShared<U16TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, ll->data && rl.castTo<U16LiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::LOr: {
							if (ExprKind::U16 != rhs->exprKind) {
								peff::SharedPtr<U16TypeNameNode> tn;

								if (!(tn = peff::makeShared<U16TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, ll->data || rl.castTo<U16LiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::Shl: {
							if (ExprKind::U32 != rhs->exprKind) {
								peff::SharedPtr<U32TypeNameNode> tn;

								if (!(tn = peff::makeShared<U32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							uint32_t nBits = rl.castTo<U32LiteralExprNode>()->data;
							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, slake::flib::shrUnsigned8(ll->data, nBits)))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::Shr: {
							if (ExprKind::U32 != rhs->exprKind) {
								peff::SharedPtr<U32TypeNameNode> tn;

								if (!(tn = peff::makeShared<U32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							uint32_t nBits = rl.castTo<U32LiteralExprNode>()->data;
							if (nBits >= 8) {
								if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, 0))) {
									return genOutOfMemoryCompError();
								}
							} else {
								if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, slake::flib::shrUnsigned8(ll->data, nBits)))) {
									return genOutOfMemoryCompError();
								}
							}

							exprOut = result.castTo<ExprNode>();
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
							if (ExprKind::U16 != rhs->exprKind) {
								peff::SharedPtr<U16TypeNameNode> tn;

								if (!(tn = peff::makeShared<U16TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleComparisonBinaryOp<U16LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<U16LiteralExprNode>(), exprOut));
							break;
					}

					break;
				}
				case ExprKind::U32: {
					peff::SharedPtr<U16LiteralExprNode> ll = lhs.castTo<U16LiteralExprNode>();
					peff::SharedPtr<ExprNode> rl;

					switch (e->binaryOp) {
						case BinaryOp::Add:
						case BinaryOp::Sub:
						case BinaryOp::Mul:
						case BinaryOp::Div:
						case BinaryOp::Mod: {
							if (ExprKind::U16 != rhs->exprKind) {
								peff::SharedPtr<U16TypeNameNode> tn;

								if (!(tn = peff::makeShared<U16TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<U16LiteralExprNode> result;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleArithmeticBinaryOp<U16LiteralExprNode, U16LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<U16LiteralExprNode>(), result));

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::And:
						case BinaryOp::Or:
						case BinaryOp::Xor: {
							if (ExprKind::U16 != rhs->exprKind) {
								peff::SharedPtr<U16TypeNameNode> tn;

								if (!(tn = peff::makeShared<U16TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<U16LiteralExprNode> result;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleBitwiseBinaryOp<U16LiteralExprNode, U16LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<U16LiteralExprNode>(), result));

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::LAnd: {
							if (ExprKind::U16 != rhs->exprKind) {
								peff::SharedPtr<U16TypeNameNode> tn;

								if (!(tn = peff::makeShared<U16TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, ll->data && rl.castTo<U16LiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::LOr: {
							if (ExprKind::U16 != rhs->exprKind) {
								peff::SharedPtr<U16TypeNameNode> tn;

								if (!(tn = peff::makeShared<U16TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, ll->data || rl.castTo<U16LiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::Shl: {
							if (ExprKind::U32 != rhs->exprKind) {
								peff::SharedPtr<U32TypeNameNode> tn;

								if (!(tn = peff::makeShared<U32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							uint32_t nBits = rl.castTo<U32LiteralExprNode>()->data;
							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, slake::flib::shrUnsigned8(ll->data, nBits)))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::Shr: {
							if (ExprKind::U32 != rhs->exprKind) {
								peff::SharedPtr<U32TypeNameNode> tn;

								if (!(tn = peff::makeShared<U32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							uint32_t nBits = rl.castTo<U32LiteralExprNode>()->data;
							if (nBits >= 8) {
								if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, 0))) {
									return genOutOfMemoryCompError();
								}
							} else {
								if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, slake::flib::shrUnsigned8(ll->data, nBits)))) {
									return genOutOfMemoryCompError();
								}
							}

							exprOut = result.castTo<ExprNode>();
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
							if (ExprKind::U16 != rhs->exprKind) {
								peff::SharedPtr<U16TypeNameNode> tn;

								if (!(tn = peff::makeShared<U16TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleComparisonBinaryOp<U16LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<U16LiteralExprNode>(), exprOut));
							break;
					}

					break;
				}
				case ExprKind::U64: {
					peff::SharedPtr<U64LiteralExprNode> ll = lhs.castTo<U64LiteralExprNode>();
					peff::SharedPtr<ExprNode> rl;

					switch (e->binaryOp) {
						case BinaryOp::Add:
						case BinaryOp::Sub:
						case BinaryOp::Mul:
						case BinaryOp::Div:
						case BinaryOp::Mod: {
							if (ExprKind::U64 != rhs->exprKind) {
								peff::SharedPtr<U64TypeNameNode> tn;

								if (!(tn = peff::makeShared<U64TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<U64LiteralExprNode> result;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleArithmeticBinaryOp<U64LiteralExprNode, U64LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<U64LiteralExprNode>(), result));

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::And:
						case BinaryOp::Or:
						case BinaryOp::Xor: {
							if (ExprKind::U64 != rhs->exprKind) {
								peff::SharedPtr<U64TypeNameNode> tn;

								if (!(tn = peff::makeShared<U64TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<U64LiteralExprNode> result;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleBitwiseBinaryOp<U64LiteralExprNode, U64LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<U64LiteralExprNode>(), result));

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::LAnd: {
							if (ExprKind::U64 != rhs->exprKind) {
								peff::SharedPtr<U64TypeNameNode> tn;

								if (!(tn = peff::makeShared<U64TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, ll->data && rl.castTo<U64LiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::LOr: {
							if (ExprKind::U64 != rhs->exprKind) {
								peff::SharedPtr<U64TypeNameNode> tn;

								if (!(tn = peff::makeShared<U64TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, ll->data || rl.castTo<U64LiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::Shl: {
							if (ExprKind::U32 != rhs->exprKind) {
								peff::SharedPtr<U32TypeNameNode> tn;

								if (!(tn = peff::makeShared<U32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							uint32_t nBits = rl.castTo<U32LiteralExprNode>()->data;
							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, slake::flib::shrUnsigned8(ll->data, nBits)))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::Shr: {
							if (ExprKind::U32 != rhs->exprKind) {
								peff::SharedPtr<U32TypeNameNode> tn;

								if (!(tn = peff::makeShared<U32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							uint32_t nBits = rl.castTo<U32LiteralExprNode>()->data;
							if (nBits >= 8) {
								if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, 0))) {
									return genOutOfMemoryCompError();
								}
							} else {
								if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, slake::flib::shrUnsigned8(ll->data, nBits)))) {
									return genOutOfMemoryCompError();
								}
							}

							exprOut = result.castTo<ExprNode>();
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
							if (ExprKind::U64 != rhs->exprKind) {
								peff::SharedPtr<U64TypeNameNode> tn;

								if (!(tn = peff::makeShared<U64TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleComparisonBinaryOp<U64LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<U64LiteralExprNode>(), exprOut));
							break;
					}

					break;
				}
				case ExprKind::F32: {
					peff::SharedPtr<F32LiteralExprNode> ll = lhs.castTo<F32LiteralExprNode>();
					peff::SharedPtr<ExprNode> rl;

					switch (e->binaryOp) {
						case BinaryOp::Add:
						case BinaryOp::Sub:
						case BinaryOp::Mul:
						case BinaryOp::Div: {
							if (ExprKind::F32 != rhs->exprKind) {
								peff::SharedPtr<F32TypeNameNode> tn;

								if (!(tn = peff::makeShared<F32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<F32LiteralExprNode> result;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleFloatingPointArithmeticBinaryOp<F32LiteralExprNode, F32LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<F32LiteralExprNode>(), result));

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::Mod: {
							if (ExprKind::F64 != rhs->exprKind) {
								peff::SharedPtr<F32TypeNameNode> tn;

								if (!(tn = peff::makeShared<F32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<F32LiteralExprNode> result;

							if (!(result = peff::makeShared<F32LiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, fmodf(ll->data, rl.castTo<F64LiteralExprNode>()->data)))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();

							break;
						}
						case BinaryOp::LAnd: {
							if (ExprKind::F32 != rhs->exprKind) {
								peff::SharedPtr<F32TypeNameNode> tn;

								if (!(tn = peff::makeShared<F32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, ll->data && rl.castTo<F32LiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::LOr: {
							if (ExprKind::F32 != rhs->exprKind) {
								peff::SharedPtr<F32TypeNameNode> tn;

								if (!(tn = peff::makeShared<F32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, ll->data || rl.castTo<F32LiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
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

								if (!(tn = peff::makeShared<F32TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleComparisonBinaryOp<F32LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<F32LiteralExprNode>(), exprOut));
							break;
					}

					break;
				}
				case ExprKind::F64: {
					peff::SharedPtr<F64LiteralExprNode> ll = lhs.castTo<F64LiteralExprNode>();
					peff::SharedPtr<ExprNode> rl;

					switch (e->binaryOp) {
						case BinaryOp::Add:
						case BinaryOp::Sub:
						case BinaryOp::Mul:
						case BinaryOp::Div: {
							if (ExprKind::F64 != rhs->exprKind) {
								peff::SharedPtr<F64TypeNameNode> tn;

								if (!(tn = peff::makeShared<F64TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<F64LiteralExprNode> result;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleFloatingPointArithmeticBinaryOp<F64LiteralExprNode, F64LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<F64LiteralExprNode>(), result));

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::Mod: {
							if (ExprKind::F64 != rhs->exprKind) {
								peff::SharedPtr<F64TypeNameNode> tn;

								if (!(tn = peff::makeShared<F64TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<F64LiteralExprNode> result;

							if (!(result = peff::makeShared<F64LiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, fmod(ll->data, rl.castTo<F64LiteralExprNode>()->data)))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();

							break;
						}
						case BinaryOp::LAnd: {
							if (ExprKind::F64 != rhs->exprKind) {
								peff::SharedPtr<F64TypeNameNode> tn;

								if (!(tn = peff::makeShared<F64TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, ll->data && rl.castTo<F64LiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::LOr: {
							if (ExprKind::F64 != rhs->exprKind) {
								peff::SharedPtr<F64TypeNameNode> tn;

								if (!(tn = peff::makeShared<F64TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, ll->data || rl.castTo<F64LiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
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

								if (!(tn = peff::makeShared<F64TypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleComparisonBinaryOp<F64LiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<F64LiteralExprNode>(), exprOut));
							break;
					}
				}
				case ExprKind::Bool: {
					peff::SharedPtr<BoolLiteralExprNode> ll = lhs.castTo<BoolLiteralExprNode>();
					peff::SharedPtr<ExprNode> rl;

					switch (e->binaryOp) {
						case BinaryOp::And:
						case BinaryOp::Or:
						case BinaryOp::Xor: {
							if (ExprKind::Bool != rhs->exprKind) {
								peff::SharedPtr<BoolTypeNameNode> tn;

								if (!(tn = peff::makeShared<BoolTypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleBitwiseBinaryOp<BoolLiteralExprNode, BoolLiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<BoolLiteralExprNode>(), result));

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::LAnd: {
							if (ExprKind::Bool != rhs->exprKind) {
								peff::SharedPtr<BoolTypeNameNode> tn;

								if (!(tn = peff::makeShared<BoolTypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, ll->data && rl.castTo<BoolLiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
							break;
						}
						case BinaryOp::LOr: {
							if (ExprKind::Bool != rhs->exprKind) {
								peff::SharedPtr<BoolTypeNameNode> tn;

								if (!(tn = peff::makeShared<BoolTypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							peff::SharedPtr<BoolLiteralExprNode> result;

							if (!(result = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, ll->data || rl.castTo<BoolLiteralExprNode>()->data))) {
								return genOutOfMemoryCompError();
							}

							exprOut = result.castTo<ExprNode>();
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
								peff::SharedPtr<BoolTypeNameNode> tn;

								if (!(tn = peff::makeShared<BoolTypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
									return genOutOfMemoryCompError();
								}

								SLKC_RETURN_IF_COMP_ERROR(_castConstExpr(compileContext, compilationContext, rhs, tn.castTo<TypeNameNode>(), rl));
							} else
								rl = rhs;

							SLKC_RETURN_IF_COMP_ERROR(_doSimpleComparisonBinaryOp<BoolLiteralExprNode>(compileContext, e->binaryOp, ll, rl.castTo<BoolLiteralExprNode>(), exprOut));
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
			peff::SharedPtr<CastExprNode> e = expr.castTo<CastExprNode>();
			peff::SharedPtr<ExprNode> src;
			SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileContext, compilationContext, e->source, src));

			switch (e->targetType->typeNameKind) {
				case TypeNameKind::I8: {
					peff::SharedPtr<I8LiteralExprNode> l;

					if (!(l = peff::makeShared<I8LiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, 0))) {
						return genOutOfMemoryCompError();
					}

					SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<int8_t, I8LiteralExprNode>(compileContext, src, l));

					exprOut = l.castTo<ExprNode>();
					break;
				}
				case TypeNameKind::I16: {
					peff::SharedPtr<I16LiteralExprNode> l;

					if (!(l = peff::makeShared<I16LiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, 0))) {
						return genOutOfMemoryCompError();
					}

					SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<int16_t, I16LiteralExprNode>(compileContext, src, l));

					exprOut = l.castTo<ExprNode>();
					break;
				}
				case TypeNameKind::I32: {
					peff::SharedPtr<I32LiteralExprNode> l;

					if (!(l = peff::makeShared<I32LiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, 0))) {
						return genOutOfMemoryCompError();
					}

					SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<int32_t, I32LiteralExprNode>(compileContext, src, l));

					exprOut = l.castTo<ExprNode>();
					break;
				}
				case TypeNameKind::I64: {
					peff::SharedPtr<I64LiteralExprNode> l;

					if (!(l = peff::makeShared<I64LiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, 0))) {
						return genOutOfMemoryCompError();
					}

					SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<int64_t, I64LiteralExprNode>(compileContext, src, l));

					exprOut = l.castTo<ExprNode>();
					break;
				}
				case TypeNameKind::U8: {
					peff::SharedPtr<U8LiteralExprNode> l;

					if (!(l = peff::makeShared<U8LiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, 0))) {
						return genOutOfMemoryCompError();
					}

					SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<int8_t, U8LiteralExprNode>(compileContext, src, l));

					exprOut = l.castTo<ExprNode>();
					break;
				}
				case TypeNameKind::U16: {
					peff::SharedPtr<U16LiteralExprNode> l;

					if (!(l = peff::makeShared<U16LiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, 0))) {
						return genOutOfMemoryCompError();
					}

					SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<int16_t, U16LiteralExprNode>(compileContext, src, l));

					exprOut = l.castTo<ExprNode>();
					break;
				}
				case TypeNameKind::U32: {
					peff::SharedPtr<U32LiteralExprNode> l;

					if (!(l = peff::makeShared<U32LiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, 0))) {
						return genOutOfMemoryCompError();
					}

					SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<int32_t, U32LiteralExprNode>(compileContext, src, l));

					exprOut = l.castTo<ExprNode>();
					break;
				}
				case TypeNameKind::U64: {
					peff::SharedPtr<U64LiteralExprNode> l;

					if (!(l = peff::makeShared<U64LiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, 0))) {
						return genOutOfMemoryCompError();
					}

					SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<int64_t, U64LiteralExprNode>(compileContext, src, l));

					exprOut = l.castTo<ExprNode>();
					break;
				}
				case TypeNameKind::F32: {
					peff::SharedPtr<F32LiteralExprNode> l;

					if (!(l = peff::makeShared<F32LiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, 0))) {
						return genOutOfMemoryCompError();
					}

					SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<float, F32LiteralExprNode>(compileContext, src, l));

					exprOut = l.castTo<ExprNode>();
					break;
				}
				case TypeNameKind::F64: {
					peff::SharedPtr<F64LiteralExprNode> l;

					if (!(l = peff::makeShared<F64LiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, 0))) {
						return genOutOfMemoryCompError();
					}

					SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<double, F64LiteralExprNode>(compileContext, src, l));

					exprOut = l.castTo<ExprNode>();
					break;
				}
				case TypeNameKind::Bool: {
					peff::SharedPtr<BoolLiteralExprNode> l;

					if (!(l = peff::makeShared<BoolLiteralExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, 0))) {
						return genOutOfMemoryCompError();
					}

					SLKC_RETURN_IF_COMP_ERROR(_doSimpleIntLiteralCast<double, BoolLiteralExprNode>(compileContext, src, l));

					exprOut = l.castTo<ExprNode>();
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
			SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileContext, compilationContext, expr.castTo<WrapperExprNode>()->target, exprOut));
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
