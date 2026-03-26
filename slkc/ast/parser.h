#ifndef _SLKC_AST_PARSER_H_
#define _SLKC_AST_PARSER_H_

#include "lexer.h"
#include "expr.h"
#include "stmt.h"
#include "typename.h"
#include "idref.h"
#include "attribute.h"
#include "fn.h"
#include "generic.h"
#include "import.h"
#include "class.h"
#include "document.h"

namespace slkc {
	enum class SyntaxErrorKind : int {
		OutOfMemory = 0,
		UnexpectedToken,
		ExpectingSingleToken,
		ExpectingTokens,
		ExpectingId,
		ExpectingOperatorName,
		ExpectingExpr,
		ExpectingStmt,
		ExpectingDecl,
		InvalidMetaTypeName,
		NoMatchingTokensFound,
		ConflictingDefinitions,
		LiteralOverflowed
	};

	struct ExpectingSingleTokenErrorExData {
		TokenId expecting_token_id;
	};

	struct ExpectingTokensErrorExData {
		peff::Set<TokenId> expecting_token_ids;

		SLAKE_FORCEINLINE ExpectingTokensErrorExData(peff::Alloc *allocator) : expecting_token_ids(allocator) {
		}
	};

	struct NoMatchingTokensFoundErrorExData {
		peff::Set<TokenId> expecting_token_ids;

		SLAKE_FORCEINLINE NoMatchingTokensFoundErrorExData(peff::Alloc *allocator) : expecting_token_ids(allocator) {
		}
	};

	struct ConflictingDefinitionsErrorExData {
		peff::String member_name;

		SLAKE_FORCEINLINE ConflictingDefinitionsErrorExData(peff::String &&name) : member_name(std::move(name)) {
		}
	};

	struct SyntaxError {
		TokenRange token_range;
		SyntaxErrorKind error_kind;
		std::variant<std::monostate, ExpectingTokensErrorExData, NoMatchingTokensFoundErrorExData, ExpectingSingleTokenErrorExData, ConflictingDefinitionsErrorExData> ex_data;

		SLAKE_FORCEINLINE SyntaxError(
			const TokenRange &token_range,
			SyntaxErrorKind error_kind)
			: token_range(token_range),
			  error_kind(error_kind) {
		}

		SLAKE_FORCEINLINE SyntaxError(
			const TokenRange &token_range,
			ExpectingTokensErrorExData &&ex_data)
			: token_range(token_range),
			  error_kind(SyntaxErrorKind::ExpectingTokens),
			  ex_data(std::move(ex_data)) {
		}

		SLAKE_FORCEINLINE SyntaxError(
			const TokenRange &token_range,
			ExpectingSingleTokenErrorExData &&ex_data)
			: token_range(token_range),
			  error_kind(SyntaxErrorKind::ExpectingSingleToken),
			  ex_data(std::move(ex_data)) {
		}

		SLAKE_FORCEINLINE SyntaxError(
			const TokenRange &token_range,
			NoMatchingTokensFoundErrorExData &&ex_data)
			: token_range(token_range),
			  error_kind(SyntaxErrorKind::NoMatchingTokensFound),
			  ex_data(std::move(ex_data)) {
		}

		SLAKE_FORCEINLINE SyntaxError(
			const TokenRange &token_range,
			ConflictingDefinitionsErrorExData &&ex_data)
			: token_range(token_range),
			  error_kind(SyntaxErrorKind::ConflictingDefinitions),
			  ex_data(std::move(ex_data)) {
		}

		SLAKE_FORCEINLINE ExpectingTokensErrorExData &get_expecting_tokens_error_ex_data() {
			return std::get<ExpectingTokensErrorExData>(ex_data);
		}

		SLAKE_FORCEINLINE const ExpectingTokensErrorExData &get_expecting_tokens_error_ex_data() const {
			return std::get<ExpectingTokensErrorExData>(ex_data);
		}

		SLAKE_FORCEINLINE const NoMatchingTokensFoundErrorExData &get_no_matching_tokens_found_error_ex_data() const {
			return std::get<NoMatchingTokensFoundErrorExData>(ex_data);
		}
	};

	class Parser;

	class Parser : public peff::SharedFromThis<Parser> {
	public:
		peff::SharedPtr<Document> document;
		AstNodePtr<MemberNode> cur_parent;
		peff::RcObjectPtr<peff::Alloc> resource_allocator;
		TokenList token_list;
		struct ParseContext {
			size_t idx_prev_token = 0, idx_current_token = 0;
		};
		ParseContext parse_context;
		peff::DynArray<SyntaxError> syntax_errors;

		SLKC_API Parser(peff::SharedPtr<Document> document, TokenList &&token_list, peff::Alloc *resource_allocator);
		SLKC_API virtual ~Parser();

		SLKC_API SyntaxError gen_out_of_memory_syntax_error() {
			return SyntaxError(TokenRange{ document->main_module, 0 }, SyntaxErrorKind::OutOfMemory);
		}

		SLKC_API peff::Option<SyntaxError> lookahead_until(size_t num_token_ids, const TokenId token_ids[]);
		SLKC_API Token *next_token(bool keep_new_line = false, bool keep_whitespace = false, bool keep_comment = false);
		SLKC_API Token *peek_token(bool keep_new_line = false, bool keep_whitespace = false, bool keep_comment = false);

		[[nodiscard]] SLAKE_FORCEINLINE peff::Option<SyntaxError> expect_token(Token *token, TokenId token_id) {
			if (token->token_id != token_id) {
				ExpectingSingleTokenErrorExData ex_data = { token_id };

				return SyntaxError(TokenRange{ document->main_module, token->index }, std::move(ex_data));
			}

			return {};
		}

		[[nodiscard]] SLAKE_FORCEINLINE peff::Option<SyntaxError> expect_token(Token *token) {
			if (token->token_id == TokenId::End) {
				ExpectingTokensErrorExData ex_data(resource_allocator.get());

				return SyntaxError(TokenRange{ document->main_module, token->index }, std::move(ex_data));
			}

			return {};
		}

		PEFF_FORCEINLINE peff::Option<SyntaxError> push_literal_overflowed_error(Token *token) noexcept {
			if (!syntax_errors.push_back(SyntaxError(TokenRange{ document->main_module, token->index }, SyntaxErrorKind::LiteralOverflowed)))
				return gen_out_of_memory_syntax_error();
			return {};
		}

		[[nodiscard]] SLKC_API peff::Option<SyntaxError> split_shr_op_token();
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> split_rdbrackets_token();

	private:
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_var_defs(peff::DynArray<VarDefEntryPtr> &var_def_entries);

		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_id_ref(IdRefPtr &id_ref_out);

		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_expr(int precedence, AstNodePtr<ExprNode> &expr_out);

		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_if_stmt(AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_with_stmt(AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_for_stmt(AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_while_stmt(AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_do_while_stmt(AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_let_stmt(AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_break_stmt(AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_continue_stmt(AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_return_stmt(AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_yield_stmt(AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_label_stmt(AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_case_stmt(AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_default_stmt(AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_block_stmt(AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_switch_stmt(AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_expr_stmt(AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_stmt(AstNodePtr<StmtNode> &stmt_out);

		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_generic_arg(AstNodePtr<AstNode> &arg_out);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_type_name(AstNodePtr<TypeNameNode> &type_name_out, bool with_circumfixes = true);

		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_attribute(AstNodePtr<AttributeNode> &attribute_out);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_attributes(peff::DynArray<AstNodePtr<AttributeNode>> &attributes_out);

		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_args(peff::DynArray<AstNodePtr<ExprNode>> &args_out, peff::DynArray<size_t> &idx_comma_tokens_out);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_generic_constraint(GenericConstraintPtr &constraint_out);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_param_type_list_generic_constraint(ParamTypeListGenericConstraintPtr &constraint_out);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_generic_params(peff::DynArray<AstNodePtr<GenericParamNode>> &generic_params_out, peff::DynArray<size_t> &idx_comma_tokens_out, size_t &l_angle_bracket_index_out, size_t &r_angle_bracket_index_out);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_params(peff::DynArray<AstNodePtr<VarNode>> &params_out, bool &var_arg_out, peff::DynArray<size_t> &idx_comma_tokens_out, size_t &l_angle_bracket_index_out, size_t &r_angle_bracket_index_out);

		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_fn(AstNodePtr<FnOverloadingNode> &fn_node_out);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_operator_name(std::string_view &name_out);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_id_name(peff::String &name_out);

		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_union_enum_item(AstNodePtr<ModuleNode> enum_out);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_enum_item(AstNodePtr<ModuleNode> enum_out);

		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_program_stmt();

	public:
		/// @brief Parse a whole program.
		/// @return The syntax error that forced the parser to interrupt the parse progress.
		/// @note Don't forget that there still may be syntax errors emitted even the parse progress is not interrupted.
		[[nodiscard]] SLKC_API virtual peff::Option<SyntaxError> parse_program(const AstNodePtr<ModuleNode> &initial_mod, IdRefPtr &module_name_out);
	};

#define SLKC_RETURN_IF_PARSE_ERROR(expr)         \
	if (peff::Option<SyntaxError> _ = (expr); _) \
		return _;                                \
	else
}

#endif
