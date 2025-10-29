#include "../parser.h"

using namespace slkc;

SLKC_API peff::Option<SyntaxError> Parser::parseAttribute(AstNodePtr<AttributeNode> &attributeOut) {
	peff::Option<SyntaxError> syntaxError;

	AstNodePtr<AttributeNode> attribute;

	if (!(attribute = makeAstNode<AttributeNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
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

			while (true) {
				if (peekToken()->tokenId == TokenId::RParenthese) {
					break;
				}

				AstNodePtr<ExprNode> arg;

				Token *nameToken;
				if ((syntaxError = expectToken((nameToken = peekToken()), TokenId::Id)))
					return syntaxError;
				nextToken();

				Token *assignToken;
				if ((syntaxError = expectToken((assignToken = peekToken()), TokenId::AssignOp)))
					return syntaxError;
				nextToken();

				if (auto e = parseExpr(0, arg); e)
					return e;

				/*if (!argsOut.pushBack(std::move(arg)))
					return genOutOfMemoryError();*/

				if (peekToken()->tokenId != TokenId::Comma) {
					break;
				}

				Token *commaToken = nextToken();
				/*if (!idxCommaTokensOut.pushBack(+commaToken->index))
					return genOutOfMemoryError();*/
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

	if (Token *forToken = peekToken(); forToken->tokenId == TokenId::ForKeyword) {
		nextToken();

		if ((syntaxError = parseTypeName(attributeOut->appliedFor)))
			return syntaxError;
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseAttributes(peff::DynArray<AstNodePtr<AttributeNode>> &attributesOut) {
	peff::Option<SyntaxError> syntaxError;
	Token *currentToken;

	for (;;) {
		if ((currentToken = peekToken())->tokenId != TokenId::LDBracket) {
			break;
		}

		nextToken();

		AstNodePtr<AttributeNode> attribute;

		if ((syntaxError = parseAttribute(attribute)))
			return syntaxError;

		if (!attributesOut.pushBack(std::move(attribute)))
			return genOutOfMemoryError();
	}

	return {};
}
