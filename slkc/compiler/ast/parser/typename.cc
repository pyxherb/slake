#include <slkc/compiler/compiler.h>

using namespace slake;
using namespace slake::slkc;

std::shared_ptr<TypeNameNode> Parser::parseTypeName(bool required) {
	std::shared_ptr<TypeNameNode> type;

	switch (Token *token = lexer->peekToken(); token->tokenId) {
		case TokenId::I8TypeName:
		case TokenId::I16TypeName:
		case TokenId::I32TypeName:
		case TokenId::I64TypeName:
		case TokenId::U8TypeName:
		case TokenId::U16TypeName:
		case TokenId::U32TypeName:
		case TokenId::U64TypeName:
		case TokenId::F32TypeName:
		case TokenId::F64TypeName:
		case TokenId::StringTypeName:
		case TokenId::BoolTypeName:
		case TokenId::AutoTypeName:
		case TokenId::VoidTypeName:
		case TokenId::AnyTypeName: {
			switch (token->tokenId) {
				case TokenId::I8TypeName:
					type = std::make_shared<I8TypeNameNode>(lexer->getTokenIndex(token));
					type->sourceLocation = token->location;
					break;
				case TokenId::I16TypeName:
					type = std::make_shared<I16TypeNameNode>(lexer->getTokenIndex(token));
					type->sourceLocation = token->location;
					break;
				case TokenId::I32TypeName:
					type = std::make_shared<I32TypeNameNode>(lexer->getTokenIndex(token));
					type->sourceLocation = token->location;
					break;
				case TokenId::I64TypeName:
					type = std::make_shared<I64TypeNameNode>(lexer->getTokenIndex(token));
					type->sourceLocation = token->location;
					break;
				case TokenId::U8TypeName:
					type = std::make_shared<U8TypeNameNode>(lexer->getTokenIndex(token));
					type->sourceLocation = token->location;
					break;
				case TokenId::U16TypeName:
					type = std::make_shared<U16TypeNameNode>(lexer->getTokenIndex(token));
					type->sourceLocation = token->location;
					break;
				case TokenId::U32TypeName:
					type = std::make_shared<U32TypeNameNode>(lexer->getTokenIndex(token));
					type->sourceLocation = token->location;
					break;
				case TokenId::U64TypeName:
					type = std::make_shared<U64TypeNameNode>(lexer->getTokenIndex(token));
					type->sourceLocation = token->location;
					break;
				case TokenId::F32TypeName:
					type = std::make_shared<F32TypeNameNode>(lexer->getTokenIndex(token));
					type->sourceLocation = token->location;
					break;
				case TokenId::F64TypeName:
					type = std::make_shared<F64TypeNameNode>(lexer->getTokenIndex(token));
					type->sourceLocation = token->location;
					break;
				case TokenId::StringTypeName:
					type = std::make_shared<StringTypeNameNode>(lexer->getTokenIndex(token));
					type->sourceLocation = token->location;
					break;
				case TokenId::BoolTypeName:
					type = std::make_shared<BoolTypeNameNode>(lexer->getTokenIndex(token));
					type->sourceLocation = token->location;
					break;
				case TokenId::AutoTypeName:
					type = std::make_shared<AutoTypeNameNode>(lexer->getTokenIndex(token));
					type->sourceLocation = token->location;
					break;
				case TokenId::VoidTypeName:
					type = std::make_shared<VoidTypeNameNode>(lexer->getTokenIndex(token));
					type->sourceLocation = token->location;
					break;
				case TokenId::AnyTypeName:
					type = std::make_shared<AnyTypeNameNode>(lexer->getTokenIndex(token));
					type->sourceLocation = token->location;
					break;
			}
			lexer->nextToken();

			break;
		}
		case TokenId::Id: {
			LexerContext savedContext = lexer->context;
			auto ref = parseRef(true);
			if (!isCompleteIdRef(ref)) {
				lexer->context = savedContext;
				type = std::make_shared<BadTypeNameNode>(
					ref[0].idxToken,
					lexer->context.curIndex);
				type->sourceLocation = SourceLocation{ ref[0].loc.beginPosition, ref.back().loc.endPosition };
				return type;
			}
			type = std::make_shared<CustomTypeNameNode>(ref, compiler, curScope.get());
			type->sourceLocation = SourceLocation{ ref[0].loc.beginPosition, ref.back().loc.endPosition };
			break;
		}
		default:
			if (required) {
				compiler->messages.push_back(
					Message(
						token->location,
						MessageType::Error,
						"Expecting a type name"));
			}
			type = std::make_shared<BadTypeNameNode>(
				lexer->getTokenIndex(token),
				lexer->getTokenIndex(token));
			type->sourceLocation = token->location;
			return type;
	}

	Token *lBracketToken;
	while ((lBracketToken = lexer->peekToken())->tokenId == TokenId::LBracket) {
		lexer->nextToken();

		if (type->getTypeId() == TypeId::Array) {
			auto t = std::static_pointer_cast<ArrayTypeNameNode>(type);
			++t->nDimensions;

			Token *rBracketToken = lexer->peekToken();
			if (rBracketToken->tokenId != TokenId::RBracket) {
				compiler->messages.push_back(
					Message(
						rBracketToken->location,
						MessageType::Error,
						"Expecting ]"));
				return t;
			}
			lexer->nextToken();

			t->sourceLocation.endPosition = rBracketToken->location.endPosition;
			t->idxRBracketToken = lexer->getTokenIndex(rBracketToken);
		} else {
			auto t = std::make_shared<ArrayTypeNameNode>(type);
			t->idxLBracketToken = lexer->getTokenIndex(lBracketToken);
			t->sourceLocation = type->sourceLocation;

			Token *rBracketToken = lexer->peekToken();
			if (rBracketToken->tokenId != TokenId::RBracket) {
				compiler->messages.push_back(
					Message(
						rBracketToken->location,
						MessageType::Error,
						"Expecting ]"));
				return t;
			}
			lexer->nextToken();

			t->sourceLocation.endPosition = rBracketToken->location.endPosition;
			t->idxRBracketToken = lexer->getTokenIndex(rBracketToken);
			type = t;
		}
	}

	if (Token *token = lexer->peekToken(); token->tokenId == TokenId::AndOp) {
		lexer->nextToken();

		type->isRef = true;
		type->idxRefIndicatorToken = lexer->getTokenIndex(token);
	}

	if (Token *token = lexer->peekToken(); token->tokenId == TokenId::MulOp) {
		lexer->nextToken();

		auto result = std::make_shared<ContextTypeNameNode>(type);
		result->idxIndicatorToken = lexer->getTokenIndex(token);
		type = result;
	}

	return type;
}
