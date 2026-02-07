#include "parser.h"

using namespace slkc;

SLKC_API Parser::Parser(peff::SharedPtr<Document> document, TokenList &&tokenList, peff::Alloc *resourceAllocator) : document(document), tokenList(std::move(tokenList)), resourceAllocator(resourceAllocator), syntaxErrors(resourceAllocator) {
}

SLKC_API Parser::~Parser() {
	assert(!document);
}

SLKC_API peff::Option<SyntaxError> Parser::parseOperatorName(std::string_view &nameOut) {
	peff::Option<SyntaxError> syntaxError;

	Token *t = peekToken();

	switch (t->tokenId) {
		case TokenId::LAndOp:
			nameOut = "&&";
			nextToken();
			break;
		case TokenId::LOrOp:
			nameOut = "||";
			nextToken();
			break;
		case TokenId::AddOp:
			nameOut = "+";
			nextToken();
			break;
		case TokenId::SubOp:
			nameOut = "-";
			nextToken();
			break;
		case TokenId::MulOp:
			nameOut = "*";
			nextToken();
			break;
		case TokenId::DivOp:
			nameOut = "/";
			nextToken();
			break;
		case TokenId::ModOp:
			nameOut = "%";
			nextToken();
			break;
		case TokenId::AndOp:
			nameOut = "&";
			nextToken();
			break;
		case TokenId::OrOp:
			nameOut = "|";
			nextToken();
			break;
		case TokenId::XorOp:
			nameOut = "^";
			nextToken();
			break;
		case TokenId::LNotOp:
			nameOut = "!";
			nextToken();
			break;
		case TokenId::NotOp:
			nameOut = "~";
			nextToken();
			break;
		case TokenId::AddAssignOp:
			nameOut = "+=";
			nextToken();
			break;
		case TokenId::SubAssignOp:
			nameOut = "-=";
			nextToken();
			break;
		case TokenId::MulAssignOp:
			nameOut = "*=";
			nextToken();
			break;
		case TokenId::DivAssignOp:
			nameOut = "/=";
			nextToken();
			break;
		case TokenId::ModAssignOp:
			nameOut = "%=";
			nextToken();
			break;
		case TokenId::AndAssignOp:
			nameOut = "&=";
			nextToken();
			break;
		case TokenId::OrAssignOp:
			nameOut = "|=";
			nextToken();
			break;
		case TokenId::XorAssignOp:
			nameOut = "^=";
			nextToken();
			break;
		case TokenId::LshAssignOp:
			nameOut = "<<=";
			nextToken();
			break;
		case TokenId::RshAssignOp:
			nameOut = ">>=";
			nextToken();
			break;
		case TokenId::EqOp:
			nameOut = "==";
			nextToken();
			break;
		case TokenId::NeqOp:
			nameOut = "!=";
			nextToken();
			break;
		case TokenId::LshOp:
			nameOut = "<<";
			nextToken();
			break;
		case TokenId::RshOp:
			nameOut = ">>";
			nextToken();
			break;
		case TokenId::LtEqOp:
			nameOut = "<=";
			nextToken();
			break;
		case TokenId::GtEqOp:
			nameOut = ">=";
			nextToken();
			break;
		case TokenId::CmpOp:
			nameOut = "<=>";
			nextToken();
			break;
		case TokenId::LParenthese:
			nextToken();

			if ((syntaxError = expectToken(peekToken(), TokenId::RParenthese))) {
				return syntaxError;
			}

			nameOut = "()";
			break;
		case TokenId::LBracket:
			nextToken();

			if ((syntaxError = expectToken(peekToken(), TokenId::RBracket))) {
				return syntaxError;
			}

			nameOut = "[]";
			break;
		case TokenId::NewKeyword:
			nameOut = "new";
			nextToken();
			break;
		case TokenId::DeleteKeyword:
			nameOut = "delete";
			nextToken();
			break;
		default:
			return SyntaxError(TokenRange{ document->mainModule, t->index }, SyntaxErrorKind::ExpectingOperatorName);
	}
	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseIdName(peff::String &nameOut) {
	peff::Option<SyntaxError> syntaxError;
	Token *t = peekToken();

	switch (t->tokenId) {
		case TokenId::Id:
			if (!nameOut.build(t->sourceText)) {
				return genOutOfMemorySyntaxError();
			}
			nextToken();
			break;
		default:
			return SyntaxError(TokenRange{ document->mainModule, t->index }, SyntaxErrorKind::ExpectingId);
	}
	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseIdRef(IdRefPtr &idRefOut) {
	peff::Option<SyntaxError> syntaxError;
	IdRefPtr idRefPtr(peff::allocAndConstruct<IdRef>(resourceAllocator.get(), ASTNODE_ALIGNMENT, resourceAllocator.get()));
	if (!idRefPtr)
		return genOutOfMemorySyntaxError();
	Token *t = peekToken();

	idRefPtr->tokenRange = TokenRange{ document->mainModule, t->index };

	if (t->tokenId == TokenId::ThisKeyword) {
		nextToken();

		IdRefEntry entry(resourceAllocator.get());
		peff::String idText(resourceAllocator.get());
		if (!idText.build("this")) {
			return genOutOfMemorySyntaxError();
		}

		entry.name = std::move(idText);
		entry.nameTokenIndex = t->index;

		if (!idRefPtr->entries.pushBack(std::move(entry)))
			return genOutOfMemorySyntaxError();

		if ((t = peekToken())->tokenId != TokenId::Dot) {
			goto end;
		}

		nextToken();

		entry.accessOpTokenIndex = t->index;
		idRefPtr->tokenRange.endIndex = t->index;
	} else if (t->tokenId == TokenId::ScopeOp) {
		nextToken();

		IdRefEntry entry(resourceAllocator.get());
		peff::String idText(resourceAllocator.get());

		entry.name = std::move(idText);

		entry.accessOpTokenIndex = t->index;

		if (!idRefPtr->entries.pushBack(std::move(entry)))
			return genOutOfMemorySyntaxError();
	}

	for (;;) {
		if ((syntaxError = expectToken(t = peekToken(), TokenId::Id)))
			return syntaxError;

		nextToken();

		IdRefEntry entry(resourceAllocator.get());
		peff::String idText(resourceAllocator.get());
		if (!idText.build(t->sourceText)) {
			return genOutOfMemorySyntaxError();
		}

		entry.name = std::move(idText);
		entry.nameTokenIndex = t->index;
		idRefPtr->tokenRange.endIndex = t->index;

		size_t prevEndIndex = t->index;
		ParseContext prevParseContext = parseContext;
		if ((t = peekToken())->tokenId == TokenId::LtOp) {
			nextToken();

			for (;;) {
				AstNodePtr<AstNode> genericArg;
				if ((syntaxError = parseGenericArg(genericArg)))
					goto genericArgParseFail;

				if (!entry.genericArgs.pushBack(std::move(genericArg))) {
					return genOutOfMemorySyntaxError();
				}

				if ((t = peekToken())->tokenId != TokenId::Comma) {
					break;
				}

				nextToken();
			}

			if ((t = peekToken())->tokenId != TokenId::GtOp) {
				goto genericArgParseFail;
			}

			idRefPtr->tokenRange.endIndex = t->index;

			nextToken();
		}

		goto succeeded;

	genericArgParseFail:
		idRefPtr->tokenRange.endIndex = prevEndIndex;
		parseContext = prevParseContext;

		entry.genericArgs.clear();

	succeeded:
		if (!idRefPtr->entries.pushBack(std::move(entry)))
			return genOutOfMemorySyntaxError();

		if ((t = peekToken())->tokenId != TokenId::Dot) {
			break;
		}

		entry.accessOpTokenIndex = t->index;
		idRefPtr->tokenRange.endIndex = t->index;

		nextToken();
	}

end:
	idRefOut = std::move(idRefPtr);

	return {};
}

[[nodiscard]] SLKC_API peff::Option<SyntaxError> Parser::parseArgs(peff::DynArray<AstNodePtr<ExprNode>> &argsOut, peff::DynArray<size_t> &idxCommaTokensOut) {
	while (true) {
		if (peekToken()->tokenId == TokenId::RParenthese) {
			break;
		}

		AstNodePtr<ExprNode> arg;

		if (auto e = parseExpr(0, arg); e)
			return e;

		if (!argsOut.pushBack(std::move(arg)))
			return genOutOfMemorySyntaxError();

		if (peekToken()->tokenId != TokenId::Comma) {
			break;
		}

		Token *commaToken = nextToken();
		if (!idxCommaTokensOut.pushBack(+commaToken->index))
			return genOutOfMemorySyntaxError();
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseFn(AstNodePtr<FnOverloadingNode> &fnNodeOut) {
	peff::Option<SyntaxError> syntaxError;

	Token *fnToken;
	Token *lvalueMarkerToken = nullptr;

	peff::String name(resourceAllocator.get());

	if (!(fnNodeOut = makeAstNode<FnOverloadingNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
		return genOutOfMemorySyntaxError();
	}

	switch ((fnToken = peekToken())->tokenId) {
		case TokenId::FnKeyword: {
			nextToken();

			fnNodeOut->overloadingKind = FnOverloadingKind::Regular;

			if ((syntaxError = parseIdName(name))) {
				return syntaxError;
			}
			break;
		}
		case TokenId::AsyncKeyword: {
			nextToken();

			fnNodeOut->overloadingKind = FnOverloadingKind::Coroutine;

			if ((syntaxError = parseIdName(name))) {
				return syntaxError;
			}
			break;
		}
		case TokenId::OperatorKeyword: {
			nextToken();

			fnNodeOut->overloadingKind = FnOverloadingKind::Regular;

			Token *lvalueMarkerToken;
			if ((lvalueMarkerToken = peekToken())->tokenId == TokenId::AssignOp) {
				fnNodeOut->fnFlags |= FN_LVALUE;
				nextToken();
			}

			std::string_view operatorName;
			if ((syntaxError = parseOperatorName(operatorName))) {
				return syntaxError;
			}

			if (!name.build(operatorName)) {
				return genOutOfMemorySyntaxError();
			}

			if (fnNodeOut->fnFlags & FN_LVALUE) {
				if (!name.append(LVALUE_OPERATOR_NAME_SUFFIX))
					return genOutOfMemorySyntaxError();
			}
			break;
		}
		case TokenId::DefKeyword: {
			nextToken();

			fnNodeOut->overloadingKind = FnOverloadingKind::Pure;

			if ((syntaxError = parseIdName(name))) {
				return syntaxError;
			}
			break;
		}
		default:
			return SyntaxError(TokenRange{ document->mainModule, fnToken->index }, SyntaxErrorKind::UnexpectedToken);
	}

	switch (curParent->getAstNodeType()) {
		case AstNodeType::Interface:
			fnNodeOut->fnFlags = FN_VIRTUAL;
			break;
		default:
			break;
	}

	AstNodePtr<MemberNode> prevParent = curParent;
	peff::ScopeGuard restoreParentGuard([this, prevParent]() noexcept {
		curParent = prevParent;
	});
	curParent = fnNodeOut.castTo<MemberNode>();

	peff::ScopeGuard setTokenRangeGuard([this, fnToken, fnNodeOut]() noexcept {
		fnNodeOut->tokenRange = TokenRange{ document->mainModule, fnToken->index, parseContext.idxPrevToken };
	});

	fnNodeOut->name = std::move(name);

	if ((syntaxError = parseGenericParams(fnNodeOut->genericParams, fnNodeOut->idxGenericParamCommaTokens, fnNodeOut->lAngleBracketIndex, fnNodeOut->rAngleBracketIndex))) {
		return syntaxError;
	}
	for (size_t i = 0; i < fnNodeOut->genericParams.size(); ++i) {
		auto gp = fnNodeOut->genericParams.at(i);
		if (fnNodeOut->genericParamIndices.contains(gp->name)) {
			peff::String s(resourceAllocator.get());

			if (!s.build(gp->name)) {
				return genOutOfMemorySyntaxError();
			}

			ConflictingDefinitionsErrorExData exData(std::move(s));

			return SyntaxError(gp->tokenRange, std::move(exData));
		}
		if (!fnNodeOut->genericParamIndices.insert(gp->name, +i))
			return genOutOfMemorySyntaxError();
	}

	bool hasVarArg = false;
	if ((syntaxError = parseParams(fnNodeOut->params, hasVarArg, fnNodeOut->idxParamCommaTokens, fnNodeOut->lParentheseIndex, fnNodeOut->rParentheseIndex))) {
		return syntaxError;
	}
	if (hasVarArg) {
		fnNodeOut->fnFlags |= FN_VARG;
	}
	// Index the parameters.
	for (size_t i = 0; i < fnNodeOut->params.size(); ++i) {
		AstNodePtr<VarNode> &curParam = fnNodeOut->params.at(i);
		if (fnNodeOut->paramIndices.contains(curParam->name)) {
			peff::String s(resourceAllocator.get());

			if (!s.build(curParam->name)) {
				return genOutOfMemorySyntaxError();
			}

			ConflictingDefinitionsErrorExData exData(std::move(s));

			if (!syntaxErrors.pushBack(SyntaxError(curParam->tokenRange, std::move(exData))))
				return genOutOfMemorySyntaxError();
		}

		if (!fnNodeOut->paramIndices.insert(curParam->name, +i)) {
			return genOutOfMemorySyntaxError();
		}
	}

	Token *virtualToken;
	if ((virtualToken = peekToken())->tokenId == TokenId::VirtualKeyword) {
		fnNodeOut->fnFlags |= FN_VIRTUAL;
		nextToken();
	}

	Token *overrideToken;
	if ((overrideToken = peekToken())->tokenId == TokenId::OverrideKeyword) {
		nextToken();

		Token *lookaheadToken = peekToken();
		switch (lookaheadToken->tokenId) {
			case TokenId::ReturnTypeOp:
			case TokenId::Semicolon:
			case TokenId::LBrace:
				break;
			default:
				if ((syntaxError = parseTypeName(fnNodeOut->overridenType))) {
					return syntaxError;
				}
				break;
		}
	}

	Token *returnTypeToken;
	if ((returnTypeToken = peekToken())->tokenId == TokenId::ReturnTypeOp) {
		nextToken();
		if ((syntaxError = parseTypeName(fnNodeOut->returnType))) {
			return syntaxError;
		}
	} else {
		if (!(fnNodeOut->returnType = makeAstNode<VoidTypeNameNode>(resourceAllocator.get(), resourceAllocator.get(), document).castTo<TypeNameNode>())) {
			return genOutOfMemorySyntaxError();
		}
	}

	Token *bodyToken = peekToken();

	switch (bodyToken->tokenId) {
		case TokenId::Semicolon: {
			nextToken();

			break;
		}
		case TokenId::LBrace: {
			nextToken();

			AstNodePtr<StmtNode> curStmt;

			if (!(fnNodeOut->body = makeAstNode<CodeBlockStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
				return genOutOfMemorySyntaxError();
			}

			while (true) {
				if ((syntaxError = expectToken(peekToken()))) {
					return syntaxError;
				}

				if (peekToken()->tokenId == TokenId::RBrace) {
					break;
				}

				if ((syntaxError = parseStmt(curStmt))) {
					if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
						return genOutOfMemorySyntaxError();
				}

				if (curStmt) {
					if (!fnNodeOut->body->body.pushBack(std::move(curStmt))) {
						return genOutOfMemorySyntaxError();
					}
				}
			}

			Token *rBraceToken;

			if ((syntaxError = expectToken((rBraceToken = peekToken()), TokenId::RBrace))) {
				return syntaxError;
			}

			nextToken();
			break;
		}
		default:
			return SyntaxError(
				TokenRange{ document->mainModule, bodyToken->index },
				SyntaxErrorKind::UnexpectedToken);
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseUnionEnumItem(AstNodePtr<ModuleNode> enumOut) {
	peff::Option<SyntaxError> syntaxError;

	Token *nameToken;
	if ((syntaxError = expectToken((nameToken = peekToken()), TokenId::Id))) {
		return syntaxError;
	}
	nextToken();

	switch (Token *token = peekToken(); token->tokenId) {
		case TokenId::LParenthese: {
			AstNodePtr<UnionEnumItemNode> enumItem;
			if (!(enumItem = makeAstNode<UnionEnumItemNode>(resourceAllocator.get(), resourceAllocator.get(), document)))
				return genOutOfMemorySyntaxError();
			nextToken();

			if (!enumItem->name.build(nameToken->sourceText))
				return genOutOfMemorySyntaxError();

			size_t idxMember;
			{
				peff::ScopeGuard setTokenRangeGuard([this, token, enumItem]() noexcept {
					enumItem->tokenRange = TokenRange{ document->mainModule, token->index, parseContext.idxPrevToken };
				});

				if ((idxMember = enumOut->pushMember(enumItem.castTo<MemberNode>())) == SIZE_MAX)
					return genOutOfMemorySyntaxError();

				while (true) {
					AstNodePtr<VarNode> enumItemEntry;

					if (!(enumItemEntry = makeAstNode<VarNode>(resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemorySyntaxError();

					size_t idxEntryMember;
					{
						peff::ScopeGuard setEnumItemTokenRangeGuard([this, token, enumItem]() noexcept {
							enumItem->tokenRange = TokenRange{ document->mainModule, token->index, parseContext.idxPrevToken };
						});
						if ((idxEntryMember = enumItem->pushMember(enumItemEntry.castTo<MemberNode>())) == SIZE_MAX) {
							return genOutOfMemorySyntaxError();
						}

						Token *entryNameToken;
						if ((syntaxError = expectToken((entryNameToken = peekToken()), TokenId::Id))) {
							return syntaxError;
						}
						nextToken();

						if (!enumItemEntry->name.build(entryNameToken->sourceText))
							return genOutOfMemorySyntaxError();

						Token *colonToken;
						if ((syntaxError = expectToken((colonToken = peekToken()), TokenId::Colon))) {
							return syntaxError;
						}
						nextToken();

						if ((syntaxError = parseTypeName(enumItemEntry->type)))
							return syntaxError;
					}

					if (auto it = enumItem->memberIndices.find(enumItemEntry->name); it != enumItem->memberIndices.end()) {
						peff::String s(resourceAllocator.get());

						if (!s.build(enumItemEntry->name)) {
							return genOutOfMemorySyntaxError();
						}

						ConflictingDefinitionsErrorExData exData(std::move(s));

						return SyntaxError(enumItem->tokenRange, std::move(exData));
					} else {
						if (!(enumItem->indexMember(idxEntryMember))) {
							return genOutOfMemorySyntaxError();
						}
					}

					if (peekToken()->tokenId != TokenId::Comma)
						break;
					Token *commaToken = nextToken();
				}
			}
			Token *rBraceToken;
			if ((syntaxError = expectToken((rBraceToken = peekToken()), TokenId::RParenthese)))
				return syntaxError;
			nextToken();

			if (auto it = enumOut->memberIndices.find(enumItem->name); it != enumOut->memberIndices.end()) {
				peff::String s(resourceAllocator.get());

				if (!s.build(enumItem->name)) {
					return genOutOfMemorySyntaxError();
				}

				ConflictingDefinitionsErrorExData exData(std::move(s));

				return SyntaxError(enumItem->tokenRange, std::move(exData));
			} else {
				if (!(enumOut->indexMember(idxMember))) {
					return genOutOfMemorySyntaxError();
				}
			}
			break;
		}
		default:
			return SyntaxError(TokenRange{ document->mainModule, token->index }, SyntaxErrorKind::UnexpectedToken);
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseEnumItem(AstNodePtr<ModuleNode> enumOut) {
	peff::Option<SyntaxError> syntaxError;

	AstNodePtr<EnumItemNode> enumItem;
	if (!(enumItem = makeAstNode<EnumItemNode>(resourceAllocator.get(), resourceAllocator.get(), document)))
		return genOutOfMemorySyntaxError();

	size_t idxMember;
	{
		peff::ScopeGuard setTokenRangeGuard([this, token = peekToken(), enumItem]() noexcept {
			enumItem->tokenRange = TokenRange{ document->mainModule, token->index, parseContext.idxPrevToken };
		});

		if ((idxMember = enumOut->pushMember(enumItem.castTo<MemberNode>())) == SIZE_MAX) {
			return genOutOfMemorySyntaxError();
		}

		Token *nameToken;
		if ((syntaxError = expectToken((nameToken = peekToken()), TokenId::Id))) {
			return syntaxError;
		}
		nextToken();

		if (!enumItem->name.build(nameToken->sourceText))
			return genOutOfMemorySyntaxError();

		if (Token *token = peekToken(); token->tokenId == TokenId::AssignOp) {
			nextToken();
			if ((syntaxError = parseExpr(0, enumItem->enumValue)))
				return syntaxError;
		}
	}

	if (auto it = enumOut->memberIndices.find(enumItem->name); it != enumOut->memberIndices.end()) {
		peff::String s(resourceAllocator.get());

		if (!s.build(enumItem->name)) {
			return genOutOfMemorySyntaxError();
		}

		ConflictingDefinitionsErrorExData exData(std::move(s));

		return SyntaxError(enumItem->tokenRange, std::move(exData));
	} else {
		if (!(enumOut->indexMember(idxMember))) {
			return genOutOfMemorySyntaxError();
		}
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseProgramStmt() {
	peff::Option<SyntaxError> syntaxError;

	peff::DynArray<AstNodePtr<AttributeNode>> attributes(resourceAllocator.get());

	if ((syntaxError = parseAttributes(attributes))) {
		return syntaxError;
	}

	slake::AccessModifier access = 0;
	Token *currentToken;

	for (;;) {
		switch ((currentToken = peekToken())->tokenId) {
			case TokenId::PublicKeyword:
				access |= slake::ACCESS_PUBLIC;
				nextToken();
				break;
			case TokenId::StaticKeyword:
				access |= slake::ACCESS_STATIC;
				nextToken();
				break;
			case TokenId::NativeKeyword:
				access |= slake::ACCESS_NATIVE;
				nextToken();
				break;
			default:
				goto accessModifierParseEnd;
		}
	}

accessModifierParseEnd:
	Token *token = peekToken();

	AstNodePtr<ModuleNode> p = curParent.castTo<ModuleNode>();

	if (p->getAstNodeType() == AstNodeType::Module) {
		access |= slake::ACCESS_STATIC;
	}

	switch (token->tokenId) {
		case TokenId::EnumKeyword: {
			nextToken();
			switch (peekToken()->tokenId) {
				case TokenId::UnionKeyword: {
					nextToken();
					AstNodePtr<UnionEnumNode> enumNode;

					if (!(enumNode = makeAstNode<UnionEnumNode>(resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemorySyntaxError();

					peff::ScopeGuard setTokenRangeGuard([this, token, enumNode]() noexcept {
						enumNode->tokenRange = TokenRange{ document->mainModule, token->index, parseContext.idxPrevToken };
					});

					Token *nameToken;
					if ((syntaxError = expectToken((nameToken = peekToken()), TokenId::Id))) {
						return syntaxError;
					}
					nextToken();

					size_t idxMember;
					if ((idxMember = p->pushMember(enumNode.castTo<MemberNode>())) == SIZE_MAX) {
						return genOutOfMemorySyntaxError();
					}

					if (!enumNode->name.build(nameToken->sourceText)) {
						return genOutOfMemorySyntaxError();
					}

					Token *lBraceToken;
					if ((syntaxError = expectToken((lBraceToken = peekToken()), TokenId::LBrace))) {
						return syntaxError;
					}
					nextToken();

					while (true) {
						if (peekToken()->tokenId == TokenId::RBrace)
							break;

						if ((syntaxError = parseUnionEnumItem(enumNode.castTo<ModuleNode>()))) {
							if (syntaxError->errorKind == SyntaxErrorKind::OutOfMemory)
								return syntaxError;
							if (!syntaxErrors.pushBack(syntaxError.move()))
								return genOutOfMemorySyntaxError();
						}

						if (peekToken()->tokenId != TokenId::Comma)
							break;
						Token *commaToken = nextToken();
					}

					Token *rBraceToken;
					if ((syntaxError = expectToken((rBraceToken = peekToken()), TokenId::RBrace))) {
						return syntaxError;
					}
					nextToken();

					if (auto it = p->memberIndices.find(enumNode->name); it != p->memberIndices.end()) {
						peff::String s(resourceAllocator.get());

						if (!s.build(enumNode->name)) {
							return genOutOfMemorySyntaxError();
						}

						ConflictingDefinitionsErrorExData exData(std::move(s));

						return SyntaxError(enumNode->tokenRange, std::move(exData));
					} else {
						if (!(p->indexMember(idxMember))) {
							return genOutOfMemorySyntaxError();
						}
					}
					break;
				}
				case TokenId::ConstKeyword: {
					nextToken();
					AstNodePtr<ConstEnumNode> enumNode;

					if (!(enumNode = makeAstNode<ConstEnumNode>(resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemorySyntaxError();

					peff::ScopeGuard setTokenRangeGuard([this, token, enumNode]() noexcept {
						enumNode->tokenRange = TokenRange{ document->mainModule, token->index, parseContext.idxPrevToken };
					});

					Token *nameToken;
					if ((syntaxError = expectToken((nameToken = peekToken()), TokenId::Id))) {
						return syntaxError;
					}
					nextToken();

					size_t idxMember;
					if ((idxMember = p->pushMember(enumNode.castTo<MemberNode>())) == SIZE_MAX) {
						return genOutOfMemorySyntaxError();
					}

					if (!enumNode->name.build(nameToken->sourceText)) {
						return genOutOfMemorySyntaxError();
					}

					if (Token *lParentheseToken = peekToken(); lParentheseToken->tokenId == TokenId::LParenthese) {
						nextToken();

						if ((syntaxError = parseTypeName(enumNode->baseType))) {
							return syntaxError;
						}

						Token *rParentheseToken;
						if ((syntaxError = expectToken((rParentheseToken = peekToken()), TokenId::RParenthese))) {
							return syntaxError;
						}
						nextToken();
					}

					Token *lBraceToken;
					if ((syntaxError = expectToken((lBraceToken = peekToken()), TokenId::LBrace))) {
						return syntaxError;
					}
					nextToken();

					while (true) {
						if (peekToken()->tokenId == TokenId::RBrace)
							break;

						if ((syntaxError = parseEnumItem(enumNode.castTo<ModuleNode>()))) {
							if (syntaxError->errorKind == SyntaxErrorKind::OutOfMemory)
								return syntaxError;
							if (!syntaxErrors.pushBack(syntaxError.move()))
								return genOutOfMemorySyntaxError();
						}

						if (peekToken()->tokenId != TokenId::Comma)
							break;
						Token *commaToken = nextToken();
					}

					Token *rBraceToken;
					if ((syntaxError = expectToken((rBraceToken = peekToken()), TokenId::RBrace))) {
						return syntaxError;
					}
					nextToken();

					if (auto it = p->memberIndices.find(enumNode->name); it != p->memberIndices.end()) {
						peff::String s(resourceAllocator.get());

						if (!s.build(enumNode->name)) {
							return genOutOfMemorySyntaxError();
						}

						ConflictingDefinitionsErrorExData exData(std::move(s));

						return SyntaxError(enumNode->tokenRange, std::move(exData));
					} else {
						if (!(p->indexMember(idxMember))) {
							return genOutOfMemorySyntaxError();
						}
					}
					break;
				}
				case TokenId::Id: {
					Token *nameToken = nextToken();
					AstNodePtr<ScopedEnumNode> enumNode;

					if (!(enumNode = makeAstNode<ScopedEnumNode>(resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemorySyntaxError();

					size_t idxMember;
					{
						peff::ScopeGuard setTokenRangeGuard([this, token, enumNode]() noexcept {
							enumNode->tokenRange = TokenRange{ document->mainModule, token->index, parseContext.idxPrevToken };
						});
						if ((idxMember = p->pushMember(enumNode.castTo<MemberNode>())) == SIZE_MAX) {
							return genOutOfMemorySyntaxError();
						}

						if (!enumNode->name.build(nameToken->sourceText)) {
							return genOutOfMemorySyntaxError();
						}

						if (Token *lParentheseToken = peekToken(); lParentheseToken->tokenId == TokenId::LParenthese) {
							nextToken();

							if ((syntaxError = parseTypeName(enumNode->baseType))) {
								return syntaxError;
							}

							Token *rParentheseToken;
							if ((syntaxError = expectToken((rParentheseToken = peekToken()), TokenId::RParenthese))) {
								return syntaxError;
							}
							nextToken();
						}
					}

					Token *lBraceToken;
					if ((syntaxError = expectToken((lBraceToken = peekToken()), TokenId::LBrace))) {
						return syntaxError;
					}
					nextToken();

					while (true) {
						if (peekToken()->tokenId == TokenId::RBrace)
							break;

						if ((syntaxError = parseEnumItem(enumNode.castTo<ModuleNode>()))) {
							if (syntaxError->errorKind == SyntaxErrorKind::OutOfMemory)
								return syntaxError;
							if (!syntaxErrors.pushBack(syntaxError.move()))
								return genOutOfMemorySyntaxError();
						}

						if (peekToken()->tokenId != TokenId::Comma)
							break;
						Token *commaToken = nextToken();
					}

					Token *rBraceToken;
					if ((syntaxError = expectToken((rBraceToken = peekToken()), TokenId::RBrace))) {
						return syntaxError;
					}
					nextToken();

					if (auto it = p->memberIndices.find(enumNode->name); it != p->memberIndices.end()) {
						peff::String s(resourceAllocator.get());

						if (!s.build(enumNode->name)) {
							return genOutOfMemorySyntaxError();
						}

						ConflictingDefinitionsErrorExData exData(std::move(s));

						return SyntaxError(enumNode->tokenRange, std::move(exData));
					} else {
						if (!(p->indexMember(idxMember))) {
							return genOutOfMemorySyntaxError();
						}
					}
					break;
				}
				default:
					return SyntaxError(TokenRange{ document->mainModule, token->index }, SyntaxErrorKind::UnexpectedToken);
			}
			break;
		}
		case TokenId::AttributeKeyword: {
			// Attribute definition.
			nextToken();

			AstNodePtr<AttributeDefNode> attributeNode;

			if (!(attributeNode = makeAstNode<AttributeDefNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
				return genOutOfMemorySyntaxError();
			}

			attributeNode->accessModifier = access;

			Token *nameToken;

			if ((syntaxError = expectToken((nameToken = peekToken()), TokenId::Id))) {
				return syntaxError;
			}
			nextToken();

			size_t idxMember;
			if ((idxMember = p->pushMember(attributeNode.castTo<MemberNode>())) == SIZE_MAX) {
				return genOutOfMemorySyntaxError();
			}

			if (!attributeNode->name.build(nameToken->sourceText)) {
				return genOutOfMemorySyntaxError();
			}

			{
				peff::ScopeGuard setTokenRangeGuard([this, token, attributeNode]() noexcept {
					attributeNode->tokenRange = TokenRange{ document->mainModule, token->index, parseContext.idxPrevToken };
				});

				AstNodePtr<MemberNode> prevParent;
				prevParent = curParent;
				peff::ScopeGuard restorePrevModGuard([this, prevParent]() noexcept {
					curParent = prevParent;
				});
				curParent = attributeNode.castTo<MemberNode>();

				Token *lBraceToken;

				if ((syntaxError = expectToken((lBraceToken = peekToken()), TokenId::LBrace))) {
					return syntaxError;
				}

				nextToken();

				Token *currentToken;
				while (true) {
					if ((syntaxError = expectToken(currentToken = peekToken()))) {
						return syntaxError;
					}
					if (currentToken->tokenId == TokenId::RBrace) {
						break;
					}

					if ((syntaxError = parseProgramStmt())) {
						// Parse the rest to make sure that we have gained all of the information,
						// instead of ignoring them.
						if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
							return genOutOfMemorySyntaxError();
						syntaxError.reset();
					}
				}

				Token *rBraceToken;

				if ((syntaxError = expectToken((rBraceToken = peekToken()), TokenId::RBrace))) {
					return syntaxError;
				}

				nextToken();
			}

			if (auto it = p->memberIndices.find(attributeNode->name); it != p->memberIndices.end()) {
				peff::String s(resourceAllocator.get());

				if (!s.build(attributeNode->name)) {
					return genOutOfMemorySyntaxError();
				}

				ConflictingDefinitionsErrorExData exData(std::move(s));

				return SyntaxError(attributeNode->tokenRange, std::move(exData));
			} else {
				if (!(p->indexMember(idxMember))) {
					return genOutOfMemorySyntaxError();
				}
			}

			break;
		}
		case TokenId::FnKeyword:
		case TokenId::AsyncKeyword:
		case TokenId::OperatorKeyword:
		case TokenId::DefKeyword: {
			// Function.
			AstNodePtr<FnOverloadingNode> fn;

			if ((syntaxError = parseFn(fn))) {
				return syntaxError;
			}

			fn->accessModifier = access;

			if (auto it = p->memberIndices.find(fn->name); it != p->memberIndices.end()) {
				if (p->members.at(it.value())->getAstNodeType() != AstNodeType::Fn) {
					peff::String s(resourceAllocator.get());

					if (!s.build(fn->name)) {
						return genOutOfMemorySyntaxError();
					}

					ConflictingDefinitionsErrorExData exData(std::move(s));

					return SyntaxError(fn->tokenRange, std::move(exData));
				}
				FnNode *fnSlot = (FnNode *)p->members.at(it.value()).get();
				fn->setParent(fnSlot);
				if (!fnSlot->overloadings.pushBack(std::move(fn))) {
					return genOutOfMemorySyntaxError();
				}
			} else {
				AstNodePtr<FnNode> fnSlot;

				if (!(fnSlot = makeAstNode<FnNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
					return genOutOfMemorySyntaxError();
				}

				if (!fnSlot->name.build(fn->name)) {
					return genOutOfMemorySyntaxError();
				}

				if (!(p->addMember(fnSlot.castTo<MemberNode>()))) {
					return genOutOfMemorySyntaxError();
				}

				fn->setParent(fnSlot.get());

				if (!fnSlot->overloadings.pushBack(std::move(fn))) {
					return genOutOfMemorySyntaxError();
				}
			}
			break;
		}
		case TokenId::ClassKeyword: {
			// Class.
			nextToken();

			AstNodePtr<ClassNode> classNode;

			if (!(classNode = makeAstNode<ClassNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
				return genOutOfMemorySyntaxError();
			}

			classNode->accessModifier = access;

			Token *nameToken;

			if ((syntaxError = expectToken((nameToken = peekToken()), TokenId::Id))) {
				return syntaxError;
			}
			nextToken();

			if (!classNode->name.build(nameToken->sourceText)) {
				return genOutOfMemorySyntaxError();
			}

			size_t idxMember;
			if ((idxMember = p->pushMember(classNode.castTo<MemberNode>())) == SIZE_MAX) {
				return genOutOfMemorySyntaxError();
			}

			{
				peff::ScopeGuard setTokenRangeGuard([this, token, classNode]() noexcept {
					classNode->tokenRange = TokenRange{ document->mainModule, token->index, parseContext.idxPrevToken };
				});

				AstNodePtr<MemberNode> prevParent;
				prevParent = curParent;
				peff::ScopeGuard restorePrevModGuard([this, prevParent]() noexcept {
					curParent = prevParent;
				});
				curParent = classNode.castTo<MemberNode>();

				if ((syntaxError = parseGenericParams(classNode->genericParams, classNode->idxGenericParamCommaTokens, classNode->idxLAngleBracketToken, classNode->idxRAngleBracketToken))) {
					return syntaxError;
				}
				for (size_t i = 0; i < classNode->genericParams.size(); ++i) {
					auto gp = classNode->genericParams.at(i);
					if (classNode->genericParamIndices.contains(gp->name)) {
						peff::String s(resourceAllocator.get());

						if (!s.build(gp->name)) {
							return genOutOfMemorySyntaxError();
						}

						ConflictingDefinitionsErrorExData exData(std::move(s));

						return SyntaxError(gp->tokenRange, std::move(exData));
					}
					if (!classNode->genericParamIndices.insert(gp->name, +i))
						return genOutOfMemorySyntaxError();
				}

				if (Token *lParentheseToken = peekToken(); lParentheseToken->tokenId == TokenId::LParenthese) {
					nextToken();

					if ((syntaxError = parseTypeName(classNode->baseType))) {
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
						AstNodePtr<TypeNameNode> tn;

						if ((syntaxError = parseTypeName(tn))) {
							return syntaxError;
						}

						if (!classNode->implTypes.pushBack(std::move(tn))) {
							return genOutOfMemorySyntaxError();
						}

						if (peekToken()->tokenId != TokenId::AddOp) {
							break;
						}

						Token *orOpToken = nextToken();
					}
				}

				Token *lBraceToken;

				if ((syntaxError = expectToken((lBraceToken = peekToken()), TokenId::LBrace))) {
					return syntaxError;
				}

				nextToken();

				Token *currentToken;
				while (true) {
					if ((syntaxError = expectToken(currentToken = peekToken()))) {
						return syntaxError;
					}
					if (currentToken->tokenId == TokenId::RBrace) {
						break;
					}

					if ((syntaxError = parseProgramStmt())) {
						// Parse the rest to make sure that we have gained all of the information,
						// instead of ignoring them.
						if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
							return genOutOfMemorySyntaxError();
						syntaxError.reset();
					}
				}

				Token *rBraceToken;

				if ((syntaxError = expectToken((rBraceToken = peekToken()), TokenId::RBrace))) {
					return syntaxError;
				}

				nextToken();
			}

			if (auto it = p->memberIndices.find(classNode->name); it != p->memberIndices.end()) {
				peff::String s(resourceAllocator.get());

				if (!s.build(classNode->name)) {
					return genOutOfMemorySyntaxError();
				}

				ConflictingDefinitionsErrorExData exData(std::move(s));

				return SyntaxError(classNode->tokenRange, std::move(exData));
			} else {
				if (!(p->indexMember(idxMember))) {
					return genOutOfMemorySyntaxError();
				}
			}

			break;
		}
		case TokenId::StructKeyword: {
			// Struct.
			nextToken();

			AstNodePtr<StructNode> structNode;

			if (!(structNode = makeAstNode<StructNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
				return genOutOfMemorySyntaxError();
			}

			structNode->accessModifier = access;

			Token *nameToken;

			if ((syntaxError = expectToken((nameToken = peekToken()), TokenId::Id))) {
				return syntaxError;
			}
			nextToken();

			if (!structNode->name.build(nameToken->sourceText)) {
				return genOutOfMemorySyntaxError();
			}

			size_t idxMember;
			if ((idxMember = p->pushMember(structNode.castTo<MemberNode>())) == SIZE_MAX) {
				return genOutOfMemorySyntaxError();
			}

			{
				peff::ScopeGuard setTokenRangeGuard([this, token, structNode]() noexcept {
					structNode->tokenRange = TokenRange{ document->mainModule, token->index, parseContext.idxPrevToken };
				});

				AstNodePtr<MemberNode> prevParent;
				prevParent = curParent;
				peff::ScopeGuard restorePrevModGuard([this, prevParent]() noexcept {
					curParent = prevParent;
				});
				curParent = structNode.castTo<MemberNode>();

				if ((syntaxError = parseGenericParams(structNode->genericParams, structNode->idxGenericParamCommaTokens, structNode->idxLAngleBracketToken, structNode->idxRAngleBracketToken))) {
					return syntaxError;
				}
				for (size_t i = 0; i < structNode->genericParams.size(); ++i) {
					auto gp = structNode->genericParams.at(i);
					if (structNode->genericParamIndices.contains(gp->name)) {
						peff::String s(resourceAllocator.get());

						if (!s.build(gp->name)) {
							return genOutOfMemorySyntaxError();
						}

						ConflictingDefinitionsErrorExData exData(std::move(s));

						return SyntaxError(gp->tokenRange, std::move(exData));
					}
					if (!structNode->genericParamIndices.insert(gp->name, +i))
						return genOutOfMemorySyntaxError();
				}

				if (Token *colonToken = peekToken(); colonToken->tokenId == TokenId::Colon) {
					nextToken();

					while (true) {
						AstNodePtr<TypeNameNode> tn;

						if ((syntaxError = parseTypeName(tn))) {
							return syntaxError;
						}

						if (!structNode->implTypes.pushBack(std::move(tn))) {
							return genOutOfMemorySyntaxError();
						}

						if (peekToken()->tokenId != TokenId::AddOp) {
							break;
						}

						Token *orOpToken = nextToken();
					}
				}

				Token *lBraceToken;

				if ((syntaxError = expectToken((lBraceToken = peekToken()), TokenId::LBrace))) {
					return syntaxError;
				}

				nextToken();

				Token *currentToken;
				while (true) {
					if ((syntaxError = expectToken(currentToken = peekToken()))) {
						return syntaxError;
					}
					if (currentToken->tokenId == TokenId::RBrace) {
						break;
					}

					if ((syntaxError = parseProgramStmt())) {
						// Parse the rest to make sure that we have gained all of the information,
						// instead of ignoring them.
						if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
							return genOutOfMemorySyntaxError();
						syntaxError.reset();
					}
				}

				Token *rBraceToken;

				if ((syntaxError = expectToken((rBraceToken = peekToken()), TokenId::RBrace))) {
					return syntaxError;
				}

				nextToken();
			}

			if (auto it = p->memberIndices.find(structNode->name); it != p->memberIndices.end()) {
				peff::String s(resourceAllocator.get());

				if (!s.build(structNode->name)) {
					return genOutOfMemorySyntaxError();
				}

				ConflictingDefinitionsErrorExData exData(std::move(s));

				return SyntaxError(structNode->tokenRange, std::move(exData));
			} else {
				if (!(p->indexMember(idxMember))) {
					return genOutOfMemorySyntaxError();
				}
			}

			break;
		}
		case TokenId::InterfaceKeyword: {
			// Interface.
			nextToken();

			AstNodePtr<InterfaceNode> interfaceNode;

			if (!(interfaceNode = makeAstNode<InterfaceNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
				return genOutOfMemorySyntaxError();
			}

			interfaceNode->accessModifier = access;

			Token *nameToken;

			if ((syntaxError = expectToken((nameToken = peekToken()), TokenId::Id))) {
				return syntaxError;
			}
			nextToken();

			if (!interfaceNode->name.build(nameToken->sourceText)) {
				return genOutOfMemorySyntaxError();
			}

			size_t idxMember;
			if ((idxMember = p->pushMember(interfaceNode.castTo<MemberNode>())) == SIZE_MAX) {
				return genOutOfMemorySyntaxError();
			}

			Token *t;

			{
				peff::ScopeGuard setTokenRangeGuard([this, token, interfaceNode]() noexcept {
					interfaceNode->tokenRange = TokenRange{ document->mainModule, token->index, parseContext.idxPrevToken };
				});

				AstNodePtr<MemberNode> prevMember;
				prevMember = curParent;
				peff::ScopeGuard restorePrevModGuard([this, prevMember]() noexcept {
					curParent = prevMember;
				});
				curParent = interfaceNode.castTo<MemberNode>();

				if ((syntaxError = parseGenericParams(interfaceNode->genericParams, interfaceNode->idxGenericParamCommaTokens, interfaceNode->idxLAngleBracketToken, interfaceNode->idxRAngleBracketToken))) {
					return syntaxError;
				}
				for (size_t i = 0; i < interfaceNode->genericParams.size(); ++i) {
					auto gp = interfaceNode->genericParams.at(i);
					if (interfaceNode->genericParamIndices.contains(gp->name)) {
						peff::String s(resourceAllocator.get());

						if (!s.build(gp->name)) {
							return genOutOfMemorySyntaxError();
						}

						ConflictingDefinitionsErrorExData exData(std::move(s));

						return SyntaxError(gp->tokenRange, std::move(exData));
					}
					if (!interfaceNode->genericParamIndices.insert(gp->name, +i))
						return genOutOfMemorySyntaxError();
				}

				if (Token *colonToken = peekToken(); colonToken->tokenId == TokenId::Colon) {
					nextToken();

					while (true) {
						AstNodePtr<TypeNameNode> tn;

						if ((syntaxError = parseTypeName(tn))) {
							return syntaxError;
						}

						if (!interfaceNode->implTypes.pushBack(std::move(tn))) {
							return genOutOfMemorySyntaxError();
						}

						if (peekToken()->tokenId != TokenId::AddOp) {
							break;
						}

						Token *orOpToken = nextToken();
					}
				}

				Token *lBraceToken;

				if ((syntaxError = expectToken((lBraceToken = peekToken()), TokenId::LBrace))) {
					return syntaxError;
				}

				nextToken();

				Token *currentToken;
				while (true) {
					if ((syntaxError = expectToken(currentToken = peekToken()))) {
						return syntaxError;
					}
					if (currentToken->tokenId == TokenId::RBrace) {
						break;
					}

					if ((syntaxError = parseProgramStmt())) {
						// Parse the rest to make sure that we have gained all of the information,
						// instead of ignoring them.
						if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
							return genOutOfMemorySyntaxError();
						syntaxError.reset();
					}
				}

				Token *rBraceToken;

				if ((syntaxError = expectToken((rBraceToken = peekToken()), TokenId::RBrace))) {
					return syntaxError;
				}

				nextToken();
			}

			if (auto it = p->memberIndices.find(interfaceNode->name); it != p->memberIndices.end()) {
				peff::String s(resourceAllocator.get());

				if (!s.build(interfaceNode->name)) {
					return genOutOfMemorySyntaxError();
				}

				ConflictingDefinitionsErrorExData exData(std::move(s));

				return SyntaxError(interfaceNode->tokenRange, std::move(exData));
			} else {
				if (!(p->indexMember(idxMember))) {
					return genOutOfMemorySyntaxError();
				}
			}

			break;
		}
		case TokenId::ImportKeyword: {
			// Import item.
			nextToken();

			AstNodePtr<ImportNode> importNode;

			if (!(importNode = makeAstNode<ImportNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
				return genOutOfMemorySyntaxError();
			}

			if ((syntaxError = parseIdRef(importNode->idRef)))
				return syntaxError;

			size_t idxMember;
			if ((idxMember = p->pushMember(importNode.castTo<MemberNode>())) == SIZE_MAX) {
				return genOutOfMemorySyntaxError();
			}

			if (Token *asToken = peekToken(); asToken->tokenId == TokenId::AsKeyword) {
				nextToken();

				Token *nameToken;

				if ((syntaxError = expectToken((nameToken = peekToken()), TokenId::Id))) {
					return syntaxError;
				}

				if (!importNode->name.build(nameToken->sourceText)) {
					return genOutOfMemorySyntaxError();
				}

				if (!p->indexMember(idxMember)) {
					return genOutOfMemorySyntaxError();
				}
			} else {
				if (!p->anonymousImports.pushBack(AstNodePtr<ImportNode>(importNode))) {
					return genOutOfMemorySyntaxError();
				}
			}

			Token *semicolonToken;

			if ((syntaxError = expectToken((semicolonToken = peekToken()), TokenId::Semicolon))) {
				return syntaxError;
			}

			nextToken();

			break;
		}
		case TokenId::LetKeyword: {
			// Global variable.
			nextToken();

			AstNodePtr<VarDefStmtNode> stmt;

			if (!(stmt = makeAstNode<VarDefStmtNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(),
					  document,
					  peff::DynArray<VarDefEntryPtr>(resourceAllocator.get())))) {
				return genOutOfMemorySyntaxError();
			}

			stmt->accessModifier = access;

			if (!p->varDefStmts.pushBack(AstNodePtr<VarDefStmtNode>(stmt))) {
				return genOutOfMemorySyntaxError();
			}

			peff::ScopeGuard setTokenRangeGuard([this, token, stmt]() noexcept {
				stmt->tokenRange = TokenRange{ document->mainModule, token->index, parseContext.idxPrevToken };
			});

			if ((syntaxError = parseVarDefs(stmt->varDefEntries))) {
				return syntaxError;
			}

			Token *semicolonToken;

			if ((syntaxError = expectToken((semicolonToken = peekToken()), TokenId::Semicolon))) {
				return syntaxError;
			}

			nextToken();

			for (auto &i : stmt->varDefEntries) {
				if (p->memberIndices.contains(i->name)) {
					peff::String s(resourceAllocator.get());

					if (!s.build(i->name))
						return genOutOfMemorySyntaxError();

					ConflictingDefinitionsErrorExData exData(std::move(s));

					if (syntaxErrors.pushBack(SyntaxError(TokenRange(p.get(), i->idxNameToken), std::move(exData))))
						return genOutOfMemorySyntaxError();
				}
				AstNodePtr<VarNode> varNode;

				if (!(varNode = makeAstNode<VarNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
					return genOutOfMemorySyntaxError();
				}

				if (!varNode->name.build(i->name))
					return genOutOfMemorySyntaxError();
				varNode->initialValue = i->initialValue;
				varNode->type = i->type;
				varNode->accessModifier = stmt->accessModifier;

				if (!p->addMember(varNode.castTo<MemberNode>()))
					return genOutOfMemorySyntaxError();
			}

			break;
		}
		default:
			nextToken();
			return SyntaxError(
				TokenRange{ document->mainModule, token->index },
				SyntaxErrorKind::ExpectingDecl);
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseProgram(const AstNodePtr<ModuleNode> &initialMod, IdRefPtr &moduleNameOut) {
	peff::Option<SyntaxError> syntaxError;

	Token *t;

	curParent = initialMod.castTo<MemberNode>();

	moduleNameOut = {};
	if ((t = peekToken())->tokenId == TokenId::ModuleKeyword) {
		nextToken();

		IdRefPtr moduleName;

		if ((syntaxError = parseIdRef(moduleName))) {
			if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
				return genOutOfMemorySyntaxError();
			syntaxError.reset();
		}

		Token *semicolonToken;
		if ((syntaxError = expectToken((semicolonToken = peekToken()), TokenId::Semicolon))) {
			return syntaxError;
		}
		nextToken();

		moduleNameOut = std::move(moduleName);
	}

	while ((t = peekToken())->tokenId != TokenId::End) {
		if ((syntaxError = parseProgramStmt())) {
			// Parse the rest to make sure that we have gained all of the information,
			// instead of ignoring them.
			if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
				return genOutOfMemorySyntaxError();
			syntaxError.reset();
		}
	}

	initialMod->setParser(sharedFromThis());

	return {};
}
