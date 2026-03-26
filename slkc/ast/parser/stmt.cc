#include "../parser.h"

using namespace slkc;

SLKC_API peff::Option<SyntaxError> Parser::parse_var_defs(peff::DynArray<VarDefEntryPtr> &var_def_entries) {
	Token *current_token;
	peff::Option<SyntaxError> syntax_error;

	for (;;) {
		peff::DynArray<AstNodePtr<AttributeNode>> attributes(resource_allocator.get());

		SLKC_RETURN_IF_PARSE_ERROR(parse_attributes(attributes));

		if ((syntax_error = expect_token((current_token = next_token()), TokenId::Id))) {
			if (!syntax_errors.push_back(std::move(syntax_error.value())))
				return gen_out_of_memory_syntax_error();
			syntax_error.reset();
			if (!syntax_errors.push_back(SyntaxError({ document->main_module, current_token->index }, SyntaxErrorKind::ExpectingId)))
				return gen_out_of_memory_syntax_error();
		}

		VarDefEntryPtr entry_ptr(
			peff::alloc_and_construct<VarDefEntry>(
				resource_allocator.get(),
				ASTNODE_ALIGNMENT,
				resource_allocator.get()));
		if (!entry_ptr) {
			return gen_out_of_memory_syntax_error();
		}

		VarDefEntry *entry = entry_ptr.get();

		if (!var_def_entries.push_back(std::move(entry_ptr))) {
			return gen_out_of_memory_syntax_error();
		}

		entry->idx_name_token = current_token->index;
		if (!entry->name.build(current_token->source_text)) {
			return gen_out_of_memory_syntax_error();
		}

		if ((current_token = peek_token())->token_id == TokenId::Colon) {
			next_token();

			if ((syntax_error = parse_type_name(entry->type))) {
				if (!syntax_errors.push_back(std::move(syntax_error.value())))
					return gen_out_of_memory_syntax_error();
				syntax_error.reset();
			}
		}

		if ((current_token = peek_token())->token_id == TokenId::AssignOp) {
			next_token();

			if ((syntax_error = parse_expr(0, entry->initial_value))) {
				if (!syntax_errors.push_back(std::move(syntax_error.value())))
					return gen_out_of_memory_syntax_error();
				syntax_error.reset();
			}
		}

		entry->attributes = std::move(attributes);

		if ((current_token = peek_token())->token_id != TokenId::Comma) {
			break;
		}

		next_token();
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parse_if_stmt(AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<IfStmtNode> if_stmt;

	if (!(if_stmt = make_ast_node<IfStmtNode>(resource_allocator.get(), resource_allocator.get(), document))) {
		return gen_out_of_memory_syntax_error();
	}

	stmt_out = if_stmt.cast_to<StmtNode>();

	Token *l_parenthese_token = peek_token();

	SLKC_RETURN_IF_PARSE_ERROR(expect_token(l_parenthese_token, TokenId::LParenthese));

	next_token();

	{
		static TokenId skipping_terminative_token[] = {
			TokenId::RParenthese,
			TokenId::Semicolon,
			TokenId::RBrace
		};

		if ((syntax_error = parse_expr(0, if_stmt->cond))) {
			SLKC_RETURN_IF_PARSE_ERROR(lookahead_until(std::size(skipping_terminative_token), skipping_terminative_token));
			return syntax_error;
		}
	}

	Token *r_parenthese_token = peek_token();

	SLKC_RETURN_IF_PARSE_ERROR(expect_token(r_parenthese_token, TokenId::RParenthese));

	next_token();

	SLKC_RETURN_IF_PARSE_ERROR(parse_stmt(if_stmt->true_body));

	Token *else_token = peek_token();

	if (else_token->token_id == TokenId::ElseKeyword) {
		next_token();

		SLKC_RETURN_IF_PARSE_ERROR(parse_stmt(if_stmt->false_body));
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parse_with_stmt(AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<WithStmtNode> with_stmt;

	if (!(with_stmt = make_ast_node<WithStmtNode>(
			  resource_allocator.get(),
			  resource_allocator.get(),
			  document))) {
		return gen_out_of_memory_syntax_error();
	}

	stmt_out = with_stmt.cast_to<StmtNode>();

	WithConstraintEntryPtr entry;

	while (true) {
		if (!(entry = WithConstraintEntryPtr(peff::alloc_and_construct<WithConstraintEntry>(resource_allocator.get(), alignof(WithConstraintEntry), resource_allocator.get())))) {
			return gen_out_of_memory_syntax_error();
		}

		Token *name_token;

		SLKC_RETURN_IF_PARSE_ERROR(expect_token((name_token = peek_token()), TokenId::Id));

		if (!entry->generic_param_name.build(name_token->source_text))
			return gen_out_of_memory_syntax_error();

		next_token();

		SLKC_RETURN_IF_PARSE_ERROR(parse_generic_constraint(entry->constraint));

		if (!with_stmt->constraints.push_back(std::move(entry)))
			return gen_out_of_memory_syntax_error();

		if (peek_token()->token_id != TokenId::Comma) {
			break;
		}

		Token *comma_token = next_token();

		/*
		if (!idx_comma_tokens_out.push_back(+comma_token->index))
			return gen_out_of_memory_syntax_error();*/
	}

	SLKC_RETURN_IF_PARSE_ERROR(parse_stmt(with_stmt->true_body));

	Token *else_token = peek_token();

	if (else_token->token_id == TokenId::ElseKeyword) {
		next_token();

		SLKC_RETURN_IF_PARSE_ERROR(parse_stmt(with_stmt->false_body));
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parse_for_stmt(AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<ForStmtNode> for_stmt;

	if (!(for_stmt = make_ast_node<ForStmtNode>(
			  resource_allocator.get(),
			  resource_allocator.get(),
			  document))) {
		return gen_out_of_memory_syntax_error();
	}

	stmt_out = for_stmt.cast_to<StmtNode>();

	Token *l_parenthese_token = peek_token();

	SLKC_RETURN_IF_PARSE_ERROR(expect_token(l_parenthese_token, TokenId::LParenthese));

	next_token();

	Token *var_def_separator_token;
	Token *cond_separator_token;
	Token *r_parenthese_token;
	{
		static TokenId skipping_terminative_token[] = {
			TokenId::RParenthese,
			TokenId::Semicolon,
			TokenId::RBrace
		};

		if ((var_def_separator_token = peek_token())->token_id != TokenId::Semicolon) {
			SLKC_RETURN_IF_PARSE_ERROR(parse_var_defs(for_stmt->var_def_entries));

			SLKC_RETURN_IF_PARSE_ERROR(expect_token((var_def_separator_token = peek_token()), TokenId::Semicolon));
			next_token();
		} else {
			next_token();
		}

		if ((cond_separator_token = peek_token())->token_id != TokenId::Semicolon) {
			if ((syntax_error = parse_expr(0, for_stmt->cond))) {
				SLKC_RETURN_IF_PARSE_ERROR(lookahead_until(std::size(skipping_terminative_token), skipping_terminative_token));
				return syntax_error;
			}

			SLKC_RETURN_IF_PARSE_ERROR(expect_token((cond_separator_token = peek_token()), TokenId::Semicolon));
			next_token();
		} else {
			next_token();
		}

		if ((r_parenthese_token = peek_token())->token_id != TokenId::RParenthese) {
			if ((syntax_error = parse_expr(-10, for_stmt->step))) {
				SLKC_RETURN_IF_PARSE_ERROR(lookahead_until(std::size(skipping_terminative_token), skipping_terminative_token));
				return syntax_error;
			}

			SLKC_RETURN_IF_PARSE_ERROR(expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese));
			next_token();
		} else {
			next_token();
		}
	}

	SLKC_RETURN_IF_PARSE_ERROR(parse_stmt(for_stmt->body));

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parse_while_stmt(AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<WhileStmtNode> while_stmt;

	if (!(while_stmt = make_ast_node<WhileStmtNode>(
			  resource_allocator.get(),
			  resource_allocator.get(),
			  document))) {
		return gen_out_of_memory_syntax_error();
	}

	stmt_out = while_stmt.cast_to<StmtNode>();

	Token *l_parenthese_token = peek_token();

	SLKC_RETURN_IF_PARSE_ERROR(expect_token(l_parenthese_token, TokenId::LParenthese));

	next_token();

	Token *r_parenthese_token;
	{
		static TokenId skipping_terminative_token[] = {
			TokenId::RParenthese,
			TokenId::Semicolon,
			TokenId::RBrace
		};

		if ((syntax_error = parse_expr(0, while_stmt->cond))) {
			SLKC_RETURN_IF_PARSE_ERROR(lookahead_until(std::size(skipping_terminative_token), skipping_terminative_token));
			return syntax_error;
		}

		SLKC_RETURN_IF_PARSE_ERROR(expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese));

		next_token();
	}

	SLKC_RETURN_IF_PARSE_ERROR(parse_stmt(while_stmt->body));

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parse_do_while_stmt(AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<DoWhileStmtNode> while_stmt;

	if (!(while_stmt = make_ast_node<DoWhileStmtNode>(
			  resource_allocator.get(),
			  resource_allocator.get(),
			  document))) {
		return gen_out_of_memory_syntax_error();
	}

	stmt_out = while_stmt.cast_to<StmtNode>();

	SLKC_RETURN_IF_PARSE_ERROR(parse_stmt(while_stmt->body));

	Token *while_token = peek_token();

	SLKC_RETURN_IF_PARSE_ERROR(expect_token(while_token, TokenId::WhileKeyword));

	next_token();

	Token *l_parenthese_token = peek_token();

	SLKC_RETURN_IF_PARSE_ERROR(expect_token(l_parenthese_token, TokenId::LParenthese));

	next_token();

	Token *r_parenthese_token;
	{
		static TokenId skipping_terminative_token[] = {
			TokenId::RParenthese,
			TokenId::Semicolon,
			TokenId::RBrace
		};

		if ((syntax_error = parse_expr(0, while_stmt->cond))) {
			SLKC_RETURN_IF_PARSE_ERROR(lookahead_until(std::size(skipping_terminative_token), skipping_terminative_token));
			return syntax_error;
		}

		SLKC_RETURN_IF_PARSE_ERROR(expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese));

		next_token();
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parse_let_stmt(AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<VarDefStmtNode> stmt;

	if (!(stmt = make_ast_node<VarDefStmtNode>(
			  resource_allocator.get(),
			  resource_allocator.get(),
			  document,
			  peff::DynArray<VarDefEntryPtr>(resource_allocator.get())))) {
		return gen_out_of_memory_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	SLKC_RETURN_IF_PARSE_ERROR(parse_var_defs(stmt->var_def_entries));

	Token *semicolon_token;

	SLKC_RETURN_IF_PARSE_ERROR(expect_token((semicolon_token = peek_token()), TokenId::Semicolon));

	next_token();

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parse_break_stmt(AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<BreakStmtNode> stmt;

	if (!(stmt = make_ast_node<BreakStmtNode>(
			  resource_allocator.get(),
			  resource_allocator.get(),
			  document))) {
		return gen_out_of_memory_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	Token *semicolon_token;

	SLKC_RETURN_IF_PARSE_ERROR(expect_token((semicolon_token = peek_token()), TokenId::Semicolon));

	next_token();

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parse_continue_stmt(AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<ContinueStmtNode> stmt;

	if (!(stmt = make_ast_node<ContinueStmtNode>(
			  resource_allocator.get(),
			  resource_allocator.get(),
			  document))) {
		return gen_out_of_memory_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	Token *semicolon_token;

	SLKC_RETURN_IF_PARSE_ERROR(expect_token((semicolon_token = peek_token()), TokenId::Semicolon));

	next_token();

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parse_return_stmt(AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<ReturnStmtNode> stmt;

	if (!(stmt = make_ast_node<ReturnStmtNode>(
			  resource_allocator.get(),
			  resource_allocator.get(),
			  document,
			  AstNodePtr<ExprNode>()))) {
		return gen_out_of_memory_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	static TokenId skipping_terminative_token[] = {
		TokenId::RParenthese,
		TokenId::Semicolon,
		TokenId::RBrace
	};

	switch (peek_token()->token_id) {
		case TokenId::Semicolon:
			next_token();
			break;
		default:
			if ((syntax_error = parse_expr(0, stmt->value))) {
				SLKC_RETURN_IF_PARSE_ERROR(lookahead_until(std::size(skipping_terminative_token), skipping_terminative_token));
				return syntax_error;
			}

			SLKC_RETURN_IF_PARSE_ERROR(expect_token(peek_token(), TokenId::Semicolon));

			next_token();
			break;
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parse_yield_stmt(AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<YieldStmtNode> stmt;

	if (!(stmt = make_ast_node<YieldStmtNode>(
			  resource_allocator.get(),
			  resource_allocator.get(),
			  document,
			  AstNodePtr<ExprNode>()))) {
		return gen_out_of_memory_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	static TokenId skipping_terminative_token[] = {
		TokenId::RParenthese,
		TokenId::Semicolon,
		TokenId::RBrace
	};

	switch (peek_token()->token_id) {
		case TokenId::Semicolon:
			next_token();
			break;
		default:
			if ((syntax_error = parse_expr(0, stmt->value))) {
				SLKC_RETURN_IF_PARSE_ERROR(lookahead_until(std::size(skipping_terminative_token), skipping_terminative_token));
				return syntax_error;
			}

			SLKC_RETURN_IF_PARSE_ERROR(expect_token(peek_token(), TokenId::Semicolon));

			next_token();
			break;
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parse_label_stmt(AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<LabelStmtNode> stmt;

	if (!(stmt = make_ast_node<LabelStmtNode>(resource_allocator.get(), resource_allocator.get(), document))) {
		return gen_out_of_memory_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	SLKC_RETURN_IF_PARSE_ERROR(expect_token(peek_token(), TokenId::Id));

	Token *name_token = next_token();

	if (!stmt->name.build(name_token->source_text)) {
		return gen_out_of_memory_syntax_error();
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parse_block_stmt(AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<CodeBlockStmtNode> stmt;
	AstNodePtr<StmtNode> cur_stmt;

	if (!(stmt = make_ast_node<CodeBlockStmtNode>(resource_allocator.get(), resource_allocator.get(), document))) {
		return gen_out_of_memory_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	while (true) {
		SLKC_RETURN_IF_PARSE_ERROR(expect_token(peek_token()));

		if (peek_token()->token_id == TokenId::RBrace) {
			break;
		}

		if ((syntax_error = parse_stmt(cur_stmt))) {
			if (!syntax_errors.push_back(std::move(syntax_error.value())))
				return gen_out_of_memory_syntax_error();
		}

		if (cur_stmt) {
			if (!stmt->body.push_back(std::move(cur_stmt))) {
				return gen_out_of_memory_syntax_error();
			}
		}
	}

	Token *r_brace_token;

	SLKC_RETURN_IF_PARSE_ERROR(expect_token((r_brace_token = peek_token()), TokenId::RBrace));

	next_token();

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parse_switch_stmt(AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<SwitchStmtNode> stmt;

	if (!(stmt = make_ast_node<SwitchStmtNode>(resource_allocator.get(), resource_allocator.get(), document))) {
		return gen_out_of_memory_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	Token *l_parenthese_token = peek_token();

	SLKC_RETURN_IF_PARSE_ERROR(expect_token(l_parenthese_token, TokenId::LParenthese));

	next_token();

	SLKC_RETURN_IF_PARSE_ERROR(parse_expr(0, stmt->condition));

	Token *r_parenthese_token = peek_token();

	SLKC_RETURN_IF_PARSE_ERROR(expect_token(r_parenthese_token, TokenId::RParenthese));

	next_token();

	Token *l_brace_token = peek_token();

	SLKC_RETURN_IF_PARSE_ERROR(expect_token(l_brace_token, TokenId::LBrace));

	next_token();

	AstNodePtr<StmtNode> cur_stmt;

	while (true) {
		SLKC_RETURN_IF_PARSE_ERROR(expect_token(peek_token()));

		if (peek_token()->token_id == TokenId::RBrace) {
			break;
		}

		if ((syntax_error = parse_stmt(cur_stmt))) {
			if (!syntax_errors.push_back(std::move(syntax_error.value())))
				return gen_out_of_memory_syntax_error();
		}

		if (cur_stmt) {
			// We detect and push case labels in advance to deal with them easier.
			if (cur_stmt->stmt_kind == StmtKind::CaseLabel) {
				if (!stmt->case_offsets.push_back(stmt->body.size()))
					return gen_out_of_memory_syntax_error();
			}

			if (!stmt->body.push_back(std::move(cur_stmt))) {
				return gen_out_of_memory_syntax_error();
			}
		}
	}

	Token *r_brace_token;

	SLKC_RETURN_IF_PARSE_ERROR(expect_token((r_brace_token = peek_token()), TokenId::RBrace));

	next_token();

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parse_case_stmt(AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<CaseLabelStmtNode> stmt;

	if (!(stmt = make_ast_node<CaseLabelStmtNode>(resource_allocator.get(), resource_allocator.get(), document))) {
		return gen_out_of_memory_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	SLKC_RETURN_IF_PARSE_ERROR(parse_expr(0, stmt->condition));

	SLKC_RETURN_IF_PARSE_ERROR(expect_token(peek_token(), TokenId::Colon));

	next_token();

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parse_default_stmt(AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<CaseLabelStmtNode> stmt;

	if (!(stmt = make_ast_node<CaseLabelStmtNode>(resource_allocator.get(), resource_allocator.get(), document))) {
		return gen_out_of_memory_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	SLKC_RETURN_IF_PARSE_ERROR(expect_token(peek_token(), TokenId::Colon));

	next_token();

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parse_expr_stmt(AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	AstNodePtr<ExprNode> cur_expr;

	AstNodePtr<ExprStmtNode> stmt;

	if (!(stmt = make_ast_node<ExprStmtNode>(resource_allocator.get(), resource_allocator.get(), document))) {
		return gen_out_of_memory_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	if ((syntax_error = parse_expr(-10, stmt->expr))) {
		if (!syntax_errors.push_back(std::move(syntax_error.value())))
			return gen_out_of_memory_syntax_error();
	}

	SLKC_RETURN_IF_PARSE_ERROR(expect_token(peek_token(), TokenId::Semicolon));

	next_token();

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parse_stmt(AstNodePtr<StmtNode> &stmt_out) {
	Token *prefix_token;

	peff::Option<SyntaxError> syntax_error;

	if ((syntax_error = expect_token((prefix_token = peek_token()))))
		goto gen_bad_stmt;

	{
		peff::ScopeGuard set_token_range_guard([this, prefix_token, &stmt_out]() noexcept {
			stmt_out->token_range = TokenRange{ document->main_module, prefix_token->index, parse_context.idx_prev_token };
		});

		switch (prefix_token->token_id) {
			case TokenId::IfKeyword:
				if ((syntax_error = parse_if_stmt(stmt_out)))
					goto gen_bad_stmt;
				break;
			case TokenId::WithKeyword:
				if ((syntax_error = parse_with_stmt(stmt_out)))
					goto gen_bad_stmt;
				break;
			case TokenId::ForKeyword:
				if ((syntax_error = parse_for_stmt(stmt_out)))
					goto gen_bad_stmt;
				break;
			case TokenId::WhileKeyword:
				if ((syntax_error = parse_while_stmt(stmt_out)))
					goto gen_bad_stmt;
				break;
			case TokenId::DoKeyword:
				if ((syntax_error = parse_do_while_stmt(stmt_out)))
					goto gen_bad_stmt;
				break;
			case TokenId::LetKeyword:
				if ((syntax_error = parse_let_stmt(stmt_out)))
					goto gen_bad_stmt;
				break;
			case TokenId::BreakKeyword:
				if ((syntax_error = parse_break_stmt(stmt_out)))
					goto gen_bad_stmt;
				break;
			case TokenId::ContinueKeyword:
				if ((syntax_error = parse_continue_stmt(stmt_out)))
					goto gen_bad_stmt;
				break;
			case TokenId::ReturnKeyword:
				if ((syntax_error = parse_return_stmt(stmt_out)))
					goto gen_bad_stmt;
				break;
			case TokenId::YieldKeyword:
				if ((syntax_error = parse_yield_stmt(stmt_out)))
					goto gen_bad_stmt;
				break;
			case TokenId::Colon:
				if ((syntax_error = parse_label_stmt(stmt_out)))
					goto gen_bad_stmt;
				break;
			case TokenId::CaseKeyword:
				if ((syntax_error = parse_case_stmt(stmt_out)))
					goto gen_bad_stmt;
				break;
			case TokenId::DefaultKeyword:
				if ((syntax_error = parse_default_stmt(stmt_out)))
					goto gen_bad_stmt;
				break;
			case TokenId::SwitchKeyword:
				if ((syntax_error = parse_switch_stmt(stmt_out)))
					goto gen_bad_stmt;
				break;
			case TokenId::LBrace:
				if ((syntax_error = parse_block_stmt(stmt_out)))
					goto gen_bad_stmt;
				break;
			default:
				if ((syntax_error = parse_expr_stmt(stmt_out)))
					goto gen_bad_stmt;
				break;
		}
	}

	return {};

gen_bad_stmt:
	if (!(stmt_out = make_ast_node<BadStmtNode>(resource_allocator.get(), resource_allocator.get(), document, stmt_out).cast_to<StmtNode>()))
		return gen_out_of_memory_syntax_error();
	stmt_out->token_range = { document->main_module, prefix_token->index, parse_context.idx_current_token };
	return syntax_error;
}
