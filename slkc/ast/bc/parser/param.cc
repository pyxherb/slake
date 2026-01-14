#include "../parser.h"

using namespace slkc;
using namespace slkc::bc;

SLKC_API peff::Option<SyntaxError> BCParser::parseParams(
	peff::DynArray<AstNodePtr<VarNode>> &paramsOut,
	bool &varArgOut,
	peff::DynArray<size_t> &idxCommaTokensOut,
	size_t &lAngleBracketIndexOut,
	size_t &rAngleBracketIndexOut) {
	peff::Option<SyntaxError> syntaxError;

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

		AstNodePtr<VarNode> paramNode;

		if (!(paramNode = makeAstNode<VarNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
			return genOutOfMemorySyntaxError();
		}

		Token *nameToken;

		if ((syntaxError = expectToken((nameToken = peekToken()), TokenId::Id))) {
			return syntaxError;
		}

		if (!paramNode->name.build(nameToken->sourceText))
			return genOutOfMemorySyntaxError();

		nextToken();

		if (peekToken()->tokenId == TokenId::Colon) {
			Token *colonToken = nextToken();

			if ((syntaxError = parseTypeName(paramNode->type))) {
				return syntaxError;
			}
		}

		if (!paramsOut.pushBack(std::move(paramNode)))
			return genOutOfMemorySyntaxError();

		if (peekToken()->tokenId != TokenId::Comma) {
			break;
		}

		Token *commaToken = nextToken();

		if (!idxCommaTokensOut.pushBack(+commaToken->index))
			return genOutOfMemorySyntaxError();
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
