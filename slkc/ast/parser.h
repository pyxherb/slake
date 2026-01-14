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
		ConflictingDefinitions
	};

	struct ExpectingSingleTokenErrorExData {
		TokenId expectingTokenId;
	};

	struct ExpectingTokensErrorExData {
		peff::Set<TokenId> expectingTokenIds;

		SLAKE_FORCEINLINE ExpectingTokensErrorExData(peff::Alloc *allocator) : expectingTokenIds(allocator) {
		}
	};

	struct NoMatchingTokensFoundErrorExData {
		peff::Set<TokenId> expectingTokenIds;

		SLAKE_FORCEINLINE NoMatchingTokensFoundErrorExData(peff::Alloc *allocator) : expectingTokenIds(allocator) {
		}
	};

	struct ConflictingDefinitionsErrorExData {
		peff::String memberName;

		SLAKE_FORCEINLINE ConflictingDefinitionsErrorExData(peff::String &&name) : memberName(std::move(name)) {
		}
	};

	struct SyntaxError {
		TokenRange tokenRange;
		SyntaxErrorKind errorKind;
		std::variant<std::monostate, ExpectingTokensErrorExData, NoMatchingTokensFoundErrorExData, ExpectingSingleTokenErrorExData, ConflictingDefinitionsErrorExData> exData;

		SLAKE_FORCEINLINE SyntaxError(
			const TokenRange &tokenRange,
			SyntaxErrorKind errorKind)
			: tokenRange(tokenRange),
			  errorKind(errorKind) {
		}

		SLAKE_FORCEINLINE SyntaxError(
			const TokenRange &tokenRange,
			ExpectingTokensErrorExData &&exData)
			: tokenRange(tokenRange),
			  errorKind(SyntaxErrorKind::ExpectingTokens),
			  exData(std::move(exData)) {
		}

		SLAKE_FORCEINLINE SyntaxError(
			const TokenRange &tokenRange,
			ExpectingSingleTokenErrorExData &&exData)
			: tokenRange(tokenRange),
			  errorKind(SyntaxErrorKind::ExpectingSingleToken),
			  exData(std::move(exData)) {
		}

		SLAKE_FORCEINLINE SyntaxError(
			const TokenRange &tokenRange,
			NoMatchingTokensFoundErrorExData &&exData)
			: tokenRange(tokenRange),
			  errorKind(SyntaxErrorKind::NoMatchingTokensFound),
			  exData(std::move(exData)) {
		}

		SLAKE_FORCEINLINE SyntaxError(
			const TokenRange &tokenRange,
			ConflictingDefinitionsErrorExData &&exData)
			: tokenRange(tokenRange),
			  errorKind(SyntaxErrorKind::ConflictingDefinitions),
			  exData(std::move(exData)) {
		}

		SLAKE_FORCEINLINE ExpectingTokensErrorExData &getExpectingTokensErrorExData() {
			return std::get<ExpectingTokensErrorExData>(exData);
		}

		SLAKE_FORCEINLINE const ExpectingTokensErrorExData &getExpectingTokensErrorExData() const {
			return std::get<ExpectingTokensErrorExData>(exData);
		}

		SLAKE_FORCEINLINE const NoMatchingTokensFoundErrorExData &getNoMatchingTokensFoundErrorExData() const {
			return std::get<NoMatchingTokensFoundErrorExData>(exData);
		}
	};

	class Parser;

	class Parser : public peff::SharedFromThis<Parser> {
	public:
		peff::SharedPtr<Document> document;
		AstNodePtr<MemberNode> curParent;
		peff::RcObjectPtr<peff::Alloc> resourceAllocator;
		TokenList tokenList;
		struct ParseContext {
			size_t idxPrevToken = 0, idxCurrentToken = 0;
		};
		ParseContext parseContext;
		peff::DynArray<SyntaxError> syntaxErrors;

		SLKC_API Parser(peff::SharedPtr<Document> document, TokenList &&tokenList, peff::Alloc *resourceAllocator);
		SLKC_API virtual ~Parser();

		SLKC_API SyntaxError genOutOfMemorySyntaxError() {
			return SyntaxError(TokenRange{ document->mainModule, 0 }, SyntaxErrorKind::OutOfMemory);
		}

		SLKC_API peff::Option<SyntaxError> lookaheadUntil(size_t nTokenIds, const TokenId tokenIds[]);
		SLKC_API Token *nextToken(bool keepNewLine = false, bool keepWhitespace = false, bool keepComment = false);
		SLKC_API Token *peekToken(bool keepNewLine = false, bool keepWhitespace = false, bool keepComment = false);

		[[nodiscard]] SLAKE_FORCEINLINE peff::Option<SyntaxError> expectToken(Token *token, TokenId tokenId) {
			if (token->tokenId != tokenId) {
				ExpectingSingleTokenErrorExData exData = { tokenId };

				return SyntaxError(TokenRange{ document->mainModule, token->index }, std::move(exData));
			}

			return {};
		}

		[[nodiscard]] SLAKE_FORCEINLINE peff::Option<SyntaxError> expectToken(Token *token) {
			if (token->tokenId == TokenId::End) {
				ExpectingTokensErrorExData exData(resourceAllocator.get());

				return SyntaxError(TokenRange{ document->mainModule, token->index }, std::move(exData));
			}

			return {};
		}

		[[nodiscard]] SLKC_API peff::Option<SyntaxError> splitRshOpToken();
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> splitRDBracketsToken();

	private:
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseVarDefs(peff::DynArray<VarDefEntryPtr> &varDefEntries);

		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseIdRef(IdRefPtr &idRefOut);

		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseExpr(int precedence, AstNodePtr<ExprNode> &exprOut);

		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseIfStmt(AstNodePtr<StmtNode> &stmtOut);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseWithStmt(AstNodePtr<StmtNode> &stmtOut);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseForStmt(AstNodePtr<StmtNode> &stmtOut);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseWhileStmt(AstNodePtr<StmtNode> &stmtOut);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseDoWhileStmt(AstNodePtr<StmtNode> &stmtOut);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseLetStmt(AstNodePtr<StmtNode> &stmtOut);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseBreakStmt(AstNodePtr<StmtNode> &stmtOut);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseContinueStmt(AstNodePtr<StmtNode> &stmtOut);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseReturnStmt(AstNodePtr<StmtNode> &stmtOut);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseYieldStmt(AstNodePtr<StmtNode> &stmtOut);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseLabelStmt(AstNodePtr<StmtNode> &stmtOut);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseCaseStmt(AstNodePtr<StmtNode> &stmtOut);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseDefaultStmt(AstNodePtr<StmtNode> &stmtOut);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseBlockStmt(AstNodePtr<StmtNode> &stmtOut);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseSwitchStmt(AstNodePtr<StmtNode> &stmtOut);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseExprStmt(AstNodePtr<StmtNode> &stmtOut);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseStmt(AstNodePtr<StmtNode> &stmtOut);

		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseGenericArg(AstNodePtr<AstNode> &argOut);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseTypeName(AstNodePtr<TypeNameNode> &typeNameOut, bool withCircumfixes = true);

		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseAttribute(AstNodePtr<AttributeNode> &attributeOut);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseAttributes(peff::DynArray<AstNodePtr<AttributeNode>> &attributesOut);

		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseArgs(peff::DynArray<AstNodePtr<ExprNode>> &argsOut, peff::DynArray<size_t> &idxCommaTokensOut);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseGenericConstraint(GenericConstraintPtr &constraintOut);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseParamTypeListGenericConstraint(ParamTypeListGenericConstraintPtr &constraintOut);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseGenericParams(peff::DynArray<AstNodePtr<GenericParamNode>> &genericParamsOut, peff::DynArray<size_t> &idxCommaTokensOut, size_t &lAngleBracketIndexOut, size_t &rAngleBracketIndexOut);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseParams(peff::DynArray<AstNodePtr<VarNode>> &paramsOut, bool &varArgOut, peff::DynArray<size_t> &idxCommaTokensOut, size_t &lAngleBracketIndexOut, size_t &rAngleBracketIndexOut);

		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseFn(AstNodePtr<FnOverloadingNode> &fnNodeOut);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseOperatorName(std::string_view &nameOut);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseIdName(peff::String &nameOut);

		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseUnionEnumItem(AstNodePtr<ModuleNode> enumOut);
		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseEnumItem(AstNodePtr<ModuleNode> enumOut);

		[[nodiscard]] SLKC_API peff::Option<SyntaxError> parseProgramStmt();

	public:
		/// @brief Parse a whole program.
		/// @return The syntax error that forced the parser to interrupt the parse progress.
		/// @note Don't forget that there still may be syntax errors emitted even the parse progress is not interrupted.
		[[nodiscard]] SLKC_API virtual peff::Option<SyntaxError> parseProgram(const AstNodePtr<ModuleNode> &initialMod, IdRefPtr &moduleNameOut);
	};
}

#endif
