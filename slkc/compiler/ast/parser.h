#ifndef _SLKC_COMPILER_SYNTAX_PARSER_H_
#define _SLKC_COMPILER_SYNTAX_PARSER_H_

#include "lexer.h"
#include <stdexcept>
#include "expr.h"
#include "stmt.h"
#include "fn.h"
#include <slake/access.h>
#include "class.h"
#include "var.h"
#include "module.h"

namespace slake {
	namespace slkc {
		class Compiler;

		class SyntaxError : public std::runtime_error {
		public:
			SourceLocation location;

			inline SyntaxError(std::string_view s, SourceLocation location) : runtime_error(s.data()), location(location) {}
			virtual ~SyntaxError() = default;
		};

		class Parser {
		private:
			void _putDefinition(
				std::string name,
				std::shared_ptr<MemberNode> member);
			void _putFnDefinition(
				std::string name,
				std::shared_ptr<FnOverloadingNode> overloading);

		public:
			using OpParselet = std::function<std::shared_ptr<ExprNode>(Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken)>;

			struct OpRegistry {
				int leftPrecedence;
				OpParselet parselet;
			};

			static std::map<TokenId, OpRegistry> prefixOpRegistries, infixOpRegistries;

			std::shared_ptr<Scope> curScope;
			std::shared_ptr<ModuleNode> curModule;
			Lexer *lexer;
			Compiler *compiler;

			inline void reset() {
				curScope.reset();
				curModule.reset();
				lexer = nullptr;
				compiler = nullptr;
			}

			inline Token *expectToken(TokenId tokenId) {
				Token *token = lexer->peekToken();
				if (token->tokenId == tokenId) {
					lexer->nextToken();
					return token;
				}
				throw SyntaxError(std::string("Expecting ") + getTokenName(tokenId), token->location);
			}

			inline Token *expectToken(Token *token) {
				if (token->tokenId == TokenId::End)
					throw SyntaxError("Expecting more tokens", token->location);

				return token;
			}

			inline Token *expectToken(Token *token, TokenId tokenId) {
				if (token->tokenId != tokenId)
					throw SyntaxError(std::string("Expecting ") + getTokenName(tokenId), token->location);

				return token;
			}

			inline Token *expectTokens(Token *token, TokenId tokenId) {
				if (token->tokenId == tokenId)
					return token;

				throw SyntaxError(std::string("Unexpected ") + getTokenName(token->tokenId), token->location);
			}

			template <typename... Args>
			inline Token *expectTokens(Token *token, TokenId tokenId, Args... args) {
				if (token.tokenId == tokenId)
					return token;

				return expectTokens(token, args...);
			}

			void splitRshOpToken();

			AccessModifier parseAccessModifier(SourceLocation &locationOut, std::deque<size_t> idxAccessModifierTokensOut);

			std::shared_ptr<TypeNameNode> parseTypeName(bool required = false);
			std::deque<std::shared_ptr<TypeNameNode>> parseGenericArgs(bool forTypeName = false);
			IdRef parseRef(bool forTypeName = false);
			void parseArgs(std::deque<std::shared_ptr<ExprNode>> &argsOut, std::deque<size_t> &idxCommaTokensOut);

			std::shared_ptr<ExprNode> parseExpr(int precedence = 0);

			void parseParentSlot(
				std::shared_ptr<TypeNameNode> &typeNameOut,
				size_t &idxLParentheseTokenOut,
				size_t &idxRParentheseTokenOut);
			void parseImplList(
				std::deque<std::shared_ptr<TypeNameNode>> &implInterfacesOut,
				size_t &idxColonTokenOut,
				std::deque<size_t> &idxSeparatorTokensOut);

			void parseVarDefs(std::shared_ptr<VarDefStmtNode> varDefStmtOut);

			std::shared_ptr<StmtNode> parseStmt();

			void parseParams(std::deque<std::shared_ptr<ParamNode>> &paramsOut, std::deque<size_t> &idxCommaTokensOut);

			std::shared_ptr<FnOverloadingNode> parseFnDecl(std::string &nameOut);
			std::shared_ptr<FnOverloadingNode> parseFnDef(std::string &nameOut);
			std::shared_ptr<FnOverloadingNode> parseOperatorDecl(std::string &nameOut);
			std::shared_ptr<FnOverloadingNode> parseOperatorDef(std::string &nameOut);

			GenericParamNodeList parseGenericParams(SourceLocation &locationOut);

			std::shared_ptr<ClassNode> parseClassDef();
			void parseClassStmt();

			std::shared_ptr<InterfaceNode> parseInterfaceDef();
			void parseInterfaceStmt();

			void parseProgramStmt();

			IdRef parseModuleRef();
			void parseModuleDecl();
			void parseImportList();

			void parse(Lexer *lexer, Compiler *compiler);
		};
	}
}

#endif
