#include "../parser.h"

using namespace slkc;

SLKC_API peff::Option<SyntaxError> Parser::parseParams(
	peff::DynArray<AstNodePtr<VarNode>> &paramsOut,
	bool &varArgOut,
	peff::DynArray<size_t> &idxCommaTokensOut,
	size_t &lAngleBracketIndexOut,
	size_t &rAngleBracketIndexOut) {
	peff::Option<SyntaxError> syntaxError;

	Token *lParentheseToken = peekToken();

	lAngleBracketIndexOut = lParentheseToken->index;

	SLKC_RETURN_IF_PARSE_ERROR(expectToken((lParentheseToken = peekToken()), TokenId::LParenthese));

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

		SLKC_RETURN_IF_PARSE_ERROR(expectToken((nameToken = peekToken()), TokenId::Id));

		if (!paramNode->name.build(nameToken->sourceText))
			return genOutOfMemorySyntaxError();

		nextToken();

		if (peekToken()->tokenId == TokenId::Colon) {
			Token *colonToken = nextToken();

			SLKC_RETURN_IF_PARSE_ERROR(parseTypeName(paramNode->type));
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

	SLKC_RETURN_IF_PARSE_ERROR(expectToken((rParentheseToken = peekToken()), TokenId::RParenthese));

	nextToken();

	rAngleBracketIndexOut = rParentheseToken->index;

	return {};
}
