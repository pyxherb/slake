#include "../parser.h"

using namespace slkc;

SLKC_API std::optional<SyntaxError> Parser::parseAttribute(peff::SharedPtr<AttributeNode> &attributeOut) {
	std::optional<SyntaxError> syntaxError;

	peff::SharedPtr<AttributeNode> attribute;

	if (!(attribute = peff::makeShared<AttributeNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
		return genOutOfMemoryError();
	}

	attributeOut = attribute;

	if ((syntaxError = parseIdRef(attribute->attributeName))) {
		return syntaxError;
	}

	{
		Token *lParentheseToken;

		if ((lParentheseToken = peekToken())->tokenId == TokenId::LParenthese) {
			nextToken();

			if ((syntaxError = parseArgs(attribute->fieldData, attribute->idxCommaTokens))) {
				return syntaxError;
			}

			Token *rParentheseToken;

			if ((syntaxError = expectToken((rParentheseToken = peekToken()), TokenId::RParenthese)))
				return syntaxError;

			nextToken();
		}
	}

	Token *rDBracketToken;
	if ((syntaxError = expectToken((rDBracketToken = peekToken()), TokenId::RDBracket)))
		return syntaxError;

	nextToken();

	if (Token *ifToken = peekToken(); ifToken->tokenId == TokenId::IfKeyword) {
		nextToken();

		peff::SharedPtr<ExprNode> condition;

		if ((syntaxError = parseExpr(0, condition)))
			return syntaxError;
	}

	return {};
}

SLKC_API std::optional<SyntaxError> Parser::parseAttributes(peff::DynArray<peff::SharedPtr<AttributeNode>> &attributesOut) {
	std::optional<SyntaxError> syntaxError;
	Token *currentToken;

	for (;;) {
		if ((currentToken = peekToken())->tokenId != TokenId::LDBracket) {
			break;
		}

		nextToken();

		peff::SharedPtr<AttributeNode> attribute;

		if ((syntaxError = parseAttribute(attribute)))
			return syntaxError;

		if (!attributesOut.pushBack(std::move(attribute)))
			return genOutOfMemoryError();
	}

	return {};
}
