#include "../parser.h"

using namespace slkc;
using namespace slkc::bc;

SLKC_API peff::Option<SyntaxError> BCParser::parse_params(
	peff::DynArray<AstNodePtr<VarNode>> &params_out,
	bool &var_arg_out,
	peff::DynArray<size_t> &idx_comma_tokens_out,
	size_t &l_angle_bracket_index_out,
	size_t &r_angle_bracket_index_out) {
	peff::Option<SyntaxError> syntax_error;

	Token *l_parenthese_token = peek_token();

	l_angle_bracket_index_out = l_parenthese_token->index;

	if ((syntax_error = expect_token((l_parenthese_token = peek_token()), TokenId::LParenthese))) {
		return syntax_error;
	}

	next_token();

	while (true) {
		if (TokenId next_token_id = peek_token()->token_id; (next_token_id == TokenId::RParenthese) || (next_token_id == TokenId::VarArg)) {
			break;
		}

		AstNodePtr<VarNode> param_node;

		if (!(param_node = make_ast_node<VarNode>(resource_allocator.get(), resource_allocator.get(), document))) {
			return gen_out_of_memory_syntax_error();
		}

		Token *name_token;

		if ((syntax_error = expect_token((name_token = peek_token()), TokenId::Id))) {
			return syntax_error;
		}

		if (!param_node->name.build(name_token->source_text))
			return gen_out_of_memory_syntax_error();

		next_token();

		if (peek_token()->token_id == TokenId::Colon) {
			Token *colon_token = next_token();

			if ((syntax_error = parse_type_name(param_node->type))) {
				return syntax_error;
			}
		}

		if (!params_out.push_back(std::move(param_node)))
			return gen_out_of_memory_syntax_error();

		if (peek_token()->token_id != TokenId::Comma) {
			break;
		}

		Token *comma_token = next_token();

		if (!idx_comma_tokens_out.push_back(+comma_token->index))
			return gen_out_of_memory_syntax_error();
	}

	Token *var_arg_token;
	if ((var_arg_token = peek_token())->token_id == TokenId::VarArg) {
		next_token();
		var_arg_out = true;
	}

	Token *r_parenthese_token;

	if ((syntax_error = expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese))) {
		return syntax_error;
	}

	next_token();

	r_angle_bracket_index_out = r_parenthese_token->index;

	return {};
}
