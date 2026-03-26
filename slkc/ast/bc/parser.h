#ifndef _SLKC_AST_BC_PARSER_H_
#define _SLKC_AST_BC_PARSER_H_

#include "../document.h"
#include "../parser.h"
#include "fn.h"
#include "stmt.h"
#include "typename.h"
#include "expr.h"

namespace slkc {
	namespace bc {
		class BCParser;

		class BCParser : public Parser {
		public:
			SLKC_API BCParser(peff::SharedPtr<Document> document, TokenList &&token_list, peff::Alloc *resource_allocator);
			SLKC_API virtual ~BCParser();

		private:
			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_id_ref(IdRefPtr &id_ref_out);

			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_generic_arg(AstNodePtr<AstNode> &arg_out);
			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_type_name(AstNodePtr<TypeNameNode> &type_name_out, bool with_circumfixes = true);

			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_attribute(AstNodePtr<AttributeNode> &attribute_out);
			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_attributes(peff::DynArray<AstNodePtr<AttributeNode>> &attributes_out);

			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_generic_constraint(GenericConstraintPtr &constraint_out);
			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_param_type_list_generic_constraint(ParamTypeListGenericConstraintPtr &constraint_out);
			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_generic_params(peff::DynArray<AstNodePtr<GenericParamNode>> &generic_params_out, peff::DynArray<size_t> &idx_comma_tokens_out, size_t &l_angle_bracket_index_out, size_t &r_angle_bracket_index_out);
			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_params(peff::DynArray<AstNodePtr<VarNode>> &params_out, bool &var_arg_out, peff::DynArray<size_t> &idx_comma_tokens_out, size_t &l_angle_bracket_index_out, size_t &r_angle_bracket_index_out);

			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_operator_name(std::string_view &name_out);
			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_id_name(peff::String &name_out);

			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_comptime_expr(AstNodePtr<ExprNode> &expr_out);
			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_stmt(AstNodePtr<BCStmtNode> &stmt_out);

			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_fn(AstNodePtr<BCFnOverloadingNode> &fn_node_out);

			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse_program_stmt();

		public:
			/// @brief Parse a whole program.
			/// @return The syntax error that forced the parser to interrupt the parse progress.
			/// @note Don't forget that there still may be syntax errors emitted even the parse progress is not interrupted.
			[[nodiscard]] SLKC_API virtual peff::Option<SyntaxError> parse_program(const AstNodePtr<ModuleNode> &initial_mod, IdRefPtr &module_name_out) override;
		};
	}
}

#endif
