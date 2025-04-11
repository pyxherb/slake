#include "parser.h"

using namespace slkc;

SLKC_API Parser::Parser(peff::SharedPtr<Document> document, TokenList &&tokenList, peff::Alloc *selfAllocator, peff::Alloc *resourceAllocator) : document(document), tokenList(std::move(tokenList)), selfAllocator(selfAllocator), resourceAllocator(resourceAllocator), syntaxErrors(resourceAllocator) {
}

SLKC_API Parser::~Parser() {
}

SLKC_API std::optional<SyntaxError> Parser::parseOperatorName(std::string_view &nameOut) {
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
		case TokenId::NewKeyword:
			nameOut = "new";
			nextToken();
			break;
		case TokenId::DeleteKeyword:
			nameOut = "delete";
			nextToken();
			break;
		default:
			return SyntaxError(TokenRange{ t->index }, SyntaxErrorKind::ExpectingOperatorName);
	}
	return {};
}

SLKC_API std::optional<SyntaxError> Parser::parseIdName(peff::String &nameOut) {
	std::optional<SyntaxError> syntaxError;
	Token *t = peekToken();

	switch (t->tokenId) {
		case TokenId::Id:
			if (!nameOut.build(t->sourceText)) {
				return genOutOfMemoryError();
			}
			nextToken();
			break;
		default:
			return SyntaxError(TokenRange{ t->index }, SyntaxErrorKind::ExpectingId);
	}
	return {};
}

SLKC_API std::optional<SyntaxError> Parser::parseIdRef(IdRefPtr &idRefOut) {
	std::optional<SyntaxError> syntaxError;
	IdRefPtr idRefPtr(peff::allocAndConstruct<IdRef>(resourceAllocator.get(), ASTNODE_ALIGNMENT, resourceAllocator.get(), document));
	if (!idRefPtr)
		return genOutOfMemoryError();
	Token *t = peekToken();

	idRefPtr->tokenRange = TokenRange{ t->index };

	if (t->tokenId == TokenId::ThisKeyword) {
		nextToken();

		IdRefEntry entry(resourceAllocator.get());
		peff::String idText(resourceAllocator.get());
		if (!idText.build("this")) {
			return genOutOfMemoryError();
		}

		entry.name = std::move(idText);
		entry.nameTokenIndex = t->index;

		if (!idRefPtr->entries.pushBack(std::move(entry)))
			return genOutOfMemoryError();

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
			return genOutOfMemoryError();
	}

	for (;;) {
		if ((syntaxError = expectToken(t = peekToken(), TokenId::Id)))
			return syntaxError;

		nextToken();

		IdRefEntry entry(resourceAllocator.get());
		peff::String idText(resourceAllocator.get());
		if (!idText.build(t->sourceText)) {
			return genOutOfMemoryError();
		}

		entry.name = std::move(idText);
		entry.nameTokenIndex = t->index;
		idRefPtr->tokenRange.endIndex = t->index;

		size_t prevEndIndex = t->index;
		ParseContext prevParseContext = parseContext;
		if ((t = peekToken())->tokenId == TokenId::LtOp) {
			nextToken();

			for (;;) {
				peff::SharedPtr<TypeNameNode> genericArg;
				if ((syntaxError = parseTypeName(genericArg)))
					goto genericArgParseFail;

				if (!entry.genericArgs.pushBack(std::move(genericArg))) {
					return genOutOfMemoryError();
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

	succeeded:
		if (!idRefPtr->entries.pushBack(std::move(entry)))
			return genOutOfMemoryError();

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

[[nodiscard]] SLKC_API std::optional<SyntaxError> Parser::parseArgs(peff::DynArray<peff::SharedPtr<ExprNode>> &argsOut, peff::DynArray<size_t> &idxCommaTokensOut) {
	while (true) {
		if (peekToken()->tokenId == TokenId::RParenthese) {
			break;
		}

		peff::SharedPtr<ExprNode> arg;

		if (auto e = parseExpr(0, arg); e)
			return e;

		if (!argsOut.pushBack(std::move(arg)))
			return genOutOfMemoryError();

		if (peekToken()->tokenId != TokenId::Comma) {
			break;
		}

		Token *commaToken = nextToken();
		if (!idxCommaTokensOut.pushBack(+commaToken->index))
			return genOutOfMemoryError();
	}

	return {};
}

SLKC_API std::optional<SyntaxError> Parser::parseFn(peff::SharedPtr<FnNode> &fnNodeOut) {
	std::optional<SyntaxError> syntaxError;

	FnFlags initialFlags = 0;
	Token *fnToken;

	peff::String name(resourceAllocator.get());

	switch ((fnToken = peekToken())->tokenId) {
		case TokenId::FnKeyword: {
			nextToken();

			Token *generatorMarkerToken;
			if ((generatorMarkerToken = peekToken())->tokenId == TokenId::MulOp) {
				nextToken();
			}

			if ((syntaxError = parseIdName(name))) {
				return syntaxError;
			}
			break;
		}
		case TokenId::OperatorKeyword: {
			nextToken();

			std::string_view operatorName;
			if ((syntaxError = parseOperatorName(operatorName))) {
				return syntaxError;
			}

			if (!name.build(operatorName)) {
				return genOutOfMemoryError();
			}
			break;
		}
		default:
			return SyntaxError(TokenRange{ fnToken->index }, SyntaxErrorKind::UnexpectedToken);
	}

	if (!(fnNodeOut = peff::makeShared<FnNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
		return genOutOfMemoryError();
	}

	peff::SharedPtr<MemberNode> prevParent = curParent;
	peff::ScopeGuard restoreParentGuard([this, prevParent]() noexcept {
		curParent = prevParent;
	});
	curParent = fnNodeOut.castTo<MemberNode>();

	peff::ScopeGuard setTokenRangeGuard([this, fnToken, fnNodeOut]() noexcept {
		fnNodeOut->tokenRange = TokenRange{ fnToken->index, parseContext.idxPrevToken };
	});

	fnNodeOut->name = std::move(name);

	fnNodeOut->fnFlags = initialFlags;

	if ((syntaxError = parseGenericParams(fnNodeOut->genericParams, fnNodeOut->idxGenericParamCommaTokens, fnNodeOut->lAngleBracketIndex, fnNodeOut->rAngleBracketIndex))) {
		return syntaxError;
	}

	if ((syntaxError = parseParams(fnNodeOut->params, fnNodeOut->idxParamCommaTokens, fnNodeOut->lParentheseIndex, fnNodeOut->rParentheseIndex))) {
		return syntaxError;
	}

	Token *colonToken;
	if ((colonToken = peekToken())->tokenId == TokenId::Colon) {
		nextToken();
		if ((syntaxError = parseTypeName(fnNodeOut->returnType))) {
			return syntaxError;
		}
	}

	Token *virtualToken;
	if ((virtualToken = peekToken())->tokenId == TokenId::VirtualKeyword) {
		nextToken();
	}

	Token *overrideToken;
	if ((overrideToken = peekToken())->tokenId == TokenId::OverrideKeyword) {
		nextToken();
	}

	Token *bodyToken = peekToken();

	switch (bodyToken->tokenId) {
		case TokenId::Semicolon: {
			nextToken();

			break;
		}
		case TokenId::LBrace: {
			nextToken();

			peff::SharedPtr<StmtNode> curStmt;

			if (!(fnNodeOut->body = peff::makeShared<CodeBlockStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
				return genOutOfMemoryError();
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
						return genOutOfMemoryError();
				}

				if (curStmt) {
					if (!fnNodeOut->body->body.pushBack(std::move(curStmt))) {
						return genOutOfMemoryError();
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
				TokenRange{ bodyToken->index },
				SyntaxErrorKind::UnexpectedToken);
	}

	return {};
}

SLKC_API std::optional<SyntaxError> Parser::parseClassStmts() {
	std::optional<SyntaxError> syntaxError;

	Token *t;

	while ((t = peekToken())->tokenId != TokenId::RBrace) {
		if ((syntaxError = parseProgramStmt())) {
			// Parse the rest to make sure that we have gained all of the information,
			// instead of ignoring them.
			if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
				return genOutOfMemoryError();
			syntaxError.reset();
		}
	}

	return {};
}

SLKC_API std::optional<SyntaxError> Parser::parseProgramStmt() {
	std::optional<SyntaxError> syntaxError;

	peff::DynArray<peff::SharedPtr<AttributeNode>> attributes(resourceAllocator.get());

	if ((syntaxError = parseAttributes(attributes))) {
		return syntaxError;
	}

	Token *currentToken;

	for (;;) {
		switch ((currentToken = peekToken())->tokenId) {
			case TokenId::PubKeyword:
				nextToken();
				break;
			case TokenId::StaticKeyword:
				nextToken();
				break;
			case TokenId::NativeKeyword:
				nextToken();
				break;
			default:
				goto accessModifierParseEnd;
		}
	}

accessModifierParseEnd:
	Token *token = peekToken();

	peff::SharedPtr<ModuleNode> p = curParent.castTo<ModuleNode>();

	switch (token->tokenId) {
		case TokenId::FnKeyword:
		case TokenId::OperatorKeyword: {
			// Function.
			peff::SharedPtr<FnNode> fn;

			if ((syntaxError = parseFn(fn))) {
				return syntaxError;
			}

			if (auto it = p->members.find(fn->name); it != p->members.end()) {
				if (it.value()->astNodeType != AstNodeType::FnSlot) {
					peff::String s(resourceAllocator.get());

					if (!s.build(fn->name)) {
						return genOutOfMemoryError();
					}

					ConflictingDefinitionsErrorExData exData(std::move(s));

					return SyntaxError(fn->tokenRange, std::move(exData));
				}
				FnSlotNode *fnSlot = (FnSlotNode *)it.value();
				if (!fnSlot->overloadings.pushBack(std::move(fn))) {
					return genOutOfMemoryError();
				}
			} else {
				peff::SharedPtr<FnSlotNode> fnSlot;

				if (!(fnSlot = peff::makeShared<FnSlotNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
					return genOutOfMemoryError();
				}

				if (!fnSlot->name.build(fn->name)) {
					return genOutOfMemoryError();
				}

				if (!(p->members.insert(fnSlot->name, std::move(fnSlot.castTo<MemberNode>())))) {
					return genOutOfMemoryError();
				}

				if (!fnSlot->overloadings.pushBack(std::move(fn))) {
					return genOutOfMemoryError();
				}
			}
			break;
		}
		case TokenId::ClassKeyword: {
			// Class.
			nextToken();

			peff::SharedPtr<ClassNode> classNode;

			if (!(classNode = peff::makeShared<ClassNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
				return genOutOfMemoryError();
			}

			Token *nameToken;

			if ((syntaxError = expectToken((nameToken = peekToken()), TokenId::Id))) {
				return syntaxError;
			}

			nextToken();

			if (!classNode->name.build(nameToken->sourceText)) {
				return genOutOfMemoryError();
			}

			{
				peff::ScopeGuard setTokenRangeGuard([this, token, classNode]() noexcept {
					classNode->tokenRange = TokenRange{ token->index, parseContext.idxPrevToken };
				});

				peff::SharedPtr<MemberNode> prevParent;
				prevParent = curParent;
				peff::ScopeGuard restorePrevModGuard([this, prevParent]() noexcept {
					curParent = prevParent;
				});
				curParent = classNode.castTo<MemberNode>();

				if ((syntaxError = parseGenericParams(classNode->genericParams, classNode->idxGenericParamCommaTokens, classNode->idxLAngleBracketToken, classNode->idxRAngleBracketToken))) {
					return syntaxError;
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
						peff::SharedPtr<TypeNameNode> tn;

						if ((syntaxError = parseTypeName(tn))) {
							return syntaxError;
						}

						if (!classNode->implementedTypes.pushBack(std::move(tn))) {
							return genOutOfMemoryError();
						}

						if (peekToken()->tokenId != TokenId::OrOp) {
							break;
						}

						Token *orOpToken = nextToken();
					}

					break;
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
							return genOutOfMemoryError();
						syntaxError.reset();
					}
				}

				Token *rBraceToken;

				if ((syntaxError = expectToken((rBraceToken = peekToken()), TokenId::RBrace))) {
					return syntaxError;
				}

				nextToken();
			}

			if (auto it = p->members.find(classNode->name); it != p->members.end()) {
				peff::String s(resourceAllocator.get());

				if (!s.build(classNode->name)) {
					return genOutOfMemoryError();
				}

				ConflictingDefinitionsErrorExData exData(std::move(s));

				return SyntaxError(classNode->tokenRange, std::move(exData));
			} else {
				if (!(p->members.insert(classNode->name, std::move(classNode.castTo<MemberNode>())))) {
					return genOutOfMemoryError();
				}
			}

			break;
		}
		case TokenId::InterfaceKeyword: {
			// Interface.
			nextToken();

			peff::SharedPtr<InterfaceNode> interfaceNode;

			if (!(interfaceNode = peff::makeShared<InterfaceNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
				return genOutOfMemoryError();
			}

			Token *nameToken;

			if ((syntaxError = expectToken((nameToken = peekToken()), TokenId::Id))) {
				return syntaxError;
			}

			if (!interfaceNode->name.build(nameToken->sourceText)) {
				return genOutOfMemoryError();
			}

			nextToken();

			Token *t;

			{
				peff::SharedPtr<MemberNode> prevMember;
				prevMember = curParent;
				peff::ScopeGuard restorePrevModGuard([this, prevMember]() noexcept {
					curParent = prevMember;
				});
				curParent = interfaceNode.castTo<MemberNode>();

				if ((syntaxError = parseGenericParams(interfaceNode->genericParams, interfaceNode->idxGenericParamCommaTokens, interfaceNode->idxLAngleBracketToken, interfaceNode->idxRAngleBracketToken))) {
					return syntaxError;
				}

				if (Token *colonToken = peekToken(); colonToken->tokenId == TokenId::Colon) {
					nextToken();

					while (true) {
						peff::SharedPtr<TypeNameNode> tn;

						if ((syntaxError = parseTypeName(tn))) {
							return syntaxError;
						}

						if (!interfaceNode->implementedTypes.pushBack(std::move(tn))) {
							return genOutOfMemoryError();
						}

						if (peekToken()->tokenId != TokenId::OrOp) {
							break;
						}

						Token *orOpToken = nextToken();
					}

					break;
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
							return genOutOfMemoryError();
						syntaxError.reset();
					}
				}

				Token *rBraceToken;

				if ((syntaxError = expectToken((rBraceToken = peekToken()), TokenId::RBrace))) {
					return syntaxError;
				}

				nextToken();
			}

			if (auto it = p->members.find(interfaceNode->name); it != p->members.end()) {
				peff::String s(resourceAllocator.get());

				if (!s.build(interfaceNode->name)) {
					return genOutOfMemoryError();
				}

				ConflictingDefinitionsErrorExData exData(std::move(s));

				return SyntaxError(interfaceNode->tokenRange, std::move(exData));
			} else {
				if (!(p->members.insert(interfaceNode->name, std::move(interfaceNode.castTo<MemberNode>())))) {
					return genOutOfMemoryError();
				}
			}

			break;
		}
		case TokenId::ImportKeyword: {
			// Import item.
			nextToken();

			IdRefPtr sourceIdRef;

			if ((syntaxError = parseIdRef(sourceIdRef)))
				return syntaxError;

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

			peff::SharedPtr<VarDefStmtNode> stmt;

			if (!(stmt = peff::makeShared<VarDefStmtNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(),
					  document,
					  peff::DynArray<VarDefEntryPtr>(resourceAllocator.get())))) {
				return genOutOfMemoryError();
			}

			if ((syntaxError = parseVarDefs(stmt->varDefEntries))) {
				return syntaxError;
			}

			if (!p->varDefStmts.pushBack(std::move(stmt))) {
				return genOutOfMemoryError();
			}

			Token *semicolonToken;

			if ((syntaxError = expectToken((semicolonToken = peekToken()), TokenId::Semicolon))) {
				return syntaxError;
			}

			nextToken();

			break;
		}
		default:
			nextToken();
			return SyntaxError(
				TokenRange{ token->index },
				SyntaxErrorKind::ExpectingDecl);
	}

	return {};
}

SLKC_API std::optional<SyntaxError> Parser::parseProgram(const peff::SharedPtr<ModuleNode> &initialMod) {
	std::optional<SyntaxError> syntaxError;

	Token *t;

	curParent = initialMod.castTo<MemberNode>();

	if ((t = peekToken())->tokenId == TokenId::ModuleKeyword) {
		nextToken();

		IdRefPtr moduleName;

		if ((syntaxError = parseIdRef(moduleName))) {
			if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
				return genOutOfMemoryError();
			syntaxError.reset();
		}

		Token *semicolonToken;
		if ((syntaxError = expectToken((semicolonToken = peekToken()), TokenId::Semicolon))) {
			return syntaxError;
		}
		nextToken();
	}

	while ((t = peekToken())->tokenId != TokenId::End) {
		if ((syntaxError = parseProgramStmt())) {
			// Parse the rest to make sure that we have gained all of the information,
			// instead of ignoring them.
			if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
				return genOutOfMemoryError();
			syntaxError.reset();
		}
	}

	return {};
}
