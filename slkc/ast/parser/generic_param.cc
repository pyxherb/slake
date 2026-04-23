#include "../parser.h"

using namespace slkc;

SLKC_API ParseCoroutine Parser::parse_generic_constraint(peff::Alloc *allocator, GenericConstraintPtr &constraint_out) {
	GenericConstraintPtr constraint(peff::alloc_and_construct<GenericConstraint>(resource_allocator.get(), alignof(GenericConstraint), resource_allocator.get()));

	if (!constraint) {
		co_return gen_oom_syntax_error();
	}

	peff::Option<SyntaxError> syntax_error;

	if (Token *l_parenthese_token = peek_token(); l_parenthese_token->token_id == TokenId::LParenthese) {
		next_token();

		SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(this->resource_allocator.get(), constraint->base_type));

		Token *r_parenthese_token;
		SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese));
		next_token();
	}

	if (Token *colon_token = peek_token(); colon_token->token_id == TokenId::Colon) {
		next_token();

		while (true) {
			AstNodePtr<TypeNameNode> tn;

			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(this->resource_allocator.get(), tn));

			if (!constraint->impl_types.push_back(std::move(tn))) {
				co_return gen_oom_syntax_error();
			}

			if (peek_token()->token_id != TokenId::AddOp) {
				break;
			}

			next_token();
		}
	}

	constraint_out = std::move(constraint);

	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_param_type_list_generic_constraint(peff::Alloc *allocator, ParamTypeListGenericConstraintPtr &constraint_out) {
	ParamTypeListGenericConstraintPtr constraint(peff::alloc_and_construct<ParamTypeListGenericConstraint>(resource_allocator.get(), alignof(ParamTypeListGenericConstraint), resource_allocator.get()));

	if (!constraint) {
		co_return gen_oom_syntax_error();
	}

	peff::Option<SyntaxError> syntax_error;

	if (Token *l_parenthese_token = peek_token(); l_parenthese_token->token_id == TokenId::LParenthese) {
		next_token();

		while (true) {
			AstNodePtr<TypeNameNode> tn;

			if (peek_token()->token_id == TokenId::VarArg) {
				next_token();

				constraint->has_var_arg = true;

				break;
			}

			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(this->resource_allocator.get(), tn));

			if (!constraint->arg_types.push_back(std::move(tn))) {
				co_return gen_oom_syntax_error();
			}

			if (peek_token()->token_id != TokenId::Comma) {
				break;
			}

			next_token();
		}

		Token *r_parenthese_token;
		SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese));
		next_token();
	}

	constraint_out = std::move(constraint);

	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_generic_params(
	peff::Alloc *allocator,
	peff::DynArray<AstNodePtr<GenericParamNode>> &generic_params_out,
	peff::DynArray<size_t> &idx_comma_tokens_out,
	size_t &l_angle_bracket_index_out,
	size_t &r_angle_bracket_index_out) {
	peff::Option<SyntaxError> syntax_error;

	Token *l_angle_bracket_token = peek_token();

	l_angle_bracket_index_out = l_angle_bracket_token->index;

	if (l_angle_bracket_token->token_id == TokenId::LtOp) {
		next_token();
		while (true) {
			AstNodePtr<GenericParamNode> generic_param_node;

			if (!(generic_param_node = make_ast_node<GenericParamNode>(resource_allocator.get(), resource_allocator.get(), get_document()))) {
				co_return gen_oom_syntax_error();
			}

			generic_param_node->outer = cur_parent.get();

			if (!generic_params_out.push_back(AstNodePtr<GenericParamNode>(generic_param_node)))
				co_return gen_oom_syntax_error();

			if (Token *varg_token = peek_token(); varg_token->token_id == TokenId::VarArg) {
				next_token();

				generic_param_node->is_param_type_list = true;

				peff::ScopeGuard set_token_range_guard([this, varg_token, &generic_param_node]() noexcept {
					if (generic_param_node) {
						generic_param_node->token_range = TokenRange{ get_document()->main_module, varg_token->index, parse_context.idx_prev_token };
					}
				});

				Token *name_token;

				SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((name_token = peek_token()), TokenId::Id));;

				if (!generic_param_node->name.build(name_token->source_text))
					co_return gen_oom_syntax_error();

				next_token();

				SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_param_type_list_generic_constraint(this->resource_allocator.get(), generic_param_node->param_type_list_generic_constraint));
			} else {
				Token *name_token;

				SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((name_token = peek_token()), TokenId::Id));;

				peff::ScopeGuard set_token_range_guard([this, name_token, &generic_param_node]() noexcept {
					if (generic_param_node) {
						generic_param_node->token_range = TokenRange{ get_document()->main_module, name_token->index, parse_context.idx_prev_token };
					}
				});

				if (!generic_param_node->name.build(name_token->source_text))
					co_return gen_oom_syntax_error();

				next_token();

				SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_generic_constraint(this->resource_allocator.get(), generic_param_node->generic_constraint));
			}

			if (peek_token()->token_id != TokenId::Comma) {
				break;
			}

			Token *comma_token = next_token();

			if (!idx_comma_tokens_out.push_back(+comma_token->index))
				co_return gen_oom_syntax_error();
		}

		Token *r_angle_bracket_token;

		SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((r_angle_bracket_token = peek_token()), TokenId::GtOp));

		next_token();

		r_angle_bracket_index_out = r_angle_bracket_token->index;
	}

	co_return {};
}
