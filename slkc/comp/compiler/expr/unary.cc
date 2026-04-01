#include "../../compiler.h"

using namespace slkc;

static peff::Option<CompilationError> _compile_simple_rvalue_unary_expr(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<UnaryExprNode> expr,
	ExprEvalPurpose eval_purpose,
	AstNodePtr<TypeNameNode> desired_type,
	CompileExprResult &result_out,
	slake::Opcode opcode,
	uint32_t idx_sld) {
	switch (eval_purpose) {
		case ExprEvalPurpose::EvalType:
		case ExprEvalPurpose::EvalTypeActual:
			break;
		case ExprEvalPurpose::Stmt:
			SLKC_RETURN_IF_COMP_ERROR(compile_env->push_warning(
				CompilationWarning(expr->token_range, CompilationWarningKind::UnusedExprResult)));
			break;
		case ExprEvalPurpose::LValue:
			return CompilationError(expr->token_range, CompilationErrorKind::ExpectingLValueExpr);
		case ExprEvalPurpose::RValue: {
			CompileExprResult result(compile_env->allocator.get());

			uint32_t output_reg;

			SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(output_reg));

			SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, expr->operand, ExprEvalPurpose::RValue, desired_type, result));
			SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(
				idx_sld,
				slake::Opcode::NEG,
				output_reg,
				{ slake::Value(slake::ValueType::RegIndex, result.idx_result_reg_out) }));

			result.idx_result_reg_out = output_reg;

			break;
		}
		case ExprEvalPurpose::Call:
			return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotCallable);
		case ExprEvalPurpose::Unpacking:
			return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotUnpackable);
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compile_unary_expr(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<UnaryExprNode> expr,
	ExprEvalPurpose eval_purpose,
	CompileExprResult &result_out) {
	AstNodePtr<TypeNameNode> operand_type, decayed_operand_type;

	SLKC_RETURN_IF_COMP_ERROR(
		eval_expr_type(compile_env, compilation_context, path_env, expr->operand, operand_type));
	SLKC_RETURN_IF_COMP_ERROR(
		remove_ref_of_type(operand_type, decayed_operand_type));

	uint32_t sld_index;
	SLKC_RETURN_IF_COMP_ERROR(compilation_context->register_source_loc_desc(token_range_to_sld(expr->token_range), sld_index));

	switch (decayed_operand_type->tn_kind) {
		case TypeNameKind::I8:
		case TypeNameKind::I16:
		case TypeNameKind::I32:
		case TypeNameKind::I64:
		case TypeNameKind::U8:
		case TypeNameKind::U16:
		case TypeNameKind::U32:
		case TypeNameKind::U64: {
			switch (expr->unary_op) {
				case UnaryOp::LNot:
					SLKC_RETURN_IF_COMP_ERROR(_compile_simple_rvalue_unary_expr(compile_env, compilation_context, path_env, expr, eval_purpose, decayed_operand_type, result_out, slake::Opcode::LNOT, sld_index));
					result_out.evaluated_type = decayed_operand_type;
					break;
				case UnaryOp::Not:
					SLKC_RETURN_IF_COMP_ERROR(_compile_simple_rvalue_unary_expr(compile_env, compilation_context, path_env, expr, eval_purpose, decayed_operand_type, result_out, slake::Opcode::NOT, sld_index));
					result_out.evaluated_type = decayed_operand_type;
					break;
				case UnaryOp::Neg:
					SLKC_RETURN_IF_COMP_ERROR(_compile_simple_rvalue_unary_expr(compile_env, compilation_context, path_env, expr, eval_purpose, decayed_operand_type, result_out, slake::Opcode::NEG, sld_index));
					result_out.evaluated_type = decayed_operand_type;
					break;
				default:
					return CompilationError(expr->token_range, CompilationErrorKind::OperatorNotFound);
			}
			break;
		}
		case TypeNameKind::F32:
		case TypeNameKind::F64: {
			switch (expr->unary_op) {
				case UnaryOp::LNot:
					SLKC_RETURN_IF_COMP_ERROR(_compile_simple_rvalue_unary_expr(compile_env, compilation_context, path_env, expr, eval_purpose, decayed_operand_type, result_out, slake::Opcode::LNOT, sld_index));
					result_out.evaluated_type = decayed_operand_type;
					break;
				case UnaryOp::Neg:
					SLKC_RETURN_IF_COMP_ERROR(_compile_simple_rvalue_unary_expr(compile_env, compilation_context, path_env, expr, eval_purpose, decayed_operand_type, result_out, slake::Opcode::NEG, sld_index));
					result_out.evaluated_type = decayed_operand_type;
					break;
				default:
					return CompilationError(expr->token_range, CompilationErrorKind::OperatorNotFound);
			}
			break;
		}
		case TypeNameKind::Bool: {
			switch (expr->unary_op) {
				case UnaryOp::LNot:
					SLKC_RETURN_IF_COMP_ERROR(_compile_simple_rvalue_unary_expr(compile_env, compilation_context, path_env, expr, eval_purpose, decayed_operand_type, result_out, slake::Opcode::LNOT, sld_index));
					result_out.evaluated_type = decayed_operand_type;
					break;
				default:
					return CompilationError(expr->token_range, CompilationErrorKind::OperatorNotFound);
			}
			break;
		}
		case TypeNameKind::UnpackedParams: {
			switch (expr->unary_op) {
				case UnaryOp::Unpacking:
					switch (eval_purpose) {
						case ExprEvalPurpose::EvalType:
						case ExprEvalPurpose::EvalTypeActual: {
							AstNodePtr<TypeNameNode> unpacked_type;

							SLKC_RETURN_IF_COMP_ERROR(get_unpacked_type_of(decayed_operand_type, unpacked_type));

							if (!unpacked_type) {
								return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotUnpackable);
							}

							// TODO: I don't know what should I say, just keep it in mind.
							result_out.evaluated_type = unpacked_type;
							break;
						}
						case ExprEvalPurpose::Stmt:
							SLKC_RETURN_IF_COMP_ERROR(compile_env->push_warning(
								CompilationWarning(expr->token_range, CompilationWarningKind::UnusedExprResult)));
							break;
						case ExprEvalPurpose::LValue:
							return CompilationError(expr->token_range, CompilationErrorKind::ExpectingLValueExpr);
						case ExprEvalPurpose::RValue: {
							// The function argument compilation goes this way (?).
							CompileExprResult result(compile_env->allocator.get());

							SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, expr->operand, ExprEvalPurpose::Unpacking, {}, result));

							AstNodePtr<TypeNameNode> unpacked_type;

							SLKC_RETURN_IF_COMP_ERROR(get_unpacked_type_of(result.evaluated_type, unpacked_type));

							if (!unpacked_type) {
								return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotUnpackable);
							}

							result_out.evaluated_type = unpacked_type;

							break;
						}
						case ExprEvalPurpose::Unpacking: {
							CompileExprResult result(compile_env->allocator.get());

							uint32_t output_reg;

							SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(output_reg));

							SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, expr->operand, ExprEvalPurpose::Unpacking, {}, result));

							SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::APTOTUPLE, output_reg, { slake::Value(slake::ValueType::RegIndex, result.idx_result_reg_out) }));

							result_out.idx_result_reg_out = output_reg;

							// TODO: Convert the evaluated type to corresponding tuple type.
							result_out.evaluated_type = result.evaluated_type;

							break;
						}
						case ExprEvalPurpose::Call:
							return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotCallable);
					}
					break;
				default:
					return CompilationError(expr->token_range, CompilationErrorKind::OperatorNotFound);
			}
			break;
		}
		case TypeNameKind::Custom: {
		}
		default:
			return CompilationError(
				expr->token_range,
				CompilationErrorKind::OperatorNotFound);
	}

	return {};
}
