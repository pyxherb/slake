#include "../parser.h"

using namespace slkc;

SLKC_API peff::Option<SyntaxError> Parser::parseAttribute(AstNodePtr<AttributeNode> &attributeOut) {
	peff::Option<SyntaxError> syntaxError;

	AstNodePtr<AttributeNode> attribute;

	if (!(attribute = makeAstNode<AttributeNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
		return genOutOfMemorySyntaxError();
	}

	attributeOut = attribute;

	SLKC_RETURN_IF_PARSE_ERROR(parseIdRef(attribute->attributeName));

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
				SLKC_RETURN_IF_PARSE_ERROR(expectToken((nameToken = peekToken()), TokenId::Id));
				nextToken();

				Token *assignToken;
				SLKC_RETURN_IF_PARSE_ERROR(expectToken((assignToken = peekToken()), TokenId::AssignOp));
				nextToken();

				if (auto e = parseExpr(0, arg); e)
					return e;

				/*if (!argsOut.pushBack(std::move(arg)))
					return genOutOfMemorySyntaxError();*/

				if (peekToken()->tokenId != TokenId::Comma) {
					break;
				}

				Token *commaToken = nextToken();
				/*if (!idxCommaTokensOut.pushBack(+commaToken->index))
					return genOutOfMemorySyntaxError();*/
			}

			Token *rParentheseToken;

			SLKC_RETURN_IF_PARSE_ERROR(expectToken((rParentheseToken = peekToken()), TokenId::RParenthese));

			nextToken();
		}
	}

	Token *rDBracketToken;
	SLKC_RETURN_IF_PARSE_ERROR(expectToken((rDBracketToken = peekToken()), TokenId::RDBracket));

	nextToken();

	if (Token *forToken = peekToken(); forToken->tokenId == TokenId::ForKeyword) {
		nextToken();

		SLKC_RETURN_IF_PARSE_ERROR(parseTypeName(attributeOut->appliedFor));
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

		SLKC_RETURN_IF_PARSE_ERROR(parseAttribute(attribute));

		if (!attributesOut.pushBack(std::move(attribute)))
			return genOutOfMemorySyntaxError();
	}

	return {};
}
