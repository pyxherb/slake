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
			SLKC_API BCParser(peff::SharedPtr<Document> document, TokenList &&tokenList, peff::Alloc *resourceAllocator);
			SLKC_API virtual ~BCParser();

		private:
			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseIdRef(IdRefPtr &idRefOut);

			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseGenericArg(AstNodePtr<AstNode> &argOut);
			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseTypeName(AstNodePtr<TypeNameNode> &typeNameOut, bool withCircumfixes = true);

			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseAttribute(AstNodePtr<AttributeNode> &attributeOut);
			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseAttributes(peff::DynArray<AstNodePtr<AttributeNode>> &attributesOut);

			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseGenericConstraint(GenericConstraintPtr &constraintOut);
			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseParamTypeListGenericConstraint(ParamTypeListGenericConstraintPtr &constraintOut);
			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseGenericParams(peff::DynArray<AstNodePtr<GenericParamNode>> &genericParamsOut, peff::DynArray<size_t> &idxCommaTokensOut, size_t &lAngleBracketIndexOut, size_t &rAngleBracketIndexOut);
			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseParams(peff::DynArray<AstNodePtr<VarNode>> &paramsOut, bool &varArgOut, peff::DynArray<size_t> &idxCommaTokensOut, size_t &lAngleBracketIndexOut, size_t &rAngleBracketIndexOut);

			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseOperatorName(std::string_view &nameOut);
			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseIdName(peff::String &nameOut);

			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseComptimeExpr(AstNodePtr<ExprNode> &exprOut);
			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseStmt(AstNodePtr<BCStmtNode> &stmtOut);

			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseFn(AstNodePtr<BCFnOverloadingNode> &fnNodeOut);

			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseProgramStmt();

		public:
			/// @brief Parse a whole program.
			/// @return The syntax error that forced the parser to interrupt the parse progress.
			/// @note Don't forget that there still may be syntax errors emitted even the parse progress is not interrupted.
			[[nodiscard]] SLKC_API virtual peff::Option<SyntaxError> parseProgram(const AstNodePtr<ModuleNode> &initialMod, IdRefPtr &moduleNameOut) override;
		};
	}
}

#endif
