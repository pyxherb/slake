#include "../parser.h"

using namespace slkc;

SLKC_API peff::Option<SyntaxError> Parser::parse_attribute(AstNodePtr<AttributeNode> &attribute_out) {
	peff::Option<SyntaxError> syntax_error;

	AstNodePtr<AttributeNode> attribute;

	if (!(attribute = make_ast_node<AttributeNode>(resource_allocator.get(), resource_allocator.get(), document))) {
		return gen_out_of_memory_syntax_error();
	}

	attribute_out = attribute;

	SLKC_RETURN_IF_PARSE_ERROR(parse_id_ref(attribute->attribute_name));

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
				SLKC_RETURN_IF_PARSE_ERROR(expect_token((name_token = peek_token()), TokenId::Id));
				next_token();

				Token *assign_token;
				SLKC_RETURN_IF_PARSE_ERROR(expect_token((assign_token = peek_token()), TokenId::AssignOp));
				next_token();

				if (auto e = parse_expr(0, arg); e)
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

			SLKC_RETURN_IF_PARSE_ERROR(expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese));

			next_token();
		}
	}

	Token *r_dbracket_token;
	SLKC_RETURN_IF_PARSE_ERROR(expect_token((r_dbracket_token = peek_token()), TokenId::RDBracket));

	next_token();

	if (Token *for_token = peek_token(); for_token->token_id == TokenId::ForKeyword) {
		next_token();

		SLKC_RETURN_IF_PARSE_ERROR(parse_type_name(attribute_out->applied_for));
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parse_attributes(peff::DynArray<AstNodePtr<AttributeNode>> &attributes_out) {
	peff::Option<SyntaxError> syntax_error;
	Token *current_token;

	for (;;) {
		if ((current_token = peek_token())->token_id != TokenId::LDBracket) {
			break;
		}

		next_token();

		AstNodePtr<AttributeNode> attribute;

		SLKC_RETURN_IF_PARSE_ERROR(parse_attribute(attribute));

		if (!attributes_out.push_back(std::move(attribute)))
			return gen_out_of_memory_syntax_error();
	}

	return {};
}
