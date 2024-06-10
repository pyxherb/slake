#include <slkc/compiler/compiler.h>

using namespace slake;
using namespace slake::slkc;

void Parser::parseParams(std::deque<std::shared_ptr<ParamNode>> &paramsOut, std::deque<size_t> &idxCommaTokensOut) {
	if (lexer->peekToken()->tokenId == TokenId::RParenthese)
		return;

	while (true) {
		if (Token *varArgToken = lexer->peekToken(); varArgToken->tokenId == TokenId::VarArg) {
			auto param = std::make_shared<ParamNode>();

			{
				auto varArgParamType =
					std::make_shared<ArrayTypeNameNode>(
						std::make_shared<AnyTypeNameNode>(
							lexer->getTokenIndex(varArgToken)));
				varArgParamType->elementType->sourceLocation = varArgToken->location;
				varArgParamType->sourceLocation = varArgToken->location;
				param->type = varArgParamType;
			}

			param->name = "...";
			param->sourceLocation = varArgToken->location;
			param->idxNameToken = lexer->getTokenIndex(varArgToken);

			paramsOut.push_back(param);
			lexer->nextToken();
			break;
		}

		Token *nameToken = lexer->peekToken();
		std::string name;
		if (nameToken->tokenId != TokenId::Id) {
			compiler->messages.push_back(
				Message(
					nameToken->location,
					MessageType::Error,
					"Expecting an identifier"));
		} else {
			name = nameToken->text;
			lexer->nextToken();
		}

		auto paramNode = std::make_shared<ParamNode>();
		paramNode->sourceLocation = nameToken->location;
		paramNode->name = name;

		if (Token *colonToken = lexer->peekToken(); colonToken->tokenId == TokenId::Colon) {
			lexer->nextToken();
			paramNode->sourceLocation.endPosition = colonToken->location.endPosition;
			paramNode->idxColonToken = lexer->getTokenIndex(colonToken);
			paramNode->type = parseTypeName(true);
		}

		paramsOut.push_back(paramNode);

		if (lexer->peekToken()->tokenId != TokenId::Comma)
			break;

		Token *commaToken = lexer->nextToken();
		idxCommaTokensOut.push_back(lexer->getTokenIndex(commaToken));
	}
}

std::shared_ptr<FnOverloadingNode> Parser::parseFnDecl(std::string &nameOut) {
	auto savedScope = curScope;
	curScope = std::make_shared<Scope>();

	Token *fnKeywordToken = expectToken(TokenId::FnKeyword);
	auto overloading = std::make_shared<FnOverloadingNode>(compiler, curScope);
	overloading->sourceLocation = fnKeywordToken->location;

	Token *nameToken = lexer->peekToken();
	switch (nameToken->tokenId) {
		case TokenId::Id:
		case TokenId::NewKeyword:
		case TokenId::DeleteKeyword:
			lexer->nextToken();
			break;
		default:
			throw SyntaxError("Expecting an identifier", nameToken->location);
	}
	nameOut = nameToken->text;
	overloading->sourceLocation.endPosition = nameToken->location.endPosition;
	overloading->idxNameToken = lexer->getTokenIndex(nameToken);

	if (lexer->peekToken()->tokenId == TokenId::LtOp) {
		SourceLocation genericParamsLocation;
		overloading->setGenericParams(parseGenericParams(genericParamsLocation));
		overloading->sourceLocation.endPosition = genericParamsLocation.endPosition;
	}

	{
		Token *paramLParentheseToken = expectToken(TokenId::LParenthese);
		overloading->sourceLocation.endPosition = paramLParentheseToken->location.endPosition;
		overloading->idxParamLParentheseToken = lexer->getTokenIndex(paramLParentheseToken);
	}

	parseParams(overloading->params, overloading->idxParamCommaTokens);
	if (overloading->params.size())
		overloading->sourceLocation.endPosition = overloading->params.back()->sourceLocation.endPosition;

	{
		Token *paramRParentheseToken = expectToken(TokenId::RParenthese);
		overloading->sourceLocation.endPosition = paramRParentheseToken->location.endPosition;
		overloading->idxParamRParentheseToken = lexer->getTokenIndex(paramRParentheseToken);
	}

	if (Token *token = lexer->peekToken(); token->tokenId == TokenId::AsyncKeyword) {
		lexer->nextToken();

		overloading->sourceLocation.endPosition = token->location.endPosition;
		overloading->idxAsyncModifierToken = lexer->getTokenIndex(token);

		overloading->isAsync = true;
	}

	if (Token *token = lexer->peekToken(); token->tokenId == TokenId::VirtualKeyword) {
		lexer->nextToken();

		overloading->sourceLocation.endPosition = token->location.endPosition;
		overloading->idxVirtualModifierToken = lexer->getTokenIndex(token);

		overloading->isVirtual = true;
	}

	if (Token *token = lexer->peekToken(); token->tokenId == TokenId::Colon) {
		lexer->nextToken();

		overloading->sourceLocation.endPosition = token->location.endPosition;
		overloading->idxReturnTypeColonToken = lexer->getTokenIndex(token);

		overloading->returnType = parseTypeName();
		overloading->sourceLocation.endPosition = overloading->returnType->sourceLocation.endPosition;
	}

	curScope->parent = savedScope.get();
	curScope = savedScope;

	return overloading;
}

std::shared_ptr<FnOverloadingNode> Parser::parseFnDef(std::string &nameOut) {
	std::shared_ptr<FnOverloadingNode> overloading = parseFnDecl(nameOut);

	if (Token *token = lexer->peekToken(); token->tokenId == TokenId::LBrace) {
		auto savedScope = curScope;
		curScope = overloading->scope;

		overloading->sourceLocation.endPosition = token->location.endPosition;
		lexer->nextToken();

		std::deque<std::shared_ptr<StmtNode>> stmts;

		while (true) {
			if (lexer->peekToken()->tokenId == TokenId::RBrace) {
				break;
			}

			auto newStmt = parseStmt();
			overloading->sourceLocation.endPosition = newStmt->sourceLocation.endPosition;
			stmts.push_back(newStmt);
		}

		Token *rBraceToken = expectToken(TokenId::RBrace);
		overloading->body = std::make_shared<CodeBlockStmtNode>(
			CodeBlock{ SourceLocation{ token->location.beginPosition, rBraceToken->location.endPosition },
				stmts });

		curScope = savedScope;
	} else {
		Token *semicolonToken = expectToken(TokenId::Semicolon);
		overloading->sourceLocation.endPosition = semicolonToken->location.endPosition;
	}

	return overloading;
}

std::shared_ptr<FnOverloadingNode> Parser::parseOperatorDecl(std::string &nameOut) {
	auto savedScope = curScope;
	curScope = std::make_shared<Scope>();

	Token *operatorKeywordToken = expectToken(TokenId::OperatorKeyword);
	auto overloading = std::make_shared<FnOverloadingNode>(compiler, curScope);
	overloading->sourceLocation = operatorKeywordToken->location;

	Token *nameToken = lexer->nextToken();
	std::string name;
	switch (nameToken->tokenId) {
		case TokenId::AddOp:
		case TokenId::SubOp:
		case TokenId::MulOp:
		case TokenId::DivOp:
		case TokenId::ModOp:
		case TokenId::AndOp:
		case TokenId::OrOp:
		case TokenId::XorOp:
		case TokenId::LAndOp:
		case TokenId::LOrOp:
		case TokenId::NotOp:
		case TokenId::LNotOp:
		case TokenId::AddAssignOp:
		case TokenId::SubAssignOp:
		case TokenId::MulAssignOp:
		case TokenId::DivAssignOp:
		case TokenId::ModAssignOp:
		case TokenId::AndAssignOp:
		case TokenId::OrAssignOp:
		case TokenId::XorAssignOp:
		case TokenId::EqOp:
		case TokenId::NeqOp:
		case TokenId::GtOp:
		case TokenId::LtOp:
		case TokenId::GtEqOp:
		case TokenId::LtEqOp:
		case TokenId::NewKeyword:
		case TokenId::DeleteKeyword:
			name = "operator" + nameToken->text;
			break;
		case TokenId::LBracket:
			name = "operator[]";
			expectToken(TokenId::RBracket);
			break;
		case TokenId::LParenthese:
			name = "operator()";
			expectToken(TokenId::RParenthese);
			break;
		default:
			throw SyntaxError("Unrecognized operator name", nameToken->location);
	}

	nameOut = name;

	overloading->sourceLocation.endPosition = nameToken->location.endPosition;
	overloading->idxNameToken = lexer->getTokenIndex(nameToken);

	{
		Token *paramLParentheseToken = expectToken(TokenId::LParenthese);
		overloading->sourceLocation.endPosition = paramLParentheseToken->location.endPosition;
		overloading->idxParamLParentheseToken = lexer->getTokenIndex(paramLParentheseToken);
	}

	parseParams(overloading->params, overloading->idxParamCommaTokens);
	if (overloading->params.size())
		overloading->sourceLocation.endPosition = overloading->params.back()->sourceLocation.endPosition;

	{
		Token *paramRParentheseToken = expectToken(TokenId::RParenthese);
		overloading->sourceLocation.endPosition = paramRParentheseToken->location.endPosition;
		overloading->idxParamRParentheseToken = lexer->getTokenIndex(paramRParentheseToken);
	}

	if (Token *colonToken = lexer->peekToken(); colonToken->tokenId == TokenId::Colon) {
		lexer->nextToken();

		overloading->sourceLocation.endPosition = colonToken->location.endPosition;
		overloading->idxReturnTypeColonToken = lexer->getTokenIndex(colonToken);

		overloading->returnType = parseTypeName();
		overloading->sourceLocation.endPosition = overloading->returnType->sourceLocation.endPosition;
	}

	curScope = savedScope;

	return overloading;
}

std::shared_ptr<FnOverloadingNode> Parser::parseOperatorDef(std::string &nameOut) {
	std::shared_ptr<FnOverloadingNode> overloading = parseOperatorDecl(nameOut);

	if (Token *token = lexer->peekToken(); token->tokenId == TokenId::LBrace) {
		auto savedScope = curScope;
		curScope = overloading->scope;

		overloading->sourceLocation.endPosition = token->location.endPosition;
		lexer->nextToken();

		std::deque<std::shared_ptr<StmtNode>> stmts;

		while (true) {
			if (lexer->peekToken()->tokenId == TokenId::RBrace) {
				break;
			}

			auto newStmt = parseStmt();
			overloading->sourceLocation.endPosition = newStmt->sourceLocation.endPosition;
			stmts.push_back(newStmt);
		}

		Token *rBraceToken = expectToken(TokenId::RBrace);
		overloading->body = std::make_shared<CodeBlockStmtNode>(
			CodeBlock{ SourceLocation{ token->location.beginPosition, rBraceToken->location.endPosition },
				stmts });

		curScope = savedScope;
	} else {
		Token *semicolonToken = expectToken(TokenId::Semicolon);
		overloading->sourceLocation.endPosition = semicolonToken->location.endPosition;
	}

	return overloading;
}
