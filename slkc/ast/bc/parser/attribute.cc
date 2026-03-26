#include "../parser.h"

using namespace slkc;
using namespace slkc::bc;

SLKC_API peff::Option<SyntaxError> BCParser::parse_attribute(AstNodePtr<AttributeNode> &attribute_out) {
	peff::Option<SyntaxError> syntax_error;

	AstNodePtr<AttributeNode> attribute;

	if (!(attribute = make_ast_node<AttributeNode>(resource_allocator.get(), resource_allocator.get(), document))) {
		return gen_out_of_memory_syntax_error();
	}

	attribute_out = attribute;

	if ((syntax_error = parse_id_ref(attribute->attribute_name))) {
		return syntax_error;
	}

	{
		Token *l_parenthese_token;

		if ((l_parenthese_token = peek_token())->token_id == TokenId::LParenthese) {
			next_token();

			while (true) {
				if (peek_token()->token_id == TokenId::RParenthese) {
					break;
				}

				AstNodePtr<ExprNode> arg;

				Token *name_token;
				if ((syntax_error = expect_token((name_token = peek_token()), TokenId::Id)))
					return syntax_error;
				next_token();

				Token *assign_token;
				if ((syntax_error = expect_token((assign_token = peek_token()), TokenId::AssignOp)))
					return syntax_error;
				next_token();

				if (auto e = parse_comptime_expr(arg); e)
					return e;

				/*if (!args_out.push_back(std::move(arg)))
					return gen_out_of_memory_syntax_error();*/

				if (peek_token()->token_id != TokenId::Comma) {
					break;
				}

				Token *comma_token = next_token();
				/*if (!idx_comma_tokens_out.push_back(+comma_token->index))
					return gen_out_of_memory_syntax_error();*/
			}

			Token *r_parenthese_token;

			if ((syntax_error = expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese)))
				return syntax_error;

			next_token();
		}
	}

	Token *r_dbracket_token;
	if ((syntax_error = expect_token((r_dbracket_token = peek_token()), TokenId::RDBracket)))
		return syntax_error;

	next_token();

	if (Token *for_token = peek_token(); for_token->token_id == TokenId::ForKeyword) {
		next_token();

		if ((syntax_error = parse_type_name(attribute_out->applied_for)))
			return syntax_error;
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> BCParser::parse_attributes(peff::DynArray<AstNodePtr<AttributeNode>> &attributes_out) {
	peff::Option<SyntaxError> syntax_error;
	Token *current_token;

	for (;;) {
		if ((current_token = peek_token())->token_id != TokenId::LDBracket) {
			break;
		}

		next_token();

		AstNodePtr<AttributeNode> attribute;

		if ((syntax_error = parse_attribute(attribute)))
			return syntax_error;

		if (!attributes_out.push_back(std::move(attribute)))
			return gen_out_of_memory_syntax_error();
	}

	return {};
}
