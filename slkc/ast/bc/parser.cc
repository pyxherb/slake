#include "parser.h"

using namespace slkc;
using namespace slkc::bc;

SLKC_API BCParser::BCParser(peff::SharedPtr<Document> document, TokenList &&tokenList, peff::Alloc *resourceAllocator) : Parser(document, std::move(tokenList), resourceAllocator) {
}

SLKC_API BCParser::~BCParser() {
}

SLKC_API peff::Option<SyntaxError> BCParser::parseOperatorName(std::string_view &nameOut) {
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

SLKC_API peff::Option<SyntaxError> BCParser::parseIdName(peff::String &nameOut) {
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

SLKC_API peff::Option<SyntaxError> BCParser::parseIdRef(IdRefPtr &idRefOut) {
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

SLKC_API peff::Option<SyntaxError> BCParser::parseComptimeExpr(AstNodePtr<ExprNode> &exprOut) {
	peff::Option<SyntaxError> syntaxError;
	Token *token = peekToken();

	peff::OneshotScopeGuard setTokenRangeGuard([this, &exprOut, token]() noexcept {
		if (exprOut)
			exprOut->tokenRange = TokenRange{ document->mainModule, token->index, parseContext.idxPrevToken };
	});

	switch (token->tokenId) {
		case TokenId::ThisKeyword:
		case TokenId::ScopeOp:
		case TokenId::Id: {
			IdRefPtr idRefPtr;
			if ((syntaxError = parseIdRef(idRefPtr)))
				goto genBadExpr;
			if (!(exprOut = makeAstNode<IdRefExprNode>(resourceAllocator.get(), resourceAllocator.get(), document, std::move(idRefPtr)).castTo<ExprNode>()))
				return genOutOfMemorySyntaxError();
			break;
		}
		case TokenId::I8TypeName:
		case TokenId::I16TypeName:
		case TokenId::I32TypeName:
		case TokenId::I64TypeName:
		case TokenId::U8TypeName:
		case TokenId::U16TypeName:
		case TokenId::U32TypeName:
		case TokenId::U64TypeName:
		case TokenId::F32TypeName:
		case TokenId::F64TypeName:
		case TokenId::StringTypeName:
		case TokenId::BoolTypeName:
		case TokenId::ObjectTypeName:
		case TokenId::AnyTypeName:
		case TokenId::VoidTypeName:
		case TokenId::SIMDTypeName: {
			AstNodePtr<TypeNameNode> tn;
			if ((syntaxError = parseTypeName(tn)))
				goto genBadExpr;
			if (!(exprOut = makeAstNode<TypeNameExprNode>(resourceAllocator.get(), resourceAllocator.get(), document, tn).castTo<ExprNode>()))
				return genOutOfMemorySyntaxError();
			break;
		}
		case TokenId::TypenameKeyword: {
			nextToken();
			AstNodePtr<TypeNameNode> tn;
			if ((syntaxError = parseTypeName(tn)))
				goto genBadExpr;
			if (!(exprOut = makeAstNode<TypeNameExprNode>(resourceAllocator.get(), resourceAllocator.get(), document, tn).castTo<ExprNode>()))
				return genOutOfMemorySyntaxError();
			break;
		}
		case TokenId::IntLiteral: {
			nextToken();
			if (!(exprOut = peff::makeSharedWithControlBlock<I32LiteralExprNode, AstNodeControlBlock<I32LiteralExprNode>>(
					  resourceAllocator.get(), resourceAllocator.get(), document,
					  ((IntTokenExtension *)token->exData.get())->data)
						.castTo<ExprNode>()))
				return genOutOfMemorySyntaxError();
			break;
		}
		case TokenId::LongLiteral: {
			nextToken();
			if (!(exprOut = peff::makeSharedWithControlBlock<I64LiteralExprNode, AstNodeControlBlock<I64LiteralExprNode>>(
					  resourceAllocator.get(), resourceAllocator.get(), document,
					  ((LongTokenExtension *)token->exData.get())->data)
						.castTo<ExprNode>()))
				return genOutOfMemorySyntaxError();
			break;
		}
		case TokenId::UIntLiteral: {
			nextToken();
			if (!(exprOut = peff::makeSharedWithControlBlock<U32LiteralExprNode, AstNodeControlBlock<U32LiteralExprNode>>(
					  resourceAllocator.get(), resourceAllocator.get(), document,
					  ((UIntTokenExtension *)token->exData.get())->data)
						.castTo<ExprNode>()))
				return genOutOfMemorySyntaxError();
			break;
		}
		case TokenId::ULongLiteral: {
			nextToken();
			if (!(exprOut = peff::makeSharedWithControlBlock<U64LiteralExprNode, AstNodeControlBlock<U64LiteralExprNode>>(
					  resourceAllocator.get(), resourceAllocator.get(), document,
					  ((ULongTokenExtension *)token->exData.get())->data)
						.castTo<ExprNode>()))
				return genOutOfMemorySyntaxError();
			break;
		}
		case TokenId::StringLiteral: {
			nextToken();
			peff::String s(resourceAllocator.get());

			if (!s.build(((StringTokenExtension *)token->exData.get())->data)) {
				return genOutOfMemorySyntaxError();
			}

			if (!(exprOut = makeAstNode<StringLiteralExprNode>(
					  resourceAllocator.get(), resourceAllocator.get(), document,
					  std::move(s))
						.castTo<ExprNode>()))
				return genOutOfMemorySyntaxError();
			break;
		}
		case TokenId::F32Literal: {
			nextToken();
			if (!(exprOut = peff::makeSharedWithControlBlock<F32LiteralExprNode, AstNodeControlBlock<F32LiteralExprNode>>(
					  resourceAllocator.get(), resourceAllocator.get(), document,
					  ((F32TokenExtension *)token->exData.get())->data)
						.castTo<ExprNode>()))
				return genOutOfMemorySyntaxError();
			break;
		}
		case TokenId::F64Literal: {
			nextToken();
			if (!(exprOut = peff::makeSharedWithControlBlock<F64LiteralExprNode, AstNodeControlBlock<F64LiteralExprNode>>(
					  resourceAllocator.get(), resourceAllocator.get(), document,
					  ((F64TokenExtension *)token->exData.get())->data)
						.castTo<ExprNode>()))
				return genOutOfMemorySyntaxError();
			break;
		}
		case TokenId::TrueKeyword: {
			nextToken();
			if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
					  resourceAllocator.get(), resourceAllocator.get(), document,
					  true)
						.castTo<ExprNode>()))
				return genOutOfMemorySyntaxError();
			break;
		}
		case TokenId::FalseKeyword: {
			nextToken();
			if (!(exprOut = makeAstNode<BoolLiteralExprNode>(
					  resourceAllocator.get(), resourceAllocator.get(), document,
					  false)
						.castTo<ExprNode>()))
				return genOutOfMemorySyntaxError();
			break;
		}
		case TokenId::NullKeyword: {
			nextToken();
			if (!(exprOut = makeAstNode<NullLiteralExprNode>(
					  resourceAllocator.get(), resourceAllocator.get(), document)
						.castTo<ExprNode>()))
				return genOutOfMemorySyntaxError();
			break;
		}
		case TokenId::ModOp: {
			nextToken();

			Token *regToken;
			if ((syntaxError = expectToken(regToken = peekToken(), TokenId::IntLiteral))) {
				return syntaxError;
			}
			nextToken();

			if (!(exprOut = makeAstNode<RegIndexExprNode>(
					  resourceAllocator.get(), resourceAllocator.get(), document,
					  ((IntTokenExtension *)regToken->exData.get())->data, AstNodePtr<TypeNameNode>{})
						.castTo<ExprNode>()))
				return genOutOfMemorySyntaxError();
			break;
		}
		case TokenId::HashTag: {
			nextToken();

			Token *labelToken;
			if ((syntaxError = expectToken(labelToken = peekToken(), TokenId::Id))) {
				return syntaxError;
			}
			nextToken();

			peff::String s(resourceAllocator.get());

			if (!s.build(labelToken->sourceText)) {
				return genOutOfMemorySyntaxError();
			}

			if (!(exprOut = makeAstNode<BCLabelExprNode>(
					  resourceAllocator.get(), resourceAllocator.get(), document,
					  std::move(s))
						.castTo<ExprNode>()))
				return genOutOfMemorySyntaxError();
			break;
		}
		default:
			return SyntaxError(TokenRange{ document->mainModule, token->index }, SyntaxErrorKind::UnexpectedToken);
	}

	return {};
genBadExpr:
	if (!(exprOut = makeAstNode<BadExprNode>(resourceAllocator.get(), resourceAllocator.get(), document, exprOut).castTo<ExprNode>()))
		return genOutOfMemorySyntaxError();
	exprOut->tokenRange = { document->mainModule, token->index, parseContext.idxCurrentToken };
	return syntaxError;
}

SLKC_API peff::Option<SyntaxError> BCParser::parseStmt(AstNodePtr<BCStmtNode> &stmtOut) {
	peff::Option<SyntaxError> syntaxError;

	Token *lParentheseToken = nullptr, *lineToken = nullptr, *columnToken = nullptr, *rParentheseToken = nullptr;

	if ((lParentheseToken = peekToken())->tokenId == TokenId::LParenthese) {
		if ((syntaxError = expectToken(lineToken = peekToken(), TokenId::IntLiteral))) {
			return syntaxError;
		}
		nextToken();

		if ((syntaxError = expectToken(lineToken = peekToken(), TokenId::IntLiteral))) {
			return syntaxError;
		}
		nextToken();

		if ((syntaxError = expectToken(rParentheseToken = peekToken(), TokenId::RParenthese))) {
			return syntaxError;
		}
		nextToken();
	}

	peff::Option<uint32_t> regOut;

	Token *token = peekToken();
	switch (token->tokenId) {
		case TokenId::ModOp: {
			nextToken();

			Token *regIdToken;

			if ((syntaxError = expectToken(regIdToken = peekToken(), TokenId::IntLiteral))) {
				return syntaxError;
			}
			nextToken();

			regOut = ((IntTokenExtension *)regIdToken->exData.get())->data;

			if ((syntaxError = expectToken(rParentheseToken = peekToken(), TokenId::AssignOp))) {
				return syntaxError;
			}
			nextToken();
		}
			[[fallthrough]];
		case TokenId::Id: {
			Token *mnemonicToken = nextToken();

			peff::OneshotScopeGuard setTokenRangeGuard([this, &stmtOut, lParentheseToken, token]() noexcept {
				if (stmtOut)
					stmtOut->tokenRange = TokenRange{ document->mainModule, lParentheseToken ? lParentheseToken->index : token->index, parseContext.idxPrevToken };
			});

			if (!regOut.hasValue()) {
				if (peekToken()->tokenId == TokenId::Colon) {
					AstNodePtr<LabelBCStmtNode> labelStmt;

					if (!(labelStmt = makeAstNode<LabelBCStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemorySyntaxError();

					nextToken();
					stmtOut = labelStmt.castTo<BCStmtNode>();

					if (!labelStmt->name.build(mnemonicToken->sourceText))
						return genOutOfMemorySyntaxError();
					break;
				}
			}

			AstNodePtr<InstructionBCStmtNode> insStmt;

			if (!(insStmt = makeAstNode<InstructionBCStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document)))
				return genOutOfMemorySyntaxError();

			stmtOut = insStmt.castTo<BCStmtNode>();

			if (regOut.hasValue())
				insStmt->regOut = regOut.value();

			if(!insStmt->mnemonic.build(mnemonicToken->sourceText))
				return genOutOfMemorySyntaxError();

			while (true) {
				if (peekToken()->tokenId == TokenId::Semicolon) {
					nextToken();
					break;
				}

				AstNodePtr<ExprNode> arg;

				if (auto e = parseComptimeExpr(arg); e)
					return e;

				if (!insStmt->operands.pushBack(std::move(arg)))
					return genOutOfMemorySyntaxError();

				if (peekToken()->tokenId != TokenId::Comma) {
					break;
				}

				Token *commaToken = nextToken();
			}

			break;
		}
		default:
			nextToken();
			return SyntaxError(TokenRange{ document->mainModule, token->index }, SyntaxErrorKind::UnexpectedToken);
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> BCParser::parseFn(AstNodePtr<BCFnOverloadingNode> &fnNodeOut) {
	peff::Option<SyntaxError> syntaxError;

	Token *fnToken;
	Token *lvalueMarkerToken = nullptr;

	peff::String name(resourceAllocator.get());

	if (!(fnNodeOut = makeAstNode<BCFnOverloadingNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
		return genOutOfMemorySyntaxError();
	}

	switch ((fnToken = peekToken())->tokenId) {
		case TokenId::FnKeyword: {
			nextToken();

			fnNodeOut->overloadingKind = BCFnOverloadingKind::Regular;

			if ((syntaxError = parseIdName(name))) {
				return syntaxError;
			}
			break;
		}
		case TokenId::AsyncKeyword: {
			nextToken();

			fnNodeOut->overloadingKind = BCFnOverloadingKind::Coroutine;

			if ((syntaxError = parseIdName(name))) {
				return syntaxError;
			}
			break;
		}
		case TokenId::OperatorKeyword: {
			nextToken();

			fnNodeOut->overloadingKind = BCFnOverloadingKind::Regular;

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

			fnNodeOut->overloadingKind = BCFnOverloadingKind::Pure;

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

	bool hasVarArg = false;
	if ((syntaxError = parseParams(fnNodeOut->params, hasVarArg, fnNodeOut->idxParamCommaTokens, fnNodeOut->lParentheseIndex, fnNodeOut->rParentheseIndex))) {
		return syntaxError;
	}
	if (hasVarArg) {
		fnNodeOut->fnFlags |= FN_VARG;
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
	}

	Token *bodyToken = peekToken();

	switch (bodyToken->tokenId) {
		case TokenId::Semicolon: {
			nextToken();

			break;
		}
		case TokenId::LBrace: {
			nextToken();

			AstNodePtr<BCStmtNode> curStmt;

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
					if (!fnNodeOut->body.pushBack(std::move(curStmt))) {
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

SLKC_API peff::Option<SyntaxError> BCParser::parseProgramStmt() {
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
			AstNodePtr<BCFnOverloadingNode> fn;

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
				BCFnNode *fnSlot = (BCFnNode *)p->members.at(it.value()).get();
				fn->setParent(fnSlot);
				if (!fnSlot->overloadings.pushBack(std::move(fn))) {
					return genOutOfMemorySyntaxError();
				}
			} else {
				AstNodePtr<BCFnNode> fnSlot;

				if (!(fnSlot = makeAstNode<BCFnNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
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
			/*
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

			peff::ScopeGuard setTokenRangeGuard([this, token, stmt]() noexcept {
				stmt->tokenRange = TokenRange{ document->mainModule, token->index, parseContext.idxPrevToken };
			});

			if ((syntaxError = parseVarDefs(stmt->varDefEntries))) {
				return syntaxError;
			}

			if (!p->varDefStmts.pushBack(std::move(stmt))) {
				return genOutOfMemorySyntaxError();
			}

			Token *semicolonToken;

			if ((syntaxError = expectToken((semicolonToken = peekToken()), TokenId::Semicolon))) {
				return syntaxError;
			}

			nextToken();

			break;
		}*/
		default:
			nextToken();
			return SyntaxError(
				TokenRange{ document->mainModule, token->index },
				SyntaxErrorKind::ExpectingDecl);
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> BCParser::parseProgram(const AstNodePtr<ModuleNode> &initialMod, IdRefPtr &moduleNameOut) {
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
