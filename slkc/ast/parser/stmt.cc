#include "../parser.h"

using namespace slkc;

SLKC_API ParseCoroutine Parser::parse_var_defs(peff::Alloc *allocator, peff::DynArray<VarDefEntryPtr> &var_def_entries) {
	Token *current_token;
	peff::Option<SyntaxError> syntax_error;

	for (;;) {
		peff::DynArray<AstNodePtr<AttributeNode>> attributes(resource_allocator.get());

		SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_attributes(this->resource_allocator.get(), attributes));

		if ((syntax_error = expect_token((current_token = next_token()), TokenId::Id))) {
			if (!syntax_errors.push_back(std::move(syntax_error.value())))
				co_return gen_out_of_memory_syntax_error();
			syntax_error.reset();
			if (!syntax_errors.push_back(SyntaxError({ document->main_module, current_token->index }, SyntaxErrorKind::ExpectingId)))
				co_return gen_out_of_memory_syntax_error();
		}

		VarDefEntryPtr entry_ptr(
			peff::alloc_and_construct<VarDefEntry>(
				resource_allocator.get(),
				ASTNODE_ALIGNMENT,
				resource_allocator.get()));
		if (!entry_ptr) {
			co_return gen_out_of_memory_syntax_error();
		}

		VarDefEntry *entry = entry_ptr.get();

		if (!var_def_entries.push_back(std::move(entry_ptr))) {
			co_return gen_out_of_memory_syntax_error();
		}

		entry->idx_name_token = current_token->index;
		if (!entry->name.build(current_token->source_text)) {
			co_return gen_out_of_memory_syntax_error();
		}

		if ((current_token = peek_token())->token_id == TokenId::Colon) {
			next_token();

			if ((syntax_error = co_await (parse_type_name(this->resource_allocator.get(), entry->type)(this)))) {
				if (!syntax_errors.push_back(std::move(syntax_error.value())))
					co_return gen_out_of_memory_syntax_error();
				syntax_error.reset();
			}
		}

		if ((current_token = peek_token())->token_id == TokenId::AssignOp) {
			next_token();

			if ((syntax_error = co_await (parse_expr(this->resource_allocator.get(), 0, entry->initial_value)(this)))) {
				if (!syntax_errors.push_back(std::move(syntax_error.value())))
					co_return gen_out_of_memory_syntax_error();
				syntax_error.reset();
			}
		}

		entry->attributes = std::move(attributes);

		if ((current_token = peek_token())->token_id != TokenId::Comma) {
			break;
		}

		next_token();
	}

	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_if_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<IfStmtNode> if_stmt;

	if (!(if_stmt = make_ast_node<IfStmtNode>(resource_allocator.get(), resource_allocator.get(), document))) {
		co_return gen_out_of_memory_syntax_error();
	}

	stmt_out = if_stmt.cast_to<StmtNode>();

	Token *l_parenthese_token = peek_token();

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(l_parenthese_token, TokenId::LParenthese));

	next_token();

	{
		static TokenId skipping_terminative_token[] = {
			TokenId::RParenthese,
			TokenId::Semicolon,
			TokenId::RBrace
		};

		if ((syntax_error = co_await (parse_expr(this->resource_allocator.get(), 0, if_stmt->cond)(this)))) {
			SLKC_CO_RETURN_IF_PARSE_ERROR(lookahead_until(std::size(skipping_terminative_token), skipping_terminative_token));
			co_return syntax_error;
		}
	}

	Token *r_parenthese_token = peek_token();

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(r_parenthese_token, TokenId::RParenthese));

	next_token();

	SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_stmt(this->resource_allocator.get(), if_stmt->true_body));

	Token *else_token = peek_token();

	if (else_token->token_id == TokenId::ElseKeyword) {
		next_token();

		SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_stmt(this->resource_allocator.get(), if_stmt->false_body));
	}

	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_with_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<WithStmtNode> with_stmt;

	if (!(with_stmt = make_ast_node<WithStmtNode>(
			  resource_allocator.get(),
			  resource_allocator.get(),
			  document))) {
		co_return gen_out_of_memory_syntax_error();
	}

	stmt_out = with_stmt.cast_to<StmtNode>();

	WithConstraintEntryPtr entry;

	while (true) {
		if (!(entry = WithConstraintEntryPtr(peff::alloc_and_construct<WithConstraintEntry>(resource_allocator.get(), alignof(WithConstraintEntry), resource_allocator.get())))) {
			co_return gen_out_of_memory_syntax_error();
		}

		Token *name_token;

		SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((name_token = peek_token()), TokenId::Id));

		if (!entry->generic_param_name.build(name_token->source_text))
			co_return gen_out_of_memory_syntax_error();

		next_token();

		SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_generic_constraint(this->resource_allocator.get(), entry->constraint));

		if (!with_stmt->constraints.push_back(std::move(entry)))
			co_return gen_out_of_memory_syntax_error();

		if (peek_token()->token_id != TokenId::Comma) {
			break;
		}

		Token *comma_token = next_token();

		/*
		if (!idx_comma_tokens_out.push_back(+comma_token->index))
			co_return gen_out_of_memory_syntax_error();*/
	}

	SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_stmt(this->resource_allocator.get(), with_stmt->true_body));

	Token *else_token = peek_token();

	if (else_token->token_id == TokenId::ElseKeyword) {
		next_token();

		SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_stmt(this->resource_allocator.get(), with_stmt->false_body));
	}

	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_for_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<ForStmtNode> for_stmt;

	if (!(for_stmt = make_ast_node<ForStmtNode>(
			  resource_allocator.get(),
			  resource_allocator.get(),
			  document))) {
		co_return gen_out_of_memory_syntax_error();
	}

	stmt_out = for_stmt.cast_to<StmtNode>();

	Token *l_parenthese_token = peek_token();

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(l_parenthese_token, TokenId::LParenthese));

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
			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_var_defs(this->resource_allocator.get(), for_stmt->var_def_entries));

			SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((var_def_separator_token = peek_token()), TokenId::Semicolon));
			next_token();
		} else {
			next_token();
		}

		if ((cond_separator_token = peek_token())->token_id != TokenId::Semicolon) {
			if ((syntax_error = co_await (parse_expr(this->resource_allocator.get(), 0, for_stmt->cond)(this)))) {
				SLKC_CO_RETURN_IF_PARSE_ERROR(lookahead_until(std::size(skipping_terminative_token), skipping_terminative_token));
				co_return syntax_error;
			}

			SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((cond_separator_token = peek_token()), TokenId::Semicolon));
			next_token();
		} else {
			next_token();
		}

		if ((r_parenthese_token = peek_token())->token_id != TokenId::RParenthese) {
			if ((syntax_error = co_await (parse_expr(this->resource_allocator.get(), -10, for_stmt->step)(this)))) {
				SLKC_CO_RETURN_IF_PARSE_ERROR(lookahead_until(std::size(skipping_terminative_token), skipping_terminative_token));
				co_return syntax_error;
			}

			SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese));
			next_token();
		} else {
			next_token();
		}
	}

	SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_stmt(this->resource_allocator.get(), for_stmt->body));

	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_while_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<WhileStmtNode> while_stmt;

	if (!(while_stmt = make_ast_node<WhileStmtNode>(
			  resource_allocator.get(),
			  resource_allocator.get(),
			  document))) {
		co_return gen_out_of_memory_syntax_error();
	}

	stmt_out = while_stmt.cast_to<StmtNode>();

	Token *l_parenthese_token = peek_token();

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(l_parenthese_token, TokenId::LParenthese));

	next_token();

	Token *r_parenthese_token;
	{
		static TokenId skipping_terminative_token[] = {
			TokenId::RParenthese,
			TokenId::Semicolon,
			TokenId::RBrace
		};

		if ((syntax_error = co_await (parse_expr(this->resource_allocator.get(), 0, while_stmt->cond)(this)))) {
			SLKC_CO_RETURN_IF_PARSE_ERROR(lookahead_until(std::size(skipping_terminative_token), skipping_terminative_token));
			co_return syntax_error;
		}

		SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese));

		next_token();
	}

	SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_stmt(this->resource_allocator.get(), while_stmt->body));

	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_do_while_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<DoWhileStmtNode> while_stmt;

	if (!(while_stmt = make_ast_node<DoWhileStmtNode>(
			  resource_allocator.get(),
			  resource_allocator.get(),
			  document))) {
		co_return gen_out_of_memory_syntax_error();
	}

	stmt_out = while_stmt.cast_to<StmtNode>();

	SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_stmt(this->resource_allocator.get(), while_stmt->body));

	Token *while_token = peek_token();

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(while_token, TokenId::WhileKeyword));

	next_token();

	Token *l_parenthese_token = peek_token();

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(l_parenthese_token, TokenId::LParenthese));

	next_token();

	Token *r_parenthese_token;
	{
		static TokenId skipping_terminative_token[] = {
			TokenId::RParenthese,
			TokenId::Semicolon,
			TokenId::RBrace
		};

		if ((syntax_error = co_await (parse_expr(this->resource_allocator.get(), 0, while_stmt->cond)(this)))) {
			SLKC_CO_RETURN_IF_PARSE_ERROR(lookahead_until(std::size(skipping_terminative_token), skipping_terminative_token));
			co_return syntax_error;
		}

		SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese));

		next_token();
	}

	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_let_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<VarDefStmtNode> stmt;

	if (!(stmt = make_ast_node<VarDefStmtNode>(
			  resource_allocator.get(),
			  resource_allocator.get(),
			  document,
			  peff::DynArray<VarDefEntryPtr>(resource_allocator.get())))) {
		co_return gen_out_of_memory_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_var_defs(this->resource_allocator.get(), stmt->var_def_entries));

	Token *semicolon_token;

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((semicolon_token = peek_token()), TokenId::Semicolon));

	next_token();

	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_break_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<BreakStmtNode> stmt;

	if (!(stmt = make_ast_node<BreakStmtNode>(
			  resource_allocator.get(),
			  resource_allocator.get(),
			  document))) {
		co_return gen_out_of_memory_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	Token *semicolon_token;

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((semicolon_token = peek_token()), TokenId::Semicolon));

	next_token();

	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_continue_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<ContinueStmtNode> stmt;

	if (!(stmt = make_ast_node<ContinueStmtNode>(
			  resource_allocator.get(),
			  resource_allocator.get(),
			  document))) {
		co_return gen_out_of_memory_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	Token *semicolon_token;

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((semicolon_token = peek_token()), TokenId::Semicolon));

	next_token();

	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_return_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<ReturnStmtNode> stmt;

	if (!(stmt = make_ast_node<ReturnStmtNode>(
			  resource_allocator.get(),
			  resource_allocator.get(),
			  document,
			  AstNodePtr<ExprNode>()))) {
		co_return gen_out_of_memory_syntax_error();
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
			if ((syntax_error = co_await (parse_expr(this->resource_allocator.get(), 0, stmt->value)(this)))) {
				SLKC_CO_RETURN_IF_PARSE_ERROR(lookahead_until(std::size(skipping_terminative_token), skipping_terminative_token));
				co_return syntax_error;
			}

			SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(peek_token(), TokenId::Semicolon));

			next_token();
			break;
	}

	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_yield_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<YieldStmtNode> stmt;

	if (!(stmt = make_ast_node<YieldStmtNode>(
			  resource_allocator.get(),
			  resource_allocator.get(),
			  document,
			  AstNodePtr<ExprNode>()))) {
		co_return gen_out_of_memory_syntax_error();
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
			if ((syntax_error = co_await (parse_expr(this->resource_allocator.get(), 0, stmt->value)(this)))) {
				SLKC_CO_RETURN_IF_PARSE_ERROR(lookahead_until(std::size(skipping_terminative_token), skipping_terminative_token));
				co_return syntax_error;
			}

			SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(peek_token(), TokenId::Semicolon));

			next_token();
			break;
	}

	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_label_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<LabelStmtNode> stmt;

	if (!(stmt = make_ast_node<LabelStmtNode>(resource_allocator.get(), resource_allocator.get(), document))) {
		co_return gen_out_of_memory_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(peek_token(), TokenId::Id));

	Token *name_token = next_token();

	if (!stmt->name.build(name_token->source_text)) {
		co_return gen_out_of_memory_syntax_error();
	}

	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_block_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<CodeBlockStmtNode> stmt;
	AstNodePtr<StmtNode> cur_stmt;

	if (!(stmt = make_ast_node<CodeBlockStmtNode>(resource_allocator.get(), resource_allocator.get(), document))) {
		co_return gen_out_of_memory_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	while (true) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(peek_token()));

		if (peek_token()->token_id == TokenId::RBrace) {
			break;
		}

		if ((syntax_error = co_await (parse_stmt(this->resource_allocator.get(), cur_stmt)(this)))) {
			if (!syntax_errors.push_back(std::move(syntax_error.value())))
				co_return gen_out_of_memory_syntax_error();
		}

		if (cur_stmt) {
			if (!stmt->body.push_back(std::move(cur_stmt))) {
				co_return gen_out_of_memory_syntax_error();
			}
		}
	}

	Token *r_brace_token;

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((r_brace_token = peek_token()), TokenId::RBrace));

	next_token();

	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_switch_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<SwitchStmtNode> stmt;

	if (!(stmt = make_ast_node<SwitchStmtNode>(resource_allocator.get(), resource_allocator.get(), document))) {
		co_return gen_out_of_memory_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	Token *l_parenthese_token = peek_token();

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(l_parenthese_token, TokenId::LParenthese));

	next_token();

	SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_expr(this->resource_allocator.get(), 0, stmt->condition));

	Token *r_parenthese_token = peek_token();

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(r_parenthese_token, TokenId::RParenthese));

	next_token();

	Token *l_brace_token = peek_token();

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(l_brace_token, TokenId::LBrace));

	next_token();

	AstNodePtr<StmtNode> cur_stmt;

	while (true) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(peek_token()));

		if (peek_token()->token_id == TokenId::RBrace) {
			break;
		}

		if ((syntax_error = co_await (parse_stmt(this->resource_allocator.get(), cur_stmt)(this)))) {
			if (!syntax_errors.push_back(std::move(syntax_error.value())))
				co_return gen_out_of_memory_syntax_error();
		}

		if (cur_stmt) {
			// We detect and push case labels in advance to deal with them easier.
			if (cur_stmt->stmt_kind == StmtKind::CaseLabel) {
				if (!stmt->case_offsets.push_back(stmt->body.size()))
					co_return gen_out_of_memory_syntax_error();
			}

			if (!stmt->body.push_back(std::move(cur_stmt))) {
				co_return gen_out_of_memory_syntax_error();
			}
		}
	}

	Token *r_brace_token;

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((r_brace_token = peek_token()), TokenId::RBrace));

	next_token();

	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_case_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<CaseLabelStmtNode> stmt;

	if (!(stmt = make_ast_node<CaseLabelStmtNode>(resource_allocator.get(), resource_allocator.get(), document))) {
		co_return gen_out_of_memory_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_expr(this->resource_allocator.get(), 0, stmt->condition));

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(peek_token(), TokenId::Colon));

	next_token();

	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_default_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	AstNodePtr<CaseLabelStmtNode> stmt;

	if (!(stmt = make_ast_node<CaseLabelStmtNode>(resource_allocator.get(), resource_allocator.get(), document))) {
		co_return gen_out_of_memory_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(peek_token(), TokenId::Colon));

	next_token();

	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_expr_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	AstNodePtr<ExprNode> cur_expr;

	AstNodePtr<ExprStmtNode> stmt;

	if (!(stmt = make_ast_node<ExprStmtNode>(resource_allocator.get(), resource_allocator.get(), document))) {
		co_return gen_out_of_memory_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	if ((syntax_error = co_await (parse_expr(this->resource_allocator.get(), -10, stmt->expr)(this)))) {
		if (!syntax_errors.push_back(std::move(syntax_error.value())))
			co_return gen_out_of_memory_syntax_error();
	}

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(peek_token(), TokenId::Semicolon));

	next_token();

	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out) {
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
				if ((syntax_error = co_await (parse_if_stmt(this->resource_allocator.get(), stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::WithKeyword:
				if ((syntax_error = co_await (parse_with_stmt(this->resource_allocator.get(), stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::ForKeyword:
				if ((syntax_error = co_await (parse_for_stmt(this->resource_allocator.get(), stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::WhileKeyword:
				if ((syntax_error = co_await (parse_while_stmt(this->resource_allocator.get(), stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::DoKeyword:
				if ((syntax_error = co_await (parse_do_while_stmt(this->resource_allocator.get(), stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::LetKeyword:
				if ((syntax_error = co_await (parse_let_stmt(this->resource_allocator.get(), stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::BreakKeyword:
				if ((syntax_error = co_await (parse_break_stmt(this->resource_allocator.get(), stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::ContinueKeyword:
				if ((syntax_error = co_await (parse_continue_stmt(this->resource_allocator.get(), stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::ReturnKeyword:
				if ((syntax_error = co_await (parse_return_stmt(this->resource_allocator.get(), stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::YieldKeyword:
				if ((syntax_error = co_await (parse_yield_stmt(this->resource_allocator.get(), stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::Colon:
				if ((syntax_error = co_await (parse_label_stmt(this->resource_allocator.get(), stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::CaseKeyword:
				if ((syntax_error = co_await (parse_case_stmt(this->resource_allocator.get(), stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::DefaultKeyword:
				if ((syntax_error = co_await (parse_default_stmt(this->resource_allocator.get(), stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::SwitchKeyword:
				if ((syntax_error = co_await (parse_switch_stmt(this->resource_allocator.get(), stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::LBrace:
				if ((syntax_error = co_await (parse_block_stmt(this->resource_allocator.get(), stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			default:
				if ((syntax_error = co_await (parse_expr_stmt(this->resource_allocator.get(), stmt_out)(this))))
					goto gen_bad_stmt;
				break;
		}
	}

	co_return {};

gen_bad_stmt:
	if (!(stmt_out = make_ast_node<BadStmtNode>(resource_allocator.get(), resource_allocator.get(), document, stmt_out).cast_to<StmtNode>()))
		co_return gen_out_of_memory_syntax_error();
	stmt_out->token_range = { document->main_module, prefix_token->index, parse_context.idx_current_token };
	co_return syntax_error;
}
