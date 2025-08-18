#include "../parser.h"
#include <climits>

using namespace slkc;

SLKC_API std::optional<SyntaxError> Parser::parseTypeName(AstNodePtr<TypeNameNode> &typeNameOut, bool withCircumfixes) {
	std::optional<SyntaxError> syntaxError;
	Token *t = peekToken();

	switch (t->tokenId) {
		case TokenId::VarArg:
			if (!(typeNameOut = makeAstNode<UnpackingTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();

			if ((syntaxError = parseTypeName(typeNameOut.castTo<UnpackingTypeNameNode>()->innerTypeName, true)))
				return syntaxError;
			break;
		case TokenId::VoidTypeName:
			if (!(typeNameOut = makeAstNode<VoidTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::I8TypeName:
			if (!(typeNameOut = peff::makeSharedWithControlBlock<I8TypeNameNode, AstNodeControlBlock<I8TypeNameNode>>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::I16TypeName:
			if (!(typeNameOut = peff::makeSharedWithControlBlock<I16TypeNameNode, AstNodeControlBlock<I16TypeNameNode>>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::I32TypeName:
			if (!(typeNameOut = peff::makeSharedWithControlBlock<I32TypeNameNode, AstNodeControlBlock<I32TypeNameNode>>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::I64TypeName:
			if (!(typeNameOut = peff::makeSharedWithControlBlock<I64TypeNameNode, AstNodeControlBlock<I64TypeNameNode>>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::U8TypeName:
			if (!(typeNameOut = peff::makeSharedWithControlBlock<U8TypeNameNode, AstNodeControlBlock<U8TypeNameNode>>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::U16TypeName:
			if (!(typeNameOut = peff::makeSharedWithControlBlock<U16TypeNameNode, AstNodeControlBlock<U16TypeNameNode>>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::U32TypeName:
			if (!(typeNameOut = peff::makeSharedWithControlBlock<U32TypeNameNode, AstNodeControlBlock<U32TypeNameNode>>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::U64TypeName:
			if (!(typeNameOut = peff::makeSharedWithControlBlock<U64TypeNameNode, AstNodeControlBlock<U64TypeNameNode>>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::ISizeTypeName:
			if (!(typeNameOut = makeAstNode<ISizeTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			break;
		case TokenId::USizeTypeName:
			if (!(typeNameOut = makeAstNode<USizeTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::F32TypeName:
			if (!(typeNameOut = peff::makeSharedWithControlBlock<F32TypeNameNode, AstNodeControlBlock<F32TypeNameNode>>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::F64TypeName:
			if (!(typeNameOut = peff::makeSharedWithControlBlock<F64TypeNameNode, AstNodeControlBlock<F64TypeNameNode>>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::BoolTypeName:
			if (!(typeNameOut = makeAstNode<BoolTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::StringTypeName:
			if (!(typeNameOut = makeAstNode<StringTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::LParenthese: {
			AstNodePtr<ParamTypeListTypeNameNode> tn;

			if (!(tn = makeAstNode<ParamTypeListTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)))
				return genOutOfMemoryError();

			typeNameOut = tn.castTo<TypeNameNode>();

			typeNameOut->tokenRange = TokenRange{ t->index };

			Token *lParentheseToken;
			if ((syntaxError = expectToken((lParentheseToken = peekToken()), TokenId::LParenthese)))
				return SyntaxError(TokenRange{ lParentheseToken->index }, ExpectingSingleTokenErrorExData{ TokenId::LParenthese });

			nextToken();

			for (;;) {
				if (peekToken()->tokenId == TokenId::RParenthese) {
					break;
				}

				if (peekToken()->tokenId == TokenId::VarArg) {
					tn->hasVarArgs = true;
					break;
				}

				AstNodePtr<TypeNameNode> paramType;

				if (auto e = parseTypeName(paramType); e)
					return e;

				if (!tn->paramTypes.pushBack(std::move(paramType)))
					return genOutOfMemoryError();

				if (peekToken()->tokenId != TokenId::Comma) {
					break;
				}

				Token *commaToken = nextToken();
				/*
				if (!idxCommaTokensOut.pushBack(+commaToken->index))
					return genOutOfMemoryError();*/
			}

			Token *rParentheseToken;
			if ((syntaxError = expectToken((rParentheseToken = peekToken()), TokenId::RParenthese)))
				return SyntaxError(TokenRange{ rParentheseToken->index }, ExpectingSingleTokenErrorExData{ TokenId::RParenthese });

			nextToken();
			break;
		}
		case TokenId::FnKeyword: {
			AstNodePtr<FnTypeNameNode> tn;
			if (!(tn = makeAstNode<FnTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)))
				return genOutOfMemoryError();
			typeNameOut = tn.castTo<TypeNameNode>();
			tn->tokenRange = TokenRange{ t->index };
			nextToken();

			Token *lParentheseToken;
			if ((syntaxError = expectToken((lParentheseToken = peekToken()), TokenId::LParenthese)))
				return SyntaxError(TokenRange{ lParentheseToken->index }, ExpectingSingleTokenErrorExData{ TokenId::LParenthese });

			nextToken();

			for (;;) {
				if (peekToken()->tokenId == TokenId::RParenthese) {
					break;
				}

				AstNodePtr<TypeNameNode> paramType;

				if (auto e = parseTypeName(paramType); e)
					return e;

				if (!tn->paramTypes.pushBack(std::move(paramType)))
					return genOutOfMemoryError();

				if (peekToken()->tokenId != TokenId::Comma) {
					break;
				}

				Token *commaToken = nextToken();
				/*
				if (!idxCommaTokensOut.pushBack(+commaToken->index))
					return genOutOfMemoryError();*/
			}

			Token *rParentheseToken;
			if ((syntaxError = expectToken((rParentheseToken = peekToken()), TokenId::RParenthese)))
				return SyntaxError(TokenRange{ rParentheseToken->index }, ExpectingSingleTokenErrorExData{ TokenId::RParenthese });

			nextToken();

			if (peekToken()->tokenId == TokenId::WithKeyword) {
				nextToken();

				if (auto e = parseTypeName(tn->thisType); e)
					return e;
			}

			if (peekToken()->tokenId == TokenId::ReturnTypeOp) {
				nextToken();

				if (auto e = parseTypeName(tn->returnType); e)
					return e;
			}

			break;
		}
		case TokenId::Id: {
			IdRefPtr id;
			if ((syntaxError = parseIdRef(id)))
				return syntaxError;

			AstNodePtr<CustomTypeNameNode> tn;

			if (!(tn = makeAstNode<CustomTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(),
					  document)))
				return genOutOfMemoryError();

			tn->contextNode = curParent;

			tn->tokenRange = id->tokenRange;
			tn->idRefPtr = std::move(id);

			typeNameOut = tn.castTo<TypeNameNode>();

			break;
		}
		default:
			return SyntaxError(TokenRange{ t->index }, SyntaxErrorKind::UnexpectedToken);
	}

	if (withCircumfixes) {
		while (true) {
			switch ((t = peekToken())->tokenId) {
				case TokenId::LBracket: {
					nextToken();

					Token *rBracketToken;
					if ((syntaxError = expectToken((rBracketToken = peekToken()), TokenId::RBracket)))
						return SyntaxError(TokenRange{ rBracketToken->index }, ExpectingSingleTokenErrorExData{ TokenId::RBracket });

					nextToken();

					if (!(typeNameOut = makeAstNode<ArrayTypeNameNode>(
							  resourceAllocator.get(),
							  resourceAllocator.get(),
							  document,
							  typeNameOut)
								.castTo<TypeNameNode>()))
						return genOutOfMemoryError();
					break;
				}
				default:
					goto end;
			}
		}
	}

end:
	if (withCircumfixes) {
		if ((t = peekToken())->tokenId == TokenId::AndOp) {
			nextToken();
			if (!(typeNameOut = makeAstNode<RefTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(),
					  document,
					  typeNameOut)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
		} else if ((t = peekToken())->tokenId == TokenId::LAndOp) {
			nextToken();
			if (!(typeNameOut = makeAstNode<TempRefTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(),
					  document,
					  typeNameOut)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
		}
	}

	return {};
}
