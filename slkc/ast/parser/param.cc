#include "../parser.h"

using namespace slkc;

SLKC_API ParseCoroutine Parser::parse_params(
	peff::Alloc *allocator,
	peff::DynArray<AstNodePtr<VarNode>> &params_out,
	bool &var_arg_out,
	peff::DynArray<size_t> &idx_comma_tokens_out,
	size_t &l_angle_bracket_index_out,
	size_t &r_angle_bracket_index_out) {
	peff::Option<SyntaxError> syntax_error;

	Token *l_parenthese_token = peek_token();

	l_angle_bracket_index_out = l_parenthese_token->index;

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((l_parenthese_token = peek_token()), TokenId::LParenthese));

	next_token();

	while (true) {
		if (TokenId next_token_id = peek_token()->token_id; (next_token_id == TokenId::RParenthese) || (next_token_id == TokenId::VarArg)) {
			break;
		}

		AstNodePtr<VarNode> param_node;

		if (!(param_node = make_ast_node<VarNode>(resource_allocator.get(), resource_allocator.get(), document))) {
			co_return gen_out_of_memory_syntax_error();
		}

		Token *name_token;

		SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((name_token = peek_token()), TokenId::Id));

		if (!param_node->name.build(name_token->source_text))
			co_return gen_out_of_memory_syntax_error();

		next_token();

		if (peek_token()->token_id == TokenId::Colon) {
			Token *colon_token = next_token();

			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(this->resource_allocator.get(), param_node->type));
		}

		if (!params_out.push_back(std::move(param_node)))
			co_return gen_out_of_memory_syntax_error();

		if (peek_token()->token_id != TokenId::Comma) {
			break;
		}

		Token *comma_token = next_token();

		if (!idx_comma_tokens_out.push_back(+comma_token->index))
			co_return gen_out_of_memory_syntax_error();
	}

	Token *var_arg_token;
	if ((var_arg_token = peek_token())->token_id == TokenId::VarArg) {
		next_token();
		var_arg_out = true;
	}

	Token *r_parenthese_token;

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese));

	next_token();

	r_angle_bracket_index_out = r_parenthese_token->index;

	co_return {};
}
