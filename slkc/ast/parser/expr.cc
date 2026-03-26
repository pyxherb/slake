#define NOMINMAX
#include "../parser.h"

using namespace slkc;

template <typename T>
SLAKE_FORCEINLINE peff::Option<SyntaxError> _parse_int(Parser *parser, Token *token, bool is_negative, const std::string_view &body_view, T &data_out) {
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

SLKC_API peff::Option<SyntaxError> Parser::parse_expr(int precedence, AstNodePtr<ExprNode> &expr_out) {
	Token *prefix_token;

	peff::Option<SyntaxError> syntax_error;
	AstNodePtr<ExprNode> lhs, rhs;

	if ((syntax_error = expect_token((prefix_token = peek_token()))))
		goto gen_bad_expr;

	{
		{
			peff::ScopeGuard set_token_range_guard([this, prefix_token, &lhs]() noexcept {
				if (lhs) {
					lhs->token_range = TokenRange{ document->main_module, prefix_token->index, parse_context.idx_prev_token };
				}
			});

			switch (prefix_token->token_id) {
				case TokenId::ThisKeyword:
				case TokenId::ScopeOp:
				case TokenId::Id: {
					IdRefPtr id_ref_ptr;
					if ((syntax_error = parse_id_ref(id_ref_ptr)))
						goto gen_bad_expr;
					if (!(lhs = make_ast_node<IdRefExprNode>(resource_allocator.get(), resource_allocator.get(), document, std::move(id_ref_ptr)).cast_to<ExprNode>()))
						return gen_out_of_memory_syntax_error();
					break;
				}
				case TokenId::LParenthese: {
					next_token();

					AstNodePtr<WrapperExprNode> expr;

					if (!(expr = make_ast_node<WrapperExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(0, expr->target)))
						goto gen_bad_expr;

					Token *r_parenthese_token;

					if ((syntax_error = expect_token((r_parenthese_token = next_token()), TokenId::RParenthese)))
						goto gen_bad_expr;
					break;
				}
				case TokenId::AllocaKeyword: {
					next_token();

					AstNodePtr<AllocaExprNode> expr;

					if (!(expr = make_ast_node<AllocaExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_type_name(expr->target_type, false)))
						goto gen_bad_expr;

					Token *l_bracket_token;

					if ((l_bracket_token = peek_token())->token_id == TokenId::LBracket) {
						next_token();

						if ((syntax_error = parse_expr(0, expr->count_expr)))
							goto gen_bad_expr;

						if ((syntax_error = split_rdbrackets_token()))
							goto gen_bad_expr;

						Token *r_bracket_token;

						if ((syntax_error = expect_token((r_bracket_token = peek_token()), TokenId::RBracket)))
							goto gen_bad_expr;

						next_token();
					}
					break;
				}
				case TokenId::NewKeyword: {
					next_token();

					AstNodePtr<NewExprNode> expr;

					if (!(expr = make_ast_node<NewExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_type_name(expr->target_type)))
						goto gen_bad_expr;

					Token *l_parenthese_token;

					if ((syntax_error = expect_token((l_parenthese_token = peek_token()), TokenId::LParenthese)))
						goto gen_bad_expr;

					next_token();

					if ((syntax_error = parse_args(expr->args, expr->idx_comma_tokens)))
						goto gen_bad_expr;

					Token *r_parenthese_token;

					if ((syntax_error = expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese)))
						goto gen_bad_expr;

					next_token();
					break;
				}
				case TokenId::I8Literal: {
					next_token();

					int8_t data = 0;
					bool is_negative = false;

					if (prefix_token->source_text.at(0) == '-')
						is_negative = true;

					std::string_view body_view = prefix_token->source_text.substr(is_negative ? 1 : 0, prefix_token->source_text.size() - (is_negative ? 1 : 0) - (sizeof("i8") - 1));

					if ((syntax_error = _parse_int<int8_t>(this, prefix_token, is_negative, body_view, data)))
						return syntax_error;

					if (!(lhs = make_ast_node<I8LiteralExprNode>(
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

					if (prefix_token->source_text.at(0) == '-')
						is_negative = true;

					std::string_view body_view = prefix_token->source_text.substr(is_negative ? 1 : 0, prefix_token->source_text.size() - (is_negative ? 1 : 0) - (sizeof("i16") - 1));

					if ((syntax_error = _parse_int<int16_t>(this, prefix_token, is_negative, body_view, data)))
						return syntax_error;

					if (!(lhs = make_ast_node<I16LiteralExprNode>(
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

					if (prefix_token->source_text.at(0) == '-')
						is_negative = true;

					std::string_view body_view = prefix_token->source_text.substr(is_negative ? 1 : 0, prefix_token->source_text.size() - (is_negative ? 1 : 0));

					if ((syntax_error = _parse_int<int32_t>(this, prefix_token, is_negative, body_view, data)))
						return syntax_error;

					if (!(lhs = make_ast_node<I32LiteralExprNode>(
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

					if (prefix_token->source_text.at(0) == '-')
						is_negative = true;

					std::string_view body_view = prefix_token->source_text.substr(is_negative ? 1 : 0, prefix_token->source_text.size() - (is_negative ? 1 : 0) - (sizeof("L") - 1));

					if ((syntax_error = _parse_int<int64_t>(this, prefix_token, is_negative, body_view, data)))
						return syntax_error;

					if (!(lhs = make_ast_node<I64LiteralExprNode>(
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

					if (prefix_token->source_text.at(0) == '-')
						is_negative = true;

					std::string_view body_view = prefix_token->source_text.substr(is_negative ? 1 : 0, prefix_token->source_text.size() - (is_negative ? 1 : 0) - (sizeof("Ui8") - 1));

					if ((syntax_error = _parse_int<uint8_t>(this, prefix_token, is_negative, body_view, data)))
						return syntax_error;

					if (!(lhs = make_ast_node<U8LiteralExprNode>(
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

					if (prefix_token->source_text.at(0) == '-')
						is_negative = true;

					std::string_view body_view = prefix_token->source_text.substr(is_negative ? 1 : 0, prefix_token->source_text.size() - (is_negative ? 1 : 0) - (sizeof("Ui16") - 1));

					if ((syntax_error = _parse_int<uint16_t>(this, prefix_token, is_negative, body_view, data)))
						return syntax_error;

					if (!(lhs = make_ast_node<U16LiteralExprNode>(
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

					if (prefix_token->source_text.at(0) == '-')
						is_negative = true;

					std::string_view body_view = prefix_token->source_text.substr(is_negative ? 1 : 0, prefix_token->source_text.size() - (is_negative ? 1 : 0) - (sizeof("U") - 1));

					if ((syntax_error = _parse_int<uint32_t>(this, prefix_token, is_negative, body_view, data)))
						return syntax_error;

					if (!(lhs = make_ast_node<U32LiteralExprNode>(
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

					if (prefix_token->source_text.at(0) == '-')
						is_negative = true;

					std::string_view body_view = prefix_token->source_text.substr(is_negative ? 1 : 0, prefix_token->source_text.size() - (is_negative ? 1 : 0) - (sizeof("UL") - 1));

					if ((syntax_error = _parse_int<uint64_t>(this, prefix_token, is_negative, body_view, data)))
						return syntax_error;

					if (!(lhs = make_ast_node<U64LiteralExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document,
							  data)
								.cast_to<ExprNode>()))
						return gen_out_of_memory_syntax_error();
					break;
				}
				case TokenId::StringLiteral: {
					next_token();
					peff::String s(resource_allocator.get());

					if (!s.build(((StringTokenExtension *)prefix_token->ex_data.get())->data)) {
						return gen_out_of_memory_syntax_error();
					}

					if (!(lhs = make_ast_node<StringLiteralExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document,
							  std::move(s))
								.cast_to<ExprNode>()))
						return gen_out_of_memory_syntax_error();
					break;
				}
				case TokenId::F32Literal: {
					next_token();

					if (!(lhs = peff::make_shared_with_control_block<F32LiteralExprNode, AstNodeControlBlock<F32LiteralExprNode>>(
							  resource_allocator.get(), resource_allocator.get(), document,
							  strtof(prefix_token->source_text.data(), nullptr))
								.cast_to<ExprNode>()))
						return gen_out_of_memory_syntax_error();
					break;
				}
				case TokenId::F64Literal: {
					next_token();
					if (!(lhs = peff::make_shared_with_control_block<F64LiteralExprNode, AstNodeControlBlock<F64LiteralExprNode>>(
							  resource_allocator.get(), resource_allocator.get(), document,
							  strtod(prefix_token->source_text.data(), nullptr))
								.cast_to<ExprNode>()))
						return gen_out_of_memory_syntax_error();
					break;
				}
				case TokenId::TrueKeyword: {
					next_token();
					if (!(lhs = make_ast_node<BoolLiteralExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document,
							  true)
								.cast_to<ExprNode>()))
						return gen_out_of_memory_syntax_error();
					break;
				}
				case TokenId::FalseKeyword: {
					next_token();
					if (!(lhs = make_ast_node<BoolLiteralExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document,
							  false)
								.cast_to<ExprNode>()))
						return gen_out_of_memory_syntax_error();
					break;
				}
				case TokenId::NullKeyword: {
					next_token();
					if (!(lhs = make_ast_node<NullLiteralExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)
								.cast_to<ExprNode>()))
						return gen_out_of_memory_syntax_error();
					break;
				}
				case TokenId::VarArg: {
					next_token();

					AstNodePtr<UnaryExprNode> expr;

					if (!(expr = make_ast_node<UnaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->unary_op = UnaryOp::Unpacking;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(131, expr->operand))) {
						goto gen_bad_expr;
					}
					break;
				}
				case TokenId::LBrace: {
					next_token();

					AstNodePtr<InitializerListExprNode> initializer_expr;

					if (!(initializer_expr = make_ast_node<InitializerListExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					lhs = initializer_expr.cast_to<ExprNode>();

					AstNodePtr<ExprNode> cur_expr;

					static TokenId matching_tokens[] = {
						TokenId::RBrace,
						TokenId::Semicolon
					};

					Token *current_token;

					for (;;) {
						if ((syntax_error = parse_expr(0, cur_expr))) {
							if (!syntax_errors.push_back(std::move(syntax_error.value())))
								return gen_out_of_memory_syntax_error();
							syntax_error.reset();
							goto gen_bad_expr;
						}

						if (!initializer_expr->elements.push_back(std::move(cur_expr))) {
							return gen_out_of_memory_syntax_error();
						}

						current_token = peek_token();

						if (current_token->token_id != TokenId::Comma)
							break;

						next_token();
					}

					if ((syntax_error = expect_token(current_token = peek_token(), TokenId::RBrace))) {
						if (!syntax_errors.push_back(std::move(syntax_error.value())))
							return gen_out_of_memory_syntax_error();
						syntax_error.reset();
						if ((syntax_error = lookahead_until(std::size(matching_tokens), matching_tokens)))
							goto gen_bad_expr;
					}

					next_token();

					break;
				}
				case TokenId::SubOp: {
					next_token();

					AstNodePtr<UnaryExprNode> expr;

					if (!(expr = make_ast_node<UnaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->unary_op = UnaryOp::Neg;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(131, expr->operand))) {
						goto gen_bad_expr;
					}
					break;
				}
				case TokenId::NotOp: {
					next_token();

					AstNodePtr<UnaryExprNode> expr;

					if (!(expr = make_ast_node<UnaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->unary_op = UnaryOp::Not;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(131, expr->operand))) {
						goto gen_bad_expr;
					}
					break;
				}
				case TokenId::LNotOp: {
					next_token();

					AstNodePtr<UnaryExprNode> expr;

					if (!(expr = make_ast_node<UnaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->unary_op = UnaryOp::LNot;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(131, expr->operand))) {
						goto gen_bad_expr;
					}

					break;
				}
				case TokenId::MatchKeyword: {
					next_token();

					AstNodePtr<MatchExprNode> expr;

					if (!(expr = make_ast_node<MatchExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					lhs = expr.cast_to<ExprNode>();

					Token *l_parenthese_token;

					if ((syntax_error = expect_token((l_parenthese_token = peek_token()), TokenId::LParenthese)))
						goto gen_bad_expr;

					next_token();

					if ((syntax_error = parse_expr(0, expr->condition)))
						goto gen_bad_expr;

					Token *r_parenthese_token;

					if ((syntax_error = expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese)))
						goto gen_bad_expr;

					next_token();

					Token *return_type_token;
					if ((return_type_token = peek_token())->token_id == TokenId::ReturnTypeOp) {
						next_token();
						SLKC_RETURN_IF_PARSE_ERROR((parse_type_name(expr->return_type)));
					}

					Token *l_brace_token = peek_token();

					if ((syntax_error = expect_token(l_brace_token, TokenId::LBrace))) {
						goto gen_bad_expr;
					}

					next_token();

					while (true) {
						AstNodePtr<ExprNode> condition_expr, result_expr;

						if (peek_token()->token_id == TokenId::DefaultKeyword) {
							next_token();
						} else {
							if ((syntax_error = parse_expr(0, condition_expr))) {
								goto gen_bad_expr;
							}
						}

						Token *colon_token = peek_token();

						if ((syntax_error = expect_token(colon_token, TokenId::Colon))) {
							goto gen_bad_expr;
						}

						next_token();

						if ((syntax_error = parse_expr(0, result_expr))) {
							goto gen_bad_expr;
						}

						if (!expr->cases.push_back({ condition_expr, result_expr })) {
							return gen_out_of_memory_syntax_error();
						}

						if (peek_token()->token_id != TokenId::Comma)
							break;

						Token *comma_token = next_token();
					}

					Token *r_brace_token = peek_token();

					if ((syntax_error = expect_token(r_brace_token, TokenId::RBrace))) {
						goto gen_bad_expr;
					}

					next_token();

					break;
				}
				default:
					next_token();
					return SyntaxError(
						TokenRange{ document->main_module, prefix_token->index },
						SyntaxErrorKind::ExpectingExpr);
			}
		}

		Token *infix_token;

		for (;;) {
			peff::ScopeGuard set_token_range_guard([this, prefix_token, &lhs]() noexcept {
				if (lhs) {
					lhs->token_range = TokenRange{ document->main_module, prefix_token->index, parse_context.idx_prev_token };
				}
			});

			switch ((infix_token = peek_token())->token_id) {
				case TokenId::LParenthese: {
					if (precedence > 140)
						goto end;
					next_token();

					AstNodePtr<CallExprNode> expr;

					if (!(expr = make_ast_node<CallExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document, AstNodePtr<ExprNode>(), peff::DynArray<AstNodePtr<ExprNode>>{ resource_allocator.get() })))
						return gen_out_of_memory_syntax_error();

					expr->target = lhs;

					lhs = expr.cast_to<ExprNode>();

					expr->l_parenthese_token_index = infix_token->index;

					if ((syntax_error = parse_args(expr->args, expr->idx_comma_tokens))) {
						goto gen_bad_expr;
					}

					Token *r_parenthese_token;

					if ((syntax_error = expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese)))
						goto gen_bad_expr;

					next_token();

					expr->r_parenthese_token_index = r_parenthese_token->index;

					if (peek_token()->token_id == TokenId::WithKeyword) {
						next_token();

						if (auto e = parse_expr(121, expr->with_object); e)
							return e;
					}

					break;
				}
				case TokenId::LBracket: {
					if (precedence > 140)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::Subscript;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(0, expr->rhs)))
						goto gen_bad_expr;

					if ((syntax_error = split_rdbrackets_token()))
						goto gen_bad_expr;

					Token *r_bracket_token;

					if ((syntax_error = expect_token((r_bracket_token = next_token()), TokenId::RBracket)))
						goto gen_bad_expr;

					break;
				}
				case TokenId::Dot: {
					if (precedence > 140)
						goto end;
					next_token();

					AstNodePtr<HeadedIdRefExprNode> expr;

					if (!(expr = make_ast_node<HeadedIdRefExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document, lhs, IdRefPtr{})))
						return gen_out_of_memory_syntax_error();

					expr->head = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_id_ref(expr->id_ref_ptr)))
						goto gen_bad_expr;

					break;
				}
				case TokenId::AsKeyword: {
					if (precedence > 130)
						goto end;
					next_token();

					AstNodePtr<CastExprNode> expr;

					if (!(expr = make_ast_node<CastExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->source = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_type_name(expr->target_type)))
						goto gen_bad_expr;

					expr->token_range.end_index = expr->target_type->token_range.end_index;

					break;
				}

				case TokenId::MulOp: {
					if (precedence > 120)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::Mul;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(121, expr->rhs)))
						goto gen_bad_expr;

					break;
				}
				case TokenId::DivOp: {
					if (precedence > 120)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::Div;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(121, expr->rhs)))
						goto gen_bad_expr;

					break;
				}
				case TokenId::ModOp: {
					if (precedence > 120)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::Mod;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(121, expr->rhs)))
						goto gen_bad_expr;

					break;
				}

				case TokenId::AddOp: {
					if (precedence > 110)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::Add;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(111, expr->rhs)))
						goto gen_bad_expr;

					break;
				}
				case TokenId::SubOp: {
					if (precedence > 110)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::Sub;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(111, expr->rhs)))
						goto gen_bad_expr;

					break;
				}

				case TokenId::ShlOp: {
					if (precedence > 100)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::Shl;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(101, expr->rhs)))
						goto gen_bad_expr;

					break;
				}
				case TokenId::ShrOp: {
					if (precedence > 100)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::Shr;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(101, expr->rhs)))
						goto gen_bad_expr;

					break;
				}

				case TokenId::CmpOp: {
					if (precedence > 90)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::Cmp;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(91, expr->rhs)))
						goto gen_bad_expr;

					break;
				}

				case TokenId::GtOp: {
					if (precedence > 80)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::Gt;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(81, expr->rhs)))
						goto gen_bad_expr;

					break;
				}
				case TokenId::GtEqOp: {
					if (precedence > 80)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::GtEq;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(81, expr->rhs)))
						goto gen_bad_expr;

					break;
				}
				case TokenId::LtOp: {
					if (precedence > 80)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::Lt;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(81, expr->rhs)))
						goto gen_bad_expr;

					break;
				}
				case TokenId::LtEqOp: {
					if (precedence > 80)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::LtEq;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(81, expr->rhs)))
						goto gen_bad_expr;

					break;
				}

				case TokenId::EqOp: {
					if (precedence > 70)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::Eq;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(71, expr->rhs)))
						goto gen_bad_expr;

					break;
				}
				case TokenId::NeqOp: {
					if (precedence > 70)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::Neq;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(71, expr->rhs)))
						goto gen_bad_expr;

					break;
				}
				case TokenId::StrictEqOp: {
					if (precedence > 70)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::StrictEq;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(71, expr->rhs)))
						goto gen_bad_expr;

					break;
				}
				case TokenId::StrictNeqOp: {
					if (precedence > 70)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::StrictNeq;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(71, expr->rhs)))
						goto gen_bad_expr;

					break;
				}

				case TokenId::AndOp: {
					if (precedence > 60)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::And;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(61, expr->rhs)))
						goto gen_bad_expr;

					break;
				}

				case TokenId::XorOp: {
					if (precedence > 50)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::Xor;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(51, expr->rhs)))
						goto gen_bad_expr;

					break;
				}

				case TokenId::OrOp: {
					if (precedence > 40)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::Or;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(41, expr->rhs)))
						goto gen_bad_expr;

					break;
				}

				case TokenId::LAndOp: {
					if (precedence > 30)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::LAnd;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(31, expr->rhs)))
						goto gen_bad_expr;

					break;
				}

				case TokenId::LOrOp: {
					if (precedence > 20)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::LOr;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(21, expr->rhs)))
						goto gen_bad_expr;

					break;
				}

				case TokenId::Question: {
					if (precedence > 10)
						goto end;
					next_token();

					AstNodePtr<TernaryExprNode> expr;

					if (!(expr = make_ast_node<TernaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(10, expr->lhs)))
						goto gen_bad_expr;

					expr->token_range.end_index = expr->lhs->token_range.end_index;

					Token *colon_token;
					if ((syntax_error = expect_token((colon_token = next_token()), TokenId::Colon)))
						goto gen_bad_expr;

					expr->token_range.end_index = colon_token->index;

					if ((syntax_error = parse_expr(10, expr->rhs)))
						goto gen_bad_expr;

					break;
				}

				case TokenId::AssignOp: {
					if (precedence > 1)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::Assign;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(0, expr->rhs)))
						goto gen_bad_expr;

					break;
				}
				case TokenId::AddAssignOp: {
					if (precedence > 1)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::AddAssign;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(0, expr->rhs)))
						goto gen_bad_expr;

					break;
				}
				case TokenId::SubAssignOp: {
					if (precedence > 1)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::SubAssign;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(0, expr->rhs)))
						goto gen_bad_expr;

					break;
				}
				case TokenId::MulAssignOp: {
					if (precedence > 1)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::MulAssign;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(0, expr->rhs)))
						goto gen_bad_expr;

					break;
				}
				case TokenId::DivAssignOp: {
					if (precedence > 1)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::DivAssign;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(0, expr->rhs)))
						goto gen_bad_expr;

					break;
				}
				case TokenId::AndAssignOp: {
					if (precedence > 1)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::AndAssign;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(0, expr->rhs)))
						goto gen_bad_expr;

					break;
				}
				case TokenId::OrAssignOp: {
					if (precedence > 1)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::OrAssign;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(0, expr->rhs)))
						goto gen_bad_expr;

					break;
				}
				case TokenId::XorAssignOp: {
					if (precedence > 1)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::XorAssign;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(0, expr->rhs)))
						goto gen_bad_expr;

					break;
				}
				case TokenId::ShlAssignOp: {
					if (precedence > 1)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::ShlAssign;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(0, expr->rhs)))
						goto gen_bad_expr;

					break;
				}
				case TokenId::ShrAssignOp: {
					if (precedence > 1)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::ShrAssign;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(0, expr->rhs)))
						goto gen_bad_expr;

					break;
				}
				case TokenId::Comma: {
					if (precedence > -9)
						goto end;
					next_token();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = make_ast_node<BinaryExprNode>(
							  resource_allocator.get(), resource_allocator.get(), document)))
						return gen_out_of_memory_syntax_error();

					expr->binary_op = BinaryOp::Comma;
					expr->lhs = lhs;

					lhs = expr.cast_to<ExprNode>();

					if ((syntax_error = parse_expr(-10, expr->rhs)))
						goto gen_bad_expr;

					break;
				}
				default:
					goto end;
			}
		}
	}

end:
	expr_out = lhs;

	return {};

gen_bad_expr:
	if (!(expr_out = make_ast_node<BadExprNode>(resource_allocator.get(), resource_allocator.get(), document, lhs).cast_to<ExprNode>()))
		return gen_out_of_memory_syntax_error();
	expr_out->token_range = { document->main_module, prefix_token->index, parse_context.idx_current_token };
	return syntax_error;
}
