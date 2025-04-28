#include "../parser.h"

using namespace slkc;

SLKC_API std::optional<SyntaxError> Parser::parseParams(
	peff::DynArray<peff::SharedPtr<VarNode>> &paramsOut,
	bool &varArgOut,
	peff::DynArray<size_t> &idxCommaTokensOut,
	size_t &lAngleBracketIndexOut,
	size_t &rAngleBracketIndexOut) {
	std::optional<SyntaxError> syntaxError;

	Token *lParentheseToken = peekToken();

	lAngleBracketIndexOut = lParentheseToken->index;

	if ((syntaxError = expectToken((lParentheseToken = peekToken()), TokenId::LParenthese))) {
		return syntaxError;
	}

	nextToken();

	while (true) {
		if (TokenId nextTokenId = peekToken()->tokenId; (nextTokenId == TokenId::RParenthese) || (nextTokenId == TokenId::VarArg)) {
			break;
		}

		peff::SharedPtr<VarNode> paramNode;

		if (!(paramNode = peff::makeShared<VarNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
			return genOutOfMemoryError();
		}

		Token *nameToken;

		if ((syntaxError = expectToken((nameToken = peekToken()), TokenId::Id))) {
			return syntaxError;
		}

		if (!paramNode->name.build(nameToken->sourceText))
			return genOutOfMemoryError();

		nextToken();

		if (peekToken()->tokenId == TokenId::FinalKeyword) {
			Token *finalToken = nextToken();

			paramNode->isTypeSealed = true;
		}

		if (peekToken()->tokenId == TokenId::Colon) {
			Token *colonToken = nextToken();

			if ((syntaxError = parseTypeName(paramNode->type))) {
				return syntaxError;
			}
		}

		if (!paramsOut.pushBack(std::move(paramNode)))
			return genOutOfMemoryError();

		if (peekToken()->tokenId != TokenId::Comma) {
			break;
		}

		Token *commaToken = nextToken();

		if (!idxCommaTokensOut.pushBack(+commaToken->index))
			return genOutOfMemoryError();
	}

	Token *varArgToken;
	if ((varArgToken = peekToken())->tokenId == TokenId::VarArg) {
		nextToken();
		varArgOut = true;
	}

	Token *rParentheseToken;

	if ((syntaxError = expectToken((rParentheseToken = peekToken()), TokenId::RParenthese))) {
		return syntaxError;
	}

	nextToken();

	rAngleBracketIndexOut = rParentheseToken->index;

	return {};
}
