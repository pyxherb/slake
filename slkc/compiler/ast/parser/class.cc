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

GenericParamNodeList Parser::parseGenericParams(SourceLocation &locationOut) {
	GenericParamNodeList genericParams;

	Token *startingToken = expectToken(TokenId::LtOp);
	locationOut = startingToken->location;

	while (true) {
		Token *nameToken = expectToken(TokenId::Id);

		auto param = std::make_shared<GenericParamNode>(nameToken->text);

		param->sourceLocation = nameToken->location;
		param->idxNameToken = lexer->getTokenIndex(nameToken);

		parseParentSlot(
			param->baseType,
			param->idxParentSlotLParentheseToken,
			param->idxParentSlotRParentheseToken);
		if (param->baseType)
			param->sourceLocation.endPosition = lexer->tokens[param->idxParentSlotRParentheseToken]->location.endPosition;

		parseImplList(
			param->interfaceTypes,
			param->idxImplInterfacesColonToken,
			param->idxImplInterfacesSeparatorTokens);
		if (param->interfaceTypes.size())
			param->sourceLocation.endPosition = param->interfaceTypes.back()->sourceLocation.endPosition;

		genericParams.push_back(param);

		if (Token *token = lexer->peekToken(); token->tokenId != TokenId::Comma)
			break;

		Token *token = lexer->nextToken();
		param->idxCommaToken = lexer->getTokenIndex(token);
	}

	splitRshOpToken();
	Token *endToken = expectToken(TokenId::GtOp);
	locationOut.endPosition = endToken->location.endPosition;

	// stub
	return genericParams;
}


std::shared_ptr<ClassNode> Parser::parseClassDef() {
	Token *beginToken = expectTokens(lexer->nextToken(), TokenId::ClassKeyword);
	Token *nameToken = expectToken(TokenId::Id);

	std::shared_ptr<ClassNode> classNode = std::make_shared<ClassNode>(
		compiler,
		nameToken->text);
	classNode->sourceLocation = SourceLocation{ beginToken->location.beginPosition, nameToken->location.endPosition };
	classNode->idxNameToken = lexer->getTokenIndex(nameToken);

	auto savedScope = curScope;
	try {
		curScope = classNode->scope;
		curScope->parent = savedScope.get();

		if (Token *token = lexer->peekToken(); token->tokenId == TokenId::LtOp) {
			SourceLocation genericParamsLocation;
			classNode->setGenericParams(parseGenericParams(genericParamsLocation));
			classNode->sourceLocation.endPosition = genericParamsLocation.endPosition;
		}

		parseParentSlot(
			classNode->parentClass,
			classNode->idxParentSlotLParentheseToken,
			classNode->idxParentSlotRParentheseToken);
		if (classNode->parentClass)
			classNode->sourceLocation.endPosition = lexer->tokens[classNode->idxParentSlotRParentheseToken]->location.endPosition;

		parseImplList(
			classNode->implInterfaces,
			classNode->idxImplInterfacesColonToken,
			classNode->idxImplInterfacesSeparatorTokens);
		if (classNode->implInterfaces.size())
			classNode->sourceLocation.endPosition = classNode->implInterfaces.back()->sourceLocation.endPosition;

		Token *lBraceToken = expectToken(TokenId::LBrace);
		classNode->sourceLocation.endPosition = lBraceToken->location.endPosition;
		classNode->idxLBraceToken = lexer->getTokenIndex(lBraceToken);

		while (true) {
			if (Token *token = lexer->peekToken();
				token->tokenId == TokenId::RBrace ||
				token->tokenId == TokenId::End)
				break;
			parseClassStmt();
			classNode->sourceLocation.endPosition = lexer->peekToken()->location.beginPosition;
		}

		Token *rBraceToken = expectToken(TokenId::RBrace);
		classNode->sourceLocation.endPosition = rBraceToken->location.endPosition;
		classNode->idxRBraceToken = lexer->getTokenIndex(rBraceToken);
	} catch (SyntaxError e) {
		compiler->messages.push_back(Message(
			e.location,
			MessageType::Error,
			e.what()));
	}
	curScope = savedScope;

	return classNode;
}

void Parser::parseClassStmt() {
	SourceLocation loc;
	std::deque<size_t> idxAccessModifierTokens;
	AccessModifier accessModifier = parseAccessModifier(loc, idxAccessModifierTokens);

	switch (Token *token = lexer->peekToken(); token->tokenId) {
		case TokenId::End:
		case TokenId::RBrace:
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
		case TokenId::OperatorKeyword: {
			std::string name;
			auto overloading = parseOperatorDef(name);

			overloading->sourceLocation.beginPosition = loc.beginPosition;
			overloading->access = accessModifier;
			overloading->idxAccessModifierTokens = std::move(idxAccessModifierTokens);

			_putFnDefinition(name, overloading);
			break;
		}
		case TokenId::FnKeyword: {
			std::string name;
			auto overloading = parseFnDef(name);

			overloading->sourceLocation.beginPosition = loc.beginPosition;
			overloading->access = accessModifier;
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
					i.second.idxCommaToken,
					true);
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

std::shared_ptr<InterfaceNode> Parser::parseInterfaceDef() {
	Token *beginToken = expectTokens(lexer->nextToken(), TokenId::InterfaceKeyword);
	Token *nameToken = expectToken(TokenId::Id);

	std::shared_ptr<InterfaceNode> interfaceNode = std::make_shared<InterfaceNode>(
		nameToken->text);

	interfaceNode->sourceLocation = SourceLocation{ beginToken->location.beginPosition, nameToken->location.endPosition };
	interfaceNode->idxNameToken = lexer->getTokenIndex(nameToken);

	auto savedScope = curScope;
	try {
		curScope = interfaceNode->scope;
		curScope->parent = savedScope.get();

		if (Token *token = lexer->peekToken(); token->tokenId == TokenId::LtOp) {
			SourceLocation genericParamsLocation;
			interfaceNode->setGenericParams(parseGenericParams(genericParamsLocation));
			interfaceNode->sourceLocation.endPosition = genericParamsLocation.endPosition;
		}

		parseImplList(
			interfaceNode->parentInterfaces,
			interfaceNode->idxImplInterfacesColonToken,
			interfaceNode->idxImplInterfacesSeparatorTokens);
		if (interfaceNode->parentInterfaces.size())
			interfaceNode->sourceLocation.endPosition = interfaceNode->parentInterfaces.back()->sourceLocation.endPosition;

		Token *lBraceToken = expectToken(TokenId::LBrace);
		interfaceNode->sourceLocation.endPosition = lBraceToken->location.endPosition;
		interfaceNode->idxLBraceToken = lexer->getTokenIndex(lBraceToken);

		while (true) {
			if (Token *token = lexer->peekToken();
				token->tokenId == TokenId::RBrace ||
				token->tokenId == TokenId::End)
				break;
			parseInterfaceStmt();
			interfaceNode->sourceLocation.endPosition = lexer->peekToken()->location.beginPosition;
		}

		Token *rBraceToken = expectToken(TokenId::RBrace);
		interfaceNode->sourceLocation.endPosition = rBraceToken->location.endPosition;
		interfaceNode->idxRBraceToken = lexer->getTokenIndex(rBraceToken);
	} catch (SyntaxError e) {
		compiler->messages.push_back(Message(
			e.location,
			MessageType::Error,
			e.what()));
	}
	curScope = savedScope;

	return interfaceNode;
}

void Parser::parseInterfaceStmt() {
	SourceLocation loc;
	std::deque<size_t> idxAccessModifierTokens;
	AccessModifier accessModifier = parseAccessModifier(loc, idxAccessModifierTokens);

	switch (Token *token = lexer->peekToken(); token->tokenId) {
		case TokenId::End:
		case TokenId::RBrace:
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
		case TokenId::OperatorKeyword: {
			std::string name;

			auto overloading = parseOperatorDef(name);

			overloading->sourceLocation.beginPosition = loc.beginPosition;
			overloading->access = accessModifier;
			overloading->idxAccessModifierTokens = std::move(idxAccessModifierTokens);
			overloading->isVirtual = true;

			_putFnDefinition(name, overloading);
			break;
		}
		case TokenId::FnKeyword: {
			std::string name;
			auto overloading = parseFnDef(name);

			overloading->sourceLocation.beginPosition = loc.beginPosition;
			overloading->access = accessModifier;
			overloading->isVirtual = true;

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
					i.second.idxCommaToken,
					true);
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
