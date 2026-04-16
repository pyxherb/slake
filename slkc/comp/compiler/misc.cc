#include "../compiler.h"

using namespace slkc;

SLKC_API peff::Option<CompilationError> slkc::get_succeeding_enum_value(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	AstNodePtr<TypeNameNode> base_type,
	AstNodePtr<ExprNode> last_value,
	AstNodePtr<ExprNode> &value_out) {
	if (last_value) {
		switch (base_type->tn_kind) {
			case TypeNameKind::I8:
				if (!(value_out = make_ast_node<I8LiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(),
						  compile_env->get_document(),
						  last_value.cast_to<I8LiteralExprNode>()->data + 1)
							.cast_to<ExprNode>()))
					return gen_oom_comp_error();
				break;
			case TypeNameKind::I16:
				if (!(value_out = make_ast_node<I16LiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(),
						  compile_env->get_document(),
						  last_value.cast_to<I16LiteralExprNode>()->data + 1)
							.cast_to<ExprNode>()))
					return gen_oom_comp_error();
				break;
			case TypeNameKind::I32:
				if (!(value_out = make_ast_node<I32LiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(),
						  compile_env->get_document(),
						  last_value.cast_to<I32LiteralExprNode>()->data + 1)
							.cast_to<ExprNode>()))
					return gen_oom_comp_error();
				break;
			case TypeNameKind::I64:
				if (!(value_out = make_ast_node<I64LiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(),
						  compile_env->get_document(),
						  last_value.cast_to<I64LiteralExprNode>()->data + 1)
							.cast_to<ExprNode>()))
					return gen_oom_comp_error();
				break;
			case TypeNameKind::U8:
				if (!(value_out = make_ast_node<U8LiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(),
						  compile_env->get_document(),
						  last_value.cast_to<U8LiteralExprNode>()->data + 1)
							.cast_to<ExprNode>()))
					return gen_oom_comp_error();
				break;
			case TypeNameKind::U16:
				if (!(value_out = make_ast_node<U16LiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(),
						  compile_env->get_document(),
						  last_value.cast_to<U16LiteralExprNode>()->data + 1)
							.cast_to<ExprNode>()))
					return gen_oom_comp_error();
				break;
			case TypeNameKind::U32:
				if (!(value_out = make_ast_node<U32LiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(),
						  compile_env->get_document(),
						  last_value.cast_to<U32LiteralExprNode>()->data + 1)
							.cast_to<ExprNode>()))
					return gen_oom_comp_error();
				break;
			case TypeNameKind::U64:
				if (!(value_out = make_ast_node<U8LiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(),
						  compile_env->get_document(),
						  last_value.cast_to<U64LiteralExprNode>()->data + 1)
							.cast_to<ExprNode>()))
					return gen_oom_comp_error();
				break;
			case TypeNameKind::F32:
				if (!(value_out = make_ast_node<F32LiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(),
						  compile_env->get_document(),
						  last_value.cast_to<F32LiteralExprNode>()->data + 1)
							.cast_to<ExprNode>()))
					return gen_oom_comp_error();
				break;
			case TypeNameKind::F64:
				if (!(value_out = make_ast_node<F64LiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(),
						  compile_env->get_document(),
						  last_value.cast_to<F64LiteralExprNode>()->data + 1)
							.cast_to<ExprNode>()))
					return gen_oom_comp_error();
				break;
			case TypeNameKind::Bool:
				if (!(value_out = make_ast_node<BoolLiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(),
						  compile_env->get_document(),
						  !last_value.cast_to<BoolLiteralExprNode>()->data)
							.cast_to<ExprNode>()))
					return gen_oom_comp_error();
				break;
			default:
				std::terminate();
		}
	} else {
		switch (base_type->tn_kind) {
			case TypeNameKind::I8:
				if (!(value_out = make_ast_node<I8LiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(),
						  compile_env->get_document(),
						  0)
							.cast_to<ExprNode>()))
					return gen_oom_comp_error();
				break;
			case TypeNameKind::I16:
				if (!(value_out = make_ast_node<I16LiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(),
						  compile_env->get_document(),
						  0)
							.cast_to<ExprNode>()))
					return gen_oom_comp_error();
				break;
			case TypeNameKind::I32:
				if (!(value_out = make_ast_node<I32LiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(),
						  compile_env->get_document(),
						  0)
							.cast_to<ExprNode>()))
					return gen_oom_comp_error();
				break;
			case TypeNameKind::I64:
				if (!(value_out = make_ast_node<I64LiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(),
						  compile_env->get_document(),
						  0)
							.cast_to<ExprNode>()))
					return gen_oom_comp_error();
				break;
			case TypeNameKind::U8:
				if (!(value_out = make_ast_node<U8LiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(),
						  compile_env->get_document(),
						  0)
							.cast_to<ExprNode>()))
					return gen_oom_comp_error();
				break;
			case TypeNameKind::U16:
				if (!(value_out = make_ast_node<U16LiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(),
						  compile_env->get_document(),
						  0)
							.cast_to<ExprNode>()))
					return gen_oom_comp_error();
				break;
			case TypeNameKind::U32:
				if (!(value_out = make_ast_node<U32LiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(),
						  compile_env->get_document(),
						  0)
							.cast_to<ExprNode>()))
					return gen_oom_comp_error();
				break;
			case TypeNameKind::U64:
				if (!(value_out = make_ast_node<U8LiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(),
						  compile_env->get_document(),
						  0)
							.cast_to<ExprNode>()))
					return gen_oom_comp_error();
				break;
			case TypeNameKind::F32:
				if (!(value_out = make_ast_node<F32LiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(),
						  compile_env->get_document(),
						  0)
							.cast_to<ExprNode>()))
					return gen_oom_comp_error();
				break;
			case TypeNameKind::F64:
				if (!(value_out = make_ast_node<F64LiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(),
						  compile_env->get_document(),
						  0)
							.cast_to<ExprNode>()))
					return gen_oom_comp_error();
				break;
			case TypeNameKind::Bool:
				if (!(value_out = make_ast_node<BoolLiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(),
						  compile_env->get_document(),
						  false)
							.cast_to<ExprNode>()))
					return gen_oom_comp_error();
				break;
			default:
				std::terminate();
		}
	}
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::fill_scoped_enum(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	AstNodePtr<ScopedEnumNode> enum_node) {
	if (!enum_node->underlying_type)
		return {};

	AstNodePtr<ExprNode> last_value;

	const size_t num_members = enum_node->scope->get_member_num();
	for (size_t i = 0; i < enum_node->scope->get_member_num(); ++i) {
		AstNodePtr<EnumItemNode> item = enum_node->scope->get_member(i).cast_to<EnumItemNode>();
		AstNodePtr<ExprNode> fill_value;

		assert(item->get_ast_node_type() == AstNodeType::EnumItem);

		if (item->enum_value) {
			bool is_same;

			AstNodePtr<TypeNameNode> tn;

			{
				PathEnv path_env(compile_env->allocator.get());
				SLKC_RETURN_IF_COMP_ERROR(eval_const_expr(compile_env, compilation_context, &path_env, item->enum_value, fill_value));
			}
			if (!fill_value)
				return CompilationError(item->enum_value->token_range, CompilationErrorKind::RequiresCompTimeExpr);

			{
				PathEnv root_path_env(compile_env->allocator.get());
				SLKC_RETURN_IF_COMP_ERROR(eval_expr_type(compile_env, compilation_context, &root_path_env, fill_value, tn, enum_node->underlying_type));
			}

			SLKC_RETURN_IF_COMP_ERROR(is_same_type(tn, enum_node->underlying_type, is_same));

			if (!is_same) {
				AstNodePtr<CastExprNode> cast_expr;

				if (!(cast_expr = make_ast_node<CastExprNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->get_document())))
					return gen_oom_comp_error();

				cast_expr->token_range = item->enum_value->token_range;
				cast_expr->source = fill_value;
				cast_expr->target_type = enum_node->underlying_type;

				{
					PathEnv path_env(compile_env->allocator.get());
					SLKC_RETURN_IF_COMP_ERROR(eval_const_expr(compile_env, compilation_context, &path_env, cast_expr.cast_to<ExprNode>(), fill_value));
				}

				if (!fill_value)
					return CompilationError(item->enum_value->token_range, CompilationErrorKind::IncompatibleInitialValueType);
			}
		} else {
			SLKC_RETURN_IF_COMP_ERROR(get_succeeding_enum_value(compile_env, compilation_context, enum_node->underlying_type, last_value, fill_value));
		}
		last_value = (item->filled_value = fill_value);
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::reindex_fn_params(
	CompileEnv *compile_env,
	AstNodePtr<FnOverloadingNode> fn) {
	for (size_t i = 0; i < fn->params.size(); ++i) {
		AstNodePtr<VarNode> &cur_param = fn->params.at(i);
		if (fn->param_indices.contains(cur_param->name)) {
			SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(cur_param->token_range, CompilationErrorKind::ParamAlreadyDefined)));
		}

		if (!fn->param_indices.insert(cur_param->name, +i)) {
			return gen_oom_comp_error();
		}
	}

	for (size_t i = 0; i < fn->scope->generic_params.size(); ++i) {
		AstNodePtr<GenericParamNode> &cur_param = fn->scope->generic_params.at(i);
		if (fn->scope->generic_param_indices.contains(cur_param->name)) {
			SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(cur_param->token_range, CompilationErrorKind::GenericParamAlreadyDefined)));
		}

		if (!fn->scope->generic_param_indices.insert(cur_param->name, +i)) {
			return gen_oom_comp_error();
		}
	}

	fn->is_params_indexed = true;
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::index_fn_params(
	CompileEnv *compile_env,
	AstNodePtr<FnOverloadingNode> fn) {
	if (fn->is_params_indexed) {
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(reindex_fn_params(compile_env, fn));
	fn->is_params_indexed = true;

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::gen_binary_op_expr(CompileEnv *compile_env, BinaryOp binary_op, AstNodePtr<ExprNode> lhs, AstNodePtr<ExprNode> rhs, TokenRange token_range, AstNodePtr<BinaryExprNode> &result_out) {
	AstNodePtr<BinaryExprNode> expr;

	if (!(expr = make_ast_node<BinaryExprNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->get_document()))) {
		return gen_oom_comp_error();
	}

	expr->binary_op = binary_op;
	expr->lhs = lhs;
	expr->rhs = rhs;

	expr->token_range = token_range;

	result_out = expr;

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::eval_const_binary_op_expr(CompileEnv *compile_env, CompilationContext *compilation_context, PathEnv *path_env, BinaryOp binary_op, AstNodePtr<ExprNode> lhs, AstNodePtr<ExprNode> rhs, AstNodePtr<ExprNode> &result_out) {
	AstNodePtr<BinaryExprNode> expr;

	if (!(expr = make_ast_node<BinaryExprNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->get_document())))
		return gen_oom_comp_error();

	expr->binary_op = binary_op;
	expr->lhs = lhs;
	expr->rhs = rhs;

	SLKC_RETURN_IF_COMP_ERROR(eval_const_expr(compile_env, compilation_context, path_env, expr.cast_to<ExprNode>(), result_out));

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::gen_implicit_cast_expr(CompileEnv *compile_env, AstNodePtr<ExprNode> source, AstNodePtr<TypeNameNode> dest_type, AstNodePtr<CastExprNode> &result_out) {
	AstNodePtr<CastExprNode> cast_expr;

	if (!(cast_expr = make_ast_node<CastExprNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->get_document())))
		return gen_oom_comp_error();

	cast_expr->source = source;
	cast_expr->target_type = dest_type;
	cast_expr->token_range = source->token_range;

	result_out = cast_expr;

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::implicit_convert_const_expr(CompileEnv *compile_env, CompilationContext *compilation_context, PathEnv *path_env, AstNodePtr<ExprNode> source, AstNodePtr<TypeNameNode> dest_type, AstNodePtr<ExprNode> &result_out) {
	AstNodePtr<CastExprNode> cast_expr;

	if (!(cast_expr = make_ast_node<CastExprNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->get_document())))
		return gen_oom_comp_error();

	cast_expr->source = source;
	cast_expr->target_type = dest_type;

	SLKC_RETURN_IF_COMP_ERROR(eval_const_expr(compile_env, compilation_context, path_env, cast_expr.cast_to<ExprNode>(), result_out));

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::is_fn_signature_same(
	AstNodePtr<VarNode> *l_params,
	AstNodePtr<VarNode> *r_params,
	size_t num_params,
	AstNodePtr<TypeNameNode> l_overriden_type,
	AstNodePtr<TypeNameNode> r_overriden_type,
	bool &whether_out) {
	if (l_overriden_type || r_overriden_type) {
		if ((!l_overriden_type) || (!r_overriden_type)) {
			whether_out = false;
			return {};
		}
		SLKC_RETURN_IF_COMP_ERROR(is_same_type(l_overriden_type, r_overriden_type, whether_out));
		if (!whether_out)
			return {};
	}

	for (size_t i = 0; i < num_params; ++i) {
		const AstNodePtr<VarNode> &l_cur_param = l_params[i], r_cur_param = r_params[i];

		SLKC_RETURN_IF_COMP_ERROR(is_same_type_in_signature(l_cur_param->type, r_cur_param->type, whether_out));

		if (!whether_out) {
			return {};
		}
	}

	whether_out = true;
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::is_fn_signature_duplicated(AstNodePtr<FnOverloadingNode> lhs, AstNodePtr<FnOverloadingNode> rhs, bool &whether_out) {
	if (lhs->params.size() != rhs->params.size()) {
		whether_out = false;
		return {};
	}
	if (lhs->scope->generic_params.size() != rhs->scope->generic_params.size()) {
		whether_out = false;
		return {};
	}
	if (lhs->is_varidic() != rhs->is_varidic()) {
		whether_out = false;
		return {};
	}
	return is_fn_signature_same(lhs->params.data(), rhs->params.data(), lhs->params.size(), lhs->overriden_type, rhs->overriden_type, whether_out);
}
