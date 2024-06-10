#include <slkc/compiler/compiler.h>

using namespace slake;
using namespace slake::slkc;

void Parser::parseProgramStmt() {
	SourceLocation loc;
	std::deque<size_t> idxAccessModifierTokens;
	AccessModifier accessModifier = parseAccessModifier(loc, idxAccessModifierTokens);

	switch (Token *token = lexer->peekToken(); token->tokenId) {
		case TokenId::End:
			return;
		case TokenId::ClassKeyword: {
			auto def = parseClassDef();

			def->sourceLocation.beginPosition = loc.beginPosition;
			def->access = accessModifier | ACCESS_STATIC;
			def->idxAccessModifierTokens = std::move(idxAccessModifierTokens);

			_putDefinition(
				def->name,
				def);
			break;
		}
		case TokenId::InterfaceKeyword: {
			auto def = parseInterfaceDef();

			def->sourceLocation.beginPosition = loc.beginPosition;
			def->access = accessModifier | ACCESS_STATIC;
			def->idxAccessModifierTokens = std::move(idxAccessModifierTokens);

			_putDefinition(
				def->name,
				def);
			break;
		}
		case TokenId::FnKeyword: {
			std::string name;
			auto overloading = parseFnDef(name);

			overloading->sourceLocation.beginPosition = loc.beginPosition;
			overloading->access = accessModifier;
			overloading->access |= ACCESS_STATIC;
			overloading->idxAccessModifierTokens = std::move(idxAccessModifierTokens);

			_putFnDefinition(name, overloading);

			break;
		}
		case TokenId::LetKeyword: {
			Token *letToken = lexer->nextToken();

			auto stmt = std::make_shared<VarDefStmtNode>();
			stmt->sourceLocation = letToken->location;

			stmt->idxLetToken = lexer->getTokenIndex(letToken);

			// TODO: Apply indices of access modifier tokens to the variables.

			parseVarDefs(stmt);

			Token *semicolonToken = expectToken(TokenId::Semicolon);
			stmt->sourceLocation.endPosition = semicolonToken->location.endPosition;
			stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);

			bool idxAccessModifierTokensMoved = false;

			for (auto &i : stmt->varDefs) {
				auto varNode = std::make_shared<VarNode>(
					compiler,
					accessModifier,
					i.second.type,
					i.first,
					i.second.initValue,
					i.second.idxNameToken,
					i.second.idxColonToken,
					i.second.idxAssignOpToken,
					i.second.idxCommaToken);
				varNode->access |= ACCESS_STATIC;
				varNode->sourceLocation = i.second.loc;

				if (!idxAccessModifierTokensMoved) {
					varNode->idxAccessModifierTokens = std::move(idxAccessModifierTokens);
					idxAccessModifierTokensMoved = true;
				}

				_putDefinition(
					i.first,
					varNode);
			}
			break;
		}
		default:
			throw SyntaxError("Unrecognized token", token->location);
	}
}

void Parser::parseModuleDecl() {
	if (Token *beginToken = lexer->peekToken(); beginToken->tokenId == TokenId::ModuleKeyword) {
		lexer->nextToken();

		curModule->moduleName = parseModuleRef();

		expectToken(TokenId::Semicolon);
	}
}

void Parser::parseImportList() {
	if (Token *beginToken = lexer->peekToken(); beginToken->tokenId == TokenId::ImportKeyword) {
		lexer->nextToken();

		Token *lBraceToken = expectToken(TokenId::LBrace);

#if SLKC_WITH_LANGUAGE_SERVER
		compiler->updateTokenInfo(lexer->getTokenIndex(lBraceToken), [](TokenInfo &tokenInfo) {
			tokenInfo.completionContext = CompletionContext::Import;
		});
#endif

		while (true) {
			auto ref = parseModuleRef();

			if (Token *asToken = lexer->peekToken(); asToken->tokenId == TokenId::AsKeyword) {
				lexer->nextToken();

				Token *nameToken = expectToken(TokenId::Id);

				curModule->imports[nameToken->text] = { ref, lexer->getTokenIndex(nameToken) };
			} else
				curModule->unnamedImports.push_back({ ref, SIZE_MAX });

			if (lexer->peekToken()->tokenId != TokenId::Comma)
				break;

			lexer->nextToken();
		}

		expectToken(TokenId::RBrace);
	}
}

void Parser::parse(Lexer *lexer, Compiler *compiler) {
	this->compiler = compiler;
	this->lexer = lexer;

	curModule = compiler->_targetModule;
	curScope = curModule->scope;

	parseModuleDecl();
	parseImportList();

	while (lexer->peekToken()->tokenId != TokenId::End) {
		parseProgramStmt();
	}
}
