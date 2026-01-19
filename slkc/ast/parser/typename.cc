#include "../parser.h"
#include <climits>

using namespace slkc;

SLKC_API peff::Option<SyntaxError> Parser::parseGenericArg(AstNodePtr<AstNode>& argOut) {
	peff::Option<SyntaxError> syntaxError;
	Token *t = peekToken();

	switch (t->tokenId) {
		case TokenId::IntLiteral:
		case TokenId::LongLiteral:
		case TokenId::UIntLiteral:
		case TokenId::ULongLiteral:
		case TokenId::F32Literal:
		case TokenId::F64Literal:
		case TokenId::StringLiteral: {
			AstNodePtr<ExprNode> e;
			if((syntaxError = parseExpr(INT_MAX, e)))
				return syntaxError;
			argOut = e.castTo<AstNode>();
			break;
		}
		case TokenId::LParenthese: {
			AstNodePtr<ExprNode> e;
			if ((syntaxError = parseExpr(INT_MAX, e)))
				return syntaxError;
			argOut = e.castTo<AstNode>();
			break;
		}
		default: {
			AstNodePtr<TypeNameNode> t;
			if ((syntaxError = parseTypeName(t)))
				return syntaxError;
			argOut = t.castTo<AstNode>();
			break;
		}
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseTypeName(AstNodePtr<TypeNameNode> &typeNameOut, bool withCircumfixes) {
	peff::Option<SyntaxError> syntaxError;
	Token *t = peekToken();

	switch (t->tokenId) {
		case TokenId::VarArg:
			if (!(typeNameOut = makeAstNode<UnpackingTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.template castTo<TypeNameNode>()))
				return genOutOfMemorySyntaxError();
			typeNameOut->tokenRange = TokenRange{ document->mainModule, t->index };
			nextToken();

			if ((syntaxError = parseTypeName(typeNameOut.template castTo<UnpackingTypeNameNode>()->innerTypeName, true)))
				return syntaxError;
			break;
		case TokenId::VoidTypeName:
			if (!(typeNameOut = makeAstNode<VoidTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.template castTo<TypeNameNode>()))
				return genOutOfMemorySyntaxError();
			typeNameOut->tokenRange = TokenRange{ document->mainModule, t->index };
			nextToken();
			break;
		case TokenId::I8TypeName:
			if (!(typeNameOut = peff::makeSharedWithControlBlock<I8TypeNameNode, AstNodeControlBlock<I8TypeNameNode>>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.template castTo<TypeNameNode>()))
				return genOutOfMemorySyntaxError();
			typeNameOut->tokenRange = TokenRange{ document->mainModule, t->index };
			nextToken();
			break;
		case TokenId::I16TypeName:
			if (!(typeNameOut = peff::makeSharedWithControlBlock<I16TypeNameNode, AstNodeControlBlock<I16TypeNameNode>>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.template castTo<TypeNameNode>()))
				return genOutOfMemorySyntaxError();
			typeNameOut->tokenRange = TokenRange{ document->mainModule, t->index };
			nextToken();
			break;
		case TokenId::I32TypeName:
			if (!(typeNameOut = peff::makeSharedWithControlBlock<I32TypeNameNode, AstNodeControlBlock<I32TypeNameNode>>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.template castTo<TypeNameNode>()))
				return genOutOfMemorySyntaxError();
			typeNameOut->tokenRange = TokenRange{ document->mainModule, t->index };
			nextToken();
			break;
		case TokenId::I64TypeName:
			if (!(typeNameOut = peff::makeSharedWithControlBlock<I64TypeNameNode, AstNodeControlBlock<I64TypeNameNode>>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.template castTo<TypeNameNode>()))
				return genOutOfMemorySyntaxError();
			typeNameOut->tokenRange = TokenRange{ document->mainModule, t->index };
			nextToken();
			break;
		case TokenId::U8TypeName:
			if (!(typeNameOut = peff::makeSharedWithControlBlock<U8TypeNameNode, AstNodeControlBlock<U8TypeNameNode>>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.template castTo<TypeNameNode>()))
				return genOutOfMemorySyntaxError();
			typeNameOut->tokenRange = TokenRange{ document->mainModule, t->index };
			nextToken();
			break;
		case TokenId::U16TypeName:
			if (!(typeNameOut = peff::makeSharedWithControlBlock<U16TypeNameNode, AstNodeControlBlock<U16TypeNameNode>>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.template castTo<TypeNameNode>()))
				return genOutOfMemorySyntaxError();
			typeNameOut->tokenRange = TokenRange{ document->mainModule, t->index };
			nextToken();
			break;
		case TokenId::U32TypeName:
			if (!(typeNameOut = peff::makeSharedWithControlBlock<U32TypeNameNode, AstNodeControlBlock<U32TypeNameNode>>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.template castTo<TypeNameNode>()))
				return genOutOfMemorySyntaxError();
			typeNameOut->tokenRange = TokenRange{ document->mainModule, t->index };
			nextToken();
			break;
		case TokenId::U64TypeName:
			if (!(typeNameOut = peff::makeSharedWithControlBlock<U64TypeNameNode, AstNodeControlBlock<U64TypeNameNode>>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.template castTo<TypeNameNode>()))
				return genOutOfMemorySyntaxError();
			typeNameOut->tokenRange = TokenRange{ document->mainModule, t->index };
			nextToken();
			break;
		case TokenId::ISizeTypeName:
			if (!(typeNameOut = makeAstNode<ISizeTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.template castTo<TypeNameNode>()))
				return genOutOfMemorySyntaxError();
			typeNameOut->tokenRange = TokenRange{ document->mainModule, t->index };
			break;
		case TokenId::USizeTypeName:
			if (!(typeNameOut = makeAstNode<USizeTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.template castTo<TypeNameNode>()))
				return genOutOfMemorySyntaxError();
			typeNameOut->tokenRange = TokenRange{ document->mainModule, t->index };
			nextToken();
			break;
		case TokenId::F32TypeName:
			if (!(typeNameOut = peff::makeSharedWithControlBlock<F32TypeNameNode, AstNodeControlBlock<F32TypeNameNode>>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.template castTo<TypeNameNode>()))
				return genOutOfMemorySyntaxError();
			typeNameOut->tokenRange = TokenRange{ document->mainModule, t->index };
			nextToken();
			break;
		case TokenId::F64TypeName:
			if (!(typeNameOut = peff::makeSharedWithControlBlock<F64TypeNameNode, AstNodeControlBlock<F64TypeNameNode>>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.template castTo<TypeNameNode>()))
				return genOutOfMemorySyntaxError();
			typeNameOut->tokenRange = TokenRange{ document->mainModule, t->index };
			nextToken();
			break;
		case TokenId::BoolTypeName:
			if (!(typeNameOut = makeAstNode<BoolTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.template castTo<TypeNameNode>()))
				return genOutOfMemorySyntaxError();
			typeNameOut->tokenRange = TokenRange{ document->mainModule, t->index };
			nextToken();
			break;
		case TokenId::StringTypeName:
			if (!(typeNameOut = makeAstNode<StringTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.template castTo<TypeNameNode>()))
				return genOutOfMemorySyntaxError();
			typeNameOut->tokenRange = TokenRange{ document->mainModule, t->index };
			nextToken();
			break;
		case TokenId::LParenthese: {
			AstNodePtr<ParamTypeListTypeNameNode> tn;

			if (!(tn = makeAstNode<ParamTypeListTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)))
				return genOutOfMemorySyntaxError();

			typeNameOut = tn.template castTo<TypeNameNode>();

			typeNameOut->tokenRange = TokenRange{ document->mainModule, t->index };

			Token *lParentheseToken = nextToken();

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
					return genOutOfMemorySyntaxError();

				if (peekToken()->tokenId != TokenId::Comma) {
					break;
				}

				Token *commaToken = nextToken();
				/*
				if (!idxCommaTokensOut.pushBack(+commaToken->index))
					return genOutOfMemorySyntaxError();*/
			}

			Token *rParentheseToken;
			if ((syntaxError = expectToken((rParentheseToken = peekToken()), TokenId::RParenthese)))
				return SyntaxError(TokenRange{ document->mainModule, rParentheseToken->index }, ExpectingSingleTokenErrorExData{ TokenId::RParenthese });

			nextToken();
			break;
		}
		case TokenId::FnKeyword: {
			AstNodePtr<FnTypeNameNode> tn;
			if (!(tn = makeAstNode<FnTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)))
				return genOutOfMemorySyntaxError();
			typeNameOut = tn.template castTo<TypeNameNode>();
			tn->tokenRange = TokenRange{ document->mainModule, t->index };
			nextToken();

			Token *lParentheseToken;
			if ((syntaxError = expectToken((lParentheseToken = peekToken()), TokenId::LParenthese)))
				return SyntaxError(TokenRange{ document->mainModule, lParentheseToken->index }, ExpectingSingleTokenErrorExData{ TokenId::LParenthese });

			nextToken();

			for (;;) {
				if (peekToken()->tokenId == TokenId::RParenthese) {
					break;
				}

				AstNodePtr<TypeNameNode> paramType;

				if (auto e = parseTypeName(paramType); e)
					return e;

				if (!tn->paramTypes.pushBack(std::move(paramType)))
					return genOutOfMemorySyntaxError();

				if (peekToken()->tokenId != TokenId::Comma) {
					break;
				}

				Token *commaToken = nextToken();
				/*
				if (!idxCommaTokensOut.pushBack(+commaToken->index))
					return genOutOfMemorySyntaxError();*/
			}

			Token *rParentheseToken;
			if ((syntaxError = expectToken((rParentheseToken = peekToken()), TokenId::RParenthese)))
				return SyntaxError(TokenRange{ document->mainModule, rParentheseToken->index }, ExpectingSingleTokenErrorExData{ TokenId::RParenthese });

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
		case TokenId::LBracket: {
			AstNodePtr<TupleTypeNameNode> tn;

			if (!(tn = makeAstNode<TupleTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(),
					  document)))
				return genOutOfMemorySyntaxError();

			typeNameOut = tn.template castTo<TypeNameNode>();

			Token *lBracketToken;

			if (auto e = expectToken(lBracketToken = peekToken(), TokenId::LBracket))
				return e;

			tn->idxLBracketToken = lBracketToken->index;

			nextToken();

			for (;;) {
				if (peekToken()->tokenId == TokenId::RParenthese) {
					break;
				}

				AstNodePtr<TypeNameNode> t;

				if (auto e = parseTypeName(t); e)
					return e;

				if (!tn->elementTypes.pushBack(std::move(t)))
					return genOutOfMemorySyntaxError();

				if (peekToken()->tokenId != TokenId::Comma) {
					break;
				}

				Token *commaToken = nextToken();

				if (!tn->idxCommaTokens.pushBack(+commaToken->index))
					return genOutOfMemorySyntaxError();
			}

			Token *rBracketToken;

			if (auto e = expectToken(rBracketToken = peekToken(), TokenId::RBracket))
				return e;

			tn->idxRBracketToken = rBracketToken->index;

			nextToken();

			break;
		}
		case TokenId::SIMDTypeName: {
			AstNodePtr<SIMDTypeNameNode> tn;

			nextToken();

			if (!(tn = makeAstNode<SIMDTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(),
					  document)))
				return genOutOfMemorySyntaxError();

			typeNameOut = tn.template castTo<TypeNameNode>();

			Token *lAngleBracketToken;

			if (auto e = expectToken(lAngleBracketToken = peekToken(), TokenId::LtOp); e)
				return e;

			tn->idxLAngleBracketToken = lAngleBracketToken->index;

			nextToken();

			if (auto e = parseTypeName(tn->elementType); e)
				return e;

			Token *commaToken;

			if (auto e = expectToken(commaToken = peekToken(), TokenId::Comma))
				return e;

			tn->idxCommaToken = commaToken->index;

			nextToken();

			if (auto e = parseExpr(140, tn->width); e)
				return e;

			Token *rAngleBracketToken;

			if (auto e = expectToken(rAngleBracketToken = peekToken(), TokenId::GtOp))
				return e;

			tn->idxRAngleBracketToken = rAngleBracketToken->index;

			nextToken();

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
				return genOutOfMemorySyntaxError();

			tn->contextNode = curParent;

			tn->tokenRange = id->tokenRange;
			tn->idRefPtr = std::move(id);

			typeNameOut = tn.template castTo<TypeNameNode>();

			break;
		}
		default:
			return SyntaxError(TokenRange{ document->mainModule, t->index }, SyntaxErrorKind::UnexpectedToken);
	}

	if (withCircumfixes) {
		while (true) {
			switch ((t = peekToken())->tokenId) {
				case TokenId::FinalKeyword: {
					nextToken();

					typeNameOut->isFinal = true;
					typeNameOut->idxFinalToken = t->index;
					break;
				}
				case TokenId::LocalKeyword: {
					nextToken();

					typeNameOut->isLocal = true;
					typeNameOut->idxLocalToken = t->index;
					break;
				}
				case TokenId::LBracket: {
					nextToken();

					Token *rBracketToken;
					if ((syntaxError = expectToken((rBracketToken = peekToken()), TokenId::RBracket)))
						return SyntaxError(TokenRange{ document->mainModule, rBracketToken->index }, ExpectingSingleTokenErrorExData{ TokenId::RBracket });

					nextToken();

					if (!(typeNameOut = makeAstNode<ArrayTypeNameNode>(
							  resourceAllocator.get(),
							  resourceAllocator.get(),
							  document,
							  typeNameOut)
								.template castTo<TypeNameNode>()))
						return genOutOfMemorySyntaxError();
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
						.template castTo<TypeNameNode>()))
				return genOutOfMemorySyntaxError();
		} else if ((t = peekToken())->tokenId == TokenId::LAndOp) {
			nextToken();
			if (!(typeNameOut = makeAstNode<TempRefTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(),
					  document,
					  typeNameOut)
						.template castTo<TypeNameNode>()))
				return genOutOfMemorySyntaxError();
		}
	}

	return {};
}
