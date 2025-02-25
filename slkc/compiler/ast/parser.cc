#include <slkc/compiler/compiler.h>

using namespace slake;
using namespace slake::slkc;

void Parser::parseProgramStmt() {
	TokenRange tokenRange;
	std::deque<size_t> idxAccessModifierTokens;
	AccessModifier accessModifier = parseAccessModifier(tokenRange, idxAccessModifierTokens);

	Token *token = lexer->peekToken(true, false, true);

	switch (token->tokenId) {
		case TokenId::End:
			return;
		case TokenId::NewLine:
			lexer->nextToken(true, false, true);
			if (isLastTokenNewline)
				resetLineCommentDocumentation();
			isLastTokenNewline = true;
			break;
		case TokenId::LineComment:
		case TokensId::BlockComment:
			lexer->nextToken(true, false, true);
			resetLineCommentDocumentation();
			break;
		case TokenId::DocumentationComment:
			lexer->nextToken(true, false, true);
			updateLineCommentDocumentation(token);
			break;
		case TokenId::ClassKeyword: {
			auto def = parseClassDef();

			def->tokenRange = tokenRange;
			def->access = accessModifier | ACCESS_STATIC;
			def->idxAccessModifierTokens = std::move(idxAccessModifierTokens);
			def->documentation = extractLineCommentDocumentation();

			_putDefinition(
				def->name,
				def);
			break;
		}
		case TokenId::InterfaceKeyword: {
			auto def = parseInterfaceDef();

			def->tokenRange = tokenRange;
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

			overloading->tokenRange = tokenRange;
			overloading->access = accessModifier;
			overloading->access |= ACCESS_STATIC;
			overloading->idxAccessModifierTokens = std::move(idxAccessModifierTokens);

			_putFnDefinition(name, overloading);

			break;
		}
		case TokenId::LetKeyword: {
			Token *letToken = lexer->nextToken();

			auto stmt = std::make_shared<VarDefStmtNode>();
			stmt->tokenRange = { curDoc, lexer->getTokenIndex(letToken) };

			stmt->idxLetToken = lexer->getTokenIndex(letToken);

			// TODO: Apply indices of access modifier tokens to the variables.

			parseVarDefs(stmt);

			Token *semicolonToken = expectToken(TokenId::Semicolon);
			stmt->tokenRange.endIndex = lexer->getTokenIndex(semicolonToken);
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
				varNode->tokenRange = i.second.tokenRange;

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
			throw SyntaxError("Unrecognized token", { curDoc, lexer->getTokenIndex(token) });
	}

	if (token->tokenId != TokenId::NewLine)
		isLastTokenNewline = false;
}

void Parser::parseModuleDecl() {
	if (Token *beginToken = lexer->peekToken(); beginToken->tokenId == TokenId::ModuleKeyword) {
		lexer->nextToken();

		compiler->sourceDocs.at(curDocName)->targetModule->moduleName = parseModuleRef();

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

				compiler->sourceDocs.at(curDocName)->targetModule->imports[nameToken->text] = { ref, lexer->getTokenIndex(nameToken) };
			} else
				compiler->sourceDocs.at(curDocName)->targetModule->unnamedImports.push_back({ ref, SIZE_MAX });

			if (lexer->peekToken()->tokenId != TokenId::Comma)
				break;

			lexer->nextToken();
		}

		expectToken(TokenId::RBrace);
	}
}

void Parser::parse(SourceDocument *curDoc, Compiler *compiler) {
	this->compiler = compiler;
	this->curDoc = curDoc;
	this->lexer = curDoc->lexer.get();

	curDocName = compiler->curDocName;
	curScope = compiler->sourceDocs.at(compiler->curDocName)->targetModule->scope;

	parseModuleDecl();
	parseImportList();

	while (lexer->peekToken()->tokenId != TokenId::End) {
		parseProgramStmt();
	}
}
