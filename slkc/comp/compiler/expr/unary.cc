#include "../../compiler.h"

using namespace slkc;

static peff::Option<CompilationError> _compileSimpleRValueUnaryExpr(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	AstNodePtr<UnaryExprNode> expr,
	ExprEvalPurpose evalPurpose,
	AstNodePtr<TypeNameNode> desiredType,
	uint32_t resultRegOut,
	CompileExprResult &resultOut,
	slake::Opcode opcode,
	uint32_t idxSld) {
	switch (evalPurpose) {
		case ExprEvalPurpose::EvalType:
			break;
		case ExprEvalPurpose::Stmt:
			SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushWarning(
				CompilationWarning(expr->tokenRange, CompilationWarningKind::UnusedExprResult)));
			break;
		case ExprEvalPurpose::LValue:
			return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
		case ExprEvalPurpose::RValue: {
			CompileExprResult result(compileEnv->allocator.get());

			uint32_t tmpReg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(tmpReg));

			SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, expr->operand, ExprEvalPurpose::RValue, desiredType, tmpReg, result));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				idxSld,
				slake::Opcode::NEG,
				resultRegOut,
				{ slake::Value(slake::ValueType::RegIndex, tmpReg) }));

			break;
		}
		case ExprEvalPurpose::Call:
			return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
		case ExprEvalPurpose::Unpacking:
			return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotUnpackable);
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compileUnaryExpr(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	AstNodePtr<UnaryExprNode> expr,
	ExprEvalPurpose evalPurpose,
	uint32_t resultRegOut,
	CompileExprResult &resultOut) {
	AstNodePtr<TypeNameNode> operandType, decayedOperandType;

	SLKC_RETURN_IF_COMP_ERROR(
		evalExprType(compileEnv, compilationContext, expr->operand, operandType));
	SLKC_RETURN_IF_COMP_ERROR(
		removeRefOfType(operandType, decayedOperandType));

	uint32_t sldIndex;
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->registerSourceLocDesc(tokenRangeToSld(expr->tokenRange), sldIndex));

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
					SLKC_RETURN_IF_COMP_ERROR(_compileSimpleRValueUnaryExpr(compileEnv, compilationContext, expr, evalPurpose, decayedOperandType, resultRegOut, resultOut, slake::Opcode::LNOT, sldIndex));
					resultOut.evaluatedType = decayedOperandType;
					break;
				case UnaryOp::Not:
					SLKC_RETURN_IF_COMP_ERROR(_compileSimpleRValueUnaryExpr(compileEnv, compilationContext, expr, evalPurpose, decayedOperandType, resultRegOut, resultOut, slake::Opcode::NOT, sldIndex));
					resultOut.evaluatedType = decayedOperandType;
					break;
				case UnaryOp::Neg:
					SLKC_RETURN_IF_COMP_ERROR(_compileSimpleRValueUnaryExpr(compileEnv, compilationContext, expr, evalPurpose, decayedOperandType, resultRegOut, resultOut, slake::Opcode::NEG, sldIndex));
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
					SLKC_RETURN_IF_COMP_ERROR(_compileSimpleRValueUnaryExpr(compileEnv, compilationContext, expr, evalPurpose, decayedOperandType, resultRegOut, resultOut, slake::Opcode::LNOT, sldIndex));
					resultOut.evaluatedType = decayedOperandType;
					break;
				case UnaryOp::Neg:
					SLKC_RETURN_IF_COMP_ERROR(_compileSimpleRValueUnaryExpr(compileEnv, compilationContext, expr, evalPurpose, decayedOperandType, resultRegOut, resultOut, slake::Opcode::NEG, sldIndex));
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
					SLKC_RETURN_IF_COMP_ERROR(_compileSimpleRValueUnaryExpr(compileEnv, compilationContext, expr, evalPurpose, decayedOperandType, resultRegOut, resultOut, slake::Opcode::LNOT, sldIndex));
					resultOut.evaluatedType = decayedOperandType;
					break;
				default:
					return CompilationError(expr->tokenRange, CompilationErrorKind::OperatorNotFound);
			}
			break;
		}
		case TypeNameKind::UnpackedParams: {
			switch (expr->unaryOp) {
				case UnaryOp::Unpacking:
					switch (evalPurpose) {
						case ExprEvalPurpose::EvalType: {
							AstNodePtr<TypeNameNode> unpackedType;

							SLKC_RETURN_IF_COMP_ERROR(getUnpackedTypeOf(decayedOperandType, unpackedType));

							if (!unpackedType) {
								return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotUnpackable);
							}

							// TODO: I don't know what should I say, just keep it in mind.
							resultOut.evaluatedType = unpackedType;
							break;
						}
						case ExprEvalPurpose::Stmt:
							SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushWarning(
								CompilationWarning(expr->tokenRange, CompilationWarningKind::UnusedExprResult)));
							break;
						case ExprEvalPurpose::LValue:
							return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
						case ExprEvalPurpose::RValue: {
							// The function argument compilation goes this way (?).
							CompileExprResult result(compileEnv->allocator.get());

							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, expr->operand, ExprEvalPurpose::Unpacking, {}, resultRegOut, result));

							AstNodePtr<TypeNameNode> unpackedType;

							SLKC_RETURN_IF_COMP_ERROR(getUnpackedTypeOf(result.evaluatedType, unpackedType));

							if (!unpackedType) {
								return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotUnpackable);
							}

							resultOut.evaluatedType = unpackedType;

							break;
						}
						case ExprEvalPurpose::Unpacking: {
							CompileExprResult result(compileEnv->allocator.get());

							uint32_t tmpReg;

							SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(tmpReg));

							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, expr->operand, ExprEvalPurpose::Unpacking, {}, tmpReg, result));

							SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(sldIndex, slake::Opcode::APTOTUPLE, resultRegOut, { slake::Value(tmpReg) }));

							// TODO: Convert the evaluated type to corresponding tuple type.
							resultOut.evaluatedType = result.evaluatedType;

							break;
						}
						case ExprEvalPurpose::Call:
							return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
					}
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
