#include <slkc/compiler/compiler.h>

using namespace slake;
using namespace slake::slkc;

std::deque<std::shared_ptr<TypeNameNode>> Parser::parseGenericArgs(bool forTypeName) {
	LexerContext savedContext = lexer->context;
	std::deque<std::shared_ptr<TypeNameNode>> genericArgs;

	if (Token *token = lexer->nextToken(); token->tokenId != TokenId::LtOp)
		goto fail;

	while (true) {
		if (Token *token = lexer->peekToken();
			(token->tokenId == TokenId::GtOp) ||
			(token->tokenId == TokenId::RshOp))
			break;

		if (auto type = parseTypeName(forTypeName); type->getTypeId() != TypeId::Bad)
			genericArgs.push_back(type);
		else {
			if (forTypeName) {
				compiler->messages.push_back(
					Message(
						type->sourceLocation,
						MessageType::Error,
						"Expecting a type name"));
				genericArgs.push_back(type);
				return genericArgs;
			} else
				goto fail;
		}

		if (Token *token = lexer->peekToken(); token->tokenId != TokenId::Comma)
			break;

		lexer->nextToken();
	}

	splitRshOpToken();

	if (Token *token = lexer->peekToken(); token->tokenId != TokenId::GtOp) {
		if (forTypeName) {
			compiler->messages.push_back(
				Message(
					token->location,
					MessageType::Error,
					"Expecting a type name"));
			return genericArgs;
		} else
			goto fail;
	}
	lexer->nextToken();

	return genericArgs;

fail:
	lexer->context = savedContext;
	return {};
}

IdRef Parser::parseModuleRef() {
	IdRef ref;
	size_t idxPrecedingAccessOp = SIZE_MAX;

	while (true) {
		Token *nameToken = lexer->peekToken();

		auto refEntry = IdRefEntry(nameToken->location, SIZE_MAX, "");
		refEntry.idxAccessOpToken = idxPrecedingAccessOp;

		if (nameToken->tokenId != TokenId::Id) {
			// Return the bad reference.
			compiler->messages.push_back(Message(
				nameToken->location,
				MessageType::Error,
				"Expecting an identifier"));
			ref.push_back(refEntry);
			return ref;
		} else {
			// Push current reference scope.
			lexer->nextToken();
			refEntry.name = nameToken->text;
			refEntry.idxToken = lexer->getTokenIndex(nameToken);
			ref.push_back(refEntry);
		}

		if (Token *token = lexer->peekToken(); token->tokenId != TokenId::Dot)
			break;

		Token *precedingAccessOpToken = lexer->nextToken();
		idxPrecedingAccessOp = lexer->getTokenIndex(precedingAccessOpToken);
	}

	return ref;
}

IdRef Parser::parseRef(bool forTypeName) {
	IdRef ref;
	size_t idxPrecedingAccessOp = SIZE_MAX;

	switch (Token *token = lexer->peekToken(); token->tokenId) {
		case TokenId::ThisKeyword:
		case TokenId::BaseKeyword:
		case TokenId::ScopeOp: {
			auto refEntry = IdRefEntry(token->location, lexer->getTokenIndex(token), "", {});

			switch (token->tokenId) {
				case TokenId::ThisKeyword:
					refEntry.name = "this";
					ref.push_back(refEntry);
					break;
				case TokenId::BaseKeyword:
					refEntry.name = "base";
					ref.push_back(refEntry);
					break;
				case TokenId::ScopeOp:
					refEntry.name = "";
					ref.push_back(refEntry);
					break;
			}

			lexer->nextToken();
			if (lexer->peekToken()->tokenId != TokenId::Dot)
				goto end;

			Token *precedingAccessOpToken = lexer->nextToken();
			idxPrecedingAccessOp = lexer->getTokenIndex(precedingAccessOpToken);

			break;
		}
	}

	while (true) {
		Token *nameToken = lexer->peekToken();

		auto refEntry = IdRefEntry(nameToken->location, SIZE_MAX, "");
		refEntry.idxAccessOpToken = idxPrecedingAccessOp;

		if (nameToken->tokenId != TokenId::Id) {
			// Return the bad reference.
			compiler->messages.push_back(Message(
				nameToken->location,
				MessageType::Error,
				"Expecting an identifier"));
			ref.push_back(refEntry);
			return ref;
		} else {
			// Push current reference scope.
			lexer->nextToken();
			refEntry.name = nameToken->text;
			refEntry.idxToken = lexer->getTokenIndex(nameToken);
			ref.push_back(refEntry);
		}

		if (lexer->peekToken()->tokenId == TokenId::LtOp)
			ref.back().genericArgs = parseGenericArgs(forTypeName);

		if (Token *token = lexer->peekToken(); token->tokenId != TokenId::Dot)
			break;

		Token *precedingAccessOpToken = lexer->nextToken();
		idxPrecedingAccessOp = lexer->getTokenIndex(precedingAccessOpToken);
	}

end:
	return ref;
}
