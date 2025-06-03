#include "../compiler.h"

using namespace slkc;

template<typename T, typename E, typename SetData = typename E::SetData>
std::optional<CompilationError> _doSimpleIntLiteralCast(
	CompileContext* compileContext,
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

SLKC_API std::optional<CompilationError> slkc::evalConstExpr(
	CompileContext* compileContext,
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
			// stub
			exprOut = {};
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
		case ExprKind::VarArg: {
			exprOut = {};
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
