#include "../../compiler.h"

using namespace slkc;

[[nodiscard]] static peff::Option<CompilationError> _compile_simple_binary_expr(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<BinaryExprNode> expr,
	ExprEvalPurpose eval_purpose,
	AstNodePtr<TypeNameNode> lhs_type,
	AstNodePtr<TypeNameNode> desired_lhs_type,
	ExprEvalPurpose lhs_eval_purpose,
	AstNodePtr<TypeNameNode> rhs_type,
	AstNodePtr<TypeNameNode> desired_rhs_type,
	ExprEvalPurpose rhs_eval_purpose,
	CompileExprResult &result_out,
	slake::Opcode opcode,
	uint32_t idx_sld);
[[nodiscard]] static peff::Option<CompilationError> _compile_simple_assign_expr(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<BinaryExprNode> expr,
	ExprEvalPurpose eval_purpose,
	AstNodePtr<TypeNameNode> lhs_type,
	AstNodePtr<TypeNameNode> desired_lhs_type,
	AstNodePtr<TypeNameNode> rhs_type,
	AstNodePtr<TypeNameNode> decayed_rhs_type,
	AstNodePtr<TypeNameNode> desired_rhs_type,
	ExprEvalPurpose rhs_eval_purpose,
	CompileExprResult &result_out,
	uint32_t idx_sld);
[[nodiscard]] static peff::Option<CompilationError> _compile_simple_land_binary_expr(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<BinaryExprNode> expr,
	ExprEvalPurpose eval_purpose,
	AstNodePtr<BoolTypeNameNode> bool_type,
	AstNodePtr<TypeNameNode> lhs_type,
	AstNodePtr<TypeNameNode> rhs_type,
	CompileExprResult &result_out,
	slake::Opcode opcode,
	uint32_t idx_sld);
[[nodiscard]] static peff::Option<CompilationError> _compile_simple_lor_binary_expr(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<BinaryExprNode> expr,
	ExprEvalPurpose eval_purpose,
	AstNodePtr<BoolTypeNameNode> bool_type,
	AstNodePtr<TypeNameNode> lhs_type,
	AstNodePtr<TypeNameNode> rhs_type,
	CompileExprResult &result_out,
	slake::Opcode opcode,
	uint32_t idx_sld);
[[nodiscard]] static peff::Option<CompilationError> _compile_simple_compound_assign_expr(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<BinaryExprNode> expr,
	ExprEvalPurpose eval_purpose,
	AstNodePtr<TypeNameNode> lhs_type,
	AstNodePtr<TypeNameNode> rhs_type,
	AstNodePtr<TypeNameNode> desired_rhs_type,
	ExprEvalPurpose rhs_eval_purpose,
	CompileExprResult &result_out,
	slake::Opcode opcode,
	uint32_t idx_sld);
[[nodiscard]] PEFF_FORCEINLINE peff::Option<CompilationError> _update_equality_judgement_involved_states(
	AstNodePtr<TypeNameNode> lhs_type,
	AstNodePtr<TypeNameNode> rhs_type,
	const CompileExprResult &lhs_result,
	const CompileExprResult &rhs_result,
	CompileExprResult &result_out);
[[nodiscard]] PEFF_FORCEINLINE peff::Option<CompilationError> _update_inequality_judgement_involved_states(
	AstNodePtr<TypeNameNode> lhs_type,
	AstNodePtr<TypeNameNode> rhs_type,
	const CompileExprResult &lhs_result,
	const CompileExprResult &rhs_result,
	CompileExprResult &result_out);

static peff::Option<CompilationError> _compile_simple_binary_expr(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<BinaryExprNode> expr,
	ExprEvalPurpose eval_purpose,
	AstNodePtr<TypeNameNode> lhs_type,
	AstNodePtr<TypeNameNode> desired_lhs_type,
	ExprEvalPurpose lhs_eval_purpose,
	AstNodePtr<TypeNameNode> rhs_type,
	AstNodePtr<TypeNameNode> desired_rhs_type,
	ExprEvalPurpose rhs_eval_purpose,
	CompileExprResult &result_out,
	slake::Opcode opcode,
	uint32_t idx_sld) {
	peff::Option<CompilationError> e;

	switch (eval_purpose) {
		case ExprEvalPurpose::EvalType:
		case ExprEvalPurpose::EvalTypeActual:
			break;
		case ExprEvalPurpose::LValue:
			return CompilationError(expr->token_range, CompilationErrorKind::ExpectingLValueExpr);
		case ExprEvalPurpose::Stmt:
			SLKC_RETURN_IF_COMP_ERROR(compile_env->push_warning(
				CompilationWarning(expr->token_range, CompilationWarningKind::UnusedExprResult)));
			[[fallthrough]];
		case ExprEvalPurpose::RValue: {
			CompileExprResult lhs_result(compile_env->allocator.get()), rhs_result(compile_env->allocator.get());

			uint32_t lhs_reg,
				rhs_reg;

			if ((e = _compile_or_cast_operand(compile_env, compilation_context, path_env, lhs_eval_purpose, desired_lhs_type, expr->lhs, lhs_type, lhs_result))) {
				if (auto re = _compile_or_cast_operand(compile_env, compilation_context, path_env, rhs_eval_purpose, desired_rhs_type, expr->rhs, rhs_type, rhs_result); re) {
					if (!compile_env->errors.push_back(std::move(*e))) {
						return gen_oom_comp_error();
					}
					e.reset();
					return re;
				} else {
					return e;
				}
			}
			lhs_reg = lhs_result.idx_result_reg_out;
			SLKC_RETURN_IF_COMP_ERROR(_compile_or_cast_operand(compile_env, compilation_context, path_env, rhs_eval_purpose, desired_rhs_type, expr->rhs, rhs_type, rhs_result));
			rhs_reg = rhs_result.idx_result_reg_out;

			switch (expr->binary_op) {
				case BinaryOp::StrictEq:
				case BinaryOp::Eq: {
					SLKC_RETURN_IF_COMP_ERROR(_update_equality_judgement_involved_states(lhs_type, rhs_type, lhs_result, rhs_result, result_out));
					break;
				}
				case BinaryOp::StrictNeq:
				case BinaryOp::Neq: {
					SLKC_RETURN_IF_COMP_ERROR(_update_inequality_judgement_involved_states(lhs_type, rhs_type, lhs_result, rhs_result, result_out));
					break;
				}
				default:
					break;
			}

			uint32_t output_reg;
			SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(output_reg));
			result_out.idx_result_reg_out = output_reg;

			SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(
				idx_sld,
				opcode,
				output_reg,
				{ slake::Value(slake::ValueType::RegIndex, lhs_reg), slake::Value(slake::ValueType::RegIndex, rhs_reg) }));

			break;
		}
		case ExprEvalPurpose::Call:
			return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotCallable);
		case ExprEvalPurpose::Unpacking:
			return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotUnpackable);
	}

	return {};
}

static peff::Option<CompilationError> _compile_simple_assign_expr(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<BinaryExprNode> expr,
	ExprEvalPurpose eval_purpose,
	AstNodePtr<TypeNameNode> lhs_type,
	AstNodePtr<TypeNameNode> desired_lhs_type,
	AstNodePtr<TypeNameNode> rhs_type,
	AstNodePtr<TypeNameNode> decayed_rhs_type,
	AstNodePtr<TypeNameNode> desired_rhs_type,
	ExprEvalPurpose rhs_eval_purpose,
	CompileExprResult &result_out,
	uint32_t idx_sld) {
	peff::Option<CompilationError> e;

	switch (eval_purpose) {
		case ExprEvalPurpose::EvalType:
		case ExprEvalPurpose::EvalTypeActual:
			break;
		case ExprEvalPurpose::LValue: {
			CompileExprResult lhs_result(compile_env->allocator.get()), rhs_result(compile_env->allocator.get());

			uint32_t rhs_reg;

			if ((e = _compile_or_cast_operand(compile_env, compilation_context, path_env, ExprEvalPurpose::LValue, desired_lhs_type, expr->lhs, lhs_type, lhs_result))) {
				if (auto re = _compile_or_cast_operand(compile_env, compilation_context, path_env, rhs_eval_purpose, desired_rhs_type, expr->rhs, decayed_rhs_type, rhs_result); re) {
					if (!compile_env->errors.push_back(std::move(*e))) {
						return gen_oom_comp_error();
					}
					e.reset();
					return re;
				} else {
					return e;
				}
			}
			result_out.idx_result_reg_out = lhs_result.idx_result_reg_out;

			if (!(rhs_type->is_nullable) && (desired_rhs_type->is_nullable)) {
				SLKC_RETURN_IF_COMP_ERROR(
					remove_nullable_of_type(desired_rhs_type, desired_rhs_type));
			}

			SLKC_RETURN_IF_COMP_ERROR(_compile_or_cast_operand(compile_env, compilation_context, path_env, rhs_eval_purpose, desired_rhs_type, expr->rhs, decayed_rhs_type, rhs_result));
			rhs_reg = rhs_result.idx_result_reg_out;
			SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(
				idx_sld,
				slake::Opcode::STORE,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::RegIndex, result_out.idx_result_reg_out), slake::Value(slake::ValueType::RegIndex, rhs_reg) }));

			if (lhs_result.evaluated_final_member && (lhs_result.evaluated_final_member->get_ast_node_type() == AstNodeType::Var)) {
				if (rhs_result.evaluated_type->tn_kind == TypeNameKind::Null) {
					SLKC_RETURN_IF_COMP_ERROR(path_env->set_local_var_nullity_override(lhs_result.evaluated_var_chain, NullOverrideType::Nullify));
				} else {
					if (rhs_result.evaluated_type->is_nullable) {
						if (rhs_result.evaluated_final_member && (rhs_result.evaluated_final_member->get_ast_node_type() == AstNodeType::Var)) {
							if (auto override_type = path_env->lookup_var_nullity_override(rhs_result.evaluated_var_chain); override_type.has_value()) {
								SLKC_RETURN_IF_COMP_ERROR(path_env->set_local_var_nullity_override(lhs_result.evaluated_var_chain, override_type.value()));
							} else
								SLKC_RETURN_IF_COMP_ERROR(path_env->set_local_var_nullity_override(lhs_result.evaluated_var_chain, NullOverrideType::Uncertain));
						} else {
							SLKC_RETURN_IF_COMP_ERROR(path_env->set_local_var_nullity_override(lhs_result.evaluated_var_chain, NullOverrideType::Uncertain));
						}
					} else
						SLKC_RETURN_IF_COMP_ERROR(path_env->set_local_var_nullity_override(lhs_result.evaluated_var_chain, NullOverrideType::Denullify));
				}
			}
			break;
		}
		case ExprEvalPurpose::Stmt:
		case ExprEvalPurpose::RValue: {
			CompileExprResult lhs_result(compile_env->allocator.get()), rhs_result(compile_env->allocator.get());

			uint32_t lhs_reg,
				rhs_reg;

			if ((e = _compile_or_cast_operand(compile_env, compilation_context, path_env, ExprEvalPurpose::LValue, desired_lhs_type, expr->lhs, lhs_type, lhs_result))) {
				if (auto re = _compile_or_cast_operand(compile_env, compilation_context, path_env, rhs_eval_purpose, desired_rhs_type, expr->rhs, decayed_rhs_type, rhs_result); re) {
					if (!compile_env->errors.push_back(std::move(*e))) {
						return gen_oom_comp_error();
					}
					e.reset();
					return re;
				} else {
					return e;
				}
			}
			lhs_reg = lhs_result.idx_result_reg_out;

			if (!(rhs_type->is_nullable) && (desired_rhs_type->is_nullable)) {
				SLKC_RETURN_IF_COMP_ERROR(
					remove_nullable_of_type(desired_rhs_type, desired_rhs_type));
			}

			SLKC_RETURN_IF_COMP_ERROR(_compile_or_cast_operand(compile_env, compilation_context, path_env, rhs_eval_purpose, desired_rhs_type, expr->rhs, decayed_rhs_type, rhs_result));
			rhs_reg = rhs_result.idx_result_reg_out;
			SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(
				idx_sld,
				slake::Opcode::STORE,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::RegIndex, lhs_reg), slake::Value(slake::ValueType::RegIndex, rhs_reg) }));
			result_out.idx_result_reg_out = rhs_reg;

			if (lhs_result.evaluated_final_member && (lhs_result.evaluated_final_member->get_ast_node_type() == AstNodeType::Var)) {
				if (rhs_result.evaluated_type->tn_kind == TypeNameKind::Null) {
					SLKC_RETURN_IF_COMP_ERROR(path_env->set_local_var_nullity_override(lhs_result.evaluated_var_chain, NullOverrideType::Nullify));
				} else {
					if (rhs_result.evaluated_type->is_nullable) {
						if (rhs_result.evaluated_final_member && (rhs_result.evaluated_final_member->get_ast_node_type() == AstNodeType::Var)) {
							if (auto override_type = path_env->lookup_var_nullity_override(rhs_result.evaluated_var_chain); override_type.has_value()) {
								SLKC_RETURN_IF_COMP_ERROR(path_env->set_local_var_nullity_override(lhs_result.evaluated_var_chain, override_type.value()));
							} else
								SLKC_RETURN_IF_COMP_ERROR(path_env->set_local_var_nullity_override(lhs_result.evaluated_var_chain, NullOverrideType::Uncertain));
						} else {
							SLKC_RETURN_IF_COMP_ERROR(path_env->set_local_var_nullity_override(lhs_result.evaluated_var_chain, NullOverrideType::Uncertain));
						}
					} else
						SLKC_RETURN_IF_COMP_ERROR(path_env->set_local_var_nullity_override(lhs_result.evaluated_var_chain, NullOverrideType::Denullify));
				}
			}
			break;
		}
		case ExprEvalPurpose::Call:
			return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotCallable);
		case ExprEvalPurpose::Unpacking:
			return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotUnpackable);
	}

	return {};
}

static peff::Option<CompilationError> _compile_simple_land_binary_expr(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<BinaryExprNode> expr,
	ExprEvalPurpose eval_purpose,
	AstNodePtr<BoolTypeNameNode> bool_type,
	AstNodePtr<TypeNameNode> lhs_type,
	AstNodePtr<TypeNameNode> rhs_type,
	CompileExprResult &result_out,
	slake::Opcode opcode,
	uint32_t idx_sld) {
	peff::Option<CompilationError> e;

	switch (eval_purpose) {
		case ExprEvalPurpose::EvalType:
		case ExprEvalPurpose::EvalTypeActual:
			break;
		case ExprEvalPurpose::LValue:
			return CompilationError(expr->token_range, CompilationErrorKind::ExpectingLValueExpr);
		case ExprEvalPurpose::Stmt:
		case ExprEvalPurpose::RValue: {
			CompileExprResult lhs_result(compile_env->allocator.get()), rhs_result(compile_env->allocator.get());

			uint32_t lhs_reg,
				rhs_reg,
				tmp_result_reg;

			SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(tmp_result_reg));

			uint32_t cmp_end_label_id;
			SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_label(cmp_end_label_id));

			if ((e = _compile_or_cast_operand(compile_env, compilation_context, path_env, ExprEvalPurpose::RValue, bool_type.cast_to<TypeNameNode>(), expr->lhs, lhs_type, lhs_result))) {
				if (auto re = _compile_or_cast_operand(compile_env, compilation_context, path_env, ExprEvalPurpose::RValue, bool_type.cast_to<TypeNameNode>(), expr->rhs, rhs_type, rhs_result); re) {
					if (!compile_env->errors.push_back(std::move(*e))) {
						return gen_oom_comp_error();
					}
					e.reset();
					return re;
				} else {
					return e;
				}
			}
			lhs_reg = lhs_result.idx_result_reg_out;

			uint32_t post_branch_label_id;
			SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_label(post_branch_label_id));

			uint32_t post_branch_phi_src_off = compilation_context->get_cur_ins_off();

			SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(
				idx_sld,
				slake::Opcode::BR,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::RegIndex, lhs_reg), slake::Value(slake::ValueType::Label, post_branch_label_id), slake::Value(slake::ValueType::Label, cmp_end_label_id) }));

			compilation_context->set_label_offset(post_branch_label_id, compilation_context->get_cur_ins_off());

			{
				PathEnv lhs_applied_path_env(compile_env->allocator.get());
				lhs_applied_path_env.set_parent(path_env);
				SLKC_RETURN_IF_COMP_ERROR(combine_path_env(lhs_applied_path_env, lhs_result.guard_path_env));
				SLKC_RETURN_IF_COMP_ERROR(_compile_or_cast_operand(compile_env, compilation_context, &lhs_applied_path_env, ExprEvalPurpose::RValue, bool_type.cast_to<TypeNameNode>(), expr->rhs, rhs_type, rhs_result));
			}
			rhs_reg = rhs_result.idx_result_reg_out;
			SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(
				idx_sld,
				slake::Opcode::LAND,
				tmp_result_reg,
				{ slake::Value(slake::ValueType::RegIndex, lhs_reg), slake::Value(slake::ValueType::RegIndex, rhs_reg) }));

			uint32_t cmp_end_phi_src_off = compilation_context->get_cur_ins_off();

			SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(
				idx_sld,
				slake::Opcode::JMP,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::Label, cmp_end_label_id) }));

			compilation_context->set_label_offset(cmp_end_label_id, compilation_context->get_cur_ins_off());

			SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(
				idx_sld,
				slake::Opcode::PHI,
				tmp_result_reg,
				{ slake::Value((uint32_t)post_branch_phi_src_off), slake::Value(slake::ValueType::RegIndex, lhs_reg),
					slake::Value((uint32_t)cmp_end_phi_src_off), slake::Value(slake::ValueType::RegIndex, tmp_result_reg) }));
			result_out.idx_result_reg_out = tmp_result_reg;

			SLKC_RETURN_IF_COMP_ERROR(combine_path_env(result_out.guard_path_env, lhs_result.guard_path_env));
			SLKC_RETURN_IF_COMP_ERROR(combine_path_env(result_out.guard_path_env, rhs_result.guard_path_env));

			break;
		}
		case ExprEvalPurpose::Call:
			return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotCallable);
		case ExprEvalPurpose::Unpacking:
			return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotUnpackable);
	}

	return {};
}

static peff::Option<CompilationError> _compile_simple_lor_binary_expr(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<BinaryExprNode> expr,
	ExprEvalPurpose eval_purpose,
	AstNodePtr<BoolTypeNameNode> bool_type,
	AstNodePtr<TypeNameNode> lhs_type,
	AstNodePtr<TypeNameNode> rhs_type,
	CompileExprResult &result_out,
	slake::Opcode opcode,
	uint32_t idx_sld) {
	peff::Option<CompilationError> e;

	switch (eval_purpose) {
		case ExprEvalPurpose::EvalType:
		case ExprEvalPurpose::EvalTypeActual:
			break;
		case ExprEvalPurpose::LValue:
			return CompilationError(expr->token_range, CompilationErrorKind::ExpectingLValueExpr);
		case ExprEvalPurpose::Stmt:
		case ExprEvalPurpose::RValue: {
			CompileExprResult lhs_result(compile_env->allocator.get()),
				rhs_result(compile_env->allocator.get());

			uint32_t lhs_reg,
				rhs_reg,
				tmp_result_reg;

			SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(tmp_result_reg));

			uint32_t cmp_end_label_id;
			SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_label(cmp_end_label_id));

			PathEnv lhs_path_env(compile_env->allocator.get()),
				rhs_path_env(compile_env->allocator.get());

			PathEnv *operands_subenvs[2] = {
				&lhs_path_env,
				&rhs_path_env
			};

			lhs_path_env.set_parent(path_env);
			rhs_path_env.set_parent(path_env);

			if ((e = _compile_or_cast_operand(compile_env, compilation_context, &lhs_path_env, ExprEvalPurpose::RValue, bool_type.cast_to<TypeNameNode>(), expr->lhs, lhs_type, lhs_result))) {
				if (auto re = _compile_or_cast_operand(compile_env, compilation_context, &rhs_path_env, ExprEvalPurpose::RValue, bool_type.cast_to<TypeNameNode>(), expr->rhs, rhs_type, rhs_result); re) {
					if (!compile_env->errors.push_back(std::move(*e))) {
						return gen_oom_comp_error();
					}
					e.reset();
					return re;
				} else {
					return e;
				}
			}
			lhs_reg = lhs_result.idx_result_reg_out;

			uint32_t post_branch_label_id;
			SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_label(post_branch_label_id));

			uint32_t post_branch_phi_src_off = compilation_context->get_cur_ins_off();

			SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(
				idx_sld,
				slake::Opcode::BR,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::RegIndex, lhs_reg), slake::Value(slake::ValueType::Label, cmp_end_label_id), slake::Value(slake::ValueType::Label, post_branch_label_id) }));

			compilation_context->set_label_offset(post_branch_label_id, compilation_context->get_cur_ins_off());

			SLKC_RETURN_IF_COMP_ERROR(_compile_or_cast_operand(compile_env, compilation_context, &rhs_path_env, ExprEvalPurpose::RValue, bool_type.cast_to<TypeNameNode>(), expr->rhs, rhs_type, rhs_result));
			rhs_reg = rhs_result.idx_result_reg_out;
			SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(
				idx_sld,
				slake::Opcode::LAND,
				tmp_result_reg,
				{ slake::Value(slake::ValueType::RegIndex, lhs_reg), slake::Value(slake::ValueType::RegIndex, rhs_reg) }));

			uint32_t cmp_end_phi_src_off = compilation_context->get_cur_ins_off();

			SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(
				idx_sld,
				slake::Opcode::JMP,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::Label, cmp_end_label_id) }));

			compilation_context->set_label_offset(cmp_end_label_id, compilation_context->get_cur_ins_off());

			SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(
				idx_sld,
				slake::Opcode::PHI,
				tmp_result_reg,
				{ slake::Value((uint32_t)post_branch_phi_src_off), slake::Value(slake::ValueType::RegIndex, lhs_reg),
					slake::Value((uint32_t)cmp_end_phi_src_off), slake::Value(slake::ValueType::RegIndex, tmp_result_reg) }));
			result_out.idx_result_reg_out = tmp_result_reg;

			SLKC_RETURN_IF_COMP_ERROR(combine_parallel_path_env(compile_env->allocator.get(), compile_env, compilation_context, *path_env, operands_subenvs, 2));

			PathEnv *expr_subenvs[2] = {
				&lhs_result.guard_path_env,
				&rhs_result.guard_path_env
			};

			SLKC_RETURN_IF_COMP_ERROR(combine_parallel_path_env(compile_env->allocator.get(), compile_env, compilation_context, result_out.guard_path_env, expr_subenvs, 2));

			break;
		}
		case ExprEvalPurpose::Call:
			return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotCallable);
		case ExprEvalPurpose::Unpacking:
			return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotUnpackable);
	}

	return {};
}

static peff::Option<CompilationError> _compile_simple_compound_assign_expr(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<BinaryExprNode> expr,
	ExprEvalPurpose eval_purpose,
	AstNodePtr<TypeNameNode> lhs_type,
	AstNodePtr<TypeNameNode> rhs_type,
	AstNodePtr<TypeNameNode> desired_rhs_type,
	ExprEvalPurpose rhs_eval_purpose,
	CompileExprResult &result_out,
	slake::Opcode opcode,
	uint32_t idx_sld) {
	switch (eval_purpose) {
		case ExprEvalPurpose::EvalType:
		case ExprEvalPurpose::EvalTypeActual:
			break;
		case ExprEvalPurpose::LValue: {
			CompileExprResult result(compile_env->allocator.get());

			uint32_t lhs_reg,
				lhs_value_reg,
				rhs_reg,
				result_value_reg;

			SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(lhs_value_reg));
			SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(result_value_reg));

			SLKC_RETURN_IF_COMP_ERROR(_compile_or_cast_operand(compile_env, compilation_context, path_env, ExprEvalPurpose::LValue, lhs_type, expr->lhs, lhs_type, result));
			lhs_reg = result.idx_result_reg_out;
			SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(
				idx_sld,
				slake::Opcode::LVALUE,
				lhs_value_reg,
				{ slake::Value(slake::ValueType::RegIndex, lhs_reg) }));

			SLKC_RETURN_IF_COMP_ERROR(_compile_or_cast_operand(compile_env, compilation_context, path_env, rhs_eval_purpose, desired_rhs_type, expr->rhs, rhs_type, result));
			rhs_reg = result.idx_result_reg_out;
			SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(
				idx_sld,
				opcode,
				result_value_reg,
				{ slake::Value(slake::ValueType::RegIndex, lhs_value_reg), slake::Value(slake::ValueType::RegIndex, rhs_reg) }));

			SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(
				idx_sld,
				slake::Opcode::STORE,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::RegIndex, lhs_reg), slake::Value(slake::ValueType::RegIndex, result_value_reg) }));

			result_out.idx_result_reg_out = lhs_reg;
			break;
		}
		case ExprEvalPurpose::Stmt:
			SLKC_RETURN_IF_COMP_ERROR(compile_env->push_warning(
				CompilationWarning(expr->token_range, CompilationWarningKind::UnusedExprResult)));
			[[fallthrough]];
		case ExprEvalPurpose::RValue: {
			CompileExprResult result(compile_env->allocator.get());

			uint32_t lhs_reg,
				lhs_value_reg,
				rhs_reg,
				result_value_reg;

			SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(lhs_value_reg));
			SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(result_value_reg));

			SLKC_RETURN_IF_COMP_ERROR(_compile_or_cast_operand(compile_env, compilation_context, path_env, ExprEvalPurpose::LValue, lhs_type, expr->lhs, lhs_type, result));
			lhs_reg = result.idx_result_reg_out;
			SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(
				idx_sld,
				slake::Opcode::LVALUE,
				lhs_value_reg,
				{ slake::Value(slake::ValueType::RegIndex, lhs_reg) }));

			SLKC_RETURN_IF_COMP_ERROR(_compile_or_cast_operand(compile_env, compilation_context, path_env, rhs_eval_purpose, desired_rhs_type, expr->rhs, rhs_type, result));
			rhs_reg = result.idx_result_reg_out;
			SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(
				idx_sld,
				opcode,
				result_value_reg,
				{ slake::Value(slake::ValueType::RegIndex, lhs_value_reg), slake::Value(slake::ValueType::RegIndex, rhs_reg) }));

			SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(
				idx_sld,
				slake::Opcode::STORE,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::RegIndex, lhs_reg), slake::Value(slake::ValueType::RegIndex, result_value_reg) }));
			result_out.idx_result_reg_out = result_value_reg;
			break;
		}
		case ExprEvalPurpose::Call:
			return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotCallable);
		case ExprEvalPurpose::Unpacking:
			return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotUnpackable);
	}

	return {};
}

PEFF_FORCEINLINE peff::Option<CompilationError> _update_equality_judgement_involved_states(
	AstNodePtr<TypeNameNode> lhs_type,
	AstNodePtr<TypeNameNode> rhs_type,
	const CompileExprResult &lhs_result,
	const CompileExprResult &rhs_result,
	CompileExprResult &result_out) {
	//
	// Apply variable nullity overrides.
	// TODO: Handle cond == true, cond == false, cond != true, cond != false
	//
	if (lhs_result.evaluated_final_member && (lhs_result.evaluated_final_member->get_ast_node_type() == AstNodeType::Var)) {
		AstNodePtr<VarNode> v = lhs_result.evaluated_final_member.cast_to<VarNode>();
		if (v->type->is_nullable) {
			if (rhs_type->tn_kind == TypeNameKind::Null) {
				// v == null
				SLKC_RETURN_IF_COMP_ERROR(result_out.guard_path_env.set_local_var_nullity_override(lhs_result.evaluated_var_chain, NullOverrideType::Nullify));
			} else if (rhs_type->is_nullable) {
				// v == T?
				SLKC_RETURN_IF_COMP_ERROR(result_out.guard_path_env.set_local_var_nullity_override(lhs_result.evaluated_var_chain, NullOverrideType::Uncertain));
			} else {
				// v == T
				SLKC_RETURN_IF_COMP_ERROR(result_out.guard_path_env.set_local_var_nullity_override(lhs_result.evaluated_var_chain, NullOverrideType::Denullify));
			}
		}
	} else {
		if (rhs_result.evaluated_final_member && (rhs_result.evaluated_final_member->get_ast_node_type() == AstNodeType::Var)) {
			AstNodePtr<VarNode> v = rhs_result.evaluated_final_member.cast_to<VarNode>();
			if (v->type->is_nullable) {
				if (lhs_type->tn_kind == TypeNameKind::Null) {
					// null == v
					SLKC_RETURN_IF_COMP_ERROR(result_out.guard_path_env.set_local_var_nullity_override(rhs_result.evaluated_var_chain, NullOverrideType::Nullify));
				} else if (lhs_type->is_nullable) {
					// T? == v
					SLKC_RETURN_IF_COMP_ERROR(result_out.guard_path_env.set_local_var_nullity_override(rhs_result.evaluated_var_chain, NullOverrideType::Uncertain));
				} else {
					// T == v
					SLKC_RETURN_IF_COMP_ERROR(result_out.guard_path_env.set_local_var_nullity_override(rhs_result.evaluated_var_chain, NullOverrideType::Denullify));
				}
			}
		}
	}
	return {};
}

PEFF_FORCEINLINE peff::Option<CompilationError> _update_inequality_judgement_involved_states(
	AstNodePtr<TypeNameNode> lhs_type,
	AstNodePtr<TypeNameNode> rhs_type,
	const CompileExprResult &lhs_result,
	const CompileExprResult &rhs_result,
	CompileExprResult &result_out) {
	if (lhs_result.evaluated_final_member && (lhs_result.evaluated_final_member->get_ast_node_type() == AstNodeType::Var)) {
		AstNodePtr<VarNode> v = lhs_result.evaluated_final_member.cast_to<VarNode>();
		if (v->type->is_nullable) {
			if (rhs_type->tn_kind == TypeNameKind::Null) {
				// v != null
				SLKC_RETURN_IF_COMP_ERROR(result_out.guard_path_env.set_local_var_nullity_override(lhs_result.evaluated_var_chain, NullOverrideType::Denullify));
			} else {
				// v != T? and v != T are undeterminable
			}
		}
	} else {
		if (rhs_result.evaluated_final_member && (rhs_result.evaluated_final_member->get_ast_node_type() == AstNodeType::Var)) {
			AstNodePtr<VarNode> v = rhs_result.evaluated_final_member.cast_to<VarNode>();
			if (v->type->is_nullable) {
				if (lhs_type->tn_kind == TypeNameKind::Null) {
					// null != v
					SLKC_RETURN_IF_COMP_ERROR(result_out.guard_path_env.set_local_var_nullity_override(rhs_result.evaluated_var_chain, NullOverrideType::Denullify));
				} else {
					// T? != v and T != v are undeterminable
				}
			}
		}
	}
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compile_binary_expr(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<BinaryExprNode> expr,
	ExprEvalPurpose eval_purpose,
	CompileExprResult &result_out) {
	AstNodePtr<TypeNameNode> lhs_type, rhs_type, decayed_lhs_type, decayed_rhs_type,
		actual_lhs_type, actual_rhs_type, decayed_actual_lhs_type, decayed_actual_rhs_type;

	peff::Option<PathEnv> lhs_applied_path_env;
	switch (expr->binary_op) {
		case BinaryOp::LAnd: {
			lhs_applied_path_env = PathEnv(compile_env->allocator.get());
			lhs_applied_path_env->set_parent(path_env);

			{
				NormalCompilationContext tmp_context(compile_env, compilation_context);
				CompileExprResult lhs_result(compile_env->allocator.get());
				if (auto e = compile_expr(compile_env, &tmp_context, path_env, expr->lhs, ExprEvalPurpose::RValue, {}, lhs_result); e) {
					if (auto re = eval_expr_type(compile_env, compilation_context, path_env, expr->rhs, rhs_type); re) {
						if (!compile_env->errors.push_back(std::move(*e))) {
							return gen_oom_comp_error();
						}
						e.reset();
						return re;
					}
					return e;
				}
				SLKC_RETURN_IF_COMP_ERROR(combine_path_env(*lhs_applied_path_env, lhs_result.guard_path_env));
				lhs_type = lhs_result.evaluated_type;
			}

			SLKC_RETURN_IF_COMP_ERROR(
				eval_expr_type(compile_env, compilation_context, &*lhs_applied_path_env, expr->rhs, rhs_type));
			break;
		}
		default:
			if (auto e = eval_expr_type(compile_env, compilation_context, path_env, expr->lhs, lhs_type); e) {
				if (auto re = eval_expr_type(compile_env, compilation_context, path_env, expr->rhs, rhs_type); re) {
					if (!compile_env->errors.push_back(std::move(*e))) {
						return gen_oom_comp_error();
					}
					e.reset();
					return re;
				}
				return e;
			}
			SLKC_RETURN_IF_COMP_ERROR(
				eval_expr_type(compile_env, compilation_context, path_env, expr->rhs, rhs_type));
	}
	SLKC_RETURN_IF_COMP_ERROR(
		remove_ref_of_type(lhs_type, decayed_lhs_type));
	SLKC_RETURN_IF_COMP_ERROR(
		remove_ref_of_type(rhs_type, decayed_rhs_type));

	uint32_t sld_index;
	SLKC_RETURN_IF_COMP_ERROR(compilation_context->register_source_loc_desc(token_range_to_sld(expr->token_range), sld_index));

	if (expr->binary_op == BinaryOp::Comma) {
		CompileExprResult result(compile_env->allocator.get());

		SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, expr->lhs, ExprEvalPurpose::Stmt, {}, result));
		SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, expr->rhs, eval_purpose, {}, result_out));
		return {};
	}

	peff::SharedPtr<U32TypeNameNode> u32_type;
	peff::SharedPtr<I32TypeNameNode> i32_type;
	AstNodePtr<BoolTypeNameNode> bool_type;

	if (!(u32_type = peff::make_shared_with_control_block<U32TypeNameNode, AstNodeControlBlock<U32TypeNameNode>>(
			  compile_env->allocator.get(),
			  compile_env->allocator.get(),
			  compile_env->get_document()))) {
		return gen_oom_comp_error();
	}

	if (!(i32_type = peff::make_shared_with_control_block<I32TypeNameNode, AstNodeControlBlock<I32TypeNameNode>>(
			  compile_env->allocator.get(),
			  compile_env->allocator.get(),
			  compile_env->get_document()))) {
		return gen_oom_comp_error();
	}

	if (!(bool_type = make_ast_node<BoolTypeNameNode>(
			  compile_env->allocator.get(),
			  compile_env->allocator.get(),
			  compile_env->get_document()))) {
		return gen_oom_comp_error();
	}

	// Deal with the RHS to LHS user binary operator.
	if ((decayed_rhs_type->tn_kind == TypeNameKind::Custom) &&
		(decayed_lhs_type->tn_kind != TypeNameKind::Custom)) {
		switch (expr->binary_op) {
			case BinaryOp::Add:
			case BinaryOp::Sub:
			case BinaryOp::Mul:
			case BinaryOp::Div:
			case BinaryOp::Mod:
			case BinaryOp::And:
			case BinaryOp::Or:
			case BinaryOp::Xor:
			case BinaryOp::LAnd:
			case BinaryOp::LOr:
			case BinaryOp::Shl:
			case BinaryOp::Shr:
			case BinaryOp::Assign:	// ???
			case BinaryOp::AddAssign:
			case BinaryOp::SubAssign:
			case BinaryOp::MulAssign:
			case BinaryOp::DivAssign:
			case BinaryOp::ModAssign:
			case BinaryOp::AndAssign:
			case BinaryOp::OrAssign:
			case BinaryOp::XorAssign:
			case BinaryOp::ShlAssign:
			case BinaryOp::ShrAssign:
			case BinaryOp::Eq:
			case BinaryOp::Neq:
			case BinaryOp::Lt:
			case BinaryOp::Gt:
			case BinaryOp::LtEq:
			case BinaryOp::GtEq:
			case BinaryOp::Cmp: {
				AstNodePtr<MemberNode> cls_node, operator_slot;

				SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(compile_env, decayed_rhs_type->document->shared_from_this(), decayed_rhs_type.cast_to<CustomTypeNameNode>(), cls_node));

				IdRefEntry e(compile_env->allocator.get());

				std::string_view operator_name = get_binary_operator_overloading_name(expr->binary_op);

				if (!e.name.build(operator_name)) {
					return gen_oom_comp_error();
				}

				SLKC_RETURN_IF_COMP_ERROR(resolve_instance_member(compile_env, compile_env->get_document(), cls_node, e, operator_slot));

				if (!operator_slot)
					return CompilationError(
						expr->token_range,
						CompilationErrorKind::OperatorNotFound);

				if (operator_slot->get_ast_node_type() != AstNodeType::Fn)
					std::terminate();

				peff::DynArray<AstNodePtr<FnOverloadingNode>> matched_overloading_indices(compile_env->allocator.get());
				peff::DynArray<AstNodePtr<TypeNameNode>> operator_param_types(compile_env->allocator.get());

				if (!operator_param_types.push_back(AstNodePtr<TypeNameNode>(lhs_type))) {
					return gen_oom_comp_error();
				}

				AstNodePtr<VoidTypeNameNode> void_type;

				if (!(void_type = make_ast_node<VoidTypeNameNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(),
						  compile_env->get_document()))) {
					return gen_oom_comp_error();
				}

				if (!operator_param_types.push_back(void_type.cast_to<TypeNameNode>())) {
					return gen_oom_comp_error();
				}

				SLKC_RETURN_IF_COMP_ERROR(determine_fn_overloading(compile_env, operator_slot.cast_to<FnNode>(), operator_param_types.data(), operator_param_types.size(), false, matched_overloading_indices));

				switch (matched_overloading_indices.size()) {
					case 0:
						return CompilationError(
							expr->token_range,
							CompilationErrorKind::OperatorNotFound);
					case 1:
						break;
					default:
						return CompilationError(
							expr->token_range,
							CompilationErrorKind::AmbiguousOperatorCall);
				}

				auto matched_overloading = matched_overloading_indices.back();

				bool accessible;
				SLKC_RETURN_IF_COMP_ERROR(is_member_accessible(compile_env, {}, matched_overloading.cast_to<MemberNode>(), accessible));
				if (!accessible)
					return CompilationError(
						expr->token_range,
						CompilationErrorKind::MemberIsNotAccessible);

				uint32_t rhs_reg;
				{
					CompileExprResult arg_result(compile_env->allocator.get());

					SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, expr->rhs, ExprEvalPurpose::RValue, decayed_lhs_type, arg_result));
					rhs_reg = arg_result.idx_result_reg_out;
				}

				uint32_t operator_reg;
				SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(operator_reg));
				if (matched_overloading->fn_flags & FN_VIRTUAL) {
					slake::HostObjectRef<slake::IdRefObject> id_ref_object;

					if (!(id_ref_object = slake::IdRefObject::alloc(compile_env->runtime))) {
						return gen_out_of_runtime_memory_comp_error();
					}

					slake::IdRefEntry e(compile_env->runtime->get_cur_gen_alloc());

					if (!e.name.build(operator_name)) {
						return gen_out_of_runtime_memory_comp_error();
					}

					if (!id_ref_object->entries.push_back(std::move(e))) {
						return gen_out_of_runtime_memory_comp_error();
					}
					SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, compilation_context, decayed_rhs_type, id_ref_object->overriden_type));

					id_ref_object->param_types = peff::DynArray<slake::TypeRef>(compile_env->runtime->get_cur_gen_alloc());

					if (!id_ref_object->param_types->resize(matched_overloading->params.size())) {
						return gen_oom_comp_error();
					}

					for (size_t i = 0; i < id_ref_object->param_types->size(); ++i) {
						SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, compilation_context, matched_overloading->params.at(i)->type, id_ref_object->param_types->at(i)));
					}

					id_ref_object->has_var_args = true;

					SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::RLOAD, operator_reg, { slake::Value(slake::ValueType::RegIndex, rhs_reg), slake::Value(slake::Reference(id_ref_object.get())) }));
				} else {
					slake::HostObjectRef<slake::IdRefObject> id_ref_object;

					IdRefPtr full_name;
					SLKC_RETURN_IF_COMP_ERROR(get_full_id_ref(compile_env->allocator.get(), operator_slot, full_name));

					SLKC_RETURN_IF_COMP_ERROR(compile_id_ref(compile_env, compilation_context, full_name->entries.data(), full_name->entries.size(), nullptr, 0, true, decayed_rhs_type, id_ref_object));

					id_ref_object->param_types = peff::DynArray<slake::TypeRef>(compile_env->runtime->get_cur_gen_alloc());

					if (!id_ref_object->param_types->resize(matched_overloading->params.size())) {
						return gen_oom_comp_error();
					}

					for (size_t i = 0; i < id_ref_object->param_types->size(); ++i) {
						SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, compilation_context, matched_overloading->params.at(i)->type, id_ref_object->param_types->at(i)));
					}

					id_ref_object->has_var_args = true;

					SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::LOAD, operator_reg, { slake::Value(slake::Reference(id_ref_object.get())) }));
				}

				uint32_t reg;
				{
					CompileExprResult arg_result(compile_env->allocator.get());

					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR(is_lvalue_type(matched_overloading->params.at(0)->type, b));

					SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, expr->rhs, b ? ExprEvalPurpose::LValue : ExprEvalPurpose::RValue, matched_overloading->params.at(0)->type, arg_result));
					reg = arg_result.idx_result_reg_out;
				}

				SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::PUSHARG, UINT32_MAX, { slake::Value(slake::ValueType::RegIndex, reg) }));

				switch (eval_purpose) {
					case ExprEvalPurpose::EvalType:
					case ExprEvalPurpose::EvalTypeActual:
						break;
					case ExprEvalPurpose::Stmt:
						SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::MCALL, UINT32_MAX, { slake::Value(slake::ValueType::RegIndex, operator_reg), slake::Value(slake::ValueType::RegIndex, rhs_reg) }));
						break;
					case ExprEvalPurpose::LValue: {
						bool b = false;
						SLKC_RETURN_IF_COMP_ERROR(is_lvalue_type(matched_overloading->return_type, b));
						if (!b) {
							return CompilationError(expr->token_range, CompilationErrorKind::ExpectingLValueExpr);
						}

						uint32_t output_reg;
						SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(output_reg));

						SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::MCALL, output_reg, { slake::Value(slake::ValueType::RegIndex, operator_reg), slake::Value(slake::ValueType::RegIndex, rhs_reg) }));
						result_out.idx_result_reg_out = output_reg;
						break;
					}
					case ExprEvalPurpose::RValue:
					case ExprEvalPurpose::Call: {
						bool b = false;
						SLKC_RETURN_IF_COMP_ERROR(is_lvalue_type(matched_overloading->return_type, b));

						if (b) {
							uint32_t tmp_reg_index;
							SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(tmp_reg_index));

							SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::MCALL, tmp_reg_index, { slake::Value(slake::ValueType::RegIndex, operator_reg), slake::Value(slake::ValueType::RegIndex, rhs_reg) }));

							uint32_t output_reg;
							SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(output_reg));

							SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::LVALUE, output_reg, { slake::Value(slake::ValueType::RegIndex, tmp_reg_index) }));
							result_out.idx_result_reg_out = output_reg;
						} else {
							uint32_t output_reg;
							SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(output_reg));

							SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::MCALL, output_reg, { slake::Value(slake::ValueType::RegIndex, operator_reg), slake::Value(slake::ValueType::RegIndex, rhs_reg) }));
							result_out.idx_result_reg_out = output_reg;
						}

						break;
					}
					case ExprEvalPurpose::Unpacking:
						return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotUnpackable);
				}

				goto rhs_to_lhs_custom_op_expr_resolved;
			}
			default:
				break;
		}
	}

	SLKC_RETURN_IF_COMP_ERROR(
		eval_actual_expr_type(compile_env, compilation_context, path_env, expr->lhs, actual_lhs_type));
	SLKC_RETURN_IF_COMP_ERROR(
		eval_actual_expr_type(compile_env, compilation_context, lhs_applied_path_env ? &*lhs_applied_path_env : path_env, expr->rhs, actual_rhs_type));
	SLKC_RETURN_IF_COMP_ERROR(
		remove_ref_of_type(actual_lhs_type, decayed_actual_lhs_type));
	SLKC_RETURN_IF_COMP_ERROR(
		remove_ref_of_type(actual_rhs_type, decayed_actual_rhs_type));
	{
		AstNodePtr<TypeNameNode> main_operation_type;

		if (decayed_lhs_type->tn_kind == TypeNameKind::Custom) {
			main_operation_type = decayed_lhs_type;
		} else {
			switch (expr->binary_op) {
				case BinaryOp::Assign:
					main_operation_type = decayed_lhs_type;
					break;
				case BinaryOp::Shl:
				case BinaryOp::Shr:
				case BinaryOp::ShlAssign:
				case BinaryOp::ShrAssign:
				case BinaryOp::Subscript:
					main_operation_type = decayed_lhs_type;
					break;
				default: {
					do {
						{
							if ((decayed_lhs_type->tn_kind == TypeNameKind::Bool)) {
								main_operation_type = decayed_lhs_type;
								break;
							}
							if ((decayed_rhs_type->tn_kind == TypeNameKind::Bool)) {
								main_operation_type = decayed_rhs_type;
								break;
							}
							bool lhs_pre, rhs_pre;
							SLKC_RETURN_IF_COMP_ERROR(is_unsigned(decayed_lhs_type, lhs_pre));
							SLKC_RETURN_IF_COMP_ERROR(is_unsigned(decayed_rhs_type, rhs_pre));
							if (lhs_pre && rhs_pre) {
								SLKC_RETURN_IF_COMP_ERROR(infer_common_type(decayed_lhs_type, decayed_actual_rhs_type, main_operation_type));
								break;
							}

							SLKC_RETURN_IF_COMP_ERROR(is_signed(decayed_lhs_type, lhs_pre));
							SLKC_RETURN_IF_COMP_ERROR(is_signed(decayed_rhs_type, rhs_pre));
							if (lhs_pre && rhs_pre) {
								SLKC_RETURN_IF_COMP_ERROR(infer_common_type(decayed_lhs_type, decayed_actual_rhs_type, main_operation_type));
								break;
							}

							SLKC_RETURN_IF_COMP_ERROR(is_floating_point(decayed_lhs_type, lhs_pre));
							SLKC_RETURN_IF_COMP_ERROR(is_floating_point(decayed_rhs_type, rhs_pre));
							if (lhs_pre && rhs_pre) {
								SLKC_RETURN_IF_COMP_ERROR(infer_common_type(decayed_lhs_type, decayed_actual_rhs_type, main_operation_type));
								break;
							}
						}

						{
							bool convertible_pre;
							SLKC_RETURN_IF_COMP_ERROR(is_convertible(decayed_rhs_type, decayed_lhs_type, true, convertible_pre));
							main_operation_type = convertible_pre ? decayed_lhs_type : decayed_rhs_type;
						}
					} while (false);
				}
			}
		}

		switch (main_operation_type->tn_kind) {
			case TypeNameKind::I8:
			case TypeNameKind::I16:
			case TypeNameKind::I32:
			case TypeNameKind::I64:
			case TypeNameKind::U8:
			case TypeNameKind::U16:
			case TypeNameKind::U32:
			case TypeNameKind::U64: {
				switch (expr->binary_op) {
					case BinaryOp::Add:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::ADD,
								sld_index));
						result_out.evaluated_type = decayed_lhs_type;
						break;
					case BinaryOp::Sub:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::SUB,
								sld_index));
						result_out.evaluated_type = decayed_lhs_type;
						break;
					case BinaryOp::Mul:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::MUL,
								sld_index));
						result_out.evaluated_type = decayed_lhs_type;
						break;
					case BinaryOp::Div:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::DIV,
								sld_index));
						result_out.evaluated_type = decayed_lhs_type;
						break;
					case BinaryOp::Mod:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::MOD,
								sld_index));
						result_out.evaluated_type = decayed_lhs_type;
						break;
					case BinaryOp::And:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::AND,
								sld_index));
						result_out.evaluated_type = decayed_lhs_type;
						break;
					case BinaryOp::Or:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::OR,
								sld_index));
						result_out.evaluated_type = decayed_lhs_type;
						break;
					case BinaryOp::Xor:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::XOR,
								sld_index));
						result_out.evaluated_type = decayed_lhs_type;
						break;
					case BinaryOp::LAnd:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_land_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								bool_type,
								decayed_lhs_type,
								decayed_rhs_type,
								result_out,
								slake::Opcode::LAND,
								sld_index));
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					case BinaryOp::LOr:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_lor_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								bool_type,
								decayed_lhs_type,
								decayed_rhs_type,
								result_out,
								slake::Opcode::LOR,
								sld_index));
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					case BinaryOp::Shl:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_rhs_type,
								u32_type.cast_to<TypeNameNode>(),
								ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::LSH,
								sld_index));
						result_out.evaluated_type = decayed_lhs_type;
						break;
					case BinaryOp::Shr:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_rhs_type,
								u32_type.cast_to<TypeNameNode>(),
								ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::RSH,
								sld_index));
						result_out.evaluated_type = decayed_lhs_type;
						break;
					case BinaryOp::Assign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_assign_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								actual_lhs_type, actual_lhs_type,
								rhs_type,
								decayed_rhs_type,
								decayed_actual_lhs_type,
								ExprEvalPurpose::RValue,
								result_out,
								sld_index));
						result_out.evaluated_type = lhs_type;
						break;
					case BinaryOp::AddAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_compound_assign_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								lhs_type,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::ADD,
								sld_index));
						result_out.evaluated_type = lhs_type;
						break;
					case BinaryOp::SubAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_compound_assign_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								lhs_type,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::SUB,
								sld_index));
						result_out.evaluated_type = lhs_type;
						break;
					case BinaryOp::MulAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_compound_assign_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								lhs_type,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::MUL,
								sld_index));
						result_out.evaluated_type = lhs_type;
						break;
					case BinaryOp::DivAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_compound_assign_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								lhs_type,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::DIV,
								sld_index));
						result_out.evaluated_type = lhs_type;
						break;
					case BinaryOp::ModAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_compound_assign_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								lhs_type,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::MOD,
								sld_index));
						result_out.evaluated_type = lhs_type;
						break;
					case BinaryOp::AndAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_compound_assign_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								lhs_type,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::AND,
								sld_index));
						result_out.evaluated_type = lhs_type;
						break;
					case BinaryOp::OrAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_compound_assign_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								lhs_type,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::OR,
								sld_index));
						result_out.evaluated_type = lhs_type;
						break;
					case BinaryOp::XorAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_compound_assign_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								lhs_type,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::XOR,
								sld_index));
						result_out.evaluated_type = lhs_type;
						break;
					case BinaryOp::ShlAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_compound_assign_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								lhs_type,
								decayed_rhs_type,
								peff::make_shared_with_control_block<U32TypeNameNode, AstNodeControlBlock<U32TypeNameNode>>(
									compile_env->allocator.get(),
									compile_env->allocator.get(),
									compile_env->get_document())
									.cast_to<TypeNameNode>(),
								ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::LSH,
								sld_index));
						result_out.evaluated_type = lhs_type;
						break;
					case BinaryOp::ShrAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_compound_assign_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								lhs_type,
								decayed_rhs_type,
								peff::make_shared_with_control_block<U32TypeNameNode, AstNodeControlBlock<U32TypeNameNode>>(
									compile_env->allocator.get(),
									compile_env->allocator.get(),
									compile_env->get_document())
									.cast_to<TypeNameNode>(),
								ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::RSH,
								sld_index));
						result_out.evaluated_type = lhs_type;
						break;
					case BinaryOp::Eq:
					case BinaryOp::StrictEq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::EQ,
								sld_index));
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					case BinaryOp::Neq:
					case BinaryOp::StrictNeq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::NEQ,
								sld_index));
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					case BinaryOp::Lt:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::LT,
								sld_index));
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					case BinaryOp::Gt:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::GT,
								sld_index));
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					case BinaryOp::LtEq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::LTEQ,
								sld_index));
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					case BinaryOp::GtEq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::GTEQ,
								sld_index));
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					case BinaryOp::Cmp:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::CMP,
								sld_index));
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					default:
						return CompilationError(expr->token_range, CompilationErrorKind::OperatorNotFound);
				}
				break;
			}
			case TypeNameKind::F32:
			case TypeNameKind::F64: {
				switch (expr->binary_op) {
					case BinaryOp::Add:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::ADD,
								sld_index));
						result_out.evaluated_type = decayed_lhs_type;
						break;
					case BinaryOp::Sub:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::SUB,
								sld_index));
						result_out.evaluated_type = decayed_lhs_type;
						break;
					case BinaryOp::Mul:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::MUL,
								sld_index));
						result_out.evaluated_type = decayed_lhs_type;
						break;
					case BinaryOp::Div:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::DIV,
								sld_index));
						result_out.evaluated_type = decayed_lhs_type;
						break;
					case BinaryOp::Mod:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::MOD,
								sld_index));
						result_out.evaluated_type = decayed_lhs_type;
						break;
					case BinaryOp::LOr:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_lor_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								bool_type,
								decayed_lhs_type,
								decayed_rhs_type,
								result_out,
								slake::Opcode::LOR,
								sld_index));
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					case BinaryOp::Assign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_assign_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								actual_lhs_type, actual_lhs_type,
								rhs_type,
								decayed_rhs_type,
								decayed_actual_lhs_type,
								ExprEvalPurpose::RValue,
								result_out,
								sld_index));
						result_out.evaluated_type = lhs_type;
						break;
					case BinaryOp::AddAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_compound_assign_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								lhs_type,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::ADD,
								sld_index));
						result_out.evaluated_type = lhs_type;
						break;
					case BinaryOp::SubAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_compound_assign_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								lhs_type,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::SUB,
								sld_index));
						result_out.evaluated_type = lhs_type;
						break;
					case BinaryOp::MulAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_compound_assign_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								lhs_type,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::MUL,
								sld_index));
						result_out.evaluated_type = lhs_type;
						break;
					case BinaryOp::DivAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_compound_assign_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								lhs_type,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::DIV,
								sld_index));
						result_out.evaluated_type = lhs_type;
						break;
					case BinaryOp::ModAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_compound_assign_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								lhs_type,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::MOD,
								sld_index));
						result_out.evaluated_type = lhs_type;
						break;
					case BinaryOp::ShlAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_compound_assign_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								lhs_type,
								decayed_rhs_type,
								peff::make_shared_with_control_block<U32TypeNameNode, AstNodeControlBlock<U32TypeNameNode>>(
									compile_env->allocator.get(),
									compile_env->allocator.get(),
									compile_env->get_document())
									.cast_to<TypeNameNode>(),
								ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::LSH,
								sld_index));
						result_out.evaluated_type = lhs_type;
						break;
					case BinaryOp::ShrAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_compound_assign_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								lhs_type,
								decayed_rhs_type,
								peff::make_shared_with_control_block<U32TypeNameNode, AstNodeControlBlock<U32TypeNameNode>>(
									compile_env->allocator.get(),
									compile_env->allocator.get(),
									compile_env->get_document())
									.cast_to<TypeNameNode>(),
								ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::RSH,
								sld_index));
						result_out.evaluated_type = lhs_type;
						break;
					case BinaryOp::Eq:
					case BinaryOp::StrictEq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::EQ,
								sld_index));
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					case BinaryOp::Neq:
					case BinaryOp::StrictNeq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::NEQ,
								sld_index));
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					case BinaryOp::Lt:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::LT,
								sld_index));
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					case BinaryOp::Gt:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::GT,
								sld_index));
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					case BinaryOp::LtEq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::LTEQ,
								sld_index));
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					case BinaryOp::GtEq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::GTEQ,
								sld_index));
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					case BinaryOp::Cmp:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::CMP,
								sld_index));
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					default:
						return CompilationError(expr->token_range, CompilationErrorKind::OperatorNotFound);
				}
				break;
			}
			case TypeNameKind::Bool: {
				switch (expr->binary_op) {
					case BinaryOp::And:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::AND,
								sld_index));
						result_out.evaluated_type = decayed_lhs_type;
						break;
					case BinaryOp::Or:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::OR,
								sld_index));
						result_out.evaluated_type = decayed_lhs_type;
						break;
					case BinaryOp::Xor:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::XOR,
								sld_index));
						result_out.evaluated_type = decayed_lhs_type;
						break;
					case BinaryOp::LAnd:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_land_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								bool_type,
								decayed_lhs_type,
								decayed_rhs_type,
								result_out,
								slake::Opcode::LAND,
								sld_index));
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					case BinaryOp::LOr:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_lor_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								bool_type,
								decayed_lhs_type,
								decayed_rhs_type,
								result_out,
								slake::Opcode::LOR,
								sld_index));
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					case BinaryOp::Assign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_assign_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								actual_lhs_type, actual_lhs_type,
								rhs_type,
								decayed_rhs_type,
								decayed_actual_lhs_type,
								ExprEvalPurpose::RValue,
								result_out,
								sld_index));
						result_out.evaluated_type = lhs_type;
						break;
					case BinaryOp::AndAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_compound_assign_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								lhs_type,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::AND,
								sld_index));
						result_out.evaluated_type = lhs_type;
						break;
					case BinaryOp::OrAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_compound_assign_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								lhs_type,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::OR,
								sld_index));
						result_out.evaluated_type = lhs_type;
						break;
					case BinaryOp::XorAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_compound_assign_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								lhs_type,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::XOR,
								sld_index));
						result_out.evaluated_type = lhs_type;
						break;
					case BinaryOp::Eq:
					case BinaryOp::StrictEq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::EQ,
								sld_index));
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					case BinaryOp::Neq:
					case BinaryOp::StrictNeq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::NEQ,
								sld_index));
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					default:
						return CompilationError(expr->token_range, CompilationErrorKind::OperatorNotFound);
				}
				break;
			}
			case TypeNameKind::Object: {
				switch (expr->binary_op) {
					case BinaryOp::StrictEq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::EQ,
								sld_index));
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					case BinaryOp::StrictNeq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::NEQ,
								sld_index));
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					default:
						return CompilationError(expr->token_range, CompilationErrorKind::OperatorNotFound);
				}
				break;
			}
			case TypeNameKind::Array: {
				switch (expr->binary_op) {
					case BinaryOp::StrictEq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::EQ,
								sld_index));
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					case BinaryOp::StrictNeq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_binary_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								decayed_actual_lhs_type, main_operation_type, ExprEvalPurpose::RValue,
								decayed_actual_rhs_type, main_operation_type, ExprEvalPurpose::RValue,
								result_out,
								slake::Opcode::NEQ,
								sld_index));
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					case BinaryOp::Subscript: {
						AstNodePtr<TypeNameNode> evaluated_type;
						if (!(evaluated_type = make_ast_node<RefTypeNameNode>(
								  compile_env->allocator.get(),
								  compile_env->allocator.get(),
								  compile_env->get_document(),
								  decayed_lhs_type.cast_to<ArrayTypeNameNode>()->element_type)
									.cast_to<TypeNameNode>())) {
							return gen_oom_comp_error();
						}
						peff::Option<CompilationError> e;

						switch (eval_purpose) {
							case ExprEvalPurpose::EvalType:
							case ExprEvalPurpose::EvalTypeActual:
								break;
							case ExprEvalPurpose::LValue: {
								CompileExprResult result(compile_env->allocator.get());

								uint32_t lhs_reg,
									rhs_reg;

								if ((e = _compile_or_cast_operand(compile_env, compilation_context, path_env, ExprEvalPurpose::RValue, lhs_type, expr->lhs, lhs_type, result))) {
									if (auto re = _compile_or_cast_operand(compile_env, compilation_context, path_env, ExprEvalPurpose::RValue, u32_type.cast_to<TypeNameNode>(), expr->rhs, rhs_type, result); re) {
										if (!compile_env->errors.push_back(std::move(*e))) {
											return gen_oom_comp_error();
										}
										e.reset();
										return re;
									} else {
										return e;
									}
								}
								lhs_reg = result.idx_result_reg_out;
								SLKC_RETURN_IF_COMP_ERROR(_compile_or_cast_operand(compile_env, compilation_context, path_env, ExprEvalPurpose::RValue, u32_type.cast_to<TypeNameNode>(), expr->rhs, rhs_type, result));
								rhs_reg = result.idx_result_reg_out;

								uint32_t output_reg;
								SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(output_reg));

								SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(
									sld_index,
									slake::Opcode::AT,
									output_reg,
									{ slake::Value(slake::ValueType::RegIndex, lhs_reg), slake::Value(slake::ValueType::RegIndex, rhs_reg) }));
								result_out.idx_result_reg_out = output_reg;

								break;
							}
							case ExprEvalPurpose::Stmt:
								SLKC_RETURN_IF_COMP_ERROR(compile_env->push_warning(
									CompilationWarning(expr->token_range, CompilationWarningKind::UnusedExprResult)));
								[[fallthrough]];
							case ExprEvalPurpose::RValue: {
								CompileExprResult result(compile_env->allocator.get());

								uint32_t lhs_reg,
									rhs_reg;

								if ((e = _compile_or_cast_operand(compile_env, compilation_context, path_env, ExprEvalPurpose::RValue, lhs_type, expr->lhs, lhs_type, result))) {
									if (auto re = _compile_or_cast_operand(compile_env, compilation_context, path_env, ExprEvalPurpose::RValue, u32_type.cast_to<TypeNameNode>(), expr->rhs, rhs_type, result); re) {
										if (!compile_env->errors.push_back(std::move(*e))) {
											return gen_oom_comp_error();
										}
										e.reset();
										return re;
									} else {
										return e;
									}
								}
								lhs_reg = result.idx_result_reg_out;
								SLKC_RETURN_IF_COMP_ERROR(_compile_or_cast_operand(compile_env, compilation_context, path_env, ExprEvalPurpose::RValue, u32_type.cast_to<TypeNameNode>(), expr->rhs, rhs_type, result));
								rhs_reg = result.idx_result_reg_out;

								uint32_t tmp_reg;

								SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(tmp_reg));

								SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(
									sld_index,
									slake::Opcode::AT,
									tmp_reg,
									{ slake::Value(slake::ValueType::RegIndex, lhs_reg), slake::Value(slake::ValueType::RegIndex, rhs_reg) }));

								uint32_t output_reg;
								SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(output_reg));
								SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(
									sld_index,
									slake::Opcode::LVALUE,
									output_reg,
									{ slake::Value(slake::ValueType::RegIndex, tmp_reg) }));
								result_out.idx_result_reg_out = output_reg;

								break;
							}
							case ExprEvalPurpose::Call:
								return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotCallable);
							case ExprEvalPurpose::Unpacking:
								return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotUnpackable);
						}

						result_out.evaluated_type = evaluated_type;
						break;
					}
					default:
						return CompilationError(expr->token_range, CompilationErrorKind::OperatorNotFound);
				}
				break;
			}
			case TypeNameKind::Custom: {
				switch (expr->binary_op) {
					case BinaryOp::Assign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compile_simple_assign_expr(
								compile_env,
								compilation_context,
								path_env,
								expr,
								eval_purpose,
								actual_lhs_type, actual_lhs_type,
								rhs_type,
								decayed_rhs_type,
								decayed_actual_lhs_type,
								ExprEvalPurpose::RValue,
								result_out,
								sld_index));
						result_out.evaluated_type = lhs_type;
						break;
					case BinaryOp::Add:
					case BinaryOp::Sub:
					case BinaryOp::Mul:
					case BinaryOp::Div:
					case BinaryOp::Mod:
					case BinaryOp::And:
					case BinaryOp::Or:
					case BinaryOp::Xor:
					case BinaryOp::LAnd:
					case BinaryOp::LOr:
					case BinaryOp::Shl:
					case BinaryOp::Shr:
					case BinaryOp::AddAssign:
					case BinaryOp::SubAssign:
					case BinaryOp::MulAssign:
					case BinaryOp::DivAssign:
					case BinaryOp::ModAssign:
					case BinaryOp::AndAssign:
					case BinaryOp::OrAssign:
					case BinaryOp::XorAssign:
					case BinaryOp::ShlAssign:
					case BinaryOp::ShrAssign:
					case BinaryOp::Eq:
					case BinaryOp::Neq:
					case BinaryOp::Lt:
					case BinaryOp::Gt:
					case BinaryOp::LtEq:
					case BinaryOp::GtEq:
					case BinaryOp::Cmp: {
						AstNodePtr<MemberNode> cls_node, operator_slot;

						SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(compile_env, decayed_lhs_type->document->shared_from_this(), decayed_lhs_type.cast_to<CustomTypeNameNode>(), cls_node));

						IdRefEntry e(compile_env->allocator.get());

						const char *operator_name = get_binary_operator_overloading_name(expr->binary_op);

						if (!operator_name)
							return CompilationError(expr->token_range, CompilationErrorKind::OperatorNotFound);

						if (!e.name.build(operator_name)) {
							return gen_oom_comp_error();
						}

						SLKC_RETURN_IF_COMP_ERROR(resolve_instance_member(compile_env, compile_env->get_document(), cls_node, e, operator_slot));

						if (!operator_slot)
							return CompilationError(
								expr->token_range,
								CompilationErrorKind::OperatorNotFound);

						if (operator_slot->get_ast_node_type() != AstNodeType::Fn)
							std::terminate();

						peff::DynArray<AstNodePtr<FnOverloadingNode>> matched_overloading_indices(compile_env->allocator.get());
						peff::DynArray<AstNodePtr<TypeNameNode>> operator_param_types(compile_env->allocator.get());

						if (!operator_param_types.push_back(AstNodePtr<TypeNameNode>(rhs_type))) {
							return gen_oom_comp_error();
						}

						SLKC_RETURN_IF_COMP_ERROR(determine_fn_overloading(compile_env, operator_slot.cast_to<FnNode>(), operator_param_types.data(), operator_param_types.size(), false, matched_overloading_indices));

						switch (matched_overloading_indices.size()) {
							case 0:
								return CompilationError(
									expr->token_range,
									CompilationErrorKind::OperatorNotFound);
							case 1:
								break;
							default:
								return CompilationError(
									expr->token_range,
									CompilationErrorKind::AmbiguousOperatorCall);
						}

						auto matched_overloading = matched_overloading_indices.back();

						bool accessible;
						SLKC_RETURN_IF_COMP_ERROR(is_member_accessible(compile_env, {}, matched_overloading.cast_to<MemberNode>(), accessible));
						if (!accessible)
							return CompilationError(
								expr->token_range,
								CompilationErrorKind::MemberIsNotAccessible);

						uint32_t lhs_reg;
						{
							CompileExprResult arg_result(compile_env->allocator.get());

							SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, expr->lhs, ExprEvalPurpose::RValue, decayed_lhs_type, arg_result));
							lhs_reg = arg_result.idx_result_reg_out;
						}

						uint32_t operator_reg;
						SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(operator_reg));
						if (matched_overloading->fn_flags & FN_VIRTUAL) {
							slake::HostObjectRef<slake::IdRefObject> id_ref_object;

							if (!(id_ref_object = slake::IdRefObject::alloc(compile_env->runtime))) {
								return gen_out_of_runtime_memory_comp_error();
							}

							slake::IdRefEntry e(compile_env->runtime->get_cur_gen_alloc());

							if (!e.name.build(operator_name)) {
								return gen_out_of_runtime_memory_comp_error();
							}

							if (!id_ref_object->entries.push_back(std::move(e))) {
								return gen_out_of_runtime_memory_comp_error();
							}
							SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, compilation_context, decayed_lhs_type, id_ref_object->overriden_type));

							id_ref_object->param_types = peff::DynArray<slake::TypeRef>(compile_env->runtime->get_cur_gen_alloc());

							if (!id_ref_object->param_types->resize(matched_overloading->params.size())) {
								return gen_oom_comp_error();
							}

							for (size_t i = 0; i < id_ref_object->param_types->size(); ++i) {
								SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, compilation_context, matched_overloading->params.at(i)->type, id_ref_object->param_types->at(i)));
							}

							SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::RLOAD, operator_reg, { slake::Value(slake::ValueType::RegIndex, lhs_reg), slake::Value(slake::Reference(id_ref_object.get())) }));
						} else {
							slake::HostObjectRef<slake::IdRefObject> id_ref_object;

							if (!(id_ref_object = slake::IdRefObject::alloc(compile_env->runtime))) {
								return gen_out_of_runtime_memory_comp_error();
							}

							IdRefPtr full_name;
							SLKC_RETURN_IF_COMP_ERROR(get_full_id_ref(compile_env->allocator.get(), operator_slot, full_name));

							SLKC_RETURN_IF_COMP_ERROR(compile_id_ref(compile_env, compilation_context, full_name->entries.data(), full_name->entries.size(), nullptr, 0, matched_overloading->fn_flags & FN_VARG, {}, id_ref_object));

							id_ref_object->param_types = peff::DynArray<slake::TypeRef>(compile_env->runtime->get_cur_gen_alloc());

							if (!id_ref_object->param_types->resize(matched_overloading->params.size())) {
								return gen_oom_comp_error();
							}

							for (size_t i = 0; i < id_ref_object->param_types->size(); ++i) {
								SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, compilation_context, matched_overloading->params.at(i)->type, id_ref_object->param_types->at(i)));
							}

							SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::LOAD, operator_reg, { slake::Value(slake::Reference(id_ref_object.get())) }));
						}

						uint32_t reg;
						SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(reg));
						{
							CompileExprResult arg_result(compile_env->allocator.get());

							bool b = false;
							SLKC_RETURN_IF_COMP_ERROR(is_lvalue_type(matched_overloading->params.at(0)->type, b));

							SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, expr->rhs, b ? ExprEvalPurpose::LValue : ExprEvalPurpose::RValue, matched_overloading->params.at(0)->type, arg_result));
							reg = arg_result.idx_result_reg_out;
						}

						SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::PUSHARG, UINT32_MAX, { slake::Value(slake::ValueType::RegIndex, reg) }));

						switch (eval_purpose) {
							case ExprEvalPurpose::EvalType:
							case ExprEvalPurpose::EvalTypeActual:
								break;
							case ExprEvalPurpose::Stmt:
								SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::MCALL, UINT32_MAX, { slake::Value(slake::ValueType::RegIndex, operator_reg), slake::Value(slake::ValueType::RegIndex, lhs_reg) }));
								break;
							case ExprEvalPurpose::LValue: {
								bool b = false;
								SLKC_RETURN_IF_COMP_ERROR(is_lvalue_type(matched_overloading->return_type, b));
								if (!b) {
									return CompilationError(expr->token_range, CompilationErrorKind::ExpectingLValueExpr);
								}

								uint32_t output_reg;
								SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(output_reg));
								SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::MCALL, output_reg, { slake::Value(slake::ValueType::RegIndex, operator_reg), slake::Value(slake::ValueType::RegIndex, lhs_reg) }));
								result_out.idx_result_reg_out = output_reg;
								break;
							}
							case ExprEvalPurpose::RValue:
							case ExprEvalPurpose::Call: {
								bool b = false;
								SLKC_RETURN_IF_COMP_ERROR(is_lvalue_type(matched_overloading->return_type, b));

								if (b) {
									uint32_t tmp_reg_index;
									SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(tmp_reg_index));

									SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::MCALL, tmp_reg_index, { slake::Value(slake::ValueType::RegIndex, operator_reg), slake::Value(slake::ValueType::RegIndex, lhs_reg) }));

									uint32_t output_reg;
									SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(output_reg));
									SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::LVALUE, output_reg, { slake::Value(slake::ValueType::RegIndex, tmp_reg_index) }));
									result_out.idx_result_reg_out = output_reg;
								} else {
									uint32_t output_reg;
									SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(output_reg));
									SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::MCALL, output_reg, { slake::Value(slake::ValueType::RegIndex, operator_reg), slake::Value(slake::ValueType::RegIndex, lhs_reg) }));
									result_out.idx_result_reg_out = output_reg;
								}

								break;
							}
							case ExprEvalPurpose::Unpacking:
								return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotUnpackable);
						}
						result_out.evaluated_type = matched_overloading->return_type;
						break;
					}
					case BinaryOp::StrictEq:
						switch (eval_purpose) {
							case ExprEvalPurpose::EvalType:
							case ExprEvalPurpose::EvalTypeActual:
								break;
							case ExprEvalPurpose::LValue:
								return CompilationError(expr->token_range, CompilationErrorKind::ExpectingLValueExpr);
							case ExprEvalPurpose::Stmt:
								SLKC_RETURN_IF_COMP_ERROR(compile_env->push_warning(
									CompilationWarning(expr->token_range, CompilationWarningKind::UnusedExprResult)));
								[[fallthrough]];
							case ExprEvalPurpose::RValue: {
								CompileExprResult lhs_result(compile_env->allocator.get()),
									rhs_result(compile_env->allocator.get());

								uint32_t lhs_reg,
									rhs_reg;

								peff::Option<CompilationError> e;

								if ((e = _compile_or_cast_operand(compile_env, compilation_context, path_env, ExprEvalPurpose::RValue, decayed_actual_lhs_type, expr->lhs, decayed_lhs_type, lhs_result))) {
									if (auto re = _compile_or_cast_operand(compile_env, compilation_context, path_env, ExprEvalPurpose::RValue, decayed_actual_lhs_type, expr->rhs, decayed_rhs_type, rhs_result); re) {
										if (!compile_env->errors.push_back(std::move(*e))) {
											return gen_oom_comp_error();
										}
										e.reset();
										return re;
									} else {
										return e;
									}
								}
								lhs_reg = lhs_result.idx_result_reg_out;
								SLKC_RETURN_IF_COMP_ERROR(_compile_or_cast_operand(compile_env, compilation_context, path_env, ExprEvalPurpose::RValue, decayed_actual_lhs_type, expr->rhs, decayed_rhs_type, rhs_result));
								rhs_reg = rhs_result.idx_result_reg_out;

								uint32_t output_reg;
								SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(output_reg));
								SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(
									sld_index,
									slake::Opcode::EQ,
									output_reg,
									{ slake::Value(slake::ValueType::RegIndex, lhs_reg), slake::Value(slake::ValueType::RegIndex, rhs_reg) }));
								result_out.idx_result_reg_out = output_reg;

								SLKC_RETURN_IF_COMP_ERROR(_update_equality_judgement_involved_states(lhs_type, rhs_type, lhs_result, rhs_result, result_out));
								break;
							}
							case ExprEvalPurpose::Call:
								return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotCallable);
							case ExprEvalPurpose::Unpacking:
								return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotUnpackable);
						}
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					case BinaryOp::StrictNeq:
						switch (eval_purpose) {
							case ExprEvalPurpose::EvalType:
							case ExprEvalPurpose::EvalTypeActual:
								break;
							case ExprEvalPurpose::LValue:
								return CompilationError(expr->token_range, CompilationErrorKind::ExpectingLValueExpr);
							case ExprEvalPurpose::Stmt:
								SLKC_RETURN_IF_COMP_ERROR(compile_env->push_warning(
									CompilationWarning(expr->token_range, CompilationWarningKind::UnusedExprResult)));
								[[fallthrough]];
							case ExprEvalPurpose::RValue: {
								CompileExprResult lhs_result(compile_env->allocator.get()),
									rhs_result(compile_env->allocator.get());

								uint32_t lhs_reg,
									rhs_reg;

								peff::Option<CompilationError> e;

								if ((e = _compile_or_cast_operand(compile_env, compilation_context, path_env, ExprEvalPurpose::RValue, decayed_actual_lhs_type, expr->lhs, decayed_lhs_type, lhs_result))) {
									if (auto re = _compile_or_cast_operand(compile_env, compilation_context, path_env, ExprEvalPurpose::RValue, decayed_actual_lhs_type, expr->rhs, decayed_rhs_type, rhs_result); re) {
										if (!compile_env->errors.push_back(std::move(*e))) {
											return gen_oom_comp_error();
										}
										e.reset();
										return re;
									} else {
										return e;
									}
								}
								lhs_reg = lhs_result.idx_result_reg_out;
								SLKC_RETURN_IF_COMP_ERROR(_compile_or_cast_operand(compile_env, compilation_context, path_env, ExprEvalPurpose::RValue, decayed_actual_lhs_type, expr->rhs, decayed_rhs_type, rhs_result));
								rhs_reg = rhs_result.idx_result_reg_out;

								uint32_t output_reg;
								SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(output_reg));
								SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(
									sld_index,
									slake::Opcode::NEQ,
									output_reg,
									{ slake::Value(slake::ValueType::RegIndex, lhs_reg), slake::Value(slake::ValueType::RegIndex, rhs_reg) }));
								result_out.idx_result_reg_out = output_reg;

								SLKC_RETURN_IF_COMP_ERROR(_update_inequality_judgement_involved_states(lhs_type, rhs_type, lhs_result, rhs_result, result_out));
								break;
							}
							case ExprEvalPurpose::Call:
								return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotCallable);
							case ExprEvalPurpose::Unpacking:
								return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotUnpackable);
						}
						result_out.evaluated_type = bool_type.cast_to<TypeNameNode>();
						break;
					default:
						return CompilationError(expr->token_range, CompilationErrorKind::OperatorNotFound);
				}
				break;
			}
			case TypeNameKind::Ref:
				std::terminate();
			default:
				return CompilationError(
					expr->token_range,
					CompilationErrorKind::OperatorNotFound);
		}
	}

rhs_to_lhs_custom_op_expr_resolved:
	return {};
}
