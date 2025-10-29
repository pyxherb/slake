#include "../parser.h"

using namespace slkc;

SLKC_API peff::Option<SyntaxError> Parser::lookaheadUntil(size_t nTokenIds, const TokenId tokenIds[]) {
	// stub.
	return {};

	Token *token;
	while ((token->tokenId != TokenId::End)) {
		for(size_t i = 0 ; i < nTokenIds; ++i) {
			if(token->tokenId == tokenIds[i]) {
				return {};
			}
		}
		token = nextToken(true, true, true);
	}

	NoMatchingTokensFoundErrorExData exData(resourceAllocator.get());

	for(size_t i = 0 ; i < nTokenIds; ++i) {
		TokenId copiedTokenId = tokenIds[i];
		if(!exData.expectingTokenIds.insert(std::move(copiedTokenId)))
			return genOutOfMemoryError();
	}

	return SyntaxError(token->index, std::move(exData));
}

SLKC_API Token *Parser::nextToken(bool keepNewLine, bool keepWhitespace, bool keepComment) {
	size_t &i = parseContext.idxCurrentToken;

	while (i < tokenList.size()) {
		Token *currentToken = tokenList.at(i).get();
		currentToken->index = i;

		switch (tokenList.at(i)->tokenId) {
			case TokenId::NewLine:
				if (keepNewLine) {
					parseContext.idxPrevToken = parseContext.idxCurrentToken;
					++i;
					return currentToken;
				}
				break;
			case TokenId::Whitespace:
				if (keepWhitespace) {
					parseContext.idxPrevToken = parseContext.idxCurrentToken;
					++i;
					return currentToken;
				}
				break;
			case TokenId::LineComment:
			case TokenId::BlockComment:
			case TokenId::DocumentationComment:
				if (keepComment) {
					parseContext.idxPrevToken = parseContext.idxCurrentToken;
					++i;
					return currentToken;
				}
				break;
			default:
				assert(isValidToken(currentToken->tokenId));
				parseContext.idxPrevToken = parseContext.idxCurrentToken;
				++i;
				return currentToken;
		}

		++i;
	}

	return tokenList.back().get();
}

SLKC_API Token *Parser::peekToken(bool keepNewLine, bool keepWhitespace, bool keepComment) {
	size_t i = parseContext.idxCurrentToken;

	while (i < tokenList.size()) {
		Token *currentToken = tokenList.at(i).get();
		currentToken->index = i;

		switch (currentToken->tokenId) {
			case TokenId::NewLine:
				if (keepNewLine)
					return currentToken;
				break;
			case TokenId::Whitespace:
				if (keepWhitespace)
					return currentToken;
				break;
			case TokenId::LineComment:
			case TokenId::BlockComment:
			case TokenId::DocumentationComment:
				if (keepComment)
					return currentToken;
				break;
			default:
				assert(isValidToken(currentToken->tokenId));
				return currentToken;
		}

		++i;
	}

	return tokenList.back().get();
}

SLKC_API peff::Option<SyntaxError> Parser::splitRshOpToken() {
	switch (Token *token = peekToken(); token->tokenId) {
		case TokenId::RshOp: {
			token->tokenId = TokenId::GtOp;
			token->sourceText = token->sourceText.substr(0, 1);
			token->sourceLocation.endPosition.column -= 1;

			OwnedTokenPtr extraClosingToken;
			if (!(extraClosingToken = OwnedTokenPtr(peff::allocAndConstruct<Token>(token->allocator.get(), ASTNODE_ALIGNMENT, token->allocator.get(), peff::WeakPtr<Document>(document))))) {
				return genOutOfMemoryError();
			}

			extraClosingToken->tokenId = TokenId::GtOp;
			extraClosingToken->sourceLocation =
				SourceLocation{
					SourcePosition{ token->sourceLocation.beginPosition.line, token->sourceLocation.beginPosition.column + 1 },
					token->sourceLocation.endPosition
				};
			extraClosingToken->sourceText = token->sourceText.substr(1);

			if(!tokenList.insert(parseContext.idxCurrentToken + 1, std::move(extraClosingToken))) {
				return genOutOfMemoryError();
			}

			break;
		}
		default:;
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::splitRDBracketsToken() {
	switch (Token *token = peekToken(); token->tokenId) {
		case TokenId::RDBracket: {
			token->tokenId = TokenId::RBracket;
			token->sourceText = token->sourceText.substr(0, 1);
			token->sourceLocation.endPosition.column -= 1;

			OwnedTokenPtr extraClosingToken;
			if (!(extraClosingToken = OwnedTokenPtr(peff::allocAndConstruct<Token>(token->allocator.get(), ASTNODE_ALIGNMENT, token->allocator.get(), peff::WeakPtr<Document>(document))))) {
				return genOutOfMemoryError();
			}

			extraClosingToken->tokenId = TokenId::RBracket;
			extraClosingToken->sourceLocation =
				SourceLocation{
					SourcePosition{ token->sourceLocation.beginPosition.line, token->sourceLocation.beginPosition.column + 1 },
					token->sourceLocation.endPosition
				};
			extraClosingToken->sourceText = token->sourceText.substr(1);

			if (!tokenList.insert(parseContext.idxCurrentToken + 1, std::move(extraClosingToken))) {
				return genOutOfMemoryError();
			}

			break;
		}
		default:;
	}

	return {};
}
