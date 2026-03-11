#include "../parser.h"

using namespace slkc;

SLKC_API peff::Option<SyntaxError> Parser::parseGenericConstraint(GenericConstraintPtr &constraintOut) {
	GenericConstraintPtr constraint(peff::allocAndConstruct<GenericConstraint>(resourceAllocator.get(), alignof(GenericConstraint), resourceAllocator.get()));

	if (!constraint) {
		return genOutOfMemorySyntaxError();
	}

	peff::Option<SyntaxError> syntaxError;

	if (Token *lParentheseToken = peekToken(); lParentheseToken->tokenId == TokenId::LParenthese) {
		nextToken();

		SLKC_RETURN_IF_PARSE_ERROR(parseTypeName(constraint->baseType));

		Token *rParentheseToken;
		SLKC_RETURN_IF_PARSE_ERROR(expectToken((rParentheseToken = peekToken()), TokenId::RParenthese));
		nextToken();
	}

	if (Token *colonToken = peekToken(); colonToken->tokenId == TokenId::Colon) {
		nextToken();

		while (true) {
			AstNodePtr<TypeNameNode> tn;

			SLKC_RETURN_IF_PARSE_ERROR(parseTypeName(tn));

			if (!constraint->implTypes.pushBack(std::move(tn))) {
				return genOutOfMemorySyntaxError();
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

SLKC_API peff::Option<SyntaxError> Parser::parseParamTypeListGenericConstraint(ParamTypeListGenericConstraintPtr &constraintOut) {
	ParamTypeListGenericConstraintPtr constraint(peff::allocAndConstruct<ParamTypeListGenericConstraint>(resourceAllocator.get(), alignof(ParamTypeListGenericConstraint), resourceAllocator.get()));

	if (!constraint) {
		return genOutOfMemorySyntaxError();
	}

	peff::Option<SyntaxError> syntaxError;

	if (Token *lParentheseToken = peekToken(); lParentheseToken->tokenId == TokenId::LParenthese) {
		nextToken();

		while (true) {
			AstNodePtr<TypeNameNode> tn;

			if (peekToken()->tokenId == TokenId::VarArg) {
				nextToken();

				constraint->hasVarArg = true;

				break;
			}

			SLKC_RETURN_IF_PARSE_ERROR(parseTypeName(tn));

			if (!constraint->argTypes.pushBack(std::move(tn))) {
				return genOutOfMemorySyntaxError();
			}

			if (peekToken()->tokenId != TokenId::Comma) {
				break;
			}

			nextToken();
		}

		Token *rParentheseToken;
		SLKC_RETURN_IF_PARSE_ERROR(expectToken((rParentheseToken = peekToken()), TokenId::RParenthese));
		nextToken();
	}

	constraintOut = std::move(constraint);

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseGenericParams(
	peff::DynArray<AstNodePtr<GenericParamNode>> &genericParamsOut,
	peff::DynArray<size_t> &idxCommaTokensOut,
	size_t &lAngleBracketIndexOut,
	size_t &rAngleBracketIndexOut) {
	peff::Option<SyntaxError> syntaxError;

	Token *lAngleBracketToken = peekToken();

	lAngleBracketIndexOut = lAngleBracketToken->index;

	if (lAngleBracketToken->tokenId == TokenId::LtOp) {
		nextToken();
		while (true) {
			AstNodePtr<GenericParamNode> genericParamNode;

			if (!(genericParamNode = makeAstNode<GenericParamNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
				return genOutOfMemorySyntaxError();
			}

			genericParamNode->parent = curParent.get();

			if (!genericParamsOut.pushBack(AstNodePtr<GenericParamNode>(genericParamNode)))
				return genOutOfMemorySyntaxError();

			if (Token *vargToken = peekToken(); vargToken->tokenId == TokenId::VarArg) {
				nextToken();

				genericParamNode->isParamTypeList = true;

				peff::ScopeGuard setTokenRangeGuard([this, vargToken, &genericParamNode]() noexcept {
					if (genericParamNode) {
						genericParamNode->tokenRange = TokenRange{ document->mainModule, vargToken->index, parseContext.idxPrevToken };
					}
				});

				Token *nameToken;

				SLKC_RETURN_IF_PARSE_ERROR(expectToken((nameToken = peekToken()), TokenId::Id));;

				if (!genericParamNode->name.build(nameToken->sourceText))
					return genOutOfMemorySyntaxError();

				nextToken();

				SLKC_RETURN_IF_PARSE_ERROR(parseParamTypeListGenericConstraint(genericParamNode->paramTypeListGenericConstraint));
			} else if (Token *token = peekToken(); token->tokenId == TokenId::ConstKeyword) {
				nextToken();

				peff::ScopeGuard setTokenRangeGuard([this, token, &genericParamNode]() noexcept {
					if (genericParamNode) {
						genericParamNode->tokenRange = TokenRange{ document->mainModule, token->index, parseContext.idxPrevToken };
					}
				});

				Token *nameToken;

				SLKC_RETURN_IF_PARSE_ERROR(expectToken((nameToken = peekToken()), TokenId::Id));;

				if (!genericParamNode->name.build(nameToken->sourceText))
					return genOutOfMemorySyntaxError();

				nextToken();

				Token *colonToken;
				SLKC_RETURN_IF_PARSE_ERROR(expectToken((colonToken = peekToken()), TokenId::Colon));;

				nextToken();

				SLKC_RETURN_IF_PARSE_ERROR(parseTypeName(genericParamNode->inputType));
			} else {
				Token *nameToken;

				SLKC_RETURN_IF_PARSE_ERROR(expectToken((nameToken = peekToken()), TokenId::Id));;

				peff::ScopeGuard setTokenRangeGuard([this, nameToken, &genericParamNode]() noexcept {
					if (genericParamNode) {
						genericParamNode->tokenRange = TokenRange{ document->mainModule, nameToken->index, parseContext.idxPrevToken };
					}
				});

				if (!genericParamNode->name.build(nameToken->sourceText))
					return genOutOfMemorySyntaxError();

				nextToken();

				SLKC_RETURN_IF_PARSE_ERROR(parseGenericConstraint(genericParamNode->genericConstraint));
			}

			if (peekToken()->tokenId != TokenId::Comma) {
				break;
			}

			Token *commaToken = nextToken();

			if (!idxCommaTokensOut.pushBack(+commaToken->index))
				return genOutOfMemorySyntaxError();
		}

		Token *rAngleBracketToken;

		SLKC_RETURN_IF_PARSE_ERROR(expectToken((rAngleBracketToken = peekToken()), TokenId::GtOp));

		nextToken();

		rAngleBracketIndexOut = rAngleBracketToken->index;
	}

	return {};
}
