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
#include "document.h"
#include <coroutine>

namespace slkc {
	class Parser;

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

	class ParseCoroutineScheduler;

	struct ParseCoroutine {
		struct promise_type;

		using Handle = std::coroutine_handle<promise_type>;

		struct promise_type {
			peff::Option<SyntaxError> result;

			PEFF_FORCEINLINE static ParseCoroutine get_return_object_on_allocation_failure() noexcept {
				return ParseCoroutine({});
			}

			PEFF_FORCEINLINE ParseCoroutine get_return_object() noexcept {
				return ParseCoroutine(Handle::from_promise(*this));
			}

			PEFF_FORCEINLINE std::suspend_always initial_suspend() noexcept {
				return {};
			}

			PEFF_FORCEINLINE std::suspend_always final_suspend() noexcept {
				return {};
			}

			PEFF_FORCEINLINE std::suspend_always yield_value(peff::Option<SyntaxError> &&value) noexcept {
				result = std::move(value);
				return {};
			}

			PEFF_FORCEINLINE void return_value(peff::Option<SyntaxError> &&value) noexcept {
				result = std::move(value);
			}

			PEFF_FORCEINLINE void unhandled_exception() { std::terminate(); }

			struct AllocatorInfo {
				peff::Alloc *allocator;
#if PEFF_ENABLE_RCOBJ_DEBUGGING
				size_t c;
#endif
			};

			template <typename First, typename... Args>
			SLAKE_FORCEINLINE static void *operator new(size_t size, First &&first, peff::Alloc *allocator, Args &&...args) noexcept {
				char *p = (char *)allocator->alloc(size + sizeof(AllocatorInfo), alignof(std::max_align_t));

				if (!p)
					return nullptr;

				memset(p, 0, size);

#if PEFF_ENABLE_RCOBJ_DEBUGGING
				auto ref_count = peff::acquire_global_rcobj_ptr_counter();
#endif

				AllocatorInfo allocator_info = {
					allocator
#if PEFF_ENABLE_RCOBJ_DEBUGGING
					,
					ref_count
#endif
				};

				memcpy(p + size, &allocator_info, sizeof(allocator_info));
#if PEFF_ENABLE_RCOBJ_DEBUGGING
				allocator->inc_ref(ref_count);
#endif

				return p;
			}

			SLAKE_FORCEINLINE static void operator delete(void *p, size_t size) noexcept {
				AllocatorInfo allocator_info;

				memcpy(&allocator_info, (char *)p + size, sizeof(allocator_info));

				peff::RcObjectPtr<peff::Alloc> allocator_holder = allocator_info.allocator;

				allocator_holder->release(p, size + sizeof(AllocatorInfo), alignof(std::max_align_t));
#if PEFF_ENABLE_RCOBJ_DEBUGGING
				allocator_holder->dec_ref(allocator_info.c);
#endif
			}
		};

		Handle coro_handle;

		static inline bool recursed = false;

		ParseCoroutine(Handle coro_handle) : coro_handle(coro_handle) {}
		~ParseCoroutine() {
			// assert(!recursed);
			// recursed = true;
			if (coro_handle)
				coro_handle.destroy();
			// recursed = false;
		}

		SLAKE_FORCEINLINE bool done() {
			return coro_handle.done();
		}

		SLAKE_API peff::Option<SyntaxError> resume(Parser *parser);

		struct Awaitable {
			ParseCoroutine &co;
			Parser *parser;
			ParseCoroutineScheduler *scheduler;
			Handle handle;

			SLKC_API Awaitable(ParseCoroutine &co, Parser *parser, ParseCoroutineScheduler *scheduler, Handle handle);
			SLKC_API bool await_ready();
			SLKC_API void await_suspend(Handle h);
			[[nodiscard]] SLKC_API peff::Option<SyntaxError> await_resume();
		};

		SLKC_API Awaitable operator()(Parser *parser);
	};

	class ParseCoroutineScheduler {
	public:
		peff::DynArray<std::coroutine_handle<ParseCoroutine::promise_type>> task_list;

		SLKC_API ParseCoroutineScheduler(peff::Alloc *allocator);
	};

	class Parser : public peff::SharedFromThis<Parser> {
	public:
		ParseCoroutineScheduler parse_coro_scheduler;
		peff::WeakPtr<Document> document;
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

		SLAKE_FORCEINLINE peff::SharedPtr<Document> get_document() const noexcept {
			return document.lock();
		}

		SLKC_API SyntaxError gen_oom_syntax_error() const noexcept {
			return SyntaxError(TokenRange{ get_document()->main_module, 0 }, SyntaxErrorKind::OutOfMemory);
		}

		SLKC_API peff::Option<SyntaxError> lookahead_until(size_t num_token_ids, const TokenId token_ids[]);
		SLKC_API Token *next_token(bool keep_new_line = false, bool keep_whitespace = false, bool keep_comment = false);
		SLKC_API Token *peek_token(bool keep_new_line = false, bool keep_whitespace = false, bool keep_comment = false);

		[[nodiscard]] SLAKE_FORCEINLINE peff::Option<SyntaxError> expect_token(Token *token, TokenId token_id) {
			if (token->token_id != token_id) {
				ExpectingSingleTokenErrorExData ex_data = { token_id };

				return SyntaxError(TokenRange{ get_document()->main_module, token->index }, std::move(ex_data));
			}

			return {};
		}

		[[nodiscard]] SLAKE_FORCEINLINE peff::Option<SyntaxError> expect_token(Token *token) {
			if (token->token_id == TokenId::End) {
				ExpectingTokensErrorExData ex_data(resource_allocator.get());

				return SyntaxError(TokenRange{ get_document()->main_module, token->index }, std::move(ex_data));
			}

			return {};
		}

		PEFF_FORCEINLINE peff::Option<SyntaxError> push_literal_overflowed_error(Token *token) noexcept {
			if (!syntax_errors.push_back(SyntaxError(TokenRange{ get_document()->main_module, token->index }, SyntaxErrorKind::LiteralOverflowed)))
				return gen_oom_syntax_error();
			return {};
		}

		[[nodiscard]] SLKC_API peff::Option<SyntaxError> split_shr_op_token();
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> split_rdbrackets_token();

	private:
		[[nodiscard]] SLKC_API ParseCoroutine parse_var_defs(peff::Alloc *allocator, peff::DynArray<VarDefEntryPtr> &var_def_entries);

		[[nodiscard]] SLKC_API ParseCoroutine parse_id_ref(peff::Alloc *allocator, IdRefPtr &id_ref_out);

		[[nodiscard]] SLKC_API ParseCoroutine parse_expr(peff::Alloc *allocator, int precedence, AstNodePtr<ExprNode> &expr_out);

		[[nodiscard]] SLKC_API ParseCoroutine parse_if_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API ParseCoroutine parse_with_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API ParseCoroutine parse_for_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API ParseCoroutine parse_while_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API ParseCoroutine parse_do_while_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API ParseCoroutine parse_let_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API ParseCoroutine parse_break_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API ParseCoroutine parse_continue_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API ParseCoroutine parse_return_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API ParseCoroutine parse_yield_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API ParseCoroutine parse_label_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API ParseCoroutine parse_case_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API ParseCoroutine parse_default_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API ParseCoroutine parse_block_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API ParseCoroutine parse_switch_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API ParseCoroutine parse_expr_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out);
		[[nodiscard]] SLKC_API ParseCoroutine parse_stmt(peff::Alloc *allocator, AstNodePtr<StmtNode> &stmt_out);

		[[nodiscard]] SLKC_API ParseCoroutine parse_generic_arg(peff::Alloc *allocator, AstNodePtr<AstNode> &arg_out);
		[[nodiscard]] SLKC_API ParseCoroutine parse_type_name(peff::Alloc *allocator, AstNodePtr<TypeNameNode> &type_name_out, bool with_circumfixes = true);

		[[nodiscard]] SLKC_API ParseCoroutine parse_attribute(peff::Alloc *allocator, AstNodePtr<AttributeNode> &attribute_out);
		[[nodiscard]] SLKC_API ParseCoroutine parse_attributes(peff::Alloc *allocator, peff::DynArray<AstNodePtr<AttributeNode>> &attributes_out);

		[[nodiscard]] SLKC_API ParseCoroutine parse_args(peff::Alloc *allocator, peff::DynArray<AstNodePtr<ExprNode>> &args_out, peff::DynArray<size_t> &idx_comma_tokens_out);
		[[nodiscard]] SLKC_API ParseCoroutine parse_generic_constraint(peff::Alloc *allocator, GenericConstraintPtr &constraint_out);
		[[nodiscard]] SLKC_API ParseCoroutine parse_param_type_list_generic_constraint(peff::Alloc *allocator, ParamTypeListGenericConstraintPtr &constraint_out);
		[[nodiscard]] SLKC_API ParseCoroutine parse_generic_params(peff::Alloc *allocator, peff::DynArray<AstNodePtr<GenericParamNode>> &generic_params_out, peff::DynArray<size_t> &idx_comma_tokens_out, size_t &l_angle_bracket_index_out, size_t &r_angle_bracket_index_out);
		[[nodiscard]] SLKC_API ParseCoroutine parse_params(peff::Alloc *allocator, peff::DynArray<AstNodePtr<VarNode>> &params_out, bool &var_arg_out, peff::DynArray<size_t> &idx_comma_tokens_out, size_t &l_angle_bracket_index_out, size_t &r_angle_bracket_index_out);

		[[nodiscard]] SLKC_API ParseCoroutine parse_fn(peff::Alloc *allocator, AstNodePtr<FnOverloadingNode> &fn_node_out);
		[[nodiscard]] SLKC_API ParseCoroutine parse_operator_name(peff::Alloc *allocator, std::string_view &name_out);
		[[nodiscard]] SLKC_API ParseCoroutine parse_id_name(peff::Alloc *allocator, peff::String &name_out);

		[[nodiscard]] SLKC_API ParseCoroutine parse_union_enum_item(peff::Alloc *allocator, AstNodePtr<ModuleNode> enum_out);
		[[nodiscard]] SLKC_API ParseCoroutine parse_enum_item(peff::Alloc *allocator, AstNodePtr<ModuleNode> enum_out);

		[[nodiscard]] SLKC_API ParseCoroutine parse_program_stmt(peff::Alloc *allocator);

		[[nodiscard]] SLKC_API virtual ParseCoroutine parse_program(peff::Alloc *allocator, const AstNodePtr<ModuleNode> &initial_mod, IdRefPtr &module_name_out);

	public:
		[[nodiscard]] SLKC_API virtual peff::Option<SyntaxError> parse(const AstNodePtr<ModuleNode> &initial_mod, IdRefPtr &module_name_out);
	};

#define SLKC_RETURN_IF_PARSE_ERROR(expr)         \
	if (peff::Option<SyntaxError> _ = (expr); _) \
		return _;                                \
	else

#define SLKC_CO_RETURN_IF_PARSE_ERROR(expr)      \
	if (peff::Option<SyntaxError> _ = (expr); _) \
		co_return _;                             \
	else

#define SLKC_CO_RETURN_IF_CO_PARSE_ERROR(expr)                    \
	if (peff::Option<SyntaxError> _ = co_await ((expr)(this)); _) \
		co_return _;                                              \
	else
}

namespace std {
	template <typename... Args>
	struct coroutine_traits<slkc::ParseCoroutine, peff::Alloc *, Args...> {
		using promise_type = slkc::ParseCoroutine::promise_type;
	};
}

static_assert(std::is_same_v<std::coroutine_traits<slkc::ParseCoroutine, peff::Alloc *>::promise_type, slkc::ParseCoroutine::promise_type>);

#endif
