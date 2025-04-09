#include "../compiler.h"

using namespace slkc;

template <typename LE, typename DT, typename TN>
SLAKE_FORCEINLINE std::optional<CompilationError> _compileLiteralExpr(
	FnCompileContext &compileContext,
	const peff::SharedPtr<ExprNode> &expr,
	ExprEvalPurpose evalPurpose,
	uint32_t resultRegOut,
	peff::SharedPtr<TypeNameNode> &evaluatedTypeOut,
	bool evalTypeOnly) {
	peff::SharedPtr<LE> e = expr.castTo<LE>();

	switch (evalPurpose) {
		case ExprEvalPurpose::Stmt:
			SLKC_RETURN_IF_COMP_ERROR(compileContext.pushWarning(
				CompilationWarning(e->tokenRange, CompilationWarningKind::UnusedExprResult)));
			break;
		case ExprEvalPurpose::RValue:
			if (!evalTypeOnly) {
				if (resultRegOut != UINT32_MAX) {
					SLKC_RETURN_IF_COMP_ERROR(
						compileContext.pushIns(
							emitIns(
								slake::Opcode::MOV,
								resultRegOut,
								{ slake::Value((DT)e->data) })));
				}
			}
			break;
		case ExprEvalPurpose::LValue:
			return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
	}

	if (!(evaluatedTypeOut = peff::makeShared<TN>(
			  compileContext.allocator.get(),
			  compileContext.allocator.get(),
			  compileContext.document)
				.castTo<TypeNameNode>())) {
		return genOutOfMemoryCompError();
	}
}

std::optional<CompilationError> Compiler::compileExpr(
	FnCompileContext &compileContext,
	const peff::SharedPtr<ExprNode> &expr,
	ExprEvalPurpose evalPurpose,
	uint32_t resultRegOut,
	peff::SharedPtr<TypeNameNode> &evaluatedTypeOut,
	bool evalTypeOnly) {
	switch (expr->exprKind) {
		case ExprKind::Unary: {
		}
		case ExprKind::I8:
			_compileLiteralExpr<I8LiteralExprNode, int8_t, I8TypeNameNode>(compileContext, expr, evalPurpose, resultRegOut, evaluatedTypeOut, evalTypeOnly);
			break;
		case ExprKind::I16:
			_compileLiteralExpr<I16LiteralExprNode, int16_t, I16TypeNameNode>(compileContext, expr, evalPurpose, resultRegOut, evaluatedTypeOut, evalTypeOnly);
			break;
		case ExprKind::I32:
			_compileLiteralExpr<I32LiteralExprNode, int32_t, I32TypeNameNode>(compileContext, expr, evalPurpose, resultRegOut, evaluatedTypeOut, evalTypeOnly);
			break;
		case ExprKind::I64:
			_compileLiteralExpr<I64LiteralExprNode, int64_t, I64TypeNameNode>(compileContext, expr, evalPurpose, resultRegOut, evaluatedTypeOut, evalTypeOnly);
			break;
		case ExprKind::U8:
			_compileLiteralExpr<U8LiteralExprNode, uint8_t, U8TypeNameNode>(compileContext, expr, evalPurpose, resultRegOut, evaluatedTypeOut, evalTypeOnly);
			break;
		case ExprKind::U16:
			_compileLiteralExpr<U16LiteralExprNode, uint16_t, U16TypeNameNode>(compileContext, expr, evalPurpose, resultRegOut, evaluatedTypeOut, evalTypeOnly);
			break;
		case ExprKind::U32:
			_compileLiteralExpr<U32LiteralExprNode, uint32_t, U32TypeNameNode>(compileContext, expr, evalPurpose, resultRegOut, evaluatedTypeOut, evalTypeOnly);
			break;
		case ExprKind::U64:
			_compileLiteralExpr<U64LiteralExprNode, uint64_t, U64TypeNameNode>(compileContext, expr, evalPurpose, resultRegOut, evaluatedTypeOut, evalTypeOnly);
			break;
		case ExprKind::F32:
			_compileLiteralExpr<F32LiteralExprNode, float, F32TypeNameNode>(compileContext, expr, evalPurpose, resultRegOut, evaluatedTypeOut, evalTypeOnly);
			break;
		case ExprKind::F64:
			_compileLiteralExpr<F64LiteralExprNode, double, F64TypeNameNode>(compileContext, expr, evalPurpose, resultRegOut, evaluatedTypeOut, evalTypeOnly);
			break;
		case ExprKind::Bool:
			_compileLiteralExpr<BoolLiteralExprNode, bool, BoolTypeNameNode>(compileContext, expr, evalPurpose, resultRegOut, evaluatedTypeOut, evalTypeOnly);
			break;
		case ExprKind::Null: {
			peff::SharedPtr<NullLiteralExprNode> e = expr.castTo<NullLiteralExprNode>();

			switch (evalPurpose) {
				case ExprEvalPurpose::Stmt:
					SLKC_RETURN_IF_COMP_ERROR(compileContext.pushWarning(
						CompilationWarning(e->tokenRange, CompilationWarningKind::UnusedExprResult)));
					break;
				case ExprEvalPurpose::RValue:
					if (!evalTypeOnly) {
						if (resultRegOut != UINT32_MAX) {
							SLKC_RETURN_IF_COMP_ERROR(
								compileContext.pushIns(
									emitIns(
										slake::Opcode::MOV,
										resultRegOut,
										{ slake::Value(nullptr) })));
						}
					}
					break;
				case ExprEvalPurpose::LValue:
					return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
			}

			if (!(evaluatedTypeOut = peff::makeShared<ObjectTypeNameNode>(
					  compileContext.allocator.get(),
					  compileContext.allocator.get(),
					  compileContext.document)
						.castTo<TypeNameNode>())) {
				return genOutOfMemoryCompError();
			}
			break;
		}
	}
}
