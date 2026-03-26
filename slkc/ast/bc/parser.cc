#define NOMINMAX
#include "parser.h"

using namespace slkc;
using namespace slkc::bc;

SLKC_API BCParser::BCParser(peff::SharedPtr<Document> document, TokenList &&token_list, peff::Alloc *resource_allocator) : Parser(document, std::move(token_list), resource_allocator) {
}

SLKC_API BCParser::~BCParser() {
}

SLKC_API peff::Option<SyntaxError> BCParser::parse_operator_name(std::string_view &name_out) {
	peff::Option<SyntaxError> syntax_error;

	Token *t = peek_token();

	switch (t->token_id) {
		case TokenId::LAndOp:
			name_out = "&&";
			next_token();
			break;
		case TokenId::LOrOp:
			name_out = "||";
			next_token();
			break;
		case TokenId::AddOp:
			name_out = "+";
			next_token();
			break;
		case TokenId::SubOp:
			name_out = "-";
			next_token();
			break;
		case TokenId::MulOp:
			name_out = "*";
			next_token();
			break;
		case TokenId::DivOp:
			name_out = "/";
			next_token();
			break;
		case TokenId::ModOp:
			name_out = "%";
			next_token();
			break;
		case TokenId::AndOp:
			name_out = "&";
			next_token();
			break;
		case TokenId::OrOp:
			name_out = "|";
			next_token();
			break;
		case TokenId::XorOp:
			name_out = "^";
			next_token();
			break;
		case TokenId::LNotOp:
			name_out = "!";
			next_token();
			break;
		case TokenId::NotOp:
			name_out = "~";
			next_token();
			break;
		case TokenId::AddAssignOp:
			name_out = "+=";
			next_token();
			break;
		case TokenId::SubAssignOp:
			name_out = "-=";
			next_token();
			break;
		case TokenId::MulAssignOp:
			name_out = "*=";
			next_token();
			break;
		case TokenId::DivAssignOp:
			name_out = "/=";
			next_token();
			break;
		case TokenId::ModAssignOp:
			name_out = "%=";
			next_token();
			break;
		case TokenId::AndAssignOp:
			name_out = "&=";
			next_token();
			break;
		case TokenId::OrAssignOp:
			name_out = "|=";
			next_token();
			break;
		case TokenId::XorAssignOp:
			name_out = "^=";
			next_token();
			break;
		case TokenId::ShlAssignOp:
			name_out = "<<=";
			next_token();
			break;
		case TokenId::ShrAssignOp:
			name_out = ">>=";
			next_token();
			break;
		case TokenId::EqOp:
			name_out = "==";
			next_token();
			break;
		case TokenId::NeqOp:
			name_out = "!=";
			next_token();
			break;
		case TokenId::ShlOp:
			name_out = "<<";
			next_token();
			break;
		case TokenId::ShrOp:
			name_out = ">>";
			next_token();
			break;
		case TokenId::LtEqOp:
			name_out = "<=";
			next_token();
			break;
		case TokenId::GtEqOp:
			name_out = ">=";
			next_token();
			break;
		case TokenId::CmpOp:
			name_out = "<=>";
			next_token();
			break;
		case TokenId::LParenthese:
			next_token();

			if ((syntax_error = expect_token(peek_token(), TokenId::RParenthese))) {
				return syntax_error;
			}

			name_out = "()";
			break;
		case TokenId::LBracket:
			next_token();

			if ((syntax_error = expect_token(peek_token(), TokenId::RBracket))) {
				return syntax_error;
			}

			name_out = "[]";
			break;
		case TokenId::NewKeyword:
			name_out = "new";
			next_token();
			break;
		case TokenId::DeleteKeyword:
			name_out = "delete";
			next_token();
			break;
		default:
			return SyntaxError(TokenRange{ document->main_module, t->index }, SyntaxErrorKind::ExpectingOperatorName);
	}
	return {};
}

SLKC_API peff::Option<SyntaxError> BCParser::parse_id_name(peff::String &name_out) {
	peff::Option<SyntaxError> syntax_error;
	Token *t = peek_token();

	switch (t->token_id) {
		case TokenId::Id:
			if (!name_out.build(t->source_text)) {
				return gen_out_of_memory_syntax_error();
			}
			next_token();
			break;
		default:
			return SyntaxError(TokenRange{ document->main_module, t->index }, SyntaxErrorKind::ExpectingId);
	}
	return {};
}

SLKC_API peff::Option<SyntaxError> BCParser::parse_id_ref(IdRefPtr &id_ref_out) {
	peff::Option<SyntaxError> syntax_error;
	IdRefPtr id_ref_ptr(peff::alloc_and_construct<IdRef>(resource_allocator.get(), ASTNODE_ALIGNMENT, resource_allocator.get()));
	if (!id_ref_ptr)
		return gen_out_of_memory_syntax_error();
	Token *t = peek_token();

	id_ref_ptr->token_range = TokenRange{ document->main_module, t->index };

	if (t->token_id == TokenId::ThisKeyword) {
		next_token();

		IdRefEntry entry(resource_allocator.get());
		peff::String id_text(resource_allocator.get());
		if (!id_text.build("this")) {
			return gen_out_of_memory_syntax_error();
		}

		entry.name = std::move(id_text);
		entry.name_token_index = t->index;

		if (!id_ref_ptr->entries.push_back(std::move(entry)))
			return gen_out_of_memory_syntax_error();

		if ((t = peek_token())->token_id != TokenId::Dot) {
			goto end;
		}

		next_token();

		entry.access_op_token_index = t->index;
		id_ref_ptr->token_range.end_index = t->index;
	} else if (t->token_id == TokenId::ScopeOp) {
		next_token();

		IdRefEntry entry(resource_allocator.get());
		peff::String id_text(resource_allocator.get());

		entry.name = std::move(id_text);

		entry.access_op_token_index = t->index;

		if (!id_ref_ptr->entries.push_back(std::move(entry)))
			return gen_out_of_memory_syntax_error();
	}

	for (;;) {
		if ((syntax_error = expect_token(t = peek_token(), TokenId::Id)))
			return syntax_error;

		next_token();

		IdRefEntry entry(resource_allocator.get());
		peff::String id_text(resource_allocator.get());
		if (!id_text.build(t->source_text)) {
			return gen_out_of_memory_syntax_error();
		}

		entry.name = std::move(id_text);
		entry.name_token_index = t->index;
		id_ref_ptr->token_range.end_index = t->index;

		size_t prev_end_index = t->index;
		ParseContext prev_parse_context = parse_context;
		if ((t = peek_token())->token_id == TokenId::LtOp) {
			next_token();

			for (;;) {
				AstNodePtr<AstNode> generic_arg;
				if ((syntax_error = parse_generic_arg(generic_arg)))
					goto generic_arg_parse_fail;

				if (!entry.generic_args.push_back(std::move(generic_arg))) {
					return gen_out_of_memory_syntax_error();
				}

				if ((t = peek_token())->token_id != TokenId::Comma) {
					break;
				}

				next_token();
			}

			if ((t = peek_token())->token_id != TokenId::GtOp) {
				goto generic_arg_parse_fail;
			}

			id_ref_ptr->token_range.end_index = t->index;

			next_token();
		}

		goto succeeded;

	generic_arg_parse_fail:
		id_ref_ptr->token_range.end_index = prev_end_index;
		parse_context = prev_parse_context;

		entry.generic_args.clear_and_shrink();

	succeeded:
		if (!id_ref_ptr->entries.push_back(std::move(entry)))
			return gen_out_of_memory_syntax_error();

		if ((t = peek_token())->token_id != TokenId::Dot) {
			break;
		}

		entry.access_op_token_index = t->index;
		id_ref_ptr->token_range.end_index = t->index;

		next_token();
	}

end:
	id_ref_out = std::move(id_ref_ptr);

	return {};
}

template <typename T>
SLAKE_FORCEINLINE peff::Option<SyntaxError> _parse_int(BCParser *parser, Token *token, bool is_negative, const std::string_view &body_view, T &data_out) {
	peff::Option<SyntaxError> syntax_error;

	bool overflow_warned = false;
	char c;
	T data = 0;
	size_t i = 0;

	switch (((IntTokenExtension *)token->ex_data.get())->token_type) {
		case IntTokenType::Decimal:
			while (i < body_view.size()) {
				c = body_view.at(i);
				if (is_negative) {
					if ((!overflow_warned) && (std::numeric_limits<T>::min() / 10 > data)) {
						if ((syntax_error = parser->push_literal_overflowed_error(token)))
							return syntax_error;
						overflow_warned = true;
					}
					data *= 10;
					data -= c - '0';
				} else {
					if ((!overflow_warned) && (std::numeric_limits<T>::max() / 10 < data)) {
						if ((syntax_error = parser->push_literal_overflowed_error(token)))
							return syntax_error;
						overflow_warned = true;
					}
					data *= 10;
					data += c - '0';
				}
				++i;
			}
			break;
		case IntTokenType::Hexadecimal:
			while (i < body_view.size()) {
				char c = body_view.at(i);
				if (is_negative) {
					if ((!overflow_warned) && (std::numeric_limits<T>::min() / 16 > data)) {
						if ((syntax_error = parser->push_literal_overflowed_error(token)))
							return syntax_error;
						overflow_warned = true;
					}
					data *= 16;
					switch (c) {
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9':
							data -= c - '0';
							break;
						case 'a':
						case 'b':
						case 'c':
						case 'd':
						case 'e':
						case 'f':
							data -= c - 'a' + 10;
							break;
						case 'A':
						case 'B':
						case 'C':
						case 'D':
						case 'E':
						case 'F':
							data -= c - 'A' + 10;
							break;
					}
				} else {
					if ((!overflow_warned) && (std::numeric_limits<T>::max() / 10 < data)) {
						if ((syntax_error = parser->push_literal_overflowed_error(token)))
							return syntax_error;
						overflow_warned = true;
					}
					data *= 16;
					switch (c) {
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9':
							data += c - '0';
							break;
						case 'a':
						case 'b':
						case 'c':
						case 'd':
						case 'e':
						case 'f':
							data += c - 'a' + 10;
							break;
						case 'A':
						case 'B':
						case 'C':
						case 'D':
						case 'E':
						case 'F':
							data += c - 'A' + 10;
							break;
					}
				}
				++i;
			}
			break;
		case IntTokenType::Octal:
			while (i < body_view.size()) {
				c = body_view.at(i);
				if (is_negative) {
					if ((!overflow_warned) && (std::numeric_limits<T>::min() / 8 > data)) {
						if ((syntax_error = parser->push_literal_overflowed_error(token)))
							return syntax_error;
						overflow_warned = true;
					}
					data *= 8;
					data -= c - '0';
				} else {
					if ((!overflow_warned) && (std::numeric_limits<T>::max() / 8 < data)) {
						if ((syntax_error = parser->push_literal_overflowed_error(token)))
							return syntax_error;
						overflow_warned = true;
					}
					data *= 8;
					data += c - '0';
				}
				++i;
			}
			break;
		case IntTokenType::Binary:
			while (i < body_view.size()) {
				c = body_view.at(i);
				if (is_negative) {
					if ((!overflow_warned) && ((std::numeric_limits<T>::min() >> 1) > data)) {
						if ((syntax_error = parser->push_literal_overflowed_error(token)))
							return syntax_error;
						overflow_warned = true;
					}
					data <<= 1;
					data -= c - '0';
				} else {
					if ((!overflow_warned) && ((std::numeric_limits<T>::max() >> 1) < data)) {
						if ((syntax_error = parser->push_literal_overflowed_error(token)))
							return syntax_error;
						overflow_warned = true;
					}
					data <<= 1;
					data += c - '0';
				}
				++i;
			}
			break;
		default:
			std::terminate();
	}

	data_out = data;

	return {};
}

SLKC_API peff::Option<SyntaxError> BCParser::parse_comptime_expr(AstNodePtr<ExprNode> &expr_out) {
	peff::Option<SyntaxError> syntax_error;
	Token *token = peek_token();

	peff::Deferred set_token_range_guard([this, &expr_out, token]() noexcept {
		if (expr_out)
			expr_out->token_range = TokenRange{ document->main_module, token->index, parse_context.idx_prev_token };
	});

	switch (token->token_id) {
		case TokenId::ThisKeyword:
		case TokenId::ScopeOp:
		case TokenId::Id: {
			IdRefPtr id_ref_ptr;
			if ((syntax_error = parse_id_ref(id_ref_ptr)))
				goto gen_bad_expr;
			if (!(expr_out = make_ast_node<IdRefExprNode>(resource_allocator.get(), resource_allocator.get(), document, std::move(id_ref_ptr)).cast_to<ExprNode>()))
				return gen_out_of_memory_syntax_error();
			break;
		}
		case TokenId::I8TypeName:
		case TokenId::I16TypeName:
		case TokenId::I32TypeName:
		case TokenId::I64TypeName:
		case TokenId::U8TypeName:
		case TokenId::U16TypeName:
		case TokenId::U32TypeName:
		case TokenId::U64TypeName:
		case TokenId::F32TypeName:
		case TokenId::F64TypeName:
		case TokenId::StringTypeName:
		case TokenId::BoolTypeName:
		case TokenId::ObjectTypeName:
		case TokenId::AnyTypeName:
		case TokenId::VoidTypeName:
		case TokenId::SIMDTypeName: {
			AstNodePtr<TypeNameNode> tn;
			if ((syntax_error = parse_type_name(tn)))
				goto gen_bad_expr;
			if (!(expr_out = make_ast_node<TypeNameExprNode>(resource_allocator.get(), resource_allocator.get(), document, tn).cast_to<ExprNode>()))
				return gen_out_of_memory_syntax_error();
			break;
		}
		case TokenId::TypenameKeyword: {
			next_token();
			AstNodePtr<TypeNameNode> tn;
			if ((syntax_error = parse_type_name(tn)))
				goto gen_bad_expr;
			if (!(expr_out = make_ast_node<TypeNameExprNode>(resource_allocator.get(), resource_allocator.get(), document, tn).cast_to<ExprNode>()))
				return gen_out_of_memory_syntax_error();
			break;
		}
		case TokenId::I8Literal: {
			next_token();

			int8_t data = 0;
			bool is_negative = false;

			if (token->source_text.at(0) == '-')
				is_negative = true;

			std::string_view body_view = token->source_text.substr(is_negative ? 1 : 0, token->source_text.size() - (is_negative ? 1 : 0) - (sizeof("i8") - 1));

			if ((syntax_error = _parse_int<int8_t>(this, token, is_negative, body_view, data)))
				return syntax_error;

			if (!(expr_out = make_ast_node<I8LiteralExprNode>(
					  resource_allocator.get(), resource_allocator.get(), document,
					  data)
						.cast_to<ExprNode>()))
				return gen_out_of_memory_syntax_error();
			break;
		}
		case TokenId::I16Literal: {
			next_token();

			int16_t data = 0;
			bool is_negative = false;

			if (token->source_text.at(0) == '-')
				is_negative = true;

			std::string_view body_view = token->source_text.substr(is_negative ? 1 : 0, token->source_text.size() - (is_negative ? 1 : 0) - (sizeof("i16") - 1));

			if ((syntax_error = _parse_int<int16_t>(this, token, is_negative, body_view, data)))
				return syntax_error;

			if (!(expr_out = make_ast_node<I16LiteralExprNode>(
					  resource_allocator.get(), resource_allocator.get(), document,
					  data)
						.cast_to<ExprNode>()))
				return gen_out_of_memory_syntax_error();
			break;
		}
		case TokenId::I32Literal: {
			next_token();

			int32_t data = 0;
			bool is_negative = false;

			if (token->source_text.at(0) == '-')
				is_negative = true;

			std::string_view body_view = token->source_text.substr(is_negative ? 1 : 0, token->source_text.size() - (is_negative ? 1 : 0));

			if ((syntax_error = _parse_int<int32_t>(this, token, is_negative, body_view, data)))
				return syntax_error;

			if (!(expr_out = make_ast_node<I32LiteralExprNode>(
					  resource_allocator.get(), resource_allocator.get(), document,
					  data)
						.cast_to<ExprNode>()))
				return gen_out_of_memory_syntax_error();
			break;
		}
		case TokenId::I64Literal: {
			next_token();

			int64_t data = 0;
			bool is_negative = false;

			if (token->source_text.at(0) == '-')
				is_negative = true;

			std::string_view body_view = token->source_text.substr(is_negative ? 1 : 0, token->source_text.size() - (is_negative ? 1 : 0) - (sizeof("L") - 1));

			if ((syntax_error = _parse_int<int64_t>(this, token, is_negative, body_view, data)))
				return syntax_error;

			if (!(expr_out = make_ast_node<I64LiteralExprNode>(
					  resource_allocator.get(), resource_allocator.get(), document,
					  data)
						.cast_to<ExprNode>()))
				return gen_out_of_memory_syntax_error();
			break;
		}
		case TokenId::U8Literal: {
			next_token();

			uint8_t data = 0;
			bool is_negative = false;

			if (token->source_text.at(0) == '-')
				is_negative = true;

			std::string_view body_view = token->source_text.substr(is_negative ? 1 : 0, token->source_text.size() - (is_negative ? 1 : 0) - (sizeof("Ui8") - 1));

			if ((syntax_error = _parse_int<uint8_t>(this, token, is_negative, body_view, data)))
				return syntax_error;

			if (!(expr_out = make_ast_node<U8LiteralExprNode>(
					  resource_allocator.get(), resource_allocator.get(), document,
					  data)
						.cast_to<ExprNode>()))
				return gen_out_of_memory_syntax_error();
			break;
		}
		case TokenId::U16Literal: {
			next_token();

			uint16_t data = 0;
			bool is_negative = false;

			if (token->source_text.at(0) == '-')
				is_negative = true;

			std::string_view body_view = token->source_text.substr(is_negative ? 1 : 0, token->source_text.size() - (is_negative ? 1 : 0) - (sizeof("Ui16") - 1));

			if ((syntax_error = _parse_int<uint16_t>(this, token, is_negative, body_view, data)))
				return syntax_error;

			if (!(expr_out = make_ast_node<U16LiteralExprNode>(
					  resource_allocator.get(), resource_allocator.get(), document,
					  data)
						.cast_to<ExprNode>()))
				return gen_out_of_memory_syntax_error();
			break;
		}
		case TokenId::U32Literal: {
			next_token();

			uint32_t data = 0;
			bool is_negative = false;

			if (token->source_text.at(0) == '-')
				is_negative = true;

			std::string_view body_view = token->source_text.substr(is_negative ? 1 : 0, token->source_text.size() - (is_negative ? 1 : 0) - (sizeof("U") - 1));

			if ((syntax_error = _parse_int<uint32_t>(this, token, is_negative, body_view, data)))
				return syntax_error;

			if (!(expr_out = make_ast_node<U32LiteralExprNode>(
					  resource_allocator.get(), resource_allocator.get(), document,
					  data)
						.cast_to<ExprNode>()))
				return gen_out_of_memory_syntax_error();
			break;
		}
		case TokenId::U64Literal: {
			next_token();

			uint64_t data = 0;
			bool is_negative = false;

			if (token->source_text.at(0) == '-')
				is_negative = true;

			std::string_view body_view = token->source_text.substr(is_negative ? 1 : 0, token->source_text.size() - (is_negative ? 1 : 0) - (sizeof("UL") - 1));

			if ((syntax_error = _parse_int<uint64_t>(this, token, is_negative, body_view, data)))
				return syntax_error;

			if (!(expr_out = make_ast_node<U64LiteralExprNode>(
					  resource_allocator.get(), resource_allocator.get(), document,
					  data)
						.cast_to<ExprNode>()))
				return gen_out_of_memory_syntax_error();
			break;
		}
		case TokenId::StringLiteral: {
			next_token();
			peff::String s(resource_allocator.get());

			if (!s.build(((StringTokenExtension *)token->ex_data.get())->data)) {
				return gen_out_of_memory_syntax_error();
			}

			if (!(expr_out = make_ast_node<StringLiteralExprNode>(
					  resource_allocator.get(), resource_allocator.get(), document,
					  std::move(s))
						.cast_to<ExprNode>()))
				return gen_out_of_memory_syntax_error();
			break;
		}
		case TokenId::F32Literal: {
			next_token();

			if (!(expr_out = peff::make_shared_with_control_block<F32LiteralExprNode, AstNodeControlBlock<F32LiteralExprNode>>(
					  resource_allocator.get(), resource_allocator.get(), document,
					  strtof(token->source_text.data(), nullptr))
						.cast_to<ExprNode>()))
				return gen_out_of_memory_syntax_error();
			break;
		}
		case TokenId::F64Literal: {
			next_token();
			if (!(expr_out = peff::make_shared_with_control_block<F64LiteralExprNode, AstNodeControlBlock<F64LiteralExprNode>>(
					  resource_allocator.get(), resource_allocator.get(), document,
					  strtod(token->source_text.data(), nullptr))
						.cast_to<ExprNode>()))
				return gen_out_of_memory_syntax_error();
			break;
		}
		case TokenId::TrueKeyword: {
			next_token();
			if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
					  resource_allocator.get(), resource_allocator.get(), document,
					  true)
						.cast_to<ExprNode>()))
				return gen_out_of_memory_syntax_error();
			break;
		}
		case TokenId::FalseKeyword: {
			next_token();
			if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
					  resource_allocator.get(), resource_allocator.get(), document,
					  false)
						.cast_to<ExprNode>()))
				return gen_out_of_memory_syntax_error();
			break;
		}
		case TokenId::NullKeyword: {
			next_token();
			if (!(expr_out = make_ast_node<NullLiteralExprNode>(
					  resource_allocator.get(), resource_allocator.get(), document)
						.cast_to<ExprNode>()))
				return gen_out_of_memory_syntax_error();
			break;
		}
		case TokenId::ModOp: {
			next_token();

			Token *reg_token;
			if ((syntax_error = expect_token(reg_token = peek_token(), TokenId::I32Literal))) {
				return syntax_error;
			}
			next_token();

			uint32_t data = 0;

			if (reg_token->source_text.at(0) == '-')
				std::terminate();

			std::string_view body_view = reg_token->source_text;

			if ((syntax_error = _parse_int<uint32_t>(this, reg_token, false, body_view, data)))
				return syntax_error;

			if (!(expr_out = make_ast_node<RegIndexExprNode>(
					  resource_allocator.get(), resource_allocator.get(), document,
					  data, AstNodePtr<TypeNameNode>())
						.cast_to<ExprNode>()))
				return gen_out_of_memory_syntax_error();
			break;
		}
		case TokenId::HashTag: {
			next_token();

			Token *label_token;
			if ((syntax_error = expect_token(label_token = peek_token(), TokenId::Id))) {
				return syntax_error;
			}
			next_token();

			peff::String s(resource_allocator.get());

			if (!s.build(label_token->source_text)) {
				return gen_out_of_memory_syntax_error();
			}

			if (!(expr_out = make_ast_node<BCLabelExprNode>(
					  resource_allocator.get(), resource_allocator.get(), document,
					  std::move(s))
						.cast_to<ExprNode>()))
				return gen_out_of_memory_syntax_error();
			break;
		}
		default:
			return SyntaxError(TokenRange{ document->main_module, token->index }, SyntaxErrorKind::UnexpectedToken);
	}

	return {};
gen_bad_expr:
	if (!(expr_out = make_ast_node<BadExprNode>(resource_allocator.get(), resource_allocator.get(), document, expr_out).cast_to<ExprNode>()))
		return gen_out_of_memory_syntax_error();
	expr_out->token_range = { document->main_module, token->index, parse_context.idx_current_token };
	return syntax_error;
}

SLKC_API peff::Option<SyntaxError> BCParser::parse_stmt(AstNodePtr<BCStmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	Token *l_parenthese_token = nullptr, *line_token = nullptr, *column_token = nullptr, *r_parenthese_token = nullptr;

	if ((l_parenthese_token = peek_token())->token_id == TokenId::LParenthese) {
		if ((syntax_error = expect_token(line_token = peek_token(), TokenId::I32Literal))) {
			return syntax_error;
		}
		next_token();

		if ((syntax_error = expect_token(line_token = peek_token(), TokenId::I32Literal))) {
			return syntax_error;
		}
		next_token();

		if ((syntax_error = expect_token(r_parenthese_token = peek_token(), TokenId::RParenthese))) {
			return syntax_error;
		}
		next_token();
	}

	peff::Option<uint32_t> reg_out;

	Token *token = peek_token();
	switch (token->token_id) {
		case TokenId::ModOp: {
			next_token();

			Token *reg_id_token;

			if ((syntax_error = expect_token(reg_id_token = peek_token(), TokenId::I32Literal))) {
				return syntax_error;
			}
			next_token();

			uint32_t data = 0;

			if (reg_id_token->source_text.at(0) == '-')
				std::terminate();

			std::string_view body_view = reg_id_token->source_text;

			if ((syntax_error = _parse_int<uint32_t>(this, reg_id_token, false, body_view, data)))
				return syntax_error;

			reg_out = std::move(data);

			if ((syntax_error = expect_token(r_parenthese_token = peek_token(), TokenId::AssignOp))) {
				return syntax_error;
			}
			next_token();
		}
			[[fallthrough]];
		case TokenId::Id: {
			Token *mnemonic_token = next_token();

			peff::Deferred set_token_range_guard([this, &stmt_out, l_parenthese_token, token]() noexcept {
				if (stmt_out)
					stmt_out->token_range = TokenRange{ document->main_module, l_parenthese_token ? l_parenthese_token->index : token->index, parse_context.idx_prev_token };
			});

			if (!reg_out.has_value()) {
				if (peek_token()->token_id == TokenId::Colon) {
					AstNodePtr<LabelBCStmtNode> label_stmt;

					if (!(label_stmt = make_ast_node<LabelBCStmtNode>(resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					next_token();
					stmt_out = label_stmt.cast_to<BCStmtNode>();

					if (!label_stmt->name.build(mnemonic_token->source_text))
						return gen_out_of_memory_syntax_error();
					break;
				}
			}

			AstNodePtr<InstructionBCStmtNode> ins_stmt;

			if (!(ins_stmt = make_ast_node<InstructionBCStmtNode>(resource_allocator.get(), resource_allocator.get(), document)))
				return gen_out_of_memory_syntax_error();

			stmt_out = ins_stmt.cast_to<BCStmtNode>();

			if (reg_out.has_value())
				ins_stmt->reg_out = reg_out.value();

			if(!ins_stmt->mnemonic.build(mnemonic_token->source_text))
				return gen_out_of_memory_syntax_error();

			while (true) {
				if (peek_token()->token_id == TokenId::Semicolon) {
					next_token();
					break;
				}

				AstNodePtr<ExprNode> arg;

				if (auto e = parse_comptime_expr(arg); e)
					return e;

				if (!ins_stmt->operands.push_back(std::move(arg)))
					return gen_out_of_memory_syntax_error();

				if (peek_token()->token_id != TokenId::Comma) {
					break;
				}

				Token *comma_token = next_token();
			}

			break;
		}
		default:
			next_token();
			return SyntaxError(TokenRange{ document->main_module, token->index }, SyntaxErrorKind::UnexpectedToken);
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> BCParser::parse_fn(AstNodePtr<BCFnOverloadingNode> &fn_node_out) {
	peff::Option<SyntaxError> syntax_error;

	Token *fn_token;
	Token *lvalue_marker_token = nullptr;

	peff::String name(resource_allocator.get());

	if (!(fn_node_out = make_ast_node<BCFnOverloadingNode>(resource_allocator.get(), resource_allocator.get(), document))) {
		return gen_out_of_memory_syntax_error();
	}

	switch ((fn_token = peek_token())->token_id) {
		case TokenId::FnKeyword: {
			next_token();

			fn_node_out->overloading_kind = BCFnOverloadingKind::Regular;

			if ((syntax_error = parse_id_name(name))) {
				return syntax_error;
			}
			break;
		}
		case TokenId::AsyncKeyword: {
			next_token();

			fn_node_out->overloading_kind = BCFnOverloadingKind::Coroutine;

			if ((syntax_error = parse_id_name(name))) {
				return syntax_error;
			}
			break;
		}
		case TokenId::OperatorKeyword: {
			next_token();

			fn_node_out->overloading_kind = BCFnOverloadingKind::Regular;

			Token *lvalue_marker_token;
			if ((lvalue_marker_token = peek_token())->token_id == TokenId::AssignOp) {
				fn_node_out->fn_flags |= FN_LVALUE;
				next_token();
			}

			std::string_view operator_name;
			if ((syntax_error = parse_operator_name(operator_name))) {
				return syntax_error;
			}

			if (!name.build(operator_name)) {
				return gen_out_of_memory_syntax_error();
			}

			if (fn_node_out->fn_flags & FN_LVALUE) {
				if (!name.append(LVALUE_OPERATOR_NAME_SUFFIX))
					return gen_out_of_memory_syntax_error();
			}
			break;
		}
		case TokenId::DefKeyword: {
			next_token();

			fn_node_out->overloading_kind = BCFnOverloadingKind::Pure;

			if ((syntax_error = parse_id_name(name))) {
				return syntax_error;
			}
			break;
		}
		default:
			return SyntaxError(TokenRange{ document->main_module, fn_token->index }, SyntaxErrorKind::UnexpectedToken);
	}

	switch (cur_parent->get_ast_node_type()) {
		case AstNodeType::Interface:
			fn_node_out->fn_flags = FN_VIRTUAL;
			break;
		default:
			break;
	}

	AstNodePtr<MemberNode> prev_parent = cur_parent;
	peff::ScopeGuard restore_parent_guard([this, prev_parent]() noexcept {
		cur_parent = prev_parent;
	});
	cur_parent = fn_node_out.cast_to<MemberNode>();

	peff::ScopeGuard set_token_range_guard([this, fn_token, fn_node_out]() noexcept {
		fn_node_out->token_range = TokenRange{ document->main_module, fn_token->index, parse_context.idx_prev_token };
	});

	fn_node_out->name = std::move(name);

	if ((syntax_error = parse_generic_params(fn_node_out->generic_params, fn_node_out->idx_generic_param_comma_tokens, fn_node_out->l_angle_bracket_index, fn_node_out->r_angle_bracket_index))) {
		return syntax_error;
	}

	bool has_var_arg = false;
	if ((syntax_error = parse_params(fn_node_out->params, has_var_arg, fn_node_out->idx_param_comma_tokens, fn_node_out->l_parenthese_index, fn_node_out->r_parenthese_index))) {
		return syntax_error;
	}
	if (has_var_arg) {
		fn_node_out->fn_flags |= FN_VARG;
	}

	Token *virtual_token;
	if ((virtual_token = peek_token())->token_id == TokenId::VirtualKeyword) {
		fn_node_out->fn_flags |= FN_VIRTUAL;
		next_token();
	}

	Token *override_token;
	if ((override_token = peek_token())->token_id == TokenId::OverrideKeyword) {
		next_token();

		Token *lookahead_token = peek_token();
		switch (lookahead_token->token_id) {
			case TokenId::ReturnTypeOp:
			case TokenId::Semicolon:
			case TokenId::LBrace:
				break;
			default:
				if ((syntax_error = parse_type_name(fn_node_out->overriden_type))) {
					return syntax_error;
				}
				break;
		}
	}

	Token *return_type_token;
	if ((return_type_token = peek_token())->token_id == TokenId::ReturnTypeOp) {
		next_token();
		if ((syntax_error = parse_type_name(fn_node_out->return_type))) {
			return syntax_error;
		}
	}

	Token *body_token = peek_token();

	switch (body_token->token_id) {
		case TokenId::Semicolon: {
			next_token();

			break;
		}
		case TokenId::LBrace: {
			next_token();

			AstNodePtr<BCStmtNode> cur_stmt;

			while (true) {
				if ((syntax_error = expect_token(peek_token()))) {
					return syntax_error;
				}

				if (peek_token()->token_id == TokenId::RBrace) {
					break;
				}

				if ((syntax_error = parse_stmt(cur_stmt))) {
					if (!syntax_errors.push_back(std::move(syntax_error.value())))
						return gen_out_of_memory_syntax_error();
				}

				if (cur_stmt) {
					if (!fn_node_out->body.push_back(std::move(cur_stmt))) {
						return gen_out_of_memory_syntax_error();
					}
				}
			}

			Token *r_brace_token;

			if ((syntax_error = expect_token((r_brace_token = peek_token()), TokenId::RBrace))) {
				return syntax_error;
			}

			next_token();
			break;
		}
		default:
			return SyntaxError(
				TokenRange{ document->main_module, body_token->index },
				SyntaxErrorKind::UnexpectedToken);
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> BCParser::parse_program_stmt() {
	peff::Option<SyntaxError> syntax_error;

	peff::DynArray<AstNodePtr<AttributeNode>> attributes(resource_allocator.get());

	if ((syntax_error = parse_attributes(attributes))) {
		return syntax_error;
	}

	slake::AccessModifier access = 0;
	Token *current_token;

	for (;;) {
		switch ((current_token = peek_token())->token_id) {
			case TokenId::PublicKeyword:
				access |= slake::ACCESS_PUBLIC;
				next_token();
				break;
			case TokenId::StaticKeyword:
				access |= slake::ACCESS_STATIC;
				next_token();
				break;
			case TokenId::NativeKeyword:
				access |= slake::ACCESS_NATIVE;
				next_token();
				break;
			default:
				goto access_modifier_parse_end;
		}
	}

access_modifier_parse_end:
	Token *token = peek_token();

	AstNodePtr<ModuleNode> p = cur_parent.cast_to<ModuleNode>();

	if (p->get_ast_node_type() == AstNodeType::Module) {
		access |= slake::ACCESS_STATIC;
	}

	switch (token->token_id) {
		case TokenId::AttributeKeyword: {
			// Attribute definition.
			next_token();

			AstNodePtr<AttributeDefNode> attribute_node;

			if (!(attribute_node = make_ast_node<AttributeDefNode>(resource_allocator.get(), resource_allocator.get(), document))) {
				return gen_out_of_memory_syntax_error();
			}

			attribute_node->access_modifier = access;

			Token *name_token;

			if ((syntax_error = expect_token((name_token = peek_token()), TokenId::Id))) {
				return syntax_error;
			}
			next_token();

			size_t idx_member;
			if ((idx_member = p->push_member(attribute_node.cast_to<MemberNode>())) == SIZE_MAX) {
				return gen_out_of_memory_syntax_error();
			}

			if (!attribute_node->name.build(name_token->source_text)) {
				return gen_out_of_memory_syntax_error();
			}

			{
				peff::ScopeGuard set_token_range_guard([this, token, attribute_node]() noexcept {
					attribute_node->token_range = TokenRange{ document->main_module, token->index, parse_context.idx_prev_token };
				});

				AstNodePtr<MemberNode> prev_parent;
				prev_parent = cur_parent;
				peff::ScopeGuard restore_prev_mod_guard([this, prev_parent]() noexcept {
					cur_parent = prev_parent;
				});
				cur_parent = attribute_node.cast_to<MemberNode>();

				Token *l_brace_token;

				if ((syntax_error = expect_token((l_brace_token = peek_token()), TokenId::LBrace))) {
					return syntax_error;
				}

				next_token();

				Token *current_token;
				while (true) {
					if ((syntax_error = expect_token(current_token = peek_token()))) {
						return syntax_error;
					}
					if (current_token->token_id == TokenId::RBrace) {
						break;
					}

					if ((syntax_error = parse_program_stmt())) {
						// Parse the rest to make sure that we have gained all of the information,
						// instead of ignoring them.
						if (!syntax_errors.push_back(std::move(syntax_error.value())))
							return gen_out_of_memory_syntax_error();
						syntax_error.reset();
					}
				}

				Token *r_brace_token;

				if ((syntax_error = expect_token((r_brace_token = peek_token()), TokenId::RBrace))) {
					return syntax_error;
				}

				next_token();
			}

			if (auto it = p->member_indices.find(attribute_node->name); it != p->member_indices.end()) {
				peff::String s(resource_allocator.get());

				if (!s.build(attribute_node->name)) {
					return gen_out_of_memory_syntax_error();
				}

				ConflictingDefinitionsErrorExData ex_data(std::move(s));

				return SyntaxError(attribute_node->token_range, std::move(ex_data));
			} else {
				if (!(p->index_member(idx_member))) {
					return gen_out_of_memory_syntax_error();
				}
			}

			break;
		}
		case TokenId::FnKeyword:
		case TokenId::AsyncKeyword:
		case TokenId::OperatorKeyword:
		case TokenId::DefKeyword: {
			// Function.
			AstNodePtr<BCFnOverloadingNode> fn;

			if ((syntax_error = parse_fn(fn))) {
				return syntax_error;
			}

			fn->access_modifier = access;

			if (auto it = p->member_indices.find(fn->name); it != p->member_indices.end()) {
				if (p->members.at(it.value())->get_ast_node_type() != AstNodeType::Fn) {
					peff::String s(resource_allocator.get());

					if (!s.build(fn->name)) {
						return gen_out_of_memory_syntax_error();
					}

					ConflictingDefinitionsErrorExData ex_data(std::move(s));

					return SyntaxError(fn->token_range, std::move(ex_data));
				}
				BCFnNode *fn_slot = (BCFnNode *)p->members.at(it.value()).get();
				fn->set_parent(fn_slot);
				if (!fn_slot->overloadings.push_back(std::move(fn))) {
					return gen_out_of_memory_syntax_error();
				}
			} else {
				AstNodePtr<BCFnNode> fn_slot;

				if (!(fn_slot = make_ast_node<BCFnNode>(resource_allocator.get(), resource_allocator.get(), document))) {
					return gen_out_of_memory_syntax_error();
				}

				if (!fn_slot->name.build(fn->name)) {
					return gen_out_of_memory_syntax_error();
				}

				if (!(p->add_member(fn_slot.cast_to<MemberNode>()))) {
					return gen_out_of_memory_syntax_error();
				}

				fn->set_parent(fn_slot.get());

				if (!fn_slot->overloadings.push_back(std::move(fn))) {
					return gen_out_of_memory_syntax_error();
				}
			}
			break;
		}
		case TokenId::ClassKeyword: {
			// Class.
			next_token();

			AstNodePtr<ClassNode> class_node;

			if (!(class_node = make_ast_node<ClassNode>(resource_allocator.get(), resource_allocator.get(), document))) {
				return gen_out_of_memory_syntax_error();
			}

			class_node->access_modifier = access;

			Token *name_token;

			if ((syntax_error = expect_token((name_token = peek_token()), TokenId::Id))) {
				return syntax_error;
			}
			next_token();

			if (!class_node->name.build(name_token->source_text)) {
				return gen_out_of_memory_syntax_error();
			}

			size_t idx_member;
			if ((idx_member = p->push_member(class_node.cast_to<MemberNode>())) == SIZE_MAX) {
				return gen_out_of_memory_syntax_error();
			}

			{
				peff::ScopeGuard set_token_range_guard([this, token, class_node]() noexcept {
					class_node->token_range = TokenRange{ document->main_module, token->index, parse_context.idx_prev_token };
				});

				AstNodePtr<MemberNode> prev_parent;
				prev_parent = cur_parent;
				peff::ScopeGuard restore_prev_mod_guard([this, prev_parent]() noexcept {
					cur_parent = prev_parent;
				});
				cur_parent = class_node.cast_to<MemberNode>();

				if ((syntax_error = parse_generic_params(class_node->generic_params, class_node->idx_generic_param_comma_tokens, class_node->idx_langle_bracket_token, class_node->idx_rangle_bracket_token))) {
					return syntax_error;
				}

				if (Token *l_parenthese_token = peek_token(); l_parenthese_token->token_id == TokenId::LParenthese) {
					next_token();

					if ((syntax_error = parse_type_name(class_node->base_type))) {
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

						if (!class_node->impl_types.push_back(std::move(tn))) {
							return gen_out_of_memory_syntax_error();
						}

						if (peek_token()->token_id != TokenId::AddOp) {
							break;
						}

						Token *or_op_token = next_token();
					}
				}

				Token *l_brace_token;

				if ((syntax_error = expect_token((l_brace_token = peek_token()), TokenId::LBrace))) {
					return syntax_error;
				}

				next_token();

				Token *current_token;
				while (true) {
					if ((syntax_error = expect_token(current_token = peek_token()))) {
						return syntax_error;
					}
					if (current_token->token_id == TokenId::RBrace) {
						break;
					}

					if ((syntax_error = parse_program_stmt())) {
						// Parse the rest to make sure that we have gained all of the information,
						// instead of ignoring them.
						if (!syntax_errors.push_back(std::move(syntax_error.value())))
							return gen_out_of_memory_syntax_error();
						syntax_error.reset();
					}
				}

				Token *r_brace_token;

				if ((syntax_error = expect_token((r_brace_token = peek_token()), TokenId::RBrace))) {
					return syntax_error;
				}

				next_token();
			}

			if (auto it = p->member_indices.find(class_node->name); it != p->member_indices.end()) {
				peff::String s(resource_allocator.get());

				if (!s.build(class_node->name)) {
					return gen_out_of_memory_syntax_error();
				}

				ConflictingDefinitionsErrorExData ex_data(std::move(s));

				return SyntaxError(class_node->token_range, std::move(ex_data));
			} else {
				if (!(p->index_member(idx_member))) {
					return gen_out_of_memory_syntax_error();
				}
			}

			break;
		}
		case TokenId::StructKeyword: {
			// Struct.
			next_token();

			AstNodePtr<StructNode> struct_node;

			if (!(struct_node = make_ast_node<StructNode>(resource_allocator.get(), resource_allocator.get(), document))) {
				return gen_out_of_memory_syntax_error();
			}

			struct_node->access_modifier = access;

			Token *name_token;

			if ((syntax_error = expect_token((name_token = peek_token()), TokenId::Id))) {
				return syntax_error;
			}
			next_token();

			if (!struct_node->name.build(name_token->source_text)) {
				return gen_out_of_memory_syntax_error();
			}

			size_t idx_member;
			if ((idx_member = p->push_member(struct_node.cast_to<MemberNode>())) == SIZE_MAX) {
				return gen_out_of_memory_syntax_error();
			}

			{
				peff::ScopeGuard set_token_range_guard([this, token, struct_node]() noexcept {
					struct_node->token_range = TokenRange{ document->main_module, token->index, parse_context.idx_prev_token };
				});

				AstNodePtr<MemberNode> prev_parent;
				prev_parent = cur_parent;
				peff::ScopeGuard restore_prev_mod_guard([this, prev_parent]() noexcept {
					cur_parent = prev_parent;
				});
				cur_parent = struct_node.cast_to<MemberNode>();

				if ((syntax_error = parse_generic_params(struct_node->generic_params, struct_node->idx_generic_param_comma_tokens, struct_node->idx_langle_bracket_token, struct_node->idx_rangle_bracket_token))) {
					return syntax_error;
				}

				if (Token *colon_token = peek_token(); colon_token->token_id == TokenId::Colon) {
					next_token();

					while (true) {
						AstNodePtr<TypeNameNode> tn;

						if ((syntax_error = parse_type_name(tn))) {
							return syntax_error;
						}

						if (!struct_node->impl_types.push_back(std::move(tn))) {
							return gen_out_of_memory_syntax_error();
						}

						if (peek_token()->token_id != TokenId::AddOp) {
							break;
						}

						Token *or_op_token = next_token();
					}
				}

				Token *l_brace_token;

				if ((syntax_error = expect_token((l_brace_token = peek_token()), TokenId::LBrace))) {
					return syntax_error;
				}

				next_token();

				Token *current_token;
				while (true) {
					if ((syntax_error = expect_token(current_token = peek_token()))) {
						return syntax_error;
					}
					if (current_token->token_id == TokenId::RBrace) {
						break;
					}

					if ((syntax_error = parse_program_stmt())) {
						// Parse the rest to make sure that we have gained all of the information,
						// instead of ignoring them.
						if (!syntax_errors.push_back(std::move(syntax_error.value())))
							return gen_out_of_memory_syntax_error();
						syntax_error.reset();
					}
				}

				Token *r_brace_token;

				if ((syntax_error = expect_token((r_brace_token = peek_token()), TokenId::RBrace))) {
					return syntax_error;
				}

				next_token();
			}

			if (auto it = p->member_indices.find(struct_node->name); it != p->member_indices.end()) {
				peff::String s(resource_allocator.get());

				if (!s.build(struct_node->name)) {
					return gen_out_of_memory_syntax_error();
				}

				ConflictingDefinitionsErrorExData ex_data(std::move(s));

				return SyntaxError(struct_node->token_range, std::move(ex_data));
			} else {
				if (!(p->index_member(idx_member))) {
					return gen_out_of_memory_syntax_error();
				}
			}

			break;
		}
		case TokenId::InterfaceKeyword: {
			// Interface.
			next_token();

			AstNodePtr<InterfaceNode> interface_node;

			if (!(interface_node = make_ast_node<InterfaceNode>(resource_allocator.get(), resource_allocator.get(), document))) {
				return gen_out_of_memory_syntax_error();
			}

			interface_node->access_modifier = access;

			Token *name_token;

			if ((syntax_error = expect_token((name_token = peek_token()), TokenId::Id))) {
				return syntax_error;
			}
			next_token();

			if (!interface_node->name.build(name_token->source_text)) {
				return gen_out_of_memory_syntax_error();
			}

			size_t idx_member;
			if ((idx_member = p->push_member(interface_node.cast_to<MemberNode>())) == SIZE_MAX) {
				return gen_out_of_memory_syntax_error();
			}

			Token *t;

			{
				peff::ScopeGuard set_token_range_guard([this, token, interface_node]() noexcept {
					interface_node->token_range = TokenRange{ document->main_module, token->index, parse_context.idx_prev_token };
				});

				AstNodePtr<MemberNode> prev_member;
				prev_member = cur_parent;
				peff::ScopeGuard restore_prev_mod_guard([this, prev_member]() noexcept {
					cur_parent = prev_member;
				});
				cur_parent = interface_node.cast_to<MemberNode>();

				if ((syntax_error = parse_generic_params(interface_node->generic_params, interface_node->idx_generic_param_comma_tokens, interface_node->idx_langle_bracket_token, interface_node->idx_rangle_bracket_token))) {
					return syntax_error;
				}

				if (Token *colon_token = peek_token(); colon_token->token_id == TokenId::Colon) {
					next_token();

					while (true) {
						AstNodePtr<TypeNameNode> tn;

						if ((syntax_error = parse_type_name(tn))) {
							return syntax_error;
						}

						if (!interface_node->impl_types.push_back(std::move(tn))) {
							return gen_out_of_memory_syntax_error();
						}

						if (peek_token()->token_id != TokenId::AddOp) {
							break;
						}

						Token *or_op_token = next_token();
					}
				}

				Token *l_brace_token;

				if ((syntax_error = expect_token((l_brace_token = peek_token()), TokenId::LBrace))) {
					return syntax_error;
				}

				next_token();

				Token *current_token;
				while (true) {
					if ((syntax_error = expect_token(current_token = peek_token()))) {
						return syntax_error;
					}
					if (current_token->token_id == TokenId::RBrace) {
						break;
					}

					if ((syntax_error = parse_program_stmt())) {
						// Parse the rest to make sure that we have gained all of the information,
						// instead of ignoring them.
						if (!syntax_errors.push_back(std::move(syntax_error.value())))
							return gen_out_of_memory_syntax_error();
						syntax_error.reset();
					}
				}

				Token *r_brace_token;

				if ((syntax_error = expect_token((r_brace_token = peek_token()), TokenId::RBrace))) {
					return syntax_error;
				}

				next_token();
			}

			if (auto it = p->member_indices.find(interface_node->name); it != p->member_indices.end()) {
				peff::String s(resource_allocator.get());

				if (!s.build(interface_node->name)) {
					return gen_out_of_memory_syntax_error();
				}

				ConflictingDefinitionsErrorExData ex_data(std::move(s));

				return SyntaxError(interface_node->token_range, std::move(ex_data));
			} else {
				if (!(p->index_member(idx_member))) {
					return gen_out_of_memory_syntax_error();
				}
			}

			break;
		}
		case TokenId::ImportKeyword: {
			// Import item.
			next_token();

			AstNodePtr<ImportNode> import_node;

			if (!(import_node = make_ast_node<ImportNode>(resource_allocator.get(), resource_allocator.get(), document))) {
				return gen_out_of_memory_syntax_error();
			}

			if ((syntax_error = parse_id_ref(import_node->id_ref)))
				return syntax_error;

			size_t idx_member;
			if ((idx_member = p->push_member(import_node.cast_to<MemberNode>())) == SIZE_MAX) {
				return gen_out_of_memory_syntax_error();
			}

			if (Token *as_token = peek_token(); as_token->token_id == TokenId::AsKeyword) {
				next_token();

				Token *name_token;

				if ((syntax_error = expect_token((name_token = peek_token()), TokenId::Id))) {
					return syntax_error;
				}

				if (!import_node->name.build(name_token->source_text)) {
					return gen_out_of_memory_syntax_error();
				}

				if (!p->index_member(idx_member)) {
					return gen_out_of_memory_syntax_error();
				}
			} else {
				if (!p->anonymous_imports.push_back(AstNodePtr<ImportNode>(import_node))) {
					return gen_out_of_memory_syntax_error();
				}
			}

			Token *semicolon_token;

			if ((syntax_error = expect_token((semicolon_token = peek_token()), TokenId::Semicolon))) {
				return syntax_error;
			}

			next_token();

			break;
		}
			/*
		case TokenId::LetKeyword: {
			// Global variable.
			next_token();

			AstNodePtr<VarDefStmtNode> stmt;

			if (!(stmt = make_ast_node<VarDefStmtNode>(
					  resource_allocator.get(),
					  resource_allocator.get(),
					  document,
					  peff::DynArray<VarDefEntryPtr>(resource_allocator.get())))) {
				return gen_out_of_memory_syntax_error();
			}

			stmt->access_modifier = access;

			peff::ScopeGuard set_token_range_guard([this, token, stmt]() noexcept {
				stmt->token_range = TokenRange{ document->main_module, token->index, parse_context.idx_prev_token };
			});

			if ((syntax_error = parse_var_defs(stmt->var_def_entries))) {
				return syntax_error;
			}

			if (!p->var_def_stmts.push_back(std::move(stmt))) {
				return gen_out_of_memory_syntax_error();
			}

			Token *semicolon_token;

			if ((syntax_error = expect_token((semicolon_token = peek_token()), TokenId::Semicolon))) {
				return syntax_error;
			}

			next_token();

			break;
		}*/
		default:
			next_token();
			return SyntaxError(
				TokenRange{ document->main_module, token->index },
				SyntaxErrorKind::ExpectingDecl);
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> BCParser::parse_program(const AstNodePtr<ModuleNode> &initial_mod, IdRefPtr &module_name_out) {
	peff::Option<SyntaxError> syntax_error;

	Token *t;

	cur_parent = initial_mod.cast_to<MemberNode>();

	module_name_out = {};
	if ((t = peek_token())->token_id == TokenId::ModuleKeyword) {
		next_token();

		IdRefPtr module_name;

		if ((syntax_error = parse_id_ref(module_name))) {
			if (!syntax_errors.push_back(std::move(syntax_error.value())))
				return gen_out_of_memory_syntax_error();
			syntax_error.reset();
		}

		Token *semicolon_token;
		if ((syntax_error = expect_token((semicolon_token = peek_token()), TokenId::Semicolon))) {
			return syntax_error;
		}
		next_token();

		module_name_out = std::move(module_name);
	}

	while ((t = peek_token())->token_id != TokenId::End) {
		if ((syntax_error = parse_program_stmt())) {
			// Parse the rest to make sure that we have gained all of the information,
			// instead of ignoring them.
			if (!syntax_errors.push_back(std::move(syntax_error.value())))
				return gen_out_of_memory_syntax_error();
			syntax_error.reset();
		}
	}

	initial_mod->set_parser(shared_from_this());

	return {};
}
