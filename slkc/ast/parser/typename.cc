#include "../parser.h"
#include <climits>

using namespace slkc;

SLKC_API ParseCoroutine Parser::parse_generic_arg(peff::Alloc *allocator, AstNodePtr<AstNode>& arg_out) {
	peff::Option<SyntaxError> syntax_error;
	Token *t = peek_token();

	switch (t->token_id) {
		case TokenId::I8Literal:
		case TokenId::I16Literal:
		case TokenId::I32Literal:
		case TokenId::I64Literal:
		case TokenId::U8Literal:
		case TokenId::U16Literal:
		case TokenId::U32Literal:
		case TokenId::U64Literal:
		case TokenId::F32Literal:
		case TokenId::F64Literal:
		case TokenId::StringLiteral: {
			AstNodePtr<ExprNode> e;
			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_expr(this->resource_allocator.get(), INT_MAX, e));
			arg_out = e.cast_to<AstNode>();
			break;
		}
		case TokenId::LParenthese: {
			AstNodePtr<ExprNode> e;
			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_expr(this->resource_allocator.get(), INT_MAX, e));
			arg_out = e.cast_to<AstNode>();
			break;
		}
		case TokenId::TypenameKeyword:
			next_token();
			[[fallthrough]];
		default: {
			AstNodePtr<TypeNameNode> t;
			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(this->resource_allocator.get(), t));
			arg_out = t.cast_to<AstNode>();
			break;
		}
	}

	co_return {};
}

SLKC_API ParseCoroutine Parser::parse_type_name(peff::Alloc *allocator, AstNodePtr<TypeNameNode> &type_name_out, bool with_circumfixes) {
	peff::Option<SyntaxError> syntax_error;
	Token *t = peek_token();

	switch (t->token_id) {
		case TokenId::VarArg:
			if (!(type_name_out = make_ast_node<UnpackingTypeNameNode>(
					  resource_allocator.get(),
					  resource_allocator.get(), document)
						.cast_to<TypeNameNode>()))
				co_return gen_out_of_memory_syntax_error();
			type_name_out->token_range = TokenRange{ document->main_module, t->index };
			next_token();

			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(this->resource_allocator.get(), type_name_out.cast_to<UnpackingTypeNameNode>()->inner_type_name, true));
			break;
		case TokenId::VoidTypeName:
			if (!(type_name_out = make_ast_node<VoidTypeNameNode>(
					  resource_allocator.get(),
					  resource_allocator.get(), document)
						.cast_to<TypeNameNode>()))
				co_return gen_out_of_memory_syntax_error();
			type_name_out->token_range = TokenRange{ document->main_module, t->index };
			next_token();
			break;
		case TokenId::I8TypeName:
			if (!(type_name_out = peff::make_shared_with_control_block<I8TypeNameNode, AstNodeControlBlock<I8TypeNameNode>>(
					  resource_allocator.get(),
					  resource_allocator.get(), document)
						.cast_to<TypeNameNode>()))
				co_return gen_out_of_memory_syntax_error();
			type_name_out->token_range = TokenRange{ document->main_module, t->index };
			next_token();
			break;
		case TokenId::I16TypeName:
			if (!(type_name_out = peff::make_shared_with_control_block<I16TypeNameNode, AstNodeControlBlock<I16TypeNameNode>>(
					  resource_allocator.get(),
					  resource_allocator.get(), document)
						.cast_to<TypeNameNode>()))
				co_return gen_out_of_memory_syntax_error();
			type_name_out->token_range = TokenRange{ document->main_module, t->index };
			next_token();
			break;
		case TokenId::I32TypeName:
			if (!(type_name_out = peff::make_shared_with_control_block<I32TypeNameNode, AstNodeControlBlock<I32TypeNameNode>>(
					  resource_allocator.get(),
					  resource_allocator.get(), document)
						.cast_to<TypeNameNode>()))
				co_return gen_out_of_memory_syntax_error();
			type_name_out->token_range = TokenRange{ document->main_module, t->index };
			next_token();
			break;
		case TokenId::I64TypeName:
			if (!(type_name_out = peff::make_shared_with_control_block<I64TypeNameNode, AstNodeControlBlock<I64TypeNameNode>>(
					  resource_allocator.get(),
					  resource_allocator.get(), document)
						.cast_to<TypeNameNode>()))
				co_return gen_out_of_memory_syntax_error();
			type_name_out->token_range = TokenRange{ document->main_module, t->index };
			next_token();
			break;
		case TokenId::U8TypeName:
			if (!(type_name_out = peff::make_shared_with_control_block<U8TypeNameNode, AstNodeControlBlock<U8TypeNameNode>>(
					  resource_allocator.get(),
					  resource_allocator.get(), document)
						.cast_to<TypeNameNode>()))
				co_return gen_out_of_memory_syntax_error();
			type_name_out->token_range = TokenRange{ document->main_module, t->index };
			next_token();
			break;
		case TokenId::U16TypeName:
			if (!(type_name_out = peff::make_shared_with_control_block<U16TypeNameNode, AstNodeControlBlock<U16TypeNameNode>>(
					  resource_allocator.get(),
					  resource_allocator.get(), document)
						.cast_to<TypeNameNode>()))
				co_return gen_out_of_memory_syntax_error();
			type_name_out->token_range = TokenRange{ document->main_module, t->index };
			next_token();
			break;
		case TokenId::U32TypeName:
			if (!(type_name_out = peff::make_shared_with_control_block<U32TypeNameNode, AstNodeControlBlock<U32TypeNameNode>>(
					  resource_allocator.get(),
					  resource_allocator.get(), document)
						.cast_to<TypeNameNode>()))
				co_return gen_out_of_memory_syntax_error();
			type_name_out->token_range = TokenRange{ document->main_module, t->index };
			next_token();
			break;
		case TokenId::U64TypeName:
			if (!(type_name_out = peff::make_shared_with_control_block<U64TypeNameNode, AstNodeControlBlock<U64TypeNameNode>>(
					  resource_allocator.get(),
					  resource_allocator.get(), document)
						.cast_to<TypeNameNode>()))
				co_return gen_out_of_memory_syntax_error();
			type_name_out->token_range = TokenRange{ document->main_module, t->index };
			next_token();
			break;
		case TokenId::ISizeTypeName:
			if (!(type_name_out = make_ast_node<ISizeTypeNameNode>(
					  resource_allocator.get(),
					  resource_allocator.get(), document)
						.cast_to<TypeNameNode>()))
				co_return gen_out_of_memory_syntax_error();
			type_name_out->token_range = TokenRange{ document->main_module, t->index };
			break;
		case TokenId::USizeTypeName:
			if (!(type_name_out = make_ast_node<USizeTypeNameNode>(
					  resource_allocator.get(),
					  resource_allocator.get(), document)
						.cast_to<TypeNameNode>()))
				co_return gen_out_of_memory_syntax_error();
			type_name_out->token_range = TokenRange{ document->main_module, t->index };
			next_token();
			break;
		case TokenId::F32TypeName:
			if (!(type_name_out = peff::make_shared_with_control_block<F32TypeNameNode, AstNodeControlBlock<F32TypeNameNode>>(
					  resource_allocator.get(),
					  resource_allocator.get(), document)
						.cast_to<TypeNameNode>()))
				co_return gen_out_of_memory_syntax_error();
			type_name_out->token_range = TokenRange{ document->main_module, t->index };
			next_token();
			break;
		case TokenId::F64TypeName:
			if (!(type_name_out = peff::make_shared_with_control_block<F64TypeNameNode, AstNodeControlBlock<F64TypeNameNode>>(
					  resource_allocator.get(),
					  resource_allocator.get(), document)
						.cast_to<TypeNameNode>()))
				co_return gen_out_of_memory_syntax_error();
			type_name_out->token_range = TokenRange{ document->main_module, t->index };
			next_token();
			break;
		case TokenId::BoolTypeName:
			if (!(type_name_out = make_ast_node<BoolTypeNameNode>(
					  resource_allocator.get(),
					  resource_allocator.get(), document)
						.cast_to<TypeNameNode>()))
				co_return gen_out_of_memory_syntax_error();
			type_name_out->token_range = TokenRange{ document->main_module, t->index };
			next_token();
			break;
		case TokenId::StringTypeName:
			if (!(type_name_out = make_ast_node<StringTypeNameNode>(
					  resource_allocator.get(),
					  resource_allocator.get(), document)
						.cast_to<TypeNameNode>()))
				co_return gen_out_of_memory_syntax_error();
			type_name_out->token_range = TokenRange{ document->main_module, t->index };
			next_token();
			break;
		case TokenId::LParenthese: {
			AstNodePtr<ParamTypeListTypeNameNode> tn;

			if (!(tn = make_ast_node<ParamTypeListTypeNameNode>(
					  resource_allocator.get(),
					  resource_allocator.get(), document)))
				co_return gen_out_of_memory_syntax_error();

			type_name_out = tn.cast_to<TypeNameNode>();

			type_name_out->token_range = TokenRange{ document->main_module, t->index };

			Token *l_parenthese_token = next_token();

			for (;;) {
				if (peek_token()->token_id == TokenId::RParenthese) {
					break;
				}

				if (peek_token()->token_id == TokenId::VarArg) {
					tn->has_var_args = true;
					break;
				}

				AstNodePtr<TypeNameNode> param_type;

				SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(this->resource_allocator.get(), param_type));

				if (!tn->param_types.push_back(std::move(param_type)))
					co_return gen_out_of_memory_syntax_error();

				if (peek_token()->token_id != TokenId::Comma) {
					break;
				}

				Token *comma_token = next_token();
				/*
				if (!idx_comma_tokens_out.push_back(+comma_token->index))
					co_return gen_out_of_memory_syntax_error();*/
			}

			Token *r_parenthese_token;
			if ((syntax_error = expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese)))
				co_return SyntaxError(TokenRange{ document->main_module, r_parenthese_token->index }, ExpectingSingleTokenErrorExData{ TokenId::RParenthese });

			next_token();
			break;
		}
		case TokenId::FnKeyword: {
			AstNodePtr<FnTypeNameNode> tn;
			if (!(tn = make_ast_node<FnTypeNameNode>(
					  resource_allocator.get(),
					  resource_allocator.get(), document)))
				co_return gen_out_of_memory_syntax_error();
			type_name_out = tn.cast_to<TypeNameNode>();
			tn->token_range = TokenRange{ document->main_module, t->index };
			next_token();

			Token *l_parenthese_token;
			if ((syntax_error = expect_token((l_parenthese_token = peek_token()), TokenId::LParenthese)))
				co_return SyntaxError(TokenRange{ document->main_module, l_parenthese_token->index }, ExpectingSingleTokenErrorExData{ TokenId::LParenthese });

			next_token();

			for (;;) {
				if (peek_token()->token_id == TokenId::RParenthese) {
					break;
				}

				AstNodePtr<TypeNameNode> param_type;

				SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(this->resource_allocator.get(), param_type));

				if (!tn->param_types.push_back(std::move(param_type)))
					co_return gen_out_of_memory_syntax_error();

				if (peek_token()->token_id != TokenId::Comma) {
					break;
				}

				Token *comma_token = next_token();
				/*
				if (!idx_comma_tokens_out.push_back(+comma_token->index))
					co_return gen_out_of_memory_syntax_error();*/
			}

			Token *r_parenthese_token;
			if ((syntax_error = expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese)))
				co_return SyntaxError(TokenRange{ document->main_module, r_parenthese_token->index }, ExpectingSingleTokenErrorExData{ TokenId::RParenthese });

			next_token();

			if (peek_token()->token_id == TokenId::WithKeyword) {
				next_token();

				SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(this->resource_allocator.get(), tn->this_type));
			}

			if (peek_token()->token_id == TokenId::ReturnTypeOp) {
				next_token();

				SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(this->resource_allocator.get(), tn->return_type));
			}

			break;
		}
		case TokenId::LBracket: {
			AstNodePtr<TupleTypeNameNode> tn;

			if (!(tn = make_ast_node<TupleTypeNameNode>(
					  resource_allocator.get(),
					  resource_allocator.get(),
					  document)))
				co_return gen_out_of_memory_syntax_error();

			type_name_out = tn.cast_to<TypeNameNode>();

			Token *l_bracket_token;

			if (auto e = expect_token(l_bracket_token = peek_token(), TokenId::LBracket))
				co_return e;

			tn->idx_lbracket_token = l_bracket_token->index;

			next_token();

			for (;;) {
				if (peek_token()->token_id == TokenId::RParenthese) {
					break;
				}

				AstNodePtr<TypeNameNode> t;

				SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(this->resource_allocator.get(), t));

				if (!tn->element_types.push_back(std::move(t)))
					co_return gen_out_of_memory_syntax_error();

				if (peek_token()->token_id != TokenId::Comma) {
					break;
				}

				Token *comma_token = next_token();

				if (!tn->idx_comma_tokens.push_back(+comma_token->index))
					co_return gen_out_of_memory_syntax_error();
			}

			Token *r_bracket_token;

			if (auto e = expect_token(r_bracket_token = peek_token(), TokenId::RBracket))
				co_return e;

			tn->idx_rbracket_token = r_bracket_token->index;

			next_token();

			break;
		}
		case TokenId::SIMDTypeName: {
			AstNodePtr<SIMDTypeNameNode> tn;

			next_token();

			if (!(tn = make_ast_node<SIMDTypeNameNode>(
					  resource_allocator.get(),
					  resource_allocator.get(),
					  document)))
				co_return gen_out_of_memory_syntax_error();

			type_name_out = tn.cast_to<TypeNameNode>();

			Token *l_angle_bracket_token;

			if (auto e = expect_token(l_angle_bracket_token = peek_token(), TokenId::LtOp); e)
				co_return e;

			tn->idx_langle_bracket_token = l_angle_bracket_token->index;

			next_token();

			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(this->resource_allocator.get(), tn->element_type));

			Token *comma_token;

			if (auto e = expect_token(comma_token = peek_token(), TokenId::Comma))
				co_return e;

			tn->idx_comma_token = comma_token->index;

			next_token();

			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_expr(this->resource_allocator.get(), 140, tn->width));

			Token *r_angle_bracket_token;

			if (auto e = expect_token(r_angle_bracket_token = peek_token(), TokenId::GtOp))
				co_return e;

			tn->idx_rangle_bracket_token = r_angle_bracket_token->index;

			next_token();

			break;
		}
		case TokenId::Id: {
			IdRefPtr id;
			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_id_ref(this->resource_allocator.get(), id));

			AstNodePtr<CustomTypeNameNode> tn;

			if (!(tn = make_ast_node<CustomTypeNameNode>(
					  resource_allocator.get(),
					  resource_allocator.get(),
					  document)))
				co_return gen_out_of_memory_syntax_error();

			tn->context_node = to_weak_ptr(cur_parent);

			tn->token_range = id->token_range;
			tn->id_ref_ptr = std::move(id);

			type_name_out = tn.cast_to<TypeNameNode>();

			break;
		}
		default:
			co_return SyntaxError(TokenRange{ document->main_module, t->index }, SyntaxErrorKind::UnexpectedToken);
	}

	if (with_circumfixes) {
		while (true) {
			switch ((t = peek_token())->token_id) {
				case TokenId::FinalKeyword: {
					next_token();

					type_name_out->is_final = true;
					type_name_out->idx_final_token = t->index;
					break;
				}
				case TokenId::LocalKeyword: {
					next_token();

					type_name_out->is_local = true;
					type_name_out->idx_local_token = t->index;
					break;
				}
				case TokenId::Question: {
					next_token();

					type_name_out->is_nullable = true;
					type_name_out->idx_nullable_token = t->index;
					break;
				}
				case TokenId::LBracket: {
					next_token();

					Token *r_bracket_token;
					if ((syntax_error = expect_token((r_bracket_token = peek_token()), TokenId::RBracket)))
						co_return SyntaxError(TokenRange{ document->main_module, r_bracket_token->index }, ExpectingSingleTokenErrorExData{ TokenId::RBracket });

					next_token();

					if (!(type_name_out = make_ast_node<ArrayTypeNameNode>(
							  resource_allocator.get(),
							  resource_allocator.get(),
							  document,
							  type_name_out)
								.cast_to<TypeNameNode>()))
						co_return gen_out_of_memory_syntax_error();
					break;
				}
				default:
					goto end;
			}
		}
	}

end:
	if (with_circumfixes) {
		if ((t = peek_token())->token_id == TokenId::AndOp) {
			next_token();
			if (!(type_name_out = make_ast_node<RefTypeNameNode>(
					  resource_allocator.get(),
					  resource_allocator.get(),
					  document,
					  type_name_out)
						.cast_to<TypeNameNode>()))
				co_return gen_out_of_memory_syntax_error();
		} else if ((t = peek_token())->token_id == TokenId::LAndOp) {
			next_token();
			if (!(type_name_out = make_ast_node<TempRefTypeNameNode>(
					  resource_allocator.get(),
					  resource_allocator.get(),
					  document,
					  type_name_out)
						.cast_to<TypeNameNode>()))
				co_return gen_out_of_memory_syntax_error();
		}
	}

	co_return {};
}
