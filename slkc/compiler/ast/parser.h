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
			Location location;

			inline SyntaxError(std::string_view s, Location location) : runtime_error(s.data()), location(location) {}
			virtual ~SyntaxError() = default;
		};

		class Parser {
		private:
			void _putDefinition(
				Location locName,
				std::string name,
				std::shared_ptr<MemberNode> member);
			void _putFnDefinition(
				Location locName,
				std::string name,
				std::shared_ptr<FnOverloadingNode> overloading);

		public:
			using OpParselet = std::function<std::shared_ptr<ExprNode>(Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken)>;

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

			inline const Token &expectToken(TokenId tokenId) {
				const auto &token = lexer->peekToken();
				if (token.tokenId == tokenId) {
					lexer->nextToken();
					return token;
				}
				throw SyntaxError(std::string("Expecting ") + getTokenName(tokenId), token.beginLocation);
			}

			inline const Token &expectToken(const Token &token) {
				if (token.tokenId == TokenId::End)
					throw SyntaxError("Expecting more tokens", token.beginLocation);

				return token;
			}

			inline const Token &expectToken(const Token &token, TokenId tokenId) {
				if (token.tokenId != tokenId)
					throw SyntaxError(std::string("Expecting ") + getTokenName(tokenId), token.beginLocation);

				return token;
			}

			inline const Token &expectTokens(const Token &token, TokenId tokenId) {
				if (token.tokenId == tokenId)
					return token;

				throw SyntaxError(std::string("Unexpected ") + getTokenName(token.tokenId), token.beginLocation);
			}

			template <typename... Args>
			inline const Token &expectTokens(const Token &token, TokenId tokenId, Args... args) {
				if (token.tokenId == tokenId)
					return token;

				return expectTokens(token, args...);
			}

			AccessModifier parseAccessModifier(Location &locationOut, std::deque<size_t> idxAccessModifierTokensOut);

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
				std::deque<size_t> &idxCommaTokensOut);
			std::deque<std::shared_ptr<TypeNameNode>> parseTraitList();

			void parseVarDefs(std::shared_ptr<VarDefStmtNode> varDefStmtOut);

			std::shared_ptr<StmtNode> parseStmt();

			void parseParams(std::deque<std::shared_ptr<ParamNode>> &paramsOut, std::deque<size_t> &idxCommaTokensOut);

			std::shared_ptr<FnOverloadingNode> parseFnDecl(std::string &nameOut);
			std::shared_ptr<FnOverloadingNode> parseFnDef(std::string &nameOut);
			std::shared_ptr<FnOverloadingNode> parseOperatorDecl(std::string &nameOut);
			std::shared_ptr<FnOverloadingNode> parseOperatorDef(std::string &nameOut);

			GenericParamNodeList parseGenericParams();

			std::shared_ptr<ClassNode> parseClassDef();
			void parseClassStmt();

			std::shared_ptr<InterfaceNode> parseInterfaceDef();
			void parseInterfaceStmt();

			std::shared_ptr<TraitNode> parseTraitDef();
			void parseTraitStmt();

			void parseProgramStmt();

			IdRef parseModuleRef();
			void parseModuleDecl();
			void parseImportList();

			void parse(Lexer *lexer, Compiler *compiler);
		};
	}
}

#endif
