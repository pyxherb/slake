#include <slkc/compiler/compiler.h>

using namespace slake;
using namespace slake::slkc;

void Parser::parseParentSlot(
	std::shared_ptr<TypeNameNode> &typeNameOut,
	size_t &idxLParentheseTokenOut,
	size_t &idxRParentheseTokenOut) {
	if (Token *lParentheseToken = lexer->peekToken(); lParentheseToken->tokenId == TokenId::LParenthese) {
		lexer->nextToken();

		idxLParentheseTokenOut = lexer->getTokenIndex(lParentheseToken);

		typeNameOut = parseTypeName();

		Token *rParentheseToken = expectToken(TokenId::RParenthese);
		idxRParentheseTokenOut = lexer->getTokenIndex(rParentheseToken);
	}
}

void Parser::parseImplList(
	std::deque<std::shared_ptr<TypeNameNode>> &implInterfacesOut,
	size_t &idxColonTokenOut,
	std::deque<size_t> &idxSeparatorTokensOut) {
	if (Token *colonToken = lexer->peekToken(); colonToken->tokenId == TokenId::Colon) {
		lexer->nextToken();
		idxColonTokenOut = lexer->getTokenIndex(colonToken);

		while (true) {
			implInterfacesOut.push_back(parseTypeName());

			if (lexer->peekToken()->tokenId != TokenId::OrOp)
				break;

			Token *commaToken = lexer->nextToken();
			idxSeparatorTokensOut.push_back(lexer->getTokenIndex(commaToken));
		}
	}
}

GenericParamNodeList Parser::parseGenericParams(TokenRange &tokenRangeOut) {
	GenericParamNodeList genericParams;

	Token *startingToken = expectToken(TokenId::LtOp);
	tokenRangeOut = lexer->getTokenIndex(startingToken);

	while (true) {
		Token *nameToken = expectToken(TokenId::Id);

		auto param = std::make_shared<GenericParamNode>(nameToken->text);

		param->tokenRange = lexer->getTokenIndex(nameToken);
		param->idxNameToken = lexer->getTokenIndex(nameToken);

		parseParentSlot(
			param->baseType,
			param->idxParentSlotLParentheseToken,
			param->idxParentSlotRParentheseToken);
		if (param->baseType)
			param->tokenRange.endIndex = lexer->getTokenIndex(lexer->tokens[param->idxParentSlotRParentheseToken].get());

		parseImplList(
			param->interfaceTypes,
			param->idxImplInterfacesColonToken,
			param->idxImplInterfacesSeparatorTokens);
		if (param->interfaceTypes.size())
			param->tokenRange.endIndex = param->interfaceTypes.back()->tokenRange.endIndex;

		genericParams.push_back(param);

		if (Token *token = lexer->peekToken(); token->tokenId != TokenId::Comma)
			break;

		Token *token = lexer->nextToken();
		param->idxCommaToken = lexer->getTokenIndex(token);
	}

	splitRshOpToken();
	Token *endToken = expectToken(TokenId::GtOp);
	tokenRangeOut.endIndex = lexer->getTokenIndex(endToken);

	// stub
	return genericParams;
}

std::shared_ptr<ClassNode> Parser::parseClassDef() {
	Token *beginToken = expectTokens(lexer->nextToken(), TokenId::ClassKeyword);
	Token *nameToken = expectToken(TokenId::Id);

	std::shared_ptr<ClassNode> classNode = std::make_shared<ClassNode>(
		compiler,
		nameToken->text);
	classNode->tokenRange = TokenRange{
		lexer->getTokenIndex(beginToken),
		lexer->getTokenIndex(nameToken)
	};
	classNode->idxNameToken = lexer->getTokenIndex(nameToken);

	auto savedScope = curScope;
	try {
		curScope = classNode->scope;
		curScope->parent = savedScope.get();

		if (Token *token = lexer->peekToken(); token->tokenId == TokenId::LtOp) {
			TokenRange genericParamsTokenRange;
			classNode->setGenericParams(parseGenericParams(genericParamsTokenRange));
			classNode->tokenRange.endIndex = genericParamsTokenRange.endIndex;
		}

		parseParentSlot(
			classNode->parentClass,
			classNode->idxParentSlotLParentheseToken,
			classNode->idxParentSlotRParentheseToken);
		if (classNode->parentClass)
			classNode->tokenRange.endIndex = lexer->getTokenIndex(lexer->tokens[classNode->idxParentSlotRParentheseToken].get());

		parseImplList(
			classNode->implInterfaces,
			classNode->idxImplInterfacesColonToken,
			classNode->idxImplInterfacesSeparatorTokens);
		if (classNode->implInterfaces.size())
			classNode->tokenRange.endIndex = classNode->implInterfaces.back()->tokenRange.endIndex;

		Token *lBraceToken = expectToken(TokenId::LBrace);
		classNode->tokenRange.endIndex = lexer->getTokenIndex(lBraceToken);
		classNode->idxLBraceToken = lexer->getTokenIndex(lBraceToken);

		while (true) {
			if (Token *token = lexer->peekToken();
				token->tokenId == TokenId::RBrace ||
				token->tokenId == TokenId::End)
				break;
			parseClassStmt();
			classNode->tokenRange.endIndex = lexer->getTokenIndex(lexer->peekToken());
		}

		Token *rBraceToken = expectToken(TokenId::RBrace);
		classNode->tokenRange.endIndex = lexer->getTokenIndex(rBraceToken);
		classNode->idxRBraceToken = lexer->getTokenIndex(rBraceToken);
	} catch (SyntaxError e) {
		compiler->messages.push_back(Message(
			compiler->tokenRangeToSourceLocation(e.tokenRange),
			MessageType::Error,
			e.what()));
	}
	curScope = savedScope;

	return classNode;
}

void Parser::parseClassStmt() {
	TokenRange tokenRange;
	std::deque<size_t> idxAccessModifierTokens;
	AccessModifier accessModifier = parseAccessModifier(tokenRange, idxAccessModifierTokens);

	switch (Token *token = lexer->peekToken(); token->tokenId) {
		case TokenId::End:
		case TokenId::RBrace:
			return;
		case TokenId::ClassKeyword: {
			auto def = parseClassDef();

			def->tokenRange.beginIndex = tokenRange.beginIndex;
			def->access = accessModifier | ACCESS_STATIC;
			def->idxAccessModifierTokens = std::move(idxAccessModifierTokens);

			_putDefinition(
				def->name,
				def);
			break;
		}
		case TokenId::InterfaceKeyword: {
			auto def = parseInterfaceDef();

			def->tokenRange.beginIndex = tokenRange.beginIndex;
			def->access = accessModifier | ACCESS_STATIC;
			def->idxAccessModifierTokens = std::move(idxAccessModifierTokens);

			_putDefinition(
				def->name,
				def);
			break;
		}
		case TokenId::OperatorKeyword: {
			std::string name;
			auto overloading = parseOperatorDef(name);

			overloading->tokenRange.beginIndex = tokenRange.beginIndex;
			overloading->access = accessModifier;
			overloading->idxAccessModifierTokens = std::move(idxAccessModifierTokens);

			_putFnDefinition(name, overloading);
			break;
		}
		case TokenId::FnKeyword: {
			std::string name;
			auto overloading = parseFnDef(name);

			overloading->tokenRange.beginIndex = tokenRange.beginIndex;
			overloading->access = accessModifier;
			overloading->idxAccessModifierTokens = std::move(idxAccessModifierTokens);

			_putFnDefinition(name, overloading);

			break;
		}
		case TokenId::LetKeyword: {
			Token *letToken = lexer->nextToken();

			auto stmt = std::make_shared<VarDefStmtNode>();
			stmt->tokenRange = lexer->getTokenIndex(letToken);

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
					i.second.idxCommaToken,
					true);
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
			throw SyntaxError("Unrecognized token", lexer->getTokenIndex(token));
	}
}

std::shared_ptr<InterfaceNode> Parser::parseInterfaceDef() {
	Token *beginToken = expectTokens(lexer->nextToken(), TokenId::InterfaceKeyword);
	Token *nameToken = expectToken(TokenId::Id);

	std::shared_ptr<InterfaceNode> interfaceNode = std::make_shared<InterfaceNode>(
		nameToken->text);

	interfaceNode->tokenRange = TokenRange{ lexer->getTokenIndex(beginToken), lexer->getTokenIndex(nameToken) };
	interfaceNode->idxNameToken = lexer->getTokenIndex(nameToken);

	auto savedScope = curScope;
	try {
		curScope = interfaceNode->scope;
		curScope->parent = savedScope.get();

		if (Token *token = lexer->peekToken(); token->tokenId == TokenId::LtOp) {
			TokenRange genericParamsTokenRange;
			interfaceNode->setGenericParams(parseGenericParams(genericParamsTokenRange));
			interfaceNode->tokenRange.endIndex = genericParamsTokenRange.endIndex;
		}

		parseImplList(
			interfaceNode->parentInterfaces,
			interfaceNode->idxImplInterfacesColonToken,
			interfaceNode->idxImplInterfacesSeparatorTokens);
		if (interfaceNode->parentInterfaces.size())
			interfaceNode->tokenRange.endIndex = interfaceNode->parentInterfaces.back()->tokenRange.endIndex;

		Token *lBraceToken = expectToken(TokenId::LBrace);
		interfaceNode->tokenRange.endIndex = lexer->getTokenIndex(lBraceToken);
		interfaceNode->idxLBraceToken = lexer->getTokenIndex(lBraceToken);

		while (true) {
			if (Token *token = lexer->peekToken();
				token->tokenId == TokenId::RBrace ||
				token->tokenId == TokenId::End)
				break;
			parseInterfaceStmt();
			interfaceNode->tokenRange.endIndex = lexer->getTokenIndex(lexer->peekToken());
		}

		Token *rBraceToken = expectToken(TokenId::RBrace);
		interfaceNode->tokenRange.endIndex = lexer->getTokenIndex(rBraceToken);
		interfaceNode->idxRBraceToken = lexer->getTokenIndex(rBraceToken);
	} catch (SyntaxError e) {
		compiler->messages.push_back(Message(
			compiler->tokenRangeToSourceLocation(e.tokenRange),
			MessageType::Error,
			e.what()));
	}
	curScope = savedScope;

	return interfaceNode;
}

void Parser::parseInterfaceStmt() {
	TokenRange tokenRange;
	std::deque<size_t> idxAccessModifierTokens;
	AccessModifier accessModifier = parseAccessModifier(tokenRange, idxAccessModifierTokens);

	switch (Token *token = lexer->peekToken(); token->tokenId) {
		case TokenId::End:
		case TokenId::RBrace:
			return;
		case TokenId::ClassKeyword: {
			auto def = parseClassDef();

			def->tokenRange = tokenRange;
			def->access = accessModifier | ACCESS_STATIC;
			def->idxAccessModifierTokens = std::move(idxAccessModifierTokens);

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
		case TokenId::OperatorKeyword: {
			std::string name;

			auto overloading = parseOperatorDef(name);

			overloading->tokenRange = tokenRange;
			overloading->access = accessModifier;
			overloading->idxAccessModifierTokens = std::move(idxAccessModifierTokens);
			overloading->isVirtual = true;

			_putFnDefinition(name, overloading);
			break;
		}
		case TokenId::FnKeyword: {
			std::string name;
			auto overloading = parseFnDef(name);

			overloading->tokenRange = tokenRange;
			overloading->access = accessModifier;
			overloading->isVirtual = true;

			_putFnDefinition(name, overloading);

			break;
		}
		case TokenId::LetKeyword: {
			Token *letToken = lexer->nextToken();

			auto stmt = std::make_shared<VarDefStmtNode>();
			stmt->tokenRange = lexer->getTokenIndex(letToken);

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
					i.second.idxCommaToken,
					true);
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
			throw SyntaxError("Unrecognized token", lexer->getTokenIndex(token));
	}
}
