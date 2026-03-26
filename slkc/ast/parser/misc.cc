#include "../parser.h"

using namespace slkc;

SLKC_API peff::Option<SyntaxError> Parser::lookahead_until(size_t num_token_ids, const TokenId token_ids[]) {
	// stub.
	return {};

	Token *token;
	while ((token->token_id != TokenId::End)) {
		for (size_t i = 0; i < num_token_ids; ++i) {
			if (token->token_id == token_ids[i]) {
				return {};
			}
		}
		token = next_token(true, true, true);
	}

	NoMatchingTokensFoundErrorExData ex_data(resource_allocator.get());

	for (size_t i = 0; i < num_token_ids; ++i) {
		TokenId copied_token_id = token_ids[i];
		if (!ex_data.expecting_token_ids.insert(std::move(copied_token_id)))
			return gen_out_of_memory_syntax_error();
	}

	return SyntaxError({ token->source_location.module_node, token->index }, std::move(ex_data));
}

SLKC_API Token *Parser::next_token(bool keep_new_line, bool keep_whitespace, bool keep_comment) {
	size_t &i = parse_context.idx_current_token;

	while (i < token_list.size()) {
		Token *current_token = token_list.at(i).get();
		current_token->index = i;

		switch (token_list.at(i)->token_id) {
			case TokenId::NewLine:
				if (keep_new_line) {
					parse_context.idx_prev_token = parse_context.idx_current_token;
					++i;
					return current_token;
				}
				break;
			case TokenId::Whitespace:
				if (keep_whitespace) {
					parse_context.idx_prev_token = parse_context.idx_current_token;
					++i;
					return current_token;
				}
				break;
			case TokenId::LineComment:
			case TokenId::BlockComment:
			case TokenId::DocumentationComment:
				if (keep_comment) {
					parse_context.idx_prev_token = parse_context.idx_current_token;
					++i;
					return current_token;
				}
				break;
			default:
				assert(is_valid_token(current_token->token_id));
				parse_context.idx_prev_token = parse_context.idx_current_token;
				++i;
				return current_token;
		}

		++i;
	}

	return token_list.back().get();
}

SLKC_API Token *Parser::peek_token(bool keep_new_line, bool keep_whitespace, bool keep_comment) {
	size_t i = parse_context.idx_current_token;

	while (i < token_list.size()) {
		Token *current_token = token_list.at(i).get();
		current_token->index = i;

		switch (current_token->token_id) {
			case TokenId::NewLine:
				if (keep_new_line)
					return current_token;
				break;
			case TokenId::Whitespace:
				if (keep_whitespace)
					return current_token;
				break;
			case TokenId::LineComment:
			case TokenId::BlockComment:
			case TokenId::DocumentationComment:
				if (keep_comment)
					return current_token;
				break;
			default:
				assert(is_valid_token(current_token->token_id));
				return current_token;
		}

		++i;
	}

	return token_list.back().get();
}

SLKC_API peff::Option<SyntaxError> Parser::split_shr_op_token() {
	switch (Token *token = peek_token(); token->token_id) {
		case TokenId::ShrOp: {
			token->token_id = TokenId::GtOp;
			token->source_text = token->source_text.substr(0, 1);
			token->source_location.end_position.column -= 1;

			OwnedTokenPtr extra_closing_token;
			if (!(extra_closing_token = OwnedTokenPtr(peff::alloc_and_construct<Token>(token->allocator.get(), ASTNODE_ALIGNMENT, token->allocator.get(), peff::WeakPtr<Document>(document))))) {
				return gen_out_of_memory_syntax_error();
			}

			extra_closing_token->token_id = TokenId::GtOp;
			extra_closing_token->source_location =
				SourceLocation{
					token->source_location.module_node,
					SourcePosition{ token->source_location.begin_position.line, token->source_location.begin_position.column + 1 },
					token->source_location.end_position
				};
			extra_closing_token->source_text = token->source_text.substr(1);

			if (!token_list.insert(parse_context.idx_current_token + 1, std::move(extra_closing_token))) {
				return gen_out_of_memory_syntax_error();
			}

			break;
		}
		default:;
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::split_rdbrackets_token() {
	switch (Token *token = peek_token(); token->token_id) {
		case TokenId::RDBracket: {
			token->token_id = TokenId::RBracket;
			token->source_text = token->source_text.substr(0, 1);
			token->source_location.end_position.column -= 1;

			OwnedTokenPtr extra_closing_token;
			if (!(extra_closing_token = OwnedTokenPtr(peff::alloc_and_construct<Token>(token->allocator.get(), ASTNODE_ALIGNMENT, token->allocator.get(), peff::WeakPtr<Document>(document))))) {
				return gen_out_of_memory_syntax_error();
			}

			extra_closing_token->token_id = TokenId::RBracket;
			extra_closing_token->source_location =
				SourceLocation{
					token->source_location.module_node,
					SourcePosition{ token->source_location.begin_position.line, token->source_location.begin_position.column + 1 },
					token->source_location.end_position
				};
			extra_closing_token->source_text = token->source_text.substr(1);

			if (!token_list.insert(parse_context.idx_current_token + 1, std::move(extra_closing_token))) {
				return gen_out_of_memory_syntax_error();
			}

			break;
		}
		default:;
	}

	return {};
}
