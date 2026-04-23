#include "parser.h"
#include "import.h"
#include "class.h"

using namespace slkc;

SLAKE_FORCEINLINE peff::Option<SyntaxError> ParseCoroutine::resume(Parser *parser) {
	if (!coro_handle)
		return parser->gen_oom_syntax_error();

	coro_handle.resume();

	while (parser->parse_coro_scheduler.task_list.size()) {
		auto h = parser->parse_coro_scheduler.task_list.back();
		parser->parse_coro_scheduler.task_list.pop_back();
		if (!h.done())
			h.resume();
		if (coro_handle.promise().result)
			return std::move(coro_handle.promise().result);
	}

	if (coro_handle.promise().result)
		return std::move(coro_handle.promise().result);
	if (!coro_handle.done())
		std::terminate();

	return {};
}

SLKC_API ParseCoroutine::Awaitable::Awaitable(
	ParseCoroutine &co,
	Parser *parser,
	ParseCoroutineScheduler *scheduler,
	Handle handle)
	: co(co),
	  parser(parser),
	  scheduler(scheduler),
	  handle(std::move(handle)) {
}

SLAKE_FORCEINLINE bool ParseCoroutine::Awaitable::await_ready() {
	return false;
}

SLAKE_FORCEINLINE void ParseCoroutine::Awaitable::await_suspend(Handle h) {
	if (!scheduler->task_list.push_back(std::move(h))) {
		co.coro_handle.promise().result = parser->gen_oom_syntax_error();
		return;
	}
	if (!scheduler->task_list.push_back(Handle(handle))) {
		co.coro_handle.promise().result = parser->gen_oom_syntax_error();
		return;
	}
}

SLAKE_FORCEINLINE peff::Option<SyntaxError> ParseCoroutine::Awaitable::await_resume() {
	if (handle) {
		if (handle.promise().result)
			return std::move(handle.promise().result);
		return {};
	}
	return parser->gen_oom_syntax_error();
}

SLKC_API ParseCoroutine::Awaitable ParseCoroutine::operator()(Parser *parser) {
	return Awaitable(*this, parser, &parser->parse_coro_scheduler, coro_handle);
}

SLKC_API ParseCoroutineScheduler::ParseCoroutineScheduler(peff::Alloc *allocator) : task_list(allocator) {
}

SLKC_API Parser::Parser(peff::SharedPtr<Document> document,
	TokenList &&token_list,
	peff::Alloc *resource_allocator)
	: document(std::move(document)),
	  token_list(std::move(token_list)),
	  resource_allocator(resource_allocator),
	  syntax_errors(resource_allocator),
	  syntax_warnings(resource_allocator),
	  parse_coro_scheduler(resource_allocator) {
}

SLKC_API Parser::~Parser() {
}

SLKC_API ParseCoroutine Parser::parse_operator_name(peff::Alloc *allocator, std::string_view &name_out) {
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

			SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token(peek_token(), TokenId::RParenthese)));

			name_out = "()";
			break;
		case TokenId::LBracket:
			next_token();

			SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token(peek_token(), TokenId::RBracket)));

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
			co_return SyntaxError(TokenRange{ get_document()->main_module, t->index }, SyntaxErrorKind::ExpectingOperatorName);
	}
	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_id_name(peff::Alloc *allocator, peff::String &name_out) {
	peff::Option<SyntaxError> syntax_error;
	Token *t = peek_token();

	switch (t->token_id) {
		case TokenId::Id:
			if (!name_out.build(t->source_text)) {
				co_return gen_oom_syntax_error();
			}
			next_token();
			break;
		default:
			co_return SyntaxError(TokenRange{ get_document()->main_module, t->index }, SyntaxErrorKind::ExpectingId);
	}
	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_id_ref(peff::Alloc *allocator, IdRefPtr &id_ref_out, bool is_parsing_type) {
	peff::Option<SyntaxError> syntax_error;
	IdRefPtr id_ref_ptr(peff::alloc_and_construct<IdRef>(resource_allocator.get(), ASTNODE_ALIGNMENT, resource_allocator.get()));
	if (!id_ref_ptr)
		co_return gen_oom_syntax_error();
	Token *t = peek_token();

	id_ref_ptr->token_range = TokenRange{ get_document()->main_module, t->index };

	if (t->token_id == TokenId::ThisKeyword) {
		next_token();

		IdRefEntry entry(resource_allocator.get());
		peff::String id_text(resource_allocator.get());
		if (!id_text.build("this")) {
			co_return gen_oom_syntax_error();
		}

		entry.name = std::move(id_text);
		entry.name_token_index = t->index;

		if (!id_ref_ptr->entries.push_back(std::move(entry)))
			co_return gen_oom_syntax_error();

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
			co_return gen_oom_syntax_error();
	}

	for (;;) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(t = peek_token(), TokenId::Id));
		next_token();

		IdRefEntry entry(resource_allocator.get());
		peff::String id_text(resource_allocator.get());
		if (!id_text.build(t->source_text)) {
			co_return gen_oom_syntax_error();
		}

		entry.name = std::move(id_text);
		entry.name_token_index = t->index;
		id_ref_ptr->token_range.end_index = t->index;

		do {
			if (is_parsing_type) {
				if ((t = peek_token())->token_id == TokenId::LtOp) {
					entry.generic_scope_token_index = t->index;

					next_token();

					for (;;) {
						AstNodePtr<TypeNameNode> generic_arg;
						SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_generic_arg(this->resource_allocator.get(), generic_arg));
						if (!entry.generic_args.push_back(std::move(generic_arg))) {
							co_return gen_oom_syntax_error();
						}

						if ((t = peek_token())->token_id != TokenId::Comma) {
							break;
						}

						if (!entry.generic_args_comma_token_indices.push_back(+t->index))
							co_return gen_oom_syntax_error();

						next_token();
					}

					SLKC_CO_RETURN_IF_PARSE_ERROR(split_shr_op_token());
					SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(t = peek_token(), TokenId::GtOp));
					id_ref_ptr->token_range.end_index = t->index;
					next_token();
					break;
				}
			}
			if ((t = peek_token())->token_id == TokenId::ScopeOp) {
				next_token();

				if (is_parsing_type) {
					if (!syntax_warnings.push_back(SyntaxWarning(TokenRange{ parse_context.mod, t->index }, SyntaxWarningKind::ScopeOpIsOmittableInIdRef)))
						co_return gen_oom_syntax_error();
				}

				entry.generic_scope_token_index = t->index;

				SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(t = peek_token(), TokenId::LtOp));
				next_token();

				for (;;) {
					AstNodePtr<TypeNameNode> generic_arg;
					SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_generic_arg(this->resource_allocator.get(), generic_arg));
					if (!entry.generic_args.push_back(std::move(generic_arg))) {
						co_return gen_oom_syntax_error();
					}

					if ((t = peek_token())->token_id != TokenId::Comma) {
						break;
					}

					if (!entry.generic_args_comma_token_indices.push_back(+t->index))
						co_return gen_oom_syntax_error();

					next_token();
				}

				SLKC_CO_RETURN_IF_PARSE_ERROR(split_shr_op_token());
				SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(t = peek_token(), TokenId::GtOp));
				id_ref_ptr->token_range.end_index = t->index;

				next_token();
			}
		} while (false);

		if (!id_ref_ptr->entries.push_back(std::move(entry)))
			co_return gen_oom_syntax_error();

		if ((t = peek_token())->token_id != TokenId::Dot) {
			break;
		}

		entry.access_op_token_index = t->index;
		id_ref_ptr->token_range.end_index = t->index;

		next_token();
	}

end:
	id_ref_out = std::move(id_ref_ptr);

	co_return {};
}

[[nodiscard]] SLKC_API ParseCoroutine Parser::parse_args(peff::Alloc *allocator, peff::DynArray<AstNodePtr<ExprNode>> &args_out, peff::DynArray<size_t> &idx_comma_tokens_out) {
	while (true) {
		if (peek_token()->token_id == TokenId::RParenthese) {
			break;
		}

		AstNodePtr<ExprNode> arg;

		SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_expr(this->resource_allocator.get(), 0, arg));

		if (!args_out.push_back(std::move(arg)))
			co_return gen_oom_syntax_error();

		if (peek_token()->token_id != TokenId::Comma) {
			break;
		}

		Token *comma_token = next_token();
		if (!idx_comma_tokens_out.push_back(+comma_token->index))
			co_return gen_oom_syntax_error();
	}

	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_fn(peff::Alloc *allocator, AstNodePtr<FnOverloadingNode> &fn_node_out) {
	peff::Option<SyntaxError> syntax_error;

	Token *fn_token;
	Token *lvalue_marker_token = nullptr;

	peff::String name(resource_allocator.get());

	if (!(fn_node_out = make_ast_node<FnOverloadingNode>(resource_allocator.get(), resource_allocator.get(), get_document())))
		co_return gen_oom_syntax_error();

	if (!(fn_node_out->alloc_scope()))
		co_return gen_oom_syntax_error();

	switch ((fn_token = peek_token())->token_id) {
		case TokenId::FnKeyword: {
			next_token();

			fn_node_out->overloading_kind = FnOverloadingKind::Regular;

			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_id_name(this->resource_allocator.get(), name));
			break;
		}
		case TokenId::AsyncKeyword: {
			next_token();

			fn_node_out->overloading_kind = FnOverloadingKind::Coroutine;

			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_id_name(this->resource_allocator.get(), name));
			break;
		}
		case TokenId::OperatorKeyword: {
			next_token();

			fn_node_out->overloading_kind = FnOverloadingKind::Regular;

			Token *lvalue_marker_token;
			if ((lvalue_marker_token = peek_token())->token_id == TokenId::AssignOp) {
				fn_node_out->fn_flags |= FN_LVALUE;
				next_token();
			}

			std::string_view operator_name;
			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_operator_name(this->resource_allocator.get(), operator_name));

			if (!name.build(operator_name)) {
				co_return gen_oom_syntax_error();
			}

			if (fn_node_out->fn_flags & FN_LVALUE) {
				if (!name.append(LVALUE_OPERATOR_NAME_SUFFIX))
					co_return gen_oom_syntax_error();
			}
			break;
		}
		case TokenId::DefKeyword: {
			next_token();

			fn_node_out->overloading_kind = FnOverloadingKind::Pure;

			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_id_name(this->resource_allocator.get(), name));
			break;
		}
		default:
			co_return SyntaxError(TokenRange{ get_document()->main_module, fn_token->index }, SyntaxErrorKind::UnexpectedToken);
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
		fn_node_out->token_range = TokenRange{ get_document()->main_module, fn_token->index, parse_context.idx_prev_token };
	});

	fn_node_out->name = std::move(name);

	SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_generic_params(this->resource_allocator.get(), fn_node_out->scope->generic_params, fn_node_out->idx_generic_param_comma_tokens, fn_node_out->l_angle_bracket_index, fn_node_out->r_angle_bracket_index));
	for (size_t i = 0; i < fn_node_out->scope->generic_params.size(); ++i) {
		auto gp = fn_node_out->scope->generic_params.at(i);
		if (fn_node_out->scope->generic_param_indices.contains(gp->name)) {
			peff::String s(resource_allocator.get());

			if (!s.build(gp->name)) {
				co_return gen_oom_syntax_error();
			}

			ConflictingDefinitionsErrorExData ex_data(std::move(s));

			co_return SyntaxError(gp->token_range, std::move(ex_data));
		}
		if (!fn_node_out->scope->generic_param_indices.insert(gp->name, +i))
			co_return gen_oom_syntax_error();
	}

	bool has_var_arg = false;
	SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_params(this->resource_allocator.get(), fn_node_out->params, has_var_arg, fn_node_out->idx_param_comma_tokens, fn_node_out->l_parenthese_index, fn_node_out->r_parenthese_index));
	if (has_var_arg) {
		fn_node_out->fn_flags |= FN_VARG;
	}
	// Index the parameters.
	for (size_t i = 0; i < fn_node_out->params.size(); ++i) {
		AstNodePtr<VarNode> &cur_param = fn_node_out->params.at(i);
		if (fn_node_out->param_indices.contains(cur_param->name)) {
			peff::String s(resource_allocator.get());

			if (!s.build(cur_param->name)) {
				co_return gen_oom_syntax_error();
			}

			ConflictingDefinitionsErrorExData ex_data(std::move(s));

			if (!syntax_errors.push_back(SyntaxError(cur_param->token_range, std::move(ex_data))))
				co_return gen_oom_syntax_error();
		}

		if (!fn_node_out->param_indices.insert(cur_param->name, +i)) {
			co_return gen_oom_syntax_error();
		}
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
				SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(this->resource_allocator.get(), fn_node_out->overriden_type));
				break;
		}
	}

	Token *return_type_token;
	if ((return_type_token = peek_token())->token_id == TokenId::ReturnTypeOp) {
		next_token();
		SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(this->resource_allocator.get(), fn_node_out->return_type));
	} else {
		if (!(fn_node_out->return_type = make_ast_node<VoidTypeNameNode>(resource_allocator.get(), resource_allocator.get(), get_document()).cast_to<TypeNameNode>())) {
			co_return gen_oom_syntax_error();
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

			AstNodePtr<StmtNode> cur_stmt;

			if (!(fn_node_out->body = make_ast_node<CodeBlockStmtNode>(resource_allocator.get(), resource_allocator.get(), get_document()))) {
				co_return gen_oom_syntax_error();
			}

			while (true) {
				SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(peek_token()));

				if (peek_token()->token_id == TokenId::RBrace) {
					break;
				}

				if ((syntax_error = co_await (parse_stmt(this->resource_allocator.get(), cur_stmt)(this)))) {
					if (!syntax_errors.push_back(std::move(syntax_error.value())))
						co_return gen_oom_syntax_error();
				}

				if (cur_stmt) {
					if (!fn_node_out->body->body.push_back(std::move(cur_stmt))) {
						co_return gen_oom_syntax_error();
					}
				}
			}

			Token *r_brace_token;

			SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((r_brace_token = peek_token()), TokenId::RBrace)));

			next_token();
			break;
		}
		default:
			co_return SyntaxError(
				TokenRange{ get_document()->main_module, body_token->index },
				SyntaxErrorKind::UnexpectedToken);
	}

	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_union_enum_item(peff::Alloc *allocator, AstNodePtr<ModuleNode> enum_out) {
	peff::Option<SyntaxError> syntax_error;

	Token *name_token;
	SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((name_token = peek_token()), TokenId::Id)));

	next_token();

	switch (Token *token = peek_token(); token->token_id) {
		case TokenId::LParenthese: {
			AstNodePtr<UnionEnumItemNode> enum_item;
			if (!(enum_item = make_ast_node<UnionEnumItemNode>(resource_allocator.get(), resource_allocator.get(), get_document())))
				co_return gen_oom_syntax_error();
			if (!enum_item->alloc_scope())
				co_return gen_oom_syntax_error();
			next_token();

			if (!enum_item->name.build(name_token->source_text))
				co_return gen_oom_syntax_error();

			size_t idx_member;
			{
				peff::ScopeGuard set_token_range_guard([this, token, enum_item]() noexcept {
					enum_item->token_range = TokenRange{ get_document()->main_module, token->index, parse_context.idx_prev_token };
				});

				if ((idx_member = enum_out->scope->push_member(enum_item.cast_to<MemberNode>())) == SIZE_MAX)
					co_return gen_oom_syntax_error();

				while (true) {
					AstNodePtr<VarNode> enum_item_entry;

					if (!(enum_item_entry = make_ast_node<VarNode>(resource_allocator.get(), resource_allocator.get(), get_document())))
						co_return gen_oom_syntax_error();

					size_t idx_entry_member;
					{
						peff::ScopeGuard set_enum_item_token_range_guard([this, token, enum_item]() noexcept {
							enum_item->token_range = TokenRange{ get_document()->main_module, token->index, parse_context.idx_prev_token };
						});
						if ((idx_entry_member = enum_item->scope->push_member(enum_item_entry.cast_to<MemberNode>())) == SIZE_MAX) {
							co_return gen_oom_syntax_error();
						}

						Token *entry_name_token;
						SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((entry_name_token = peek_token()), TokenId::Id)));

						next_token();

						if (!enum_item_entry->name.build(entry_name_token->source_text))
							co_return gen_oom_syntax_error();

						Token *colon_token;
						SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((colon_token = peek_token()), TokenId::Colon)));

						next_token();

						SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(this->resource_allocator.get(), enum_item_entry->type));
					}

					if (auto it = enum_item->scope->_member_indices.find(enum_item_entry->name); it != enum_item->scope->_member_indices.end()) {
						peff::String s(resource_allocator.get());

						if (!s.build(enum_item_entry->name)) {
							co_return gen_oom_syntax_error();
						}

						ConflictingDefinitionsErrorExData ex_data(std::move(s));

						co_return SyntaxError(enum_item->token_range, std::move(ex_data));
					} else {
						if (!(enum_item->scope->index_member(idx_entry_member))) {
							co_return gen_oom_syntax_error();
						}
					}

					if (peek_token()->token_id != TokenId::Comma)
						break;
					Token *comma_token = next_token();
				}
			}
			Token *r_brace_token;
			SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((r_brace_token = peek_token()), TokenId::RParenthese));
			next_token();

			if (auto it = enum_out->scope->_member_indices.find(enum_item->name); it != enum_out->scope->_member_indices.end()) {
				peff::String s(resource_allocator.get());

				if (!s.build(enum_item->name)) {
					co_return gen_oom_syntax_error();
				}

				ConflictingDefinitionsErrorExData ex_data(std::move(s));

				co_return SyntaxError(enum_item->token_range, std::move(ex_data));
			} else {
				if (!(enum_out->scope->index_member(idx_member))) {
					co_return gen_oom_syntax_error();
				}
			}
			break;
		}
		default:
			co_return SyntaxError(TokenRange{ get_document()->main_module, token->index }, SyntaxErrorKind::UnexpectedToken);
	}

	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_enum_item(peff::Alloc *allocator, AstNodePtr<ModuleNode> enum_out) {
	peff::Option<SyntaxError> syntax_error;

	AstNodePtr<EnumItemNode> enum_item;
	if (!(enum_item = make_ast_node<EnumItemNode>(resource_allocator.get(), resource_allocator.get(), get_document())))
		co_return gen_oom_syntax_error();

	if (!(enum_item->alloc_scope()))
		co_return gen_oom_syntax_error();

	size_t idx_member;
	{
		peff::ScopeGuard set_token_range_guard([this, token = peek_token(), enum_item]() noexcept {
			enum_item->token_range = TokenRange{ get_document()->main_module, token->index, parse_context.idx_prev_token };
		});

		if ((idx_member = enum_out->scope->push_member(enum_item.cast_to<MemberNode>())) == SIZE_MAX) {
			co_return gen_oom_syntax_error();
		}

		Token *name_token;
		SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((name_token = peek_token()), TokenId::Id)));

		next_token();

		if (!enum_item->name.build(name_token->source_text))
			co_return gen_oom_syntax_error();

		if (Token *token = peek_token(); token->token_id == TokenId::AssignOp) {
			next_token();
			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_expr(this->resource_allocator.get(), 0, enum_item->enum_value));
		}
	}

	if (auto it = enum_out->scope->_member_indices.find(enum_item->name); it != enum_out->scope->_member_indices.end()) {
		peff::String s(resource_allocator.get());

		if (!s.build(enum_item->name)) {
			co_return gen_oom_syntax_error();
		}

		ConflictingDefinitionsErrorExData ex_data(std::move(s));

		co_return SyntaxError(enum_item->token_range, std::move(ex_data));
	} else {
		if (!(enum_out->scope->index_member(idx_member))) {
			co_return gen_oom_syntax_error();
		}
	}

	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_program_stmt(peff::Alloc *allocator) {
	peff::Option<SyntaxError> syntax_error;

	peff::DynArray<AstNodePtr<AttributeNode>> attributes(resource_allocator.get());

	SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_attributes(this->resource_allocator.get(), attributes));

	slake::AccessModifier access = 0;
	Token *current_token;

	for (;;) {
		switch ((current_token = peek_token())->token_id) {
			case TokenId::PublicKeyword:
				access |= slake::make_access_modifier(slake::AccessMode::Public, access);
				next_token();
				break;
			case TokenId::PrivateKeyword:
				access |= slake::make_access_modifier(slake::AccessMode::Private, access);
				next_token();
				break;
			case TokenId::ProtectedKeyword:
				access |= slake::make_access_modifier(slake::AccessMode::Protected, access);
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
		case TokenId::EnumKeyword: {
			next_token();
			switch (peek_token()->token_id) {
				case TokenId::UnionKeyword: {
					next_token();
					AstNodePtr<UnionEnumNode> enum_node;

					if (!(enum_node = make_ast_node<UnionEnumNode>(resource_allocator.get(), resource_allocator.get(), get_document())))
						co_return gen_oom_syntax_error();

					if (!(enum_node->alloc_scope()))
						co_return gen_oom_syntax_error();

					peff::ScopeGuard set_token_range_guard([this, token, enum_node]() noexcept {
						enum_node->token_range = TokenRange{ get_document()->main_module, token->index, parse_context.idx_prev_token };
					});

					Token *name_token;
					SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((name_token = peek_token()), TokenId::Id)));

					next_token();

					size_t idx_member;
					if ((idx_member = p->scope->push_member(enum_node.cast_to<MemberNode>())) == SIZE_MAX) {
						co_return gen_oom_syntax_error();
					}

					if (!enum_node->name.build(name_token->source_text)) {
						co_return gen_oom_syntax_error();
					}

					Token *l_brace_token;
					SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((l_brace_token = peek_token()), TokenId::LBrace)));

					next_token();

					while (true) {
						if (peek_token()->token_id == TokenId::RBrace)
							break;

						if ((syntax_error = (co_await parse_union_enum_item(this->resource_allocator.get(), enum_node.cast_to<ModuleNode>())(this)))) {
							if (syntax_error->error_kind == SyntaxErrorKind::OutOfMemory)
								co_return syntax_error;
							if (!syntax_errors.push_back(syntax_error.move()))
								co_return gen_oom_syntax_error();
						}

						if (peek_token()->token_id != TokenId::Comma)
							break;
						Token *comma_token = next_token();
					}

					Token *r_brace_token;
					SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((r_brace_token = peek_token()), TokenId::RBrace)));

					next_token();

					if (auto it = p->scope->_member_indices.find(enum_node->name); it != p->scope->_member_indices.end()) {
						peff::String s(resource_allocator.get());

						if (!s.build(enum_node->name)) {
							co_return gen_oom_syntax_error();
						}

						ConflictingDefinitionsErrorExData ex_data(std::move(s));

						co_return SyntaxError(enum_node->token_range, std::move(ex_data));
					} else {
						if (!(p->scope->index_member(idx_member))) {
							co_return gen_oom_syntax_error();
						}
					}
					break;
				}
				case TokenId::ConstKeyword: {
					next_token();
					AstNodePtr<ConstEnumNode> enum_node;

					if (!(enum_node = make_ast_node<ConstEnumNode>(resource_allocator.get(), resource_allocator.get(), get_document())))
						co_return gen_oom_syntax_error();

					if (!(enum_node->alloc_scope()))
						co_return gen_oom_syntax_error();

					peff::ScopeGuard set_token_range_guard([this, token, enum_node]() noexcept {
						enum_node->token_range = TokenRange{ get_document()->main_module, token->index, parse_context.idx_prev_token };
					});

					Token *name_token;
					SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((name_token = peek_token()), TokenId::Id)));

					next_token();

					size_t idx_member;
					if ((idx_member = p->scope->push_member(enum_node.cast_to<MemberNode>())) == SIZE_MAX) {
						co_return gen_oom_syntax_error();
					}

					if (!enum_node->name.build(name_token->source_text)) {
						co_return gen_oom_syntax_error();
					}

					if (Token *l_parenthese_token = peek_token(); l_parenthese_token->token_id == TokenId::LParenthese) {
						next_token();

						SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(this->resource_allocator.get(), enum_node->underlying_type));

						Token *r_parenthese_token;
						SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese)));

						next_token();
					}

					Token *l_brace_token;
					SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((l_brace_token = peek_token()), TokenId::LBrace)));

					next_token();

					while (true) {
						if (peek_token()->token_id == TokenId::RBrace)
							break;

						if ((syntax_error = (co_await parse_enum_item(this->resource_allocator.get(), enum_node.cast_to<ModuleNode>())(this)))) {
							if (syntax_error->error_kind == SyntaxErrorKind::OutOfMemory)
								co_return syntax_error;
							if (!syntax_errors.push_back(syntax_error.move()))
								co_return gen_oom_syntax_error();
						}

						if (peek_token()->token_id != TokenId::Comma)
							break;
						Token *comma_token = next_token();
					}

					Token *r_brace_token;
					SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((r_brace_token = peek_token()), TokenId::RBrace)));

					next_token();

					if (auto it = p->scope->_member_indices.find(enum_node->name); it != p->scope->_member_indices.end()) {
						peff::String s(resource_allocator.get());

						if (!s.build(enum_node->name)) {
							co_return gen_oom_syntax_error();
						}

						ConflictingDefinitionsErrorExData ex_data(std::move(s));

						co_return SyntaxError(enum_node->token_range, std::move(ex_data));
					} else {
						if (!(p->scope->index_member(idx_member))) {
							co_return gen_oom_syntax_error();
						}
					}
					break;
				}
				case TokenId::Id: {
					Token *name_token = next_token();
					AstNodePtr<ScopedEnumNode> enum_node;

					if (!(enum_node = make_ast_node<ScopedEnumNode>(resource_allocator.get(), resource_allocator.get(), get_document())))
						co_return gen_oom_syntax_error();

					if (!(enum_node->alloc_scope()))
						co_return gen_oom_syntax_error();

					size_t idx_member;
					{
						peff::ScopeGuard set_token_range_guard([this, token, enum_node]() noexcept {
							enum_node->token_range = TokenRange{ get_document()->main_module, token->index, parse_context.idx_prev_token };
						});
						if ((idx_member = p->scope->push_member(enum_node.cast_to<MemberNode>())) == SIZE_MAX) {
							co_return gen_oom_syntax_error();
						}

						if (!enum_node->name.build(name_token->source_text)) {
							co_return gen_oom_syntax_error();
						}

						if (Token *l_parenthese_token = peek_token(); l_parenthese_token->token_id == TokenId::LParenthese) {
							next_token();

							SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(this->resource_allocator.get(), enum_node->underlying_type));

							Token *r_parenthese_token;
							SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese)));

							next_token();
						}
					}

					Token *l_brace_token;
					SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((l_brace_token = peek_token()), TokenId::LBrace)));

					next_token();

					while (true) {
						if (peek_token()->token_id == TokenId::RBrace)
							break;

						if ((syntax_error = (co_await parse_enum_item(this->resource_allocator.get(), enum_node.cast_to<ModuleNode>())(this)))) {
							if (syntax_error->error_kind == SyntaxErrorKind::OutOfMemory)
								co_return syntax_error;
							if (!syntax_errors.push_back(syntax_error.move()))
								co_return gen_oom_syntax_error();
						}

						if (peek_token()->token_id != TokenId::Comma)
							break;
						Token *comma_token = next_token();
					}

					Token *r_brace_token;
					SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((r_brace_token = peek_token()), TokenId::RBrace)));

					next_token();

					if (auto it = p->scope->_member_indices.find(enum_node->name); it != p->scope->_member_indices.end()) {
						peff::String s(resource_allocator.get());

						if (!s.build(enum_node->name)) {
							co_return gen_oom_syntax_error();
						}

						ConflictingDefinitionsErrorExData ex_data(std::move(s));

						co_return SyntaxError(enum_node->token_range, std::move(ex_data));
					} else {
						if (!(p->scope->index_member(idx_member))) {
							co_return gen_oom_syntax_error();
						}
					}
					break;
				}
				default:
					co_return SyntaxError(TokenRange{ get_document()->main_module, token->index }, SyntaxErrorKind::UnexpectedToken);
			}
			break;
		}
		case TokenId::AttributeKeyword: {
			// Attribute definition.
			next_token();

			AstNodePtr<AttributeDefNode> attribute_node;

			if (!(attribute_node = make_ast_node<AttributeDefNode>(resource_allocator.get(), resource_allocator.get(), get_document()))) {
				co_return gen_oom_syntax_error();
			}

			if (!(attribute_node->alloc_scope()))
				co_return gen_oom_syntax_error();

			attribute_node->access_modifier = access;

			Token *name_token;

			SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((name_token = peek_token()), TokenId::Id)));

			next_token();

			size_t idx_member;
			if ((idx_member = p->scope->push_member(attribute_node.cast_to<MemberNode>())) == SIZE_MAX) {
				co_return gen_oom_syntax_error();
			}

			if (!attribute_node->name.build(name_token->source_text)) {
				co_return gen_oom_syntax_error();
			}

			{
				peff::ScopeGuard set_token_range_guard([this, token, attribute_node]() noexcept {
					attribute_node->token_range = TokenRange{ get_document()->main_module, token->index, parse_context.idx_prev_token };
				});

				AstNodePtr<MemberNode> prev_parent;
				prev_parent = cur_parent;
				peff::ScopeGuard restore_prev_mod_guard([this, prev_parent]() noexcept {
					cur_parent = prev_parent;
				});
				cur_parent = attribute_node.cast_to<MemberNode>();

				Token *l_brace_token;

				SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((l_brace_token = peek_token()), TokenId::LBrace)));

				next_token();

				Token *current_token;
				while (true) {
					SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(current_token = peek_token()));

					if (current_token->token_id == TokenId::RBrace) {
						break;
					}

					if ((syntax_error = (co_await parse_program_stmt(this->resource_allocator.get())(this)))) {
						// Parse the rest to make sure that we have gained all of the information,
						// instead of ignoring them.
						if (!syntax_errors.push_back(std::move(syntax_error.value())))
							co_return gen_oom_syntax_error();
						syntax_error.reset();
					}
				}

				Token *r_brace_token;

				SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((r_brace_token = peek_token()), TokenId::RBrace)));

				next_token();
			}

			if (auto it = p->scope->_member_indices.find(attribute_node->name); it != p->scope->_member_indices.end()) {
				peff::String s(resource_allocator.get());

				if (!s.build(attribute_node->name)) {
					co_return gen_oom_syntax_error();
				}

				ConflictingDefinitionsErrorExData ex_data(std::move(s));

				co_return SyntaxError(attribute_node->token_range, std::move(ex_data));
			} else {
				if (!(p->scope->index_member(idx_member))) {
					co_return gen_oom_syntax_error();
				}
			}

			break;
		}
		case TokenId::FnKeyword:
		case TokenId::AsyncKeyword:
		case TokenId::OperatorKeyword:
		case TokenId::DefKeyword: {
			// Function.
			AstNodePtr<FnOverloadingNode> fn;

			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_fn(this->resource_allocator.get(), fn));

			fn->access_modifier = access;

			if (auto it = p->scope->_member_indices.find(fn->name); it != p->scope->_member_indices.end()) {
				if (p->scope->_members.at(it.value())->get_ast_node_type() != AstNodeType::Fn) {
					peff::String s(resource_allocator.get());

					if (!s.build(fn->name)) {
						co_return gen_oom_syntax_error();
					}

					ConflictingDefinitionsErrorExData ex_data(std::move(s));

					co_return SyntaxError(fn->token_range, std::move(ex_data));
				}
				FnNode *fn_slot = (FnNode *)p->scope->_members.at(it.value()).get();
				fn->set_parent(fn_slot);
				if (!fn_slot->overloadings.push_back(std::move(fn))) {
					co_return gen_oom_syntax_error();
				}
			} else {
				AstNodePtr<FnNode> fn_slot;

				if (!(fn_slot = make_ast_node<FnNode>(resource_allocator.get(), resource_allocator.get(), get_document()))) {
					co_return gen_oom_syntax_error();
				}

				if (!fn_slot->name.build(fn->name)) {
					co_return gen_oom_syntax_error();
				}

				if (!(p->scope->add_member(fn_slot.cast_to<MemberNode>()))) {
					co_return gen_oom_syntax_error();
				}

				fn->set_parent(fn_slot.get());

				if (!fn_slot->overloadings.push_back(std::move(fn))) {
					co_return gen_oom_syntax_error();
				}
			}
			break;
		}
		case TokenId::ClassKeyword: {
			// Class.
			next_token();

			AstNodePtr<ClassNode> class_node;

			if (!(class_node = make_ast_node<ClassNode>(resource_allocator.get(), resource_allocator.get(), get_document()))) {
				co_return gen_oom_syntax_error();
			}

			if (!(class_node->alloc_scope()))
				co_return gen_oom_syntax_error();

			class_node->access_modifier = access;

			Token *name_token;

			SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((name_token = peek_token()), TokenId::Id)));

			next_token();

			if (!class_node->name.build(name_token->source_text)) {
				co_return gen_oom_syntax_error();
			}

			size_t idx_member;
			if ((idx_member = p->scope->push_member(class_node.cast_to<MemberNode>())) == SIZE_MAX) {
				co_return gen_oom_syntax_error();
			}

			{
				peff::ScopeGuard set_token_range_guard([this, token, class_node]() noexcept {
					class_node->token_range = TokenRange{ get_document()->main_module, token->index, parse_context.idx_prev_token };
				});

				AstNodePtr<MemberNode> prev_parent;
				prev_parent = cur_parent;
				peff::ScopeGuard restore_prev_mod_guard([this, prev_parent]() noexcept {
					cur_parent = prev_parent;
				});
				cur_parent = class_node.cast_to<MemberNode>();

				SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_generic_params(this->resource_allocator.get(), class_node->scope->generic_params, class_node->idx_generic_param_comma_tokens, class_node->idx_langle_bracket_token, class_node->idx_rangle_bracket_token));
				for (size_t i = 0; i < class_node->scope->generic_params.size(); ++i) {
					auto gp = class_node->scope->generic_params.at(i);
					if (class_node->scope->generic_param_indices.contains(gp->name)) {
						peff::String s(resource_allocator.get());

						if (!s.build(gp->name)) {
							co_return gen_oom_syntax_error();
						}

						ConflictingDefinitionsErrorExData ex_data(std::move(s));

						co_return SyntaxError(gp->token_range, std::move(ex_data));
					}
					if (!class_node->scope->generic_param_indices.insert(gp->name, +i))
						co_return gen_oom_syntax_error();
				}

				if (Token *l_parenthese_token = peek_token(); l_parenthese_token->token_id == TokenId::LParenthese) {
					next_token();

					SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(this->resource_allocator.get(), class_node->scope->base_type));

					Token *r_parenthese_token;
					SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese)));

					next_token();
				}

				if (Token *colon_token = peek_token(); colon_token->token_id == TokenId::Colon) {
					next_token();

					while (true) {
						AstNodePtr<TypeNameNode> tn;

						SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(this->resource_allocator.get(), tn));

						if (!class_node->scope->impl_types.push_back(std::move(tn))) {
							co_return gen_oom_syntax_error();
						}

						if (peek_token()->token_id != TokenId::AddOp) {
							break;
						}

						Token *or_op_token = next_token();
					}
				}

				Token *l_brace_token;

				SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((l_brace_token = peek_token()), TokenId::LBrace)));

				next_token();

				Token *current_token;
				while (true) {
					SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(current_token = peek_token()));

					if (current_token->token_id == TokenId::RBrace) {
						break;
					}

					if ((syntax_error = (co_await parse_program_stmt(this->resource_allocator.get())(this)))) {
						// Parse the rest to make sure that we have gained all of the information,
						// instead of ignoring them.
						if (!syntax_errors.push_back(std::move(syntax_error.value())))
							co_return gen_oom_syntax_error();
						syntax_error.reset();
					}
				}

				Token *r_brace_token;

				SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((r_brace_token = peek_token()), TokenId::RBrace)));

				next_token();
			}

			if (auto it = p->scope->_member_indices.find(class_node->name); it != p->scope->_member_indices.end()) {
				peff::String s(resource_allocator.get());

				if (!s.build(class_node->name)) {
					co_return gen_oom_syntax_error();
				}

				ConflictingDefinitionsErrorExData ex_data(std::move(s));

				co_return SyntaxError(class_node->token_range, std::move(ex_data));
			} else {
				if (!(p->scope->index_member(idx_member))) {
					co_return gen_oom_syntax_error();
				}
			}

			break;
		}
		case TokenId::StructKeyword: {
			// Struct.
			next_token();

			AstNodePtr<StructNode> struct_node;

			if (!(struct_node = make_ast_node<StructNode>(resource_allocator.get(), resource_allocator.get(), get_document()))) {
				co_return gen_oom_syntax_error();
			}

			if (!(struct_node->alloc_scope()))
				co_return gen_oom_syntax_error();

			struct_node->access_modifier = access;

			Token *name_token;

			SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((name_token = peek_token()), TokenId::Id)));

			next_token();

			if (!struct_node->name.build(name_token->source_text)) {
				co_return gen_oom_syntax_error();
			}

			size_t idx_member;
			if ((idx_member = p->scope->push_member(struct_node.cast_to<MemberNode>())) == SIZE_MAX) {
				co_return gen_oom_syntax_error();
			}

			{
				peff::ScopeGuard set_token_range_guard([this, token, struct_node]() noexcept {
					struct_node->token_range = TokenRange{ get_document()->main_module, token->index, parse_context.idx_prev_token };
				});

				AstNodePtr<MemberNode> prev_parent;
				prev_parent = cur_parent;
				peff::ScopeGuard restore_prev_mod_guard([this, prev_parent]() noexcept {
					cur_parent = prev_parent;
				});
				cur_parent = struct_node.cast_to<MemberNode>();

				SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_generic_params(this->resource_allocator.get(), struct_node->scope->generic_params, struct_node->idx_generic_param_comma_tokens, struct_node->idx_langle_bracket_token, struct_node->idx_rangle_bracket_token));
				for (size_t i = 0; i < struct_node->scope->generic_params.size(); ++i) {
					auto gp = struct_node->scope->generic_params.at(i);
					if (struct_node->scope->generic_param_indices.contains(gp->name)) {
						peff::String s(resource_allocator.get());

						if (!s.build(gp->name)) {
							co_return gen_oom_syntax_error();
						}

						ConflictingDefinitionsErrorExData ex_data(std::move(s));

						co_return SyntaxError(gp->token_range, std::move(ex_data));
					}
					if (!struct_node->scope->generic_param_indices.insert(gp->name, +i))
						co_return gen_oom_syntax_error();
				}

				if (Token *colon_token = peek_token(); colon_token->token_id == TokenId::Colon) {
					next_token();

					while (true) {
						AstNodePtr<TypeNameNode> tn;

						SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(this->resource_allocator.get(), tn));

						if (!struct_node->scope->impl_types.push_back(std::move(tn))) {
							co_return gen_oom_syntax_error();
						}

						if (peek_token()->token_id != TokenId::AddOp) {
							break;
						}

						Token *or_op_token = next_token();
					}
				}

				Token *l_brace_token;

				SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((l_brace_token = peek_token()), TokenId::LBrace)));

				next_token();

				Token *current_token;
				while (true) {
					SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(current_token = peek_token()));

					if (current_token->token_id == TokenId::RBrace) {
						break;
					}

					if ((syntax_error = (co_await parse_program_stmt(this->resource_allocator.get())(this)))) {
						// Parse the rest to make sure that we have gained all of the information,
						// instead of ignoring them.
						if (!syntax_errors.push_back(std::move(syntax_error.value())))
							co_return gen_oom_syntax_error();
						syntax_error.reset();
					}
				}

				Token *r_brace_token;

				SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((r_brace_token = peek_token()), TokenId::RBrace)));

				next_token();
			}

			if (auto it = p->scope->_member_indices.find(struct_node->name); it != p->scope->_member_indices.end()) {
				peff::String s(resource_allocator.get());

				if (!s.build(struct_node->name)) {
					co_return gen_oom_syntax_error();
				}

				ConflictingDefinitionsErrorExData ex_data(std::move(s));

				co_return SyntaxError(struct_node->token_range, std::move(ex_data));
			} else {
				if (!(p->scope->index_member(idx_member))) {
					co_return gen_oom_syntax_error();
				}
			}

			break;
		}
		case TokenId::InterfaceKeyword: {
			// Interface.
			next_token();

			AstNodePtr<InterfaceNode> interface_node;

			if (!(interface_node = make_ast_node<InterfaceNode>(resource_allocator.get(), resource_allocator.get(), get_document()))) {
				co_return gen_oom_syntax_error();
			}

			if (!(interface_node->alloc_scope()))
				co_return gen_oom_syntax_error();

			interface_node->access_modifier = access;

			Token *name_token;

			SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((name_token = peek_token()), TokenId::Id)));

			next_token();

			if (!interface_node->name.build(name_token->source_text)) {
				co_return gen_oom_syntax_error();
			}

			size_t idx_member;
			if ((idx_member = p->scope->push_member(interface_node.cast_to<MemberNode>())) == SIZE_MAX) {
				co_return gen_oom_syntax_error();
			}

			Token *t;

			{
				peff::ScopeGuard set_token_range_guard([this, token, interface_node]() noexcept {
					interface_node->token_range = TokenRange{ get_document()->main_module, token->index, parse_context.idx_prev_token };
				});

				AstNodePtr<MemberNode> prev_member;
				prev_member = cur_parent;
				peff::ScopeGuard restore_prev_mod_guard([this, prev_member]() noexcept {
					cur_parent = prev_member;
				});
				cur_parent = interface_node.cast_to<MemberNode>();

				SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_generic_params(this->resource_allocator.get(), interface_node->scope->generic_params, interface_node->idx_generic_param_comma_tokens, interface_node->idx_langle_bracket_token, interface_node->idx_rangle_bracket_token));
				for (size_t i = 0; i < interface_node->scope->generic_params.size(); ++i) {
					auto gp = interface_node->scope->generic_params.at(i);
					if (interface_node->scope->generic_param_indices.contains(gp->name)) {
						peff::String s(resource_allocator.get());

						if (!s.build(gp->name)) {
							co_return gen_oom_syntax_error();
						}

						ConflictingDefinitionsErrorExData ex_data(std::move(s));

						co_return SyntaxError(gp->token_range, std::move(ex_data));
					}
					if (!interface_node->scope->generic_param_indices.insert(gp->name, +i))
						co_return gen_oom_syntax_error();
				}

				if (Token *colon_token = peek_token(); colon_token->token_id == TokenId::Colon) {
					next_token();

					while (true) {
						AstNodePtr<TypeNameNode> tn;

						SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(this->resource_allocator.get(), tn));

						if (!interface_node->scope->impl_types.push_back(std::move(tn))) {
							co_return gen_oom_syntax_error();
						}

						if (peek_token()->token_id != TokenId::AddOp) {
							break;
						}

						Token *or_op_token = next_token();
					}
				}

				Token *l_brace_token;

				SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((l_brace_token = peek_token()), TokenId::LBrace)));

				next_token();

				Token *current_token;
				while (true) {
					SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(current_token = peek_token()));

					if (current_token->token_id == TokenId::RBrace) {
						break;
					}

					if ((syntax_error = (co_await parse_program_stmt(this->resource_allocator.get())(this)))) {
						// Parse the rest to make sure that we have gained all of the information,
						// instead of ignoring them.
						if (!syntax_errors.push_back(std::move(syntax_error.value())))
							co_return gen_oom_syntax_error();
						syntax_error.reset();
					}
				}

				Token *r_brace_token;

				SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((r_brace_token = peek_token()), TokenId::RBrace)));

				next_token();
			}

			if (auto it = p->scope->_member_indices.find(interface_node->name); it != p->scope->_member_indices.end()) {
				peff::String s(resource_allocator.get());

				if (!s.build(interface_node->name)) {
					co_return gen_oom_syntax_error();
				}

				ConflictingDefinitionsErrorExData ex_data(std::move(s));

				co_return SyntaxError(interface_node->token_range, std::move(ex_data));
			} else {
				if (!(p->scope->index_member(idx_member))) {
					co_return gen_oom_syntax_error();
				}
			}

			break;
		}
		case TokenId::ImportKeyword: {
			// Import item.
			next_token();

			AstNodePtr<ImportNode> import_node;

			if (!(import_node = make_ast_node<ImportNode>(resource_allocator.get(), resource_allocator.get(), get_document()))) {
				co_return gen_oom_syntax_error();
			}

			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_id_ref(this->resource_allocator.get(), import_node->id_ref));
			size_t idx_member;
			if ((idx_member = p->scope->push_member(import_node.cast_to<MemberNode>())) == SIZE_MAX) {
				co_return gen_oom_syntax_error();
			}

			if (Token *as_token = peek_token(); as_token->token_id == TokenId::AsKeyword) {
				next_token();

				Token *name_token;

				SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((name_token = peek_token()), TokenId::Id)));

				if (!import_node->name.build(name_token->source_text)) {
					co_return gen_oom_syntax_error();
				}

				if (!p->scope->index_member(idx_member)) {
					co_return gen_oom_syntax_error();
				}
			} else {
				if (!p->scope->anonymous_imports.push_back(AstNodePtr<ImportNode>(import_node))) {
					co_return gen_oom_syntax_error();
				}
			}

			Token *semicolon_token;

			SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((semicolon_token = peek_token()), TokenId::Semicolon)));

			next_token();

			break;
		}
		case TokenId::LetKeyword: {
			// Global variable.
			next_token();

			AstNodePtr<VarDefStmtNode> stmt;

			if (!(stmt = make_ast_node<VarDefStmtNode>(
					  resource_allocator.get(),
					  resource_allocator.get(),
					  get_document(),
					  peff::DynArray<VarDefEntryPtr>(resource_allocator.get())))) {
				co_return gen_oom_syntax_error();
			}

			stmt->access_modifier = access;

			if (!p->var_def_stmts.push_back(AstNodePtr<VarDefStmtNode>(stmt))) {
				co_return gen_oom_syntax_error();
			}

			peff::ScopeGuard set_token_range_guard([this, token, stmt]() noexcept {
				stmt->token_range = TokenRange{ get_document()->main_module, token->index, parse_context.idx_prev_token };
			});

			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_var_defs(this->resource_allocator.get(), stmt->var_def_entries));

			Token *semicolon_token;

			SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((semicolon_token = peek_token()), TokenId::Semicolon)));

			next_token();

			for (auto &i : stmt->var_def_entries) {
				if (p->scope->_member_indices.contains(i->name)) {
					peff::String s(resource_allocator.get());

					if (!s.build(i->name))
						co_return gen_oom_syntax_error();

					ConflictingDefinitionsErrorExData ex_data(std::move(s));

					if (syntax_errors.push_back(SyntaxError(TokenRange(p.get(), i->idx_name_token), std::move(ex_data))))
						co_return gen_oom_syntax_error();
				}
				AstNodePtr<VarNode> var_node;

				if (!(var_node = make_ast_node<VarNode>(resource_allocator.get(), resource_allocator.get(), get_document()))) {
					co_return gen_oom_syntax_error();
				}

				if (!var_node->name.build(i->name))
					co_return gen_oom_syntax_error();
				var_node->initial_value = i->initial_value;
				var_node->type = i->type;
				var_node->access_modifier = stmt->access_modifier;

				if (!p->scope->add_member(var_node.cast_to<MemberNode>()))
					co_return gen_oom_syntax_error();
			}

			break;
		}
		default:
			next_token();
			co_return SyntaxError(
				TokenRange{ get_document()->main_module, token->index },
				SyntaxErrorKind::ExpectingDecl);
	}

	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_program(peff::Alloc *allocator, const AstNodePtr<ModuleNode> &initial_mod, IdRefPtr &module_name_out) {
	peff::Option<SyntaxError> syntax_error;

	Token *t;

	parse_context.mod = initial_mod.get();
	cur_parent = initial_mod.cast_to<MemberNode>();

	module_name_out = {};
	if ((t = peek_token())->token_id == TokenId::ModuleKeyword) {
		next_token();

		IdRefPtr module_name;

		if ((syntax_error = (co_await parse_id_ref(this->resource_allocator.get(), module_name)(this)))) {
			if (!syntax_errors.push_back(std::move(syntax_error.value())))
				co_return gen_oom_syntax_error();
			syntax_error.reset();
		}

		Token *semicolon_token;
		SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((semicolon_token = peek_token()), TokenId::Semicolon)));

		next_token();

		module_name_out = std::move(module_name);
	}

	while ((t = peek_token())->token_id != TokenId::End) {
		if ((syntax_error = (co_await parse_program_stmt(this->resource_allocator.get())(this)))) {
			// Parse the rest to make sure that we have gained all of the information,
			// instead of ignoring them.
			if (!syntax_errors.push_back(std::move(syntax_error.value())))
				co_return gen_oom_syntax_error();
			syntax_error.reset();
		}
	}

	initial_mod->set_parser(shared_from_this());

	co_return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parse(const AstNodePtr<ModuleNode> &initial_mod, IdRefPtr &module_name_out) {
	return parse_program(this->resource_allocator.get(), initial_mod, module_name_out).resume(this);
}
