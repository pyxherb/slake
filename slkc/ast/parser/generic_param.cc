#include "../parser.h"

using namespace slkc;

SLKC_API std::optional<SyntaxError> Parser::parseGenericParams(
	peff::DynArray<peff::SharedPtr<GenericParamNode>> &genericParamsOut,
	peff::DynArray<size_t> &idxCommaTokensOut,
	size_t &lAngleBracketIndexOut,
	size_t &rAngleBracketIndexOut) {
	std::optional<SyntaxError> syntaxError;

	Token *lAngleBracketToken = peekToken();

	lAngleBracketIndexOut = lAngleBracketToken->index;

	if (lAngleBracketToken->tokenId == TokenId::LtOp) {
		nextToken();
		while (true) {
			peff::SharedPtr<GenericParamNode> genericParamNode;

			if (!(genericParamNode = peff::makeShared<GenericParamNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
				return genOutOfMemoryError();
			}

			Token *nameToken;

			if ((syntaxError = expectToken((nameToken = peekToken()), TokenId::Id))) {
				return syntaxError;
			}

			if (!genericParamNode->name.build(nameToken->sourceText))
				return genOutOfMemoryError();

			nextToken();

			if (Token *lParentheseToken = peekToken(); lParentheseToken->tokenId == TokenId::LParenthese) {
				nextToken();

				if ((syntaxError = parseTypeName(genericParamNode->baseType))) {
					return syntaxError;
				}

				Token *rParentheseToken;
				if ((syntaxError = expectToken((rParentheseToken = peekToken()), TokenId::RParenthese))) {
					return syntaxError;
				}
				nextToken();

				break;
			}

			if (Token *colonToken = peekToken(); colonToken->tokenId == TokenId::Colon) {
				nextToken();

				while (true) {
					peff::SharedPtr<TypeNameNode> tn;

					if ((syntaxError = parseTypeName(tn))) {
						return syntaxError;
					}

					if (!genericParamNode->implementedTypes.pushBack(std::move(tn))) {
						return genOutOfMemoryError();
					}

					if (peekToken()->tokenId != TokenId::OrOp) {
						break;
					}

					nextToken();
				}

				break;
			}

			if (!genericParamsOut.pushBack(std::move(genericParamNode)))
				return genOutOfMemoryError();

			if (peekToken()->tokenId != TokenId::Comma) {
				break;
			}

			Token *commaToken = nextToken();

			if (!idxCommaTokensOut.pushBack(+commaToken->index))
				return genOutOfMemoryError();
		}

		Token *rAngleBracketToken;

		if ((syntaxError = expectToken((rAngleBracketToken = peekToken()), TokenId::GtOp))) {
			return syntaxError;
		}

		nextToken();

		rAngleBracketIndexOut = rAngleBracketToken->index;
	}

	return {};
}
