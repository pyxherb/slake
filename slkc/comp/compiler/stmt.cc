#include "../compiler.h"

using namespace slkc;

SLKC_API peff::Option<CompilationError> slkc::compile_var_def_stmt(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<VarDefStmtNode> s,
	uint32_t sld_index) {
	size_t ref_count = compile_env->get_document().strong_ref_num();
	for (auto &i : s->var_def_entries) {
		if (compilation_context->get_local_var_in_cur_level(i->name)) {
			SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(i->initial_value->token_range, CompilationErrorKind::LocalVarAlreadyExists)));
		} else {
			AstNodePtr<VarNode> new_var;

			uint32_t local_var_reg;

			SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(local_var_reg));

			SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_local_var(s->token_range, i->name, local_var_reg, i->type, new_var));

			if (i->initial_value) {
				uint32_t initial_value_reg;

				CompileExprResult result(compile_env->allocator.get());

				if (i->type) {
					new_var->type = i->type;

					{
						slake::TypeRef type;
						SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, compilation_context, new_var->type, type));

						SLKC_RETURN_IF_COMP_ERROR(
							compilation_context->emit_ins(
								sld_index, slake::Opcode::LVAR,
								local_var_reg,
								{ slake::Value(type) }));
					}

					AstNodePtr<TypeNameNode> expr_type;

					SLKC_RETURN_IF_COMP_ERROR(eval_ref_removed_expr_type(compile_env, compilation_context, path_env, i->initial_value, expr_type));

					bool same;

					SLKC_RETURN_IF_COMP_ERROR(is_same_type(i->type, expr_type, same));

					if (!same) {
						bool b = false;

						SLKC_RETURN_IF_COMP_ERROR(is_lvalue_type(i->type, b));

						SLKC_RETURN_IF_COMP_ERROR(_compile_or_cast_operand(compile_env, compilation_context, path_env, b ? ExprEvalPurpose::LValue : ExprEvalPurpose::RValue, i->type, i->initial_value, expr_type, result));
						initial_value_reg = result.idx_result_reg_out;
					} else {
						bool b = false;

						SLKC_RETURN_IF_COMP_ERROR(is_lvalue_type(i->type, b));

						if (b) {
							SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, i->initial_value, ExprEvalPurpose::LValue, i->type, result));
						} else {
							SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, i->initial_value, ExprEvalPurpose::RValue, i->type, result));
						}
						initial_value_reg = result.idx_result_reg_out;
					}
				} else {
					AstNodePtr<TypeNameNode> deduced_type;

					new_var->is_type_deduced_from_initial_value = true;

					SLKC_RETURN_IF_COMP_ERROR(eval_ref_removed_expr_type(compile_env, compilation_context, path_env, i->initial_value, deduced_type));

					if (!deduced_type) {
						return CompilationError(s->token_range, CompilationErrorKind::ErrorDeducingVarType);
					}

					new_var->type = deduced_type;

					{
						slake::TypeRef type;
						SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, compilation_context, new_var->type, type));

						SLKC_RETURN_IF_COMP_ERROR(
							compilation_context->emit_ins(
								sld_index, slake::Opcode::LVAR,
								local_var_reg,
								{ slake::Value(type) }));
					}

					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR(is_lvalue_type(deduced_type, b));

					if (b) {
						SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, i->initial_value, ExprEvalPurpose::LValue, {}, result));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, i->initial_value, ExprEvalPurpose::RValue, {}, result));
					}
					initial_value_reg = result.idx_result_reg_out;
				}

				SLKC_RETURN_IF_COMP_ERROR(
					compilation_context->emit_ins(
						sld_index, slake::Opcode::STORE,
						UINT32_MAX,
						{ slake::Value(slake::ValueType::RegIndex, local_var_reg), slake::Value(slake::ValueType::RegIndex, initial_value_reg) }));
			} else {
				if (!i->type) {
					return CompilationError(s->token_range, CompilationErrorKind::RequiresInitialValue);
				}

				new_var->type = i->type;

				{
					slake::TypeRef type;
					SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, compilation_context, new_var->type, type));

					SLKC_RETURN_IF_COMP_ERROR(
						compilation_context->emit_ins(
							sld_index, slake::Opcode::LVAR,
							local_var_reg,
							{ slake::Value(type) }));
				}
			}
		}
	}

	assert(compile_env->get_document().strong_ref_num() == ref_count);

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compile_for_stmt(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<ForStmtNode> s,
	uint32_t sld_index) {
	AstNodePtr<BoolTypeNameNode> bool_type;

	if (!(bool_type = make_ast_node<BoolTypeNameNode>(
			  compile_env->allocator.get(),
			  compile_env->allocator.get(),
			  compile_env->get_document()))) {
		return gen_oom_comp_error();
	}

	PrevBreakPointHolder break_point_holder(compilation_context);
	PrevContinuePointHolder continue_point_holder(compilation_context);

	SLKC_RETURN_IF_COMP_ERROR(
		compilation_context->emit_ins(
			sld_index, slake::Opcode::ENTER,
			UINT32_MAX,
			{}));
	SLKC_RETURN_IF_COMP_ERROR(compilation_context->enter_block());
	peff::ScopeGuard pop_block_context_guard([compilation_context]() noexcept {
		compilation_context->leave_block();
	});

	uint32_t body_label;
	SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_label(body_label));

	uint32_t normal_exit_label, break_label, cond_label, step_label;
	SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_label(normal_exit_label));
	SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_label(break_label));
	compilation_context->set_break_label(break_label, compilation_context->get_block_level());
	SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_label(cond_label));
	SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_label(step_label));
	compilation_context->set_continue_label(step_label, compilation_context->get_block_level());

	for (auto &i : s->var_def_entries) {
		if (compilation_context->get_local_var_in_cur_level(i->name)) {
			SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(i->initial_value->token_range, CompilationErrorKind::LocalVarAlreadyExists)));
		} else {
			AstNodePtr<VarNode> new_var;

			uint32_t local_var_reg;

			SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(local_var_reg));

			SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_local_var(s->token_range, i->name, local_var_reg, i->type, new_var));

			if (i->initial_value) {
				uint32_t initial_value_reg;

				CompileExprResult result(compile_env->allocator.get());

				if (i->type) {
					new_var->type = i->type;

					{
						slake::TypeRef type;
						SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, compilation_context, new_var->type, type));

						SLKC_RETURN_IF_COMP_ERROR(
							compilation_context->emit_ins(
								sld_index, slake::Opcode::LVAR,
								local_var_reg,
								{ slake::Value(type) }));
					}

					AstNodePtr<TypeNameNode> expr_type;

					SLKC_RETURN_IF_COMP_ERROR(eval_ref_removed_expr_type(compile_env, compilation_context, path_env, i->initial_value, expr_type));

					bool same;

					SLKC_RETURN_IF_COMP_ERROR(is_same_type(i->type, expr_type, same));

					if (!same) {
						bool b = false;

						SLKC_RETURN_IF_COMP_ERROR(is_lvalue_type(i->type, b));

						SLKC_RETURN_IF_COMP_ERROR(_compile_or_cast_operand(compile_env, compilation_context, path_env, b ? ExprEvalPurpose::LValue : ExprEvalPurpose::RValue, i->type, i->initial_value, expr_type, result));
						initial_value_reg = result.idx_result_reg_out;
					} else {
						bool b = false;

						SLKC_RETURN_IF_COMP_ERROR(is_lvalue_type(i->type, b));

						if (b) {
							SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, i->initial_value, ExprEvalPurpose::LValue, i->type, result));
						} else {
							SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, i->initial_value, ExprEvalPurpose::RValue, i->type, result));
						}
						initial_value_reg = result.idx_result_reg_out;
					}
				} else {
					AstNodePtr<TypeNameNode> deduced_type;

					new_var->is_type_deduced_from_initial_value = true;

					SLKC_RETURN_IF_COMP_ERROR(eval_ref_removed_expr_type(compile_env, compilation_context, path_env, i->initial_value, deduced_type));

					if (!deduced_type) {
						return CompilationError(s->token_range, CompilationErrorKind::ErrorDeducingVarType);
					}

					new_var->type = deduced_type;

					{
						slake::TypeRef type;
						SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, compilation_context, new_var->type, type));

						SLKC_RETURN_IF_COMP_ERROR(
							compilation_context->emit_ins(
								sld_index, slake::Opcode::LVAR,
								local_var_reg,
								{ slake::Value(type) }));
					}

					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR(is_lvalue_type(deduced_type, b));

					if (b) {
						SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, i->initial_value, ExprEvalPurpose::LValue, {}, result));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, i->initial_value, ExprEvalPurpose::RValue, {}, result));
					}
					initial_value_reg = result.idx_result_reg_out;
				}

				SLKC_RETURN_IF_COMP_ERROR(
					compilation_context->emit_ins(
						sld_index, slake::Opcode::STORE,
						UINT32_MAX,
						{ slake::Value(slake::ValueType::RegIndex, local_var_reg), slake::Value(slake::ValueType::RegIndex, initial_value_reg) }));
			} else {
				if (!i->type) {
					return CompilationError(s->token_range, CompilationErrorKind::RequiresInitialValue);
				}

				new_var->type = i->type;

				{
					slake::TypeRef type;
					SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, compilation_context, new_var->type, type));

					SLKC_RETURN_IF_COMP_ERROR(
						compilation_context->emit_ins(
							sld_index, slake::Opcode::LVAR,
							local_var_reg,
							{ slake::Value(type) }));
				}
			}
		}
	}

	if (s->cond) {
		CompileExprResult cond_result(compile_env->allocator.get()), step_result(compile_env->allocator.get());

		uint32_t condition_reg;

		bool is_same;

		AstNodePtr<TypeNameNode> expr_type;

		SLKC_RETURN_IF_COMP_ERROR(eval_ref_removed_expr_type(compile_env, compilation_context, path_env, s->cond, expr_type, bool_type.cast_to<TypeNameNode>()));

		SLKC_RETURN_IF_COMP_ERROR(is_same_type(bool_type.cast_to<TypeNameNode>(), expr_type, is_same));

		AstNodePtr<ExprNode> const_cond_expr;

		if (!is_same) {
			AstNodePtr<CastExprNode> cast_expr;

			SLKC_RETURN_IF_COMP_ERROR(gen_implicit_cast_expr(
				compile_env,
				s->cond,
				bool_type.cast_to<TypeNameNode>(),
				cast_expr));

			SLKC_RETURN_IF_COMP_ERROR(eval_const_expr(compile_env, compilation_context, path_env, cast_expr.cast_to<ExprNode>(), const_cond_expr));
		} else {
			SLKC_RETURN_IF_COMP_ERROR(eval_const_expr(compile_env, compilation_context, path_env, s->cond, const_cond_expr));
		}

		compilation_context->set_label_offset(cond_label, compilation_context->get_cur_ins_off());

		SLKC_RETURN_IF_COMP_ERROR(_compile_or_cast_operand(compile_env, compilation_context, path_env, ExprEvalPurpose::RValue, bool_type.cast_to<TypeNameNode>(), s->cond, expr_type, cond_result));
		condition_reg = cond_result.idx_result_reg_out;

		SLKC_RETURN_IF_COMP_ERROR(
			compilation_context->emit_ins(
				sld_index, slake::Opcode::BR,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::RegIndex, condition_reg), slake::Value(slake::ValueType::Label, body_label), slake::Value(slake::ValueType::Label, normal_exit_label) }));

		compilation_context->set_label_offset(body_label, compilation_context->get_cur_ins_off());

		{
			PathEnv body_path_env(compile_env->allocator.get());
			body_path_env.exec_possibility =
				const_cond_expr
					? (const_cond_expr.cast_to<BoolLiteralExprNode>()->data
							  ? PathPossibility::Must
							  : PathPossibility::Never)
					: PathPossibility::May;
			body_path_env.no_return_possibility =
				const_cond_expr
					? (const_cond_expr.cast_to<BoolLiteralExprNode>()->data
							  ? PathPossibility::Must
							  : PathPossibility::Never)
					: PathPossibility::May;
			body_path_env.break_possibility = PathPossibility::May;

			SLKC_RETURN_IF_COMP_ERROR(combine_path_env(body_path_env, cond_result.guard_path_env));

			SLKC_RETURN_IF_COMP_ERROR(compile_stmt(compile_env, compilation_context, path_env, s->body));

			compilation_context->set_label_offset(step_label, compilation_context->get_cur_ins_off());

			if (s->step) {
				PathEnv step_path_env(compile_env->allocator.get());
				SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, s->step, ExprEvalPurpose::Stmt, {}, step_result));
				SLKC_RETURN_IF_COMP_ERROR(combine_path_env(body_path_env, step_path_env));
			}

			SLKC_RETURN_IF_COMP_ERROR(combine_path_env(*path_env, body_path_env));
		}

		SLKC_RETURN_IF_COMP_ERROR(
			compilation_context->emit_ins(
				sld_index, slake::Opcode::JMP,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::Label, cond_label) }));
	} else {
		CompileExprResult step_result(compile_env->allocator.get());

		compilation_context->set_label_offset(cond_label, compilation_context->get_cur_ins_off());

		compilation_context->set_label_offset(body_label, compilation_context->get_cur_ins_off());

		{
			PathEnv body_path_env(compile_env->allocator.get());
			body_path_env.exec_possibility = PathPossibility::Must;
			body_path_env.no_return_possibility = PathPossibility::Must;
			body_path_env.break_possibility = PathPossibility::May;

			SLKC_RETURN_IF_COMP_ERROR(compile_stmt(compile_env, compilation_context, path_env, s->body));

			compilation_context->set_label_offset(step_label, compilation_context->get_cur_ins_off());

			if (s->step) {
				PathEnv step_path_env(compile_env->allocator.get());
				SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, s->step, ExprEvalPurpose::Stmt, {}, step_result));
				SLKC_RETURN_IF_COMP_ERROR(combine_path_env(body_path_env, step_path_env));
			}

			SLKC_RETURN_IF_COMP_ERROR(combine_path_env(*path_env, body_path_env));
		}

		SLKC_RETURN_IF_COMP_ERROR(
			compilation_context->emit_ins(
				sld_index, slake::Opcode::JMP,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::Label, cond_label) }));
	}

	compilation_context->set_label_offset(normal_exit_label, compilation_context->get_cur_ins_off());

	SLKC_RETURN_IF_COMP_ERROR(
		compilation_context->emit_ins(
			sld_index, slake::Opcode::LEAVE,
			UINT32_MAX,
			{ slake::Value((uint32_t)1) }));

	compilation_context->set_label_offset(break_label, compilation_context->get_cur_ins_off());

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compile_if_stmt(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<IfStmtNode> s,
	uint32_t sld_index) {
	AstNodePtr<BoolTypeNameNode> bool_type;

	if (!(bool_type = make_ast_node<BoolTypeNameNode>(
			  compile_env->allocator.get(),
			  compile_env->allocator.get(),
			  compile_env->get_document()))) {
		return gen_oom_comp_error();
	}

	uint32_t condition_reg;

	CompileExprResult cond_result(compile_env->allocator.get());

	AstNodePtr<TypeNameNode> expr_type;

	SLKC_RETURN_IF_COMP_ERROR(eval_ref_removed_expr_type(compile_env, compilation_context, path_env, s->cond, expr_type, bool_type.cast_to<TypeNameNode>()));

	PathEnv cond_env(compile_env->allocator.get());
	cond_env.set_parent(path_env);

	PathEnv true_path_env(compile_env->allocator.get());
	PathEnv false_path_env(compile_env->allocator.get());
	PathEnv *inner_path_env[2] = {
		&true_path_env,
		&false_path_env
	};

	SLKC_RETURN_IF_COMP_ERROR(_compile_or_cast_operand(compile_env, compilation_context, &cond_env, ExprEvalPurpose::RValue, bool_type.cast_to<TypeNameNode>(), s->cond, expr_type, cond_result));

	SLKC_RETURN_IF_COMP_ERROR(combine_path_env(true_path_env, cond_result.guard_path_env));

	condition_reg = cond_result.idx_result_reg_out;

	uint32_t end_label, true_label, false_label;
	SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_label(end_label));
	SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_label(true_label));
	SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_label(false_label));

	{
		bool is_same;

		SLKC_RETURN_IF_COMP_ERROR(is_same_type(bool_type.cast_to<TypeNameNode>(), expr_type, is_same));

		AstNodePtr<ExprNode> const_cond_expr;

		if (!is_same) {
			AstNodePtr<CastExprNode> cast_expr;

			SLKC_RETURN_IF_COMP_ERROR(gen_implicit_cast_expr(
				compile_env,
				s->cond,
				compile_env->cur_overloading->return_type,
				cast_expr));

			SLKC_RETURN_IF_COMP_ERROR(eval_const_expr(compile_env, compilation_context, path_env, cast_expr.cast_to<ExprNode>(), const_cond_expr));
		} else {
			SLKC_RETURN_IF_COMP_ERROR(eval_const_expr(compile_env, compilation_context, path_env, s->cond, const_cond_expr));
		}

		SLKC_RETURN_IF_COMP_ERROR(
			compilation_context->emit_ins(
				sld_index, slake::Opcode::BR,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::RegIndex, condition_reg), slake::Value(slake::ValueType::Label, true_label), slake::Value(slake::ValueType::Label, false_label) }));

		compilation_context->set_label_offset(true_label, compilation_context->get_cur_ins_off());

		{
			true_path_env.set_parent(&cond_env);
			true_path_env.exec_possibility =
				const_cond_expr
					? (const_cond_expr.cast_to<BoolLiteralExprNode>()->data
							  ? PathPossibility::Must
							  : PathPossibility::Never)
					: PathPossibility::May;
			SLKC_RETURN_IF_COMP_ERROR(compile_stmt(compile_env, compilation_context, &true_path_env, s->true_body));
		}

		SLKC_RETURN_IF_COMP_ERROR(
			compilation_context->emit_ins(
				sld_index, slake::Opcode::JMP,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::Label, end_label) }));

		compilation_context->set_label_offset(false_label, compilation_context->get_cur_ins_off());

		false_path_env.set_parent(path_env);
		false_path_env.exec_possibility =
			const_cond_expr
				? (const_cond_expr.cast_to<BoolLiteralExprNode>()->data
						  ? PathPossibility::Never
						  : PathPossibility::Must)
				: PathPossibility::May;
		if (s->false_body) {
			SLKC_RETURN_IF_COMP_ERROR(compile_stmt(compile_env, compilation_context, &false_path_env, s->false_body));
		}

		SLKC_RETURN_IF_COMP_ERROR(combine_parallel_path_env(compile_env->allocator.get(), compile_env, compilation_context, cond_env, inner_path_env, 2));

		compilation_context->set_label_offset(end_label, compilation_context->get_cur_ins_off());

		SLKC_RETURN_IF_COMP_ERROR(combine_path_env(*path_env, cond_env));
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compile_while_stmt(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<WhileStmtNode> s,
	uint32_t sld_index) {
	AstNodePtr<BoolTypeNameNode> bool_type;

	if (!(bool_type = make_ast_node<BoolTypeNameNode>(
			  compile_env->allocator.get(),
			  compile_env->allocator.get(),
			  compile_env->get_document()))) {
		return gen_oom_comp_error();
	}

	CompileExprResult cond_result(compile_env->allocator.get());

	AstNodePtr<TypeNameNode> expr_type;

	SLKC_RETURN_IF_COMP_ERROR(eval_ref_removed_expr_type(compile_env, compilation_context, path_env, s->cond, expr_type, bool_type.cast_to<TypeNameNode>()));

	PrevBreakPointHolder break_point_holder(compilation_context);
	PrevContinuePointHolder continue_point_holder(compilation_context);

	uint32_t condition_reg;

	uint32_t body_label;
	SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_label(body_label));

	uint32_t normal_exit_label, break_label, continue_label;
	SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_label(normal_exit_label));
	SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_label(break_label));
	compilation_context->set_break_label(break_label, compilation_context->get_block_level());
	SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_label(continue_label));
	compilation_context->set_continue_label(continue_label, compilation_context->get_block_level());

	bool is_same;

	SLKC_RETURN_IF_COMP_ERROR(is_same_type(bool_type.cast_to<TypeNameNode>(), expr_type, is_same));

	AstNodePtr<ExprNode> const_cond_expr;

	if (!is_same) {
		AstNodePtr<CastExprNode> cast_expr;

		SLKC_RETURN_IF_COMP_ERROR(gen_implicit_cast_expr(
			compile_env,
			s->cond,
			compile_env->cur_overloading->return_type,
			cast_expr));

		SLKC_RETURN_IF_COMP_ERROR(eval_const_expr(compile_env, compilation_context, path_env, cast_expr.cast_to<ExprNode>(), const_cond_expr));
	} else {
		SLKC_RETURN_IF_COMP_ERROR(eval_const_expr(compile_env, compilation_context, path_env, s->cond, const_cond_expr));
	}

	SLKC_RETURN_IF_COMP_ERROR(
		compilation_context->emit_ins(
			sld_index, slake::Opcode::ENTER,
			UINT32_MAX,
			{}));
	SLKC_RETURN_IF_COMP_ERROR(compilation_context->enter_block());
	peff::ScopeGuard pop_block_context_guard([compilation_context]() noexcept {
		compilation_context->leave_block();
	});

	{
		compilation_context->set_label_offset(continue_label, compilation_context->get_cur_ins_off());

		SLKC_RETURN_IF_COMP_ERROR(_compile_or_cast_operand(compile_env, compilation_context, path_env, ExprEvalPurpose::RValue, bool_type.cast_to<TypeNameNode>(), s->cond, expr_type, cond_result));

		condition_reg = cond_result.idx_result_reg_out;

		SLKC_RETURN_IF_COMP_ERROR(
			compilation_context->emit_ins(
				sld_index, slake::Opcode::BR,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::RegIndex, condition_reg),
					slake::Value(slake::ValueType::Label, body_label),
					slake::Value(slake::ValueType::Label, normal_exit_label) }));
	}

	compilation_context->set_label_offset(body_label, compilation_context->get_cur_ins_off());

	{
		PathEnv body_path_env(compile_env->allocator.get());
		body_path_env.set_parent(path_env);
		body_path_env.exec_possibility =
			const_cond_expr
				? (const_cond_expr.cast_to<BoolLiteralExprNode>()->data
						  ? PathPossibility::Must
						  : PathPossibility::Never)
				: PathPossibility::May;
		body_path_env.no_return_possibility =
			const_cond_expr
				? (const_cond_expr.cast_to<BoolLiteralExprNode>()->data
						  ? PathPossibility::Must
						  : PathPossibility::Never)
				: PathPossibility::May;
		body_path_env.break_possibility = PathPossibility::May;

		SLKC_RETURN_IF_COMP_ERROR(combine_path_env(body_path_env, cond_result.guard_path_env));

		SLKC_RETURN_IF_COMP_ERROR(try_compile_stmt(compile_env, compilation_context, &body_path_env, s->body));
		SLKC_RETURN_IF_COMP_ERROR(compile_stmt(compile_env, compilation_context, &body_path_env, s->body));

		SLKC_RETURN_IF_COMP_ERROR(combine_path_env(*path_env, body_path_env));
	}

	SLKC_RETURN_IF_COMP_ERROR(
		compilation_context->emit_ins(
			sld_index, slake::Opcode::JMP,
			UINT32_MAX,
			{ slake::Value(slake::ValueType::Label, continue_label) }));

	compilation_context->set_label_offset(normal_exit_label, compilation_context->get_cur_ins_off());
	SLKC_RETURN_IF_COMP_ERROR(
		compilation_context->emit_ins(
			sld_index, slake::Opcode::LEAVE,
			UINT32_MAX,
			{ slake::Value((uint32_t)1) }));
	compilation_context->set_label_offset(break_label, compilation_context->get_cur_ins_off());

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compile_do_while_stmt(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<DoWhileStmtNode> s,
	uint32_t sld_index) {
	AstNodePtr<BoolTypeNameNode> bool_type;

	if (!(bool_type = make_ast_node<BoolTypeNameNode>(
			  compile_env->allocator.get(),
			  compile_env->allocator.get(),
			  compile_env->get_document()))) {
		return gen_oom_comp_error();
	}

	CompileExprResult cond_result(compile_env->allocator.get());

	AstNodePtr<TypeNameNode> expr_type;

	SLKC_RETURN_IF_COMP_ERROR(eval_ref_removed_expr_type(compile_env, compilation_context, path_env, s->cond, expr_type, bool_type.cast_to<TypeNameNode>()));

	PrevBreakPointHolder break_point_holder(compilation_context);
	PrevContinuePointHolder continue_point_holder(compilation_context);

	uint32_t condition_reg;

	uint32_t body_label;
	SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_label(body_label));

	uint32_t normal_exit_label, break_label, continue_label;
	SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_label(normal_exit_label));
	SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_label(break_label));
	compilation_context->set_break_label(break_label, compilation_context->get_block_level());
	SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_label(continue_label));
	compilation_context->set_continue_label(continue_label, compilation_context->get_block_level());

	bool is_same;

	SLKC_RETURN_IF_COMP_ERROR(is_same_type(bool_type.cast_to<TypeNameNode>(), expr_type, is_same));

	AstNodePtr<ExprNode> const_cond_expr;

	if (!is_same) {
		AstNodePtr<CastExprNode> cast_expr;

		SLKC_RETURN_IF_COMP_ERROR(gen_implicit_cast_expr(
			compile_env,
			s->cond,
			compile_env->cur_overloading->return_type,
			cast_expr));

		SLKC_RETURN_IF_COMP_ERROR(eval_const_expr(compile_env, compilation_context, path_env, cast_expr.cast_to<ExprNode>(), const_cond_expr));
	} else {
		SLKC_RETURN_IF_COMP_ERROR(eval_const_expr(compile_env, compilation_context, path_env, s->cond, const_cond_expr));
	}

	SLKC_RETURN_IF_COMP_ERROR(
		compilation_context->emit_ins(
			sld_index, slake::Opcode::ENTER,
			UINT32_MAX,
			{}));
	SLKC_RETURN_IF_COMP_ERROR(compilation_context->enter_block());
	peff::ScopeGuard pop_block_context_guard([compilation_context]() noexcept {
		compilation_context->leave_block();
	});

	compilation_context->set_label_offset(body_label, compilation_context->get_cur_ins_off());

	PathEnv body_path_env(compile_env->allocator.get());
	// exec_possibility is defaultly set to must.
	body_path_env.no_return_possibility =
		const_cond_expr
			? (const_cond_expr.cast_to<BoolLiteralExprNode>()->data
					  ? PathPossibility::Must
					  : PathPossibility::Never)
			: PathPossibility::May;
	body_path_env.break_possibility = PathPossibility::May;

	SLKC_RETURN_IF_COMP_ERROR(combine_path_env(body_path_env, cond_result.guard_path_env));

	SLKC_RETURN_IF_COMP_ERROR(compile_stmt(compile_env, compilation_context, &body_path_env, s->body));

	SLKC_RETURN_IF_COMP_ERROR(combine_path_env(*path_env, body_path_env));

	compilation_context->set_label_offset(continue_label, compilation_context->get_cur_ins_off());

	SLKC_RETURN_IF_COMP_ERROR(_compile_or_cast_operand(compile_env, compilation_context, path_env, ExprEvalPurpose::RValue, bool_type.cast_to<TypeNameNode>(), s->cond, expr_type, cond_result));
	condition_reg = cond_result.idx_result_reg_out;

	SLKC_RETURN_IF_COMP_ERROR(
		compilation_context->emit_ins(
			sld_index, slake::Opcode::BR,
			UINT32_MAX,
			{ slake::Value(slake::ValueType::RegIndex, condition_reg),
				slake::Value(slake::ValueType::Label, body_label),
				slake::Value(slake::ValueType::Label, break_label) }));

	compilation_context->set_label_offset(normal_exit_label, compilation_context->get_cur_ins_off());
	SLKC_RETURN_IF_COMP_ERROR(
		compilation_context->emit_ins(
			sld_index, slake::Opcode::LEAVE,
			UINT32_MAX,
			{ slake::Value((uint32_t)1) }));
	compilation_context->set_label_offset(break_label, compilation_context->get_cur_ins_off());

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compile_with_stmt(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<WithStmtNode> s,
	uint32_t sld_index) {
	AstNodePtr<CustomTypeNameNode> tn;

	if (!(tn = make_ast_node<CustomTypeNameNode>(
			  compile_env->allocator.get(),
			  compile_env->allocator.get(),
			  compile_env->get_document())))
		return gen_oom_comp_error();

	peff::DynArray<AstNodePtr<GenericParamNode>> involved_generic_params(compile_env->allocator.get());

	AstNodePtr<MemberNode> m;

	for (auto &i : s->constraints) {
		tn->id_ref_ptr.reset();

		IdRefPtr id_ref_ptr(peff::alloc_and_construct<IdRef>(compile_env->allocator.get(), ASTNODE_ALIGNMENT, compile_env->allocator.get()));
		if (!id_ref_ptr)
			return gen_oom_comp_error();

		IdRefEntry e(compile_env->allocator.get());

		if (!e.name.build(i->generic_param_name))
			return gen_oom_comp_error();

		if (!id_ref_ptr->entries.push_back(std::move(e)))
			return gen_oom_comp_error();

		tn->token_range = s->token_range;

		tn->id_ref_ptr = std::move(id_ref_ptr);

		tn->context_node = to_weak_ptr(compile_env->this_node->this_type);

		SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(compile_env, compile_env->get_document(), tn, m));

		if (!m)
			return CompilationError(tn->token_range, CompilationErrorKind::ExpectingTypeName);

		if (AstNodeType::GenericParam != m->get_ast_node_type()) {
			return CompilationError(tn->token_range, CompilationErrorKind::TypeIsNotSubstitutable);
		}

		if (!involved_generic_params.push_back(m.cast_to<GenericParamNode>()))
			return gen_oom_comp_error();
	}

	peff::DynArray<GenericConstraintPtr> original_constraints(compile_env->allocator.get());

	for (auto &i : involved_generic_params) {
		GenericConstraintPtr constraint;

		if (!(constraint = duplicate_generic_constraint(compile_env->allocator.get(), i->generic_constraint.get())))
			return gen_oom_comp_error();

		if (!(original_constraints.push_back(std::move(constraint))))
			return gen_oom_comp_error();
	}

	peff::ScopeGuard restore_constraints_guard([&involved_generic_params, &original_constraints]() noexcept {
		for (size_t i = 0; i < involved_generic_params.size(); ++i) {
			involved_generic_params.at(i)->generic_constraint = std::move(original_constraints.at(i));
		}
	});

	for (size_t i = 0; i < involved_generic_params.size(); ++i) {
		// TODO: Replace the base type and add the additional implemented types.
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compile_switch_stmt(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<SwitchStmtNode> s,
	uint32_t sld_index) {
	PrevBreakPointHolder break_point_holder(compilation_context);

	AstNodePtr<TypeNameNode> match_type;

	SLKC_RETURN_IF_COMP_ERROR(eval_ref_removed_expr_type(compile_env, compilation_context, path_env, s->condition, match_type));

	if (!match_type)
		return CompilationError(s->condition->token_range, CompilationErrorKind::ErrorDeducingSwitchConditionType);

	uint32_t condition_reg;

	CompileExprResult result(compile_env->allocator.get());

	SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, s->condition, ExprEvalPurpose::RValue, {}, result));

	condition_reg = result.idx_result_reg_out;

	if (!result.evaluated_type)
		return CompilationError(s->condition->token_range, CompilationErrorKind::ErrorDeducingSwitchConditionType);

	AstNodePtr<TypeNameNode> condition_type = result.evaluated_type;

	uint32_t break_label;
	SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_label(break_label));
	compilation_context->set_break_label(break_label, compilation_context->get_block_level());

	uint32_t default_label;

	bool is_default_set = false;

	// Key = jump source, value = value register
	peff::Map<uint32_t, uint32_t> match_value_eval_labels(compile_env->allocator.get());

	// NOTE: Current switch statement is obsolete, we have to design a new one.
	peff::Set<AstNodePtr<ExprNode>> prev_case_conditions(compile_env->allocator.get());

	for (size_t i = 0; i < s->case_offsets.size(); ++i) {
		auto cur_case = s->body.at(s->case_offsets.at(i)).cast_to<CaseLabelStmtNode>();

		uint32_t eval_value_label;
		SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_label(eval_value_label));

		if (cur_case->is_default_case()) {
			if (is_default_set)
				return CompilationError(cur_case->condition->token_range, CompilationErrorKind::DuplicatedSwitchCaseBranch);

			default_label = eval_value_label;

			is_default_set = true;
		} else {
			AstNodePtr<ExprNode> result_expr;

			SLKC_RETURN_IF_COMP_ERROR(eval_const_expr(compile_env, compilation_context, path_env, cur_case->condition, result_expr));

			if (!result_expr) {
				return CompilationError(cur_case->condition->token_range, CompilationErrorKind::ErrorEvaluatingConstSwitchCaseCondition);
			}

			AstNodePtr<TypeNameNode> result_expr_type;

			SLKC_RETURN_IF_COMP_ERROR(eval_expr_type(compile_env, compilation_context, path_env, result_expr, result_expr_type));

			if (!result_expr_type) {
				return CompilationError(cur_case->condition->token_range, CompilationErrorKind::MismatchedSwitchCaseConditionType);
			}

			bool b;

			SLKC_RETURN_IF_COMP_ERROR(is_same_type(condition_type, result_expr_type, b));

			if (!b)
				return CompilationError(cur_case->condition->token_range, CompilationErrorKind::MismatchedSwitchCaseConditionType);

			for (auto &j : prev_case_conditions) {
				bool b;

				AstNodePtr<BinaryExprNode> ce;

				if (!(ce = make_ast_node<BinaryExprNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->get_document())))
					return gen_oom_comp_error();

				ce->binary_op = BinaryOp::Eq;

				ce->lhs = j;

				ce->rhs = result_expr;

				AstNodePtr<ExprNode> cmp_result;

				SLKC_RETURN_IF_COMP_ERROR(eval_const_expr(compile_env, compilation_context, path_env, ce.cast_to<ExprNode>(), cmp_result));

				assert(cmp_result);

				assert(cmp_result->expr_kind == ExprKind::Bool);

				if (cmp_result.cast_to<BoolLiteralExprNode>()->data) {
					return CompilationError(cur_case->condition->token_range, CompilationErrorKind::DuplicatedSwitchCaseBranch);
				}
			}

			AstNodePtr<BinaryExprNode> cmp_expr;

			if (!(cmp_expr = make_ast_node<BinaryExprNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->get_document()))) {
				return gen_oom_comp_error();
			}

			cmp_expr->binary_op = BinaryOp::Eq;

			cmp_expr->token_range = cur_case->condition->token_range;

			if (!(cmp_expr->lhs = make_ast_node<RegIndexExprNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->get_document(), condition_reg, condition_type).cast_to<ExprNode>())) {
				return gen_oom_comp_error();
			}
			cmp_expr->lhs->token_range = cur_case->condition->token_range;

			cmp_expr->rhs = cur_case->condition;

			AstNodePtr<BoolTypeNameNode> bool_type_name;

			if (!(bool_type_name = make_ast_node<BoolTypeNameNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->get_document())))
				return gen_oom_comp_error();

			uint32_t cmp_result_reg;
			{
				CompileExprResult cmp_expr_result(compile_env->allocator.get());

				SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, cmp_expr.cast_to<ExprNode>(), ExprEvalPurpose::RValue, bool_type_name.cast_to<TypeNameNode>(), cmp_expr_result));

				cmp_result_reg = cmp_expr_result.idx_result_reg_out;
			}

			if (!prev_case_conditions.insert(AstNodePtr<ExprNode>(result_expr)))
				return gen_oom_comp_error();

			uint32_t succeeded_value_label;
			SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_label(succeeded_value_label));

			SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::BR, UINT32_MAX, { slake::Value(slake::ValueType::RegIndex, cmp_result_reg), slake::Value(slake::ValueType::Label, eval_value_label), slake::Value(slake::ValueType::Label, succeeded_value_label) }));

			compilation_context->set_label_offset(succeeded_value_label, compilation_context->get_cur_ins_off());
		}

		match_value_eval_labels.insert(s->case_offsets.at(i), +eval_value_label);
	}

	if (is_default_set) {
		SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::JMP, UINT32_MAX, { slake::Value(slake::ValueType::Label, default_label) }));
	}

	for (size_t i = 0; i < s->body.size(); ++i) {
		AstNodePtr<StmtNode> cur_stmt = s->body.at(i);

		if (cur_stmt->stmt_kind == StmtKind::CaseLabel) {
			compilation_context->set_label_offset(match_value_eval_labels.at(i), compilation_context->get_cur_ins_off());
			continue;
		}

		SLKC_RETURN_IF_COMP_ERROR(compile_stmt(compile_env, compilation_context, path_env, cur_stmt));
	}

	compilation_context->set_label_offset(break_label, compilation_context->get_cur_ins_off());

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compile_expr_stmt(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<ExprStmtNode> s,
	uint32_t sld_index) {
	CompileExprResult result(compile_env->allocator.get());

	SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, s->expr, ExprEvalPurpose::Stmt, {}, result));

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compile_break_stmt(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<BreakStmtNode> s,
	uint32_t sld_index) {
	uint32_t break_label_id = compilation_context->get_break_label();
	if (compilation_context->get_break_label() == UINT32_MAX) {
		return CompilationError(s->token_range, CompilationErrorKind::InvalidBreakUsage);
	}
	uint32_t level = compilation_context->get_break_label_block_level();
	if (uint32_t cur_level = compilation_context->get_block_level();
		cur_level > level) {
		SLKC_RETURN_IF_COMP_ERROR(
			compilation_context->emit_ins(
				sld_index, slake::Opcode::LEAVE,
				UINT32_MAX,
				{ slake::Value((uint32_t)(cur_level - level)) }));
	}
	SLKC_RETURN_IF_COMP_ERROR(
		compilation_context->emit_ins(
			sld_index, slake::Opcode::JMP,
			UINT32_MAX,
			{ slake::Value(slake::ValueType::Label, break_label_id) }));

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compile_continue_stmt(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<ContinueStmtNode> s,
	uint32_t sld_index) {
	uint32_t break_label_id = compilation_context->get_continue_label();
	if (compilation_context->get_continue_label() == UINT32_MAX) {
		return CompilationError(s->token_range, CompilationErrorKind::InvalidBreakUsage);
	}
	uint32_t level = compilation_context->get_continue_label_block_level();
	if (uint32_t cur_level = compilation_context->get_block_level();
		cur_level > level) {
		SLKC_RETURN_IF_COMP_ERROR(
			compilation_context->emit_ins(
				sld_index, slake::Opcode::LEAVE,
				UINT32_MAX,
				{ slake::Value(cur_level - level) }));
	}
	SLKC_RETURN_IF_COMP_ERROR(
		compilation_context->emit_ins(
			sld_index, slake::Opcode::JMP,
			UINT32_MAX,
			{ slake::Value(slake::ValueType::Label, break_label_id) }));

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compile_return_stmt(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<ReturnStmtNode> s,
	uint32_t sld_index) {
	uint32_t reg;

	if (s->value) {
		if (compile_env->cur_overloading->return_type->tn_kind == TypeNameKind::Void)
			return CompilationError(s->value->token_range, CompilationErrorKind::ReturnValueTypeDoesNotMatch);
		CompileExprResult result(compile_env->allocator.get());

		AstNodePtr<TypeNameNode> value_type;

		SLKC_RETURN_IF_COMP_ERROR(eval_expr_type(compile_env, compilation_context, path_env, s->value, value_type));

		bool l;
		SLKC_RETURN_IF_COMP_ERROR(is_lvalue_type(compile_env->cur_overloading->return_type, l));

		bool same;
		SLKC_RETURN_IF_COMP_ERROR(is_same_type(value_type, compile_env->cur_overloading->return_type, same));
		if (!same) {
			bool convertible;
			SLKC_RETURN_IF_COMP_ERROR(is_convertible(value_type, compile_env->cur_overloading->return_type, true, convertible));
			if (convertible) {
				AstNodePtr<CastExprNode> cast_expr;

				SLKC_RETURN_IF_COMP_ERROR(gen_implicit_cast_expr(
					compile_env,
					s->value,
					compile_env->cur_overloading->return_type,
					cast_expr));

				SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, cast_expr.cast_to<ExprNode>(), ExprEvalPurpose::RValue, compile_env->cur_overloading->return_type, result));
			} else
				return CompilationError(s->value->token_range, CompilationErrorKind::ReturnValueTypeDoesNotMatch);
		} else {
			SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, s->value, l ? ExprEvalPurpose::LValue : ExprEvalPurpose::RValue, compile_env->cur_overloading->return_type, result));
		}
		reg = result.idx_result_reg_out;

		SLKC_RETURN_IF_COMP_ERROR(
			compilation_context->emit_ins(
				sld_index, slake::Opcode::RET,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::RegIndex, reg) }));
	} else {
		SLKC_RETURN_IF_COMP_ERROR(
			compilation_context->emit_ins(
				sld_index, slake::Opcode::RET,
				UINT32_MAX,
				{}));
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compile_yield_stmt(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<YieldStmtNode> s,
	uint32_t sld_index) {
	uint32_t reg;

	if (s->value) {
		CompileExprResult result(compile_env->allocator.get());

		SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, s->value, ExprEvalPurpose::RValue, compile_env->cur_overloading->return_type, result));
		reg = result.idx_result_reg_out;

		SLKC_RETURN_IF_COMP_ERROR(
			compilation_context->emit_ins(
				sld_index, slake::Opcode::YIELD,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::RegIndex, reg) }));
	} else {
		SLKC_RETURN_IF_COMP_ERROR(
			compilation_context->emit_ins(
				sld_index, slake::Opcode::YIELD,
				UINT32_MAX,
				{}));
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compile_stmt(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	const AstNodePtr<StmtNode> &stmt) {
	uint32_t sld_index;
	SLKC_RETURN_IF_COMP_ERROR(compilation_context->register_source_loc_desc(token_range_to_sld(stmt->token_range), sld_index));

	switch (stmt->stmt_kind) {
		case StmtKind::Expr: {
			SLKC_RETURN_IF_COMP_ERROR(compile_expr_stmt(compile_env, compilation_context, path_env, stmt.cast_to<ExprStmtNode>(), sld_index));
			break;
		}
		case StmtKind::VarDef: {
			SLKC_RETURN_IF_COMP_ERROR(compile_var_def_stmt(compile_env, compilation_context, path_env, stmt.cast_to<VarDefStmtNode>(), sld_index));
			break;
		}
		case StmtKind::Break: {
			SLKC_RETURN_IF_COMP_ERROR(compile_break_stmt(compile_env, compilation_context, path_env, stmt.cast_to<BreakStmtNode>(), sld_index));
			break;
		}
		case StmtKind::Continue: {
			SLKC_RETURN_IF_COMP_ERROR(compile_continue_stmt(compile_env, compilation_context, path_env, stmt.cast_to<ContinueStmtNode>(), sld_index));
			break;
		}
		case StmtKind::For: {
			SLKC_RETURN_IF_COMP_ERROR(compile_for_stmt(compile_env, compilation_context, path_env, stmt.cast_to<ForStmtNode>(), sld_index));
			break;
		}
		case StmtKind::While: {
			SLKC_RETURN_IF_COMP_ERROR(compile_while_stmt(compile_env, compilation_context, path_env, stmt.cast_to<WhileStmtNode>(), sld_index));
			break;
		}
		case StmtKind::DoWhile: {
			SLKC_RETURN_IF_COMP_ERROR(compile_do_while_stmt(compile_env, compilation_context, path_env, stmt.cast_to<DoWhileStmtNode>(), sld_index));
			break;
		}
		case StmtKind::Return: {
			SLKC_RETURN_IF_COMP_ERROR(compile_return_stmt(compile_env, compilation_context, path_env, stmt.cast_to<ReturnStmtNode>(), sld_index));
			break;
		}
		case StmtKind::Yield: {
			SLKC_RETURN_IF_COMP_ERROR(compile_yield_stmt(compile_env, compilation_context, path_env, stmt.cast_to<YieldStmtNode>(), sld_index));
			break;
		}
		case StmtKind::If:
			SLKC_RETURN_IF_COMP_ERROR(compile_if_stmt(compile_env, compilation_context, path_env, stmt.cast_to<IfStmtNode>(), sld_index));
			break;
		case StmtKind::With:
			SLKC_RETURN_IF_COMP_ERROR(compile_with_stmt(compile_env, compilation_context, path_env, stmt.cast_to<WithStmtNode>(), sld_index));
			break;
		case StmtKind::Switch:
			SLKC_RETURN_IF_COMP_ERROR(compile_switch_stmt(compile_env, compilation_context, path_env, stmt.cast_to<SwitchStmtNode>(), sld_index));
			break;
		case StmtKind::CaseLabel:
			return CompilationError(stmt->token_range, CompilationErrorKind::InvalidCaseLabelUsage);
		case StmtKind::CodeBlock: {
			AstNodePtr<CodeBlockStmtNode> s = stmt.cast_to<CodeBlockStmtNode>();

			SLKC_RETURN_IF_COMP_ERROR(
				compilation_context->emit_ins(
					sld_index, slake::Opcode::ENTER,
					UINT32_MAX,
					{}));
			SLKC_RETURN_IF_COMP_ERROR(compilation_context->enter_block());
			peff::ScopeGuard pop_block_context_guard([compilation_context]() noexcept {
				compilation_context->leave_block();
			});

			for (size_t i = 0; i < s->body.size(); ++i) {
				SLKC_RETURN_IF_COMP_ERROR(compile_stmt(compile_env, compilation_context, path_env, s->body.at(i)));
			}

			SLKC_RETURN_IF_COMP_ERROR(
				compilation_context->emit_ins(
					sld_index, slake::Opcode::LEAVE,
					UINT32_MAX,
					{ slake::Value((uint32_t)1) }));
			break;
		}
		case StmtKind::Goto:
			break;
		case StmtKind::Bad:
			break;
		default:
			std::terminate();
	}

	return {};
}

[[nodiscard]] SLKC_API peff::Option<CompilationError> slkc::try_compile_stmt(
	CompileEnv *compile_env,
	CompilationContext *parent_compilation_context,
	PathEnv *path_env,
	const AstNodePtr<StmtNode> &stmt) {
	compile_env->disable_messages();
	peff::ScopeGuard enable_messages_guard([compile_env]() noexcept {
		compile_env->enable_messages();
	});
	NormalCompilationContext tmp_ctxt(compile_env, parent_compilation_context);
	return compile_stmt(compile_env, &tmp_ctxt, path_env, stmt);
}
