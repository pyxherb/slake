#include "../../compiler.h"

using namespace slkc;

static peff::Option<CompilationError> _compileSimpleRValueUnaryExpr(
	CompileEnv *compileEnv,
	CompilationContext *compilationContext,
	PathEnv *pathEnv,
	AstNodePtr<UnaryExprNode> expr,
	ExprEvalPurpose evalPurpose,
	AstNodePtr<TypeNameNode> desiredType,
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

			uint32_t outputReg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(outputReg));

			SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, pathEnv, expr->operand, ExprEvalPurpose::RValue, desiredType, result));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				idxSld,
				slake::Opcode::NEG,
				outputReg,
				{ slake::Value(slake::ValueType::RegIndex, result.idxResultRegOut) }));

			result.idxResultRegOut = outputReg;

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
	CompileEnv *compileEnv,
	CompilationContext *compilationContext,
	PathEnv *pathEnv,
	AstNodePtr<UnaryExprNode> expr,
	ExprEvalPurpose evalPurpose,
	CompileExprResult &resultOut) {
	AstNodePtr<TypeNameNode> operandType, decayedOperandType;

	SLKC_RETURN_IF_COMP_ERROR(
		evalExprType(compileEnv, compilationContext, pathEnv, expr->operand, operandType));
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
					SLKC_RETURN_IF_COMP_ERROR(_compileSimpleRValueUnaryExpr(compileEnv, compilationContext, pathEnv, expr, evalPurpose, decayedOperandType, resultOut, slake::Opcode::LNOT, sldIndex));
					resultOut.evaluatedType = decayedOperandType;
					break;
				case UnaryOp::Not:
					SLKC_RETURN_IF_COMP_ERROR(_compileSimpleRValueUnaryExpr(compileEnv, compilationContext, pathEnv, expr, evalPurpose, decayedOperandType, resultOut, slake::Opcode::NOT, sldIndex));
					resultOut.evaluatedType = decayedOperandType;
					break;
				case UnaryOp::Neg:
					SLKC_RETURN_IF_COMP_ERROR(_compileSimpleRValueUnaryExpr(compileEnv, compilationContext, pathEnv, expr, evalPurpose, decayedOperandType, resultOut, slake::Opcode::NEG, sldIndex));
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
					SLKC_RETURN_IF_COMP_ERROR(_compileSimpleRValueUnaryExpr(compileEnv, compilationContext, pathEnv, expr, evalPurpose, decayedOperandType, resultOut, slake::Opcode::LNOT, sldIndex));
					resultOut.evaluatedType = decayedOperandType;
					break;
				case UnaryOp::Neg:
					SLKC_RETURN_IF_COMP_ERROR(_compileSimpleRValueUnaryExpr(compileEnv, compilationContext, pathEnv, expr, evalPurpose, decayedOperandType, resultOut, slake::Opcode::NEG, sldIndex));
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
					SLKC_RETURN_IF_COMP_ERROR(_compileSimpleRValueUnaryExpr(compileEnv, compilationContext, pathEnv, expr, evalPurpose, decayedOperandType, resultOut, slake::Opcode::LNOT, sldIndex));
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

							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, pathEnv, expr->operand, ExprEvalPurpose::Unpacking, {}, result));

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

							uint32_t outputReg;

							SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(outputReg));

							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, pathEnv, expr->operand, ExprEvalPurpose::Unpacking, {}, result));

							SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(sldIndex, slake::Opcode::APTOTUPLE, outputReg, { slake::Value(slake::ValueType::RegIndex, result.idxResultRegOut) }));

							resultOut.idxResultRegOut = outputReg;

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
