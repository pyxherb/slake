#include "../../compiler.h"

using namespace slkc;

static std::optional<CompilationError> _compileSimpleRValueUnaryExpr(
	CompileContext *compileContext,
	CompilationContext *compilationContext,
	peff::SharedPtr<UnaryExprNode> expr,
	ExprEvalPurpose evalPurpose,
	peff::SharedPtr<TypeNameNode> desiredType,
	uint32_t resultRegOut,
	CompileExprResult &resultOut,
	slake::Opcode opcode) {
	switch (evalPurpose) {
		case ExprEvalPurpose::EvalType:
			break;
		case ExprEvalPurpose::Stmt:
			SLKC_RETURN_IF_COMP_ERROR(compileContext->pushWarning(
				CompilationWarning(expr->tokenRange, CompilationWarningKind::UnusedExprResult)));
			break;
		case ExprEvalPurpose::LValue:
			return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
		case ExprEvalPurpose::RValue: {
			CompileExprResult result(compileContext->allocator.get());

			uint32_t tmpReg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(tmpReg));

			SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, expr->operand, ExprEvalPurpose::RValue, desiredType, tmpReg, result));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				slake::Opcode::NEG,
				resultRegOut,
				{ slake::Value(slake::ValueType::RegRef, tmpReg) }));

			break;
		}
		case ExprEvalPurpose::Call:
			return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::compileUnaryExpr(
	CompileContext *compileContext,
	CompilationContext *compilationContext,
	peff::SharedPtr<UnaryExprNode> expr,
	ExprEvalPurpose evalPurpose,
	uint32_t resultRegOut,
	CompileExprResult &resultOut) {
	peff::SharedPtr<TypeNameNode> operandType, decayedOperandType;

	SLKC_RETURN_IF_COMP_ERROR(
		evalExprType(compileContext, compilationContext, expr->operand, operandType));
	SLKC_RETURN_IF_COMP_ERROR(
		removeRefOfType(operandType, decayedOperandType));

	switch (decayedOperandType->typeNameKind) {
		case TypeNameKind::I8:
		case TypeNameKind::I16:
		case TypeNameKind::I32:
		case TypeNameKind::I64:
		case TypeNameKind::U8:
		case TypeNameKind::U16:
		case TypeNameKind::U32:
		case TypeNameKind::U64: {
			switch (expr->unaryOp) {
				case UnaryOp::LNot:
					SLKC_RETURN_IF_COMP_ERROR(_compileSimpleRValueUnaryExpr(compileContext, compilationContext, expr, evalPurpose, decayedOperandType, resultRegOut, resultOut, slake::Opcode::LNOT));
					resultOut.evaluatedType = decayedOperandType;
					break;
				case UnaryOp::Not:
					SLKC_RETURN_IF_COMP_ERROR(_compileSimpleRValueUnaryExpr(compileContext, compilationContext, expr, evalPurpose, decayedOperandType, resultRegOut, resultOut, slake::Opcode::NOT));
					resultOut.evaluatedType = decayedOperandType;
					break;
				case UnaryOp::Neg:
					SLKC_RETURN_IF_COMP_ERROR(_compileSimpleRValueUnaryExpr(compileContext, compilationContext, expr, evalPurpose, decayedOperandType, resultRegOut, resultOut, slake::Opcode::NEG));
					resultOut.evaluatedType = decayedOperandType;
					break;
				default:
					return CompilationError(expr->tokenRange, CompilationErrorKind::OperatorNotFound);
			}
			break;
		}
		case TypeNameKind::F32:
		case TypeNameKind::F64: {
			switch (expr->unaryOp) {
				case UnaryOp::LNot:
					SLKC_RETURN_IF_COMP_ERROR(_compileSimpleRValueUnaryExpr(compileContext, compilationContext, expr, evalPurpose, decayedOperandType, resultRegOut, resultOut, slake::Opcode::LNOT));
					resultOut.evaluatedType = decayedOperandType;
					break;
				case UnaryOp::Neg:
					SLKC_RETURN_IF_COMP_ERROR(_compileSimpleRValueUnaryExpr(compileContext, compilationContext, expr, evalPurpose, decayedOperandType, resultRegOut, resultOut, slake::Opcode::NEG));
					resultOut.evaluatedType = decayedOperandType;
					break;
				default:
					return CompilationError(expr->tokenRange, CompilationErrorKind::OperatorNotFound);
			}
			break;
		}
		case TypeNameKind::Bool: {
			switch (expr->unaryOp) {
				case UnaryOp::LNot:
					SLKC_RETURN_IF_COMP_ERROR(_compileSimpleRValueUnaryExpr(compileContext, compilationContext, expr, evalPurpose, decayedOperandType, resultRegOut, resultOut, slake::Opcode::LNOT));
					resultOut.evaluatedType = decayedOperandType;
					break;
				default:
					return CompilationError(expr->tokenRange, CompilationErrorKind::OperatorNotFound);
			}
			break;
		}
		case TypeNameKind::Custom: {
		}
		default:
			return CompilationError(
				expr->tokenRange,
				CompilationErrorKind::OperatorNotFound);
	}

	return {};
}
