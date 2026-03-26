#include "../parser.h"

using namespace slkc;
using namespace slkc::bc;

SLKC_API peff::Option<SyntaxError> BCParser::parse_generic_constraint(GenericConstraintPtr &constraint_out) {
	GenericConstraintPtr constraint(peff::alloc_and_construct<GenericConstraint>(resource_allocator.get(), alignof(GenericConstraint), resource_allocator.get()));

	if (!constraint) {
		return gen_out_of_memory_syntax_error();
	}

	peff::Option<SyntaxError> syntax_error;

	if (Token *l_parenthese_token = peek_token(); l_parenthese_token->token_id == TokenId::LParenthese) {
		next_token();

		if ((syntax_error = parse_type_name(constraint->base_type))) {
			return syntax_error;
		}

		Token *r_parenthese_token;
		if ((syntax_error = expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese))) {
			return syntax_error;
		}
		next_token();
	}

	if (Token *colon_token = peek_token(); colon_token->token_id == TokenId::Colon) {
		next_token();

		while (true) {
			AstNodePtr<TypeNameNode> tn;

			if ((syntax_error = parse_type_name(tn))) {
				return syntax_error;
			}

			if (!constraint->impl_types.push_back(std::move(tn))) {
				return gen_out_of_memory_syntax_error();
			}

			if (peek_token()->token_id != TokenId::AddOp) {
				break;
			}

			next_token();
		}
	}

	constraint_out = std::move(constraint);

	return {};
}

SLKC_API peff::Option<SyntaxError> BCParser::parse_param_type_list_generic_constraint(ParamTypeListGenericConstraintPtr &constraint_out) {
	ParamTypeListGenericConstraintPtr constraint(peff::alloc_and_construct<ParamTypeListGenericConstraint>(resource_allocator.get(), alignof(ParamTypeListGenericConstraint), resource_allocator.get()));

	if (!constraint) {
		return gen_out_of_memory_syntax_error();
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

			if ((syntax_error = parse_type_name(tn))) {
				return syntax_error;
			}

			if (!constraint->arg_types.push_back(std::move(tn))) {
				return gen_out_of_memory_syntax_error();
			}

			if (peek_token()->token_id != TokenId::Comma) {
				break;
			}

			next_token();
		}

		Token *r_parenthese_token;
		if ((syntax_error = expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese))) {
			return syntax_error;
		}
		next_token();
	}

	constraint_out = std::move(constraint);

	return {};
}

SLKC_API peff::Option<SyntaxError> BCParser::parse_generic_params(
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

			if (!(generic_param_node = make_ast_node<GenericParamNode>(resource_allocator.get(), resource_allocator.get(), document))) {
				return gen_out_of_memory_syntax_error();
			}

			generic_param_node->parent = cur_parent.get();

			if (Token *l_parenthese_token = peek_token(); l_parenthese_token->token_id == TokenId::VarArg) {
				next_token();

				generic_param_node->is_param_type_list = true;

				Token *name_token;

				if ((syntax_error = expect_token((name_token = peek_token()), TokenId::Id))) {
					return syntax_error;
				};

				if (!generic_param_node->name.build(name_token->source_text))
					return gen_out_of_memory_syntax_error();

				next_token();

				{
					peff::ScopeGuard set_token_range_guard([this, name_token, &generic_param_node]() noexcept {
						if (generic_param_node) {
							generic_param_node->token_range = TokenRange{ document->main_module, name_token->index, parse_context.idx_prev_token };
						}
					});

					if ((syntax_error = parse_param_type_list_generic_constraint(generic_param_node->param_type_list_generic_constraint))) {
						return syntax_error;
					}

					if (!generic_params_out.push_back(std::move(generic_param_node)))
						return gen_out_of_memory_syntax_error();
				}
			} else {
				Token *name_token;

				if ((syntax_error = expect_token((name_token = peek_token()), TokenId::Id))) {
					return syntax_error;
				};

				if (!generic_param_node->name.build(name_token->source_text))
					return gen_out_of_memory_syntax_error();

				next_token();

				{
					peff::ScopeGuard set_token_range_guard([this, name_token, &generic_param_node]() noexcept {
						if (generic_param_node) {
							generic_param_node->token_range = TokenRange{ document->main_module, name_token->index, parse_context.idx_prev_token };
						}
					});

					if (peek_token()->token_id == TokenId::AsKeyword) {
						next_token();
						if ((syntax_error = parse_type_name(generic_param_node->input_type))) {
							return syntax_error;
						}
					} else {
						if ((syntax_error = parse_generic_constraint(generic_param_node->generic_constraint))) {
							return syntax_error;
						}
					}

					if (!generic_params_out.push_back(std::move(generic_param_node)))
						return gen_out_of_memory_syntax_error();
				}
			}

			if (peek_token()->token_id != TokenId::Comma) {
				break;
			}

			Token *comma_token = next_token();

			if (!idx_comma_tokens_out.push_back(+comma_token->index))
				return gen_out_of_memory_syntax_error();
		}

		Token *r_angle_bracket_token;

		if ((syntax_error = expect_token((r_angle_bracket_token = peek_token()), TokenId::GtOp))) {
			return syntax_error;
		}

		next_token();

		r_angle_bracket_index_out = r_angle_bracket_token->index;
	}

	return {};
}
