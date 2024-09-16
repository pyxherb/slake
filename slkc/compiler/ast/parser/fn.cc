#include <slkc/compiler/compiler.h>

using namespace slake;
using namespace slake::slkc;

void Parser::parseParams(
	std::deque<std::shared_ptr<ParamNode>> &paramsOut,
	std::shared_ptr<ParamNode> &varArgParamOut,
	std::deque<size_t> &idxCommaTokensOut) {
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
				varArgParamType->elementType->tokenRange = lexer->getTokenIndex(varArgToken);
				varArgParamType->tokenRange = lexer->getTokenIndex(varArgToken);
				param->type = varArgParamType;
			}

			param->name = "...";
			param->tokenRange = lexer->getTokenIndex(varArgToken);
			param->idxNameToken = lexer->getTokenIndex(varArgToken);

			varArgParamOut = param;
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
		paramNode->tokenRange = lexer->getTokenIndex(nameToken);
		paramNode->name = name;

		if (Token *colonToken = lexer->peekToken(); colonToken->tokenId == TokenId::Colon) {
			lexer->nextToken();
			paramNode->tokenRange.endIndex = lexer->getTokenIndex(colonToken);
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
	overloading->tokenRange = lexer->getTokenIndex(fnKeywordToken);

	Token *nameToken = lexer->peekToken();
	switch (nameToken->tokenId) {
		case TokenId::Id:
		case TokenId::NewKeyword:
		case TokenId::DeleteKeyword:
			lexer->nextToken();
			break;
		default:
			throw SyntaxError("Expecting an identifier", lexer->getTokenIndex(nameToken));
	}
	nameOut = nameToken->text;
	overloading->tokenRange.endIndex = lexer->getTokenIndex(nameToken);
	overloading->idxNameToken = lexer->getTokenIndex(nameToken);

	if (lexer->peekToken()->tokenId == TokenId::LtOp) {
		TokenRange genericParamsTokenRange;
		overloading->setGenericParams(parseGenericParams(genericParamsTokenRange));
		overloading->tokenRange.endIndex = genericParamsTokenRange.endIndex;
	}

	{
		Token *paramLParentheseToken = expectToken(TokenId::LParenthese);
		overloading->tokenRange.endIndex = lexer->getTokenIndex(paramLParentheseToken);
		overloading->idxParamLParentheseToken = lexer->getTokenIndex(paramLParentheseToken);
	}

	parseParams(overloading->params, overloading->varArgParam, overloading->idxParamCommaTokens);
	if (overloading->params.size())
		overloading->tokenRange.endIndex = overloading->params.back()->tokenRange.endIndex;

	{
		Token *paramRParentheseToken = expectToken(TokenId::RParenthese);
		overloading->tokenRange.endIndex = lexer->getTokenIndex(paramRParentheseToken);
		overloading->idxParamRParentheseToken = lexer->getTokenIndex(paramRParentheseToken);
	}

	if (Token *token = lexer->peekToken(); token->tokenId == TokenId::AsyncKeyword) {
		lexer->nextToken();

		overloading->tokenRange.endIndex = lexer->getTokenIndex(token);
		overloading->idxAsyncModifierToken = lexer->getTokenIndex(token);

		overloading->isAsync = true;
	}

	if (Token *token = lexer->peekToken(); token->tokenId == TokenId::VirtualKeyword) {
		lexer->nextToken();

		overloading->tokenRange.endIndex = lexer->getTokenIndex(token);
		overloading->idxVirtualModifierToken = lexer->getTokenIndex(token);

		overloading->isVirtual = true;
	}

	if (Token *token = lexer->peekToken(); token->tokenId == TokenId::Colon) {
		lexer->nextToken();

		overloading->tokenRange.endIndex = lexer->getTokenIndex(token);
		overloading->idxReturnTypeColonToken = lexer->getTokenIndex(token);

		overloading->returnType = parseTypeName();
		overloading->tokenRange.endIndex = overloading->returnType->tokenRange.endIndex;
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

		overloading->tokenRange.endIndex = lexer->getTokenIndex(token);
		lexer->nextToken();

		std::deque<std::shared_ptr<StmtNode>> stmts;

		while (true) {
			if (lexer->peekToken()->tokenId == TokenId::RBrace) {
				break;
			}

			auto newStmt = parseStmt();
			overloading->tokenRange.endIndex = newStmt->tokenRange.endIndex;
			stmts.push_back(newStmt);
		}

		Token *rBraceToken = expectToken(TokenId::RBrace);
		overloading->body = std::make_shared<CodeBlockStmtNode>(
			CodeBlock{ TokenRange{ lexer->getTokenIndex(token), lexer->getTokenIndex(rBraceToken) },
				stmts });

		curScope = savedScope;
	} else {
		Token *semicolonToken = expectToken(TokenId::Semicolon);
		overloading->tokenRange.endIndex = lexer->getTokenIndex(semicolonToken);
	}

	return overloading;
}

std::shared_ptr<FnOverloadingNode> Parser::parseOperatorDecl(std::string &nameOut) {
	auto savedScope = curScope;
	curScope = std::make_shared<Scope>();

	Token *operatorKeywordToken = expectToken(TokenId::OperatorKeyword);
	auto overloading = std::make_shared<FnOverloadingNode>(compiler, curScope);
	overloading->tokenRange = lexer->getTokenIndex(operatorKeywordToken);

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
			throw SyntaxError("Unrecognized operator name", lexer->getTokenIndex(nameToken));
	}

	nameOut = name;

	overloading->tokenRange.endIndex = lexer->getTokenIndex(nameToken);
	overloading->idxNameToken = lexer->getTokenIndex(nameToken);

	{
		Token *paramLParentheseToken = expectToken(TokenId::LParenthese);
		overloading->tokenRange.endIndex = lexer->getTokenIndex(paramLParentheseToken);
		overloading->idxParamLParentheseToken = lexer->getTokenIndex(paramLParentheseToken);
	}

	parseParams(overloading->params, overloading->varArgParam, overloading->idxParamCommaTokens);
	if (overloading->params.size())
		overloading->tokenRange.endIndex = overloading->params.back()->tokenRange.endIndex;

	{
		Token *paramRParentheseToken = expectToken(TokenId::RParenthese);
		overloading->tokenRange.endIndex = lexer->getTokenIndex(paramRParentheseToken);
		overloading->idxParamRParentheseToken = lexer->getTokenIndex(paramRParentheseToken);
	}

	if (Token *colonToken = lexer->peekToken(); colonToken->tokenId == TokenId::Colon) {
		lexer->nextToken();

		overloading->tokenRange.endIndex = lexer->getTokenIndex(colonToken);
		overloading->idxReturnTypeColonToken = lexer->getTokenIndex(colonToken);

		overloading->returnType = parseTypeName();
		overloading->tokenRange.endIndex = overloading->returnType->tokenRange.endIndex;
	}

	curScope = savedScope;

	return overloading;
}

std::shared_ptr<FnOverloadingNode> Parser::parseOperatorDef(std::string &nameOut) {
	std::shared_ptr<FnOverloadingNode> overloading = parseOperatorDecl(nameOut);

	if (Token *token = lexer->peekToken(); token->tokenId == TokenId::LBrace) {
		auto savedScope = curScope;
		curScope = overloading->scope;

		overloading->tokenRange.endIndex = lexer->getTokenIndex(token);
		lexer->nextToken();

		std::deque<std::shared_ptr<StmtNode>> stmts;

		while (true) {
			if (lexer->peekToken()->tokenId == TokenId::RBrace) {
				break;
			}

			auto newStmt = parseStmt();
			overloading->tokenRange.endIndex = newStmt->tokenRange.endIndex;
			stmts.push_back(newStmt);
		}

		Token *rBraceToken = expectToken(TokenId::RBrace);
		overloading->body = std::make_shared<CodeBlockStmtNode>(
			CodeBlock{ TokenRange{ lexer->getTokenIndex(token), lexer->getTokenIndex(rBraceToken) },
				stmts });

		curScope = savedScope;
	} else {
		Token *semicolonToken = expectToken(TokenId::Semicolon);
		overloading->tokenRange.endIndex = lexer->getTokenIndex(semicolonToken);
	}

	return overloading;
}
