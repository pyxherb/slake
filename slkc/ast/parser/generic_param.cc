#include "../parser.h"

using namespace slkc;

SLKC_API std::optional<SyntaxError> Parser::parseGenericConstraint(GenericConstraintPtr& constraintOut) {
	GenericConstraintPtr constraint(peff::allocAndConstruct<GenericConstraint>(resourceAllocator.get(), alignof(GenericConstraint), resourceAllocator.get()));

	if (!constraint) {
		return genOutOfMemoryError();
	}

	std::optional<SyntaxError> syntaxError;

	if (Token *lParentheseToken = peekToken(); lParentheseToken->tokenId == TokenId::LParenthese) {
		nextToken();

		if ((syntaxError = parseTypeName(constraint->baseType))) {
			return syntaxError;
		}

		Token *rParentheseToken;
		if ((syntaxError = expectToken((rParentheseToken = peekToken()), TokenId::RParenthese))) {
			return syntaxError;
		}
		nextToken();
	}

	if (Token *colonToken = peekToken(); colonToken->tokenId == TokenId::Colon) {
		nextToken();

		while (true) {
			peff::SharedPtr<TypeNameNode> tn;

			if ((syntaxError = parseTypeName(tn))) {
				return syntaxError;
			}

			if (!constraint->implTypes.pushBack(std::move(tn))) {
				return genOutOfMemoryError();
			}

			if (peekToken()->tokenId != TokenId::AddOp) {
				break;
			}

			nextToken();
		}
	}

	constraintOut = std::move(constraint);

	return {};
}

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

			genericParamNode->parent = curParent;

			Token *nameToken;

			if ((syntaxError = expectToken((nameToken = peekToken()), TokenId::Id))) {
				return syntaxError;
			};

			if (!genericParamNode->name.build(nameToken->sourceText))
				return genOutOfMemoryError();

			nextToken();

			{
				peff::ScopeGuard setTokenRangeGuard([this, nameToken, &genericParamNode]() noexcept {
					if (genericParamNode) {
						genericParamNode->tokenRange = TokenRange{ nameToken->index, parseContext.idxPrevToken };
					}
				});

				if ((syntaxError = parseGenericConstraint(genericParamNode->genericConstraint))) {
					return syntaxError;
				}

				if (!genericParamsOut.pushBack(std::move(genericParamNode)))
					return genOutOfMemoryError();

				if (peekToken()->tokenId != TokenId::Comma) {
					break;
				}
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
