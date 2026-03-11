#include "../parser.h"

using namespace slkc;

SLKC_API peff::Option<SyntaxError> Parser::parseVarDefs(peff::DynArray<VarDefEntryPtr> &varDefEntries) {
	Token *currentToken;
	peff::Option<SyntaxError> syntaxError;

	for (;;) {
		peff::DynArray<AstNodePtr<AttributeNode>> attributes(resourceAllocator.get());

		SLKC_RETURN_IF_PARSE_ERROR(parseAttributes(attributes));

		if ((syntaxError = expectToken((currentToken = nextToken()), TokenId::Id))) {
			if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
				return genOutOfMemorySyntaxError();
			syntaxError.reset();
			if (!syntaxErrors.pushBack(SyntaxError({ document->mainModule, currentToken->index }, SyntaxErrorKind::ExpectingId)))
				return genOutOfMemorySyntaxError();
		}

		VarDefEntryPtr entryPtr(
			peff::allocAndConstruct<VarDefEntry>(
				resourceAllocator.get(),
				ASTNODE_ALIGNMENT,
				resourceAllocator.get()));
		if (!entryPtr) {
			return genOutOfMemorySyntaxError();
		}

		VarDefEntry *entry = entryPtr.get();

		if (!varDefEntries.pushBack(std::move(entryPtr))) {
			return genOutOfMemorySyntaxError();
		}

		entry->idxNameToken = currentToken->index;
		if (!entry->name.build(currentToken->sourceText)) {
			return genOutOfMemorySyntaxError();
		}

		if ((currentToken = peekToken())->tokenId == TokenId::Colon) {
			nextToken();

			if ((syntaxError = parseTypeName(entry->type))) {
				if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
					return genOutOfMemorySyntaxError();
				syntaxError.reset();
			}
		}

		if ((currentToken = peekToken())->tokenId == TokenId::AssignOp) {
			nextToken();

			if ((syntaxError = parseExpr(0, entry->initialValue))) {
				if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
					return genOutOfMemorySyntaxError();
				syntaxError.reset();
			}
		}

		entry->attributes = std::move(attributes);

		if ((currentToken = peekToken())->tokenId != TokenId::Comma) {
			break;
		}

		nextToken();
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseIfStmt(AstNodePtr<StmtNode> &stmtOut) {
	peff::Option<SyntaxError> syntaxError;

	nextToken();

	AstNodePtr<IfStmtNode> ifStmt;

	if (!(ifStmt = makeAstNode<IfStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
		return genOutOfMemorySyntaxError();
	}

	stmtOut = ifStmt.castTo<StmtNode>();

	Token *lParentheseToken = peekToken();

	SLKC_RETURN_IF_PARSE_ERROR(expectToken(lParentheseToken, TokenId::LParenthese));

	nextToken();

	{
		static TokenId skippingTerminativeToken[] = {
			TokenId::RParenthese,
			TokenId::Semicolon,
			TokenId::RBrace
		};

		if ((syntaxError = parseExpr(0, ifStmt->cond))) {
			SLKC_RETURN_IF_PARSE_ERROR(lookaheadUntil(std::size(skippingTerminativeToken), skippingTerminativeToken));
			return syntaxError;
		}
	}

	Token *rParentheseToken = peekToken();

	SLKC_RETURN_IF_PARSE_ERROR(expectToken(rParentheseToken, TokenId::RParenthese));

	nextToken();

	SLKC_RETURN_IF_PARSE_ERROR(parseStmt(ifStmt->trueBody));

	Token *elseToken = peekToken();

	if (elseToken->tokenId == TokenId::ElseKeyword) {
		nextToken();

		SLKC_RETURN_IF_PARSE_ERROR(parseStmt(ifStmt->falseBody));
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseWithStmt(AstNodePtr<StmtNode> &stmtOut) {
	peff::Option<SyntaxError> syntaxError;

	nextToken();

	AstNodePtr<WithStmtNode> withStmt;

	if (!(withStmt = makeAstNode<WithStmtNode>(
			  resourceAllocator.get(),
			  resourceAllocator.get(),
			  document))) {
		return genOutOfMemorySyntaxError();
	}

	stmtOut = withStmt.castTo<StmtNode>();

	WithConstraintEntryPtr entry;

	while (true) {
		if (!(entry = WithConstraintEntryPtr(peff::allocAndConstruct<WithConstraintEntry>(resourceAllocator.get(), alignof(WithConstraintEntry), resourceAllocator.get())))) {
			return genOutOfMemorySyntaxError();
		}

		Token *nameToken;

		SLKC_RETURN_IF_PARSE_ERROR(expectToken((nameToken = peekToken()), TokenId::Id));

		if (!entry->genericParamName.build(nameToken->sourceText))
			return genOutOfMemorySyntaxError();

		nextToken();

		SLKC_RETURN_IF_PARSE_ERROR(parseGenericConstraint(entry->constraint));

		if (!withStmt->constraints.pushBack(std::move(entry)))
			return genOutOfMemorySyntaxError();

		if (peekToken()->tokenId != TokenId::Comma) {
			break;
		}

		Token *commaToken = nextToken();

		/*
		if (!idxCommaTokensOut.pushBack(+commaToken->index))
			return genOutOfMemorySyntaxError();*/
	}

	SLKC_RETURN_IF_PARSE_ERROR(parseStmt(withStmt->trueBody));

	Token *elseToken = peekToken();

	if (elseToken->tokenId == TokenId::ElseKeyword) {
		nextToken();

		SLKC_RETURN_IF_PARSE_ERROR(parseStmt(withStmt->falseBody));
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseForStmt(AstNodePtr<StmtNode> &stmtOut) {
	peff::Option<SyntaxError> syntaxError;

	nextToken();

	AstNodePtr<ForStmtNode> forStmt;

	if (!(forStmt = makeAstNode<ForStmtNode>(
			  resourceAllocator.get(),
			  resourceAllocator.get(),
			  document))) {
		return genOutOfMemorySyntaxError();
	}

	stmtOut = forStmt.castTo<StmtNode>();

	Token *lParentheseToken = peekToken();

	SLKC_RETURN_IF_PARSE_ERROR(expectToken(lParentheseToken, TokenId::LParenthese));

	nextToken();

	Token *varDefSeparatorToken;
	Token *condSeparatorToken;
	Token *rParentheseToken;
	{
		static TokenId skippingTerminativeToken[] = {
			TokenId::RParenthese,
			TokenId::Semicolon,
			TokenId::RBrace
		};

		if ((varDefSeparatorToken = peekToken())->tokenId != TokenId::Semicolon) {
			SLKC_RETURN_IF_PARSE_ERROR(parseVarDefs(forStmt->varDefEntries));

			SLKC_RETURN_IF_PARSE_ERROR(expectToken((varDefSeparatorToken = peekToken()), TokenId::Semicolon));
			nextToken();
		} else {
			nextToken();
		}

		if ((condSeparatorToken = peekToken())->tokenId != TokenId::Semicolon) {
			if ((syntaxError = parseExpr(0, forStmt->cond))) {
				SLKC_RETURN_IF_PARSE_ERROR(lookaheadUntil(std::size(skippingTerminativeToken), skippingTerminativeToken));
				return syntaxError;
			}

			SLKC_RETURN_IF_PARSE_ERROR(expectToken((condSeparatorToken = peekToken()), TokenId::Semicolon));
			nextToken();
		} else {
			nextToken();
		}

		if ((rParentheseToken = peekToken())->tokenId != TokenId::RParenthese) {
			if ((syntaxError = parseExpr(-10, forStmt->step))) {
				SLKC_RETURN_IF_PARSE_ERROR(lookaheadUntil(std::size(skippingTerminativeToken), skippingTerminativeToken));
				return syntaxError;
			}

			SLKC_RETURN_IF_PARSE_ERROR(expectToken((rParentheseToken = peekToken()), TokenId::RParenthese));
			nextToken();
		} else {
			nextToken();
		}
	}

	SLKC_RETURN_IF_PARSE_ERROR(parseStmt(forStmt->body));

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseWhileStmt(AstNodePtr<StmtNode> &stmtOut) {
	peff::Option<SyntaxError> syntaxError;

	nextToken();

	AstNodePtr<WhileStmtNode> whileStmt;

	if (!(whileStmt = makeAstNode<WhileStmtNode>(
			  resourceAllocator.get(),
			  resourceAllocator.get(),
			  document))) {
		return genOutOfMemorySyntaxError();
	}

	stmtOut = whileStmt.castTo<StmtNode>();

	Token *lParentheseToken = peekToken();

	SLKC_RETURN_IF_PARSE_ERROR(expectToken(lParentheseToken, TokenId::LParenthese));

	nextToken();

	Token *rParentheseToken;
	{
		static TokenId skippingTerminativeToken[] = {
			TokenId::RParenthese,
			TokenId::Semicolon,
			TokenId::RBrace
		};

		if ((syntaxError = parseExpr(0, whileStmt->cond))) {
			SLKC_RETURN_IF_PARSE_ERROR(lookaheadUntil(std::size(skippingTerminativeToken), skippingTerminativeToken));
			return syntaxError;
		}

		SLKC_RETURN_IF_PARSE_ERROR(expectToken((rParentheseToken = peekToken()), TokenId::RParenthese));

		nextToken();
	}

	SLKC_RETURN_IF_PARSE_ERROR(parseStmt(whileStmt->body));

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseDoWhileStmt(AstNodePtr<StmtNode> &stmtOut) {
	peff::Option<SyntaxError> syntaxError;

	nextToken();

	AstNodePtr<DoWhileStmtNode> whileStmt;

	if (!(whileStmt = makeAstNode<DoWhileStmtNode>(
			  resourceAllocator.get(),
			  resourceAllocator.get(),
			  document))) {
		return genOutOfMemorySyntaxError();
	}

	stmtOut = whileStmt.castTo<StmtNode>();

	SLKC_RETURN_IF_PARSE_ERROR(parseStmt(whileStmt->body));

	Token *whileToken = peekToken();

	SLKC_RETURN_IF_PARSE_ERROR(expectToken(whileToken, TokenId::WhileKeyword));

	nextToken();

	Token *lParentheseToken = peekToken();

	SLKC_RETURN_IF_PARSE_ERROR(expectToken(lParentheseToken, TokenId::LParenthese));

	nextToken();

	Token *rParentheseToken;
	{
		static TokenId skippingTerminativeToken[] = {
			TokenId::RParenthese,
			TokenId::Semicolon,
			TokenId::RBrace
		};

		if ((syntaxError = parseExpr(0, whileStmt->cond))) {
			SLKC_RETURN_IF_PARSE_ERROR(lookaheadUntil(std::size(skippingTerminativeToken), skippingTerminativeToken));
			return syntaxError;
		}

		SLKC_RETURN_IF_PARSE_ERROR(expectToken((rParentheseToken = peekToken()), TokenId::RParenthese));

		nextToken();
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseLetStmt(AstNodePtr<StmtNode> &stmtOut) {
	peff::Option<SyntaxError> syntaxError;

	nextToken();

	AstNodePtr<VarDefStmtNode> stmt;

	if (!(stmt = makeAstNode<VarDefStmtNode>(
			  resourceAllocator.get(),
			  resourceAllocator.get(),
			  document,
			  peff::DynArray<VarDefEntryPtr>(resourceAllocator.get())))) {
		return genOutOfMemorySyntaxError();
	}

	stmtOut = stmt.castTo<StmtNode>();

	SLKC_RETURN_IF_PARSE_ERROR(parseVarDefs(stmt->varDefEntries));

	Token *semicolonToken;

	SLKC_RETURN_IF_PARSE_ERROR(expectToken((semicolonToken = peekToken()), TokenId::Semicolon));

	nextToken();

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseBreakStmt(AstNodePtr<StmtNode> &stmtOut) {
	peff::Option<SyntaxError> syntaxError;

	nextToken();

	AstNodePtr<BreakStmtNode> stmt;

	if (!(stmt = makeAstNode<BreakStmtNode>(
			  resourceAllocator.get(),
			  resourceAllocator.get(),
			  document))) {
		return genOutOfMemorySyntaxError();
	}

	stmtOut = stmt.castTo<StmtNode>();

	Token *semicolonToken;

	SLKC_RETURN_IF_PARSE_ERROR(expectToken((semicolonToken = peekToken()), TokenId::Semicolon));

	nextToken();

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseContinueStmt(AstNodePtr<StmtNode> &stmtOut) {
	peff::Option<SyntaxError> syntaxError;

	nextToken();

	AstNodePtr<ContinueStmtNode> stmt;

	if (!(stmt = makeAstNode<ContinueStmtNode>(
			  resourceAllocator.get(),
			  resourceAllocator.get(),
			  document))) {
		return genOutOfMemorySyntaxError();
	}

	stmtOut = stmt.castTo<StmtNode>();

	Token *semicolonToken;

	SLKC_RETURN_IF_PARSE_ERROR(expectToken((semicolonToken = peekToken()), TokenId::Semicolon));

	nextToken();

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseReturnStmt(AstNodePtr<StmtNode> &stmtOut) {
	peff::Option<SyntaxError> syntaxError;

	nextToken();

	AstNodePtr<ReturnStmtNode> stmt;

	if (!(stmt = makeAstNode<ReturnStmtNode>(
			  resourceAllocator.get(),
			  resourceAllocator.get(),
			  document,
			  AstNodePtr<ExprNode>()))) {
		return genOutOfMemorySyntaxError();
	}

	stmtOut = stmt.castTo<StmtNode>();

	static TokenId skippingTerminativeToken[] = {
		TokenId::RParenthese,
		TokenId::Semicolon,
		TokenId::RBrace
	};

	switch (peekToken()->tokenId) {
		case TokenId::Semicolon:
			nextToken();
			break;
		default:
			if ((syntaxError = parseExpr(0, stmt->value))) {
				SLKC_RETURN_IF_PARSE_ERROR(lookaheadUntil(std::size(skippingTerminativeToken), skippingTerminativeToken));
				return syntaxError;
			}

			SLKC_RETURN_IF_PARSE_ERROR(expectToken(peekToken(), TokenId::Semicolon));

			nextToken();
			break;
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseYieldStmt(AstNodePtr<StmtNode> &stmtOut) {
	peff::Option<SyntaxError> syntaxError;

	nextToken();

	AstNodePtr<YieldStmtNode> stmt;

	if (!(stmt = makeAstNode<YieldStmtNode>(
			  resourceAllocator.get(),
			  resourceAllocator.get(),
			  document,
			  AstNodePtr<ExprNode>()))) {
		return genOutOfMemorySyntaxError();
	}

	stmtOut = stmt.castTo<StmtNode>();

	static TokenId skippingTerminativeToken[] = {
		TokenId::RParenthese,
		TokenId::Semicolon,
		TokenId::RBrace
	};

	switch (peekToken()->tokenId) {
		case TokenId::Semicolon:
			nextToken();
			break;
		default:
			if ((syntaxError = parseExpr(0, stmt->value))) {
				SLKC_RETURN_IF_PARSE_ERROR(lookaheadUntil(std::size(skippingTerminativeToken), skippingTerminativeToken));
				return syntaxError;
			}

			SLKC_RETURN_IF_PARSE_ERROR(expectToken(peekToken(), TokenId::Semicolon));

			nextToken();
			break;
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseLabelStmt(AstNodePtr<StmtNode> &stmtOut) {
	peff::Option<SyntaxError> syntaxError;

	nextToken();

	AstNodePtr<LabelStmtNode> stmt;

	if (!(stmt = makeAstNode<LabelStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
		return genOutOfMemorySyntaxError();
	}

	stmtOut = stmt.castTo<StmtNode>();

	SLKC_RETURN_IF_PARSE_ERROR(expectToken(peekToken(), TokenId::Id));

	Token *nameToken = nextToken();

	if (!stmt->name.build(nameToken->sourceText)) {
		return genOutOfMemorySyntaxError();
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseBlockStmt(AstNodePtr<StmtNode> &stmtOut) {
	peff::Option<SyntaxError> syntaxError;

	nextToken();

	AstNodePtr<CodeBlockStmtNode> stmt;
	AstNodePtr<StmtNode> curStmt;

	if (!(stmt = makeAstNode<CodeBlockStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
		return genOutOfMemorySyntaxError();
	}

	stmtOut = stmt.castTo<StmtNode>();

	while (true) {
		SLKC_RETURN_IF_PARSE_ERROR(expectToken(peekToken()));

		if (peekToken()->tokenId == TokenId::RBrace) {
			break;
		}

		if ((syntaxError = parseStmt(curStmt))) {
			if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
				return genOutOfMemorySyntaxError();
		}

		if (curStmt) {
			if (!stmt->body.pushBack(std::move(curStmt))) {
				return genOutOfMemorySyntaxError();
			}
		}
	}

	Token *rBraceToken;

	SLKC_RETURN_IF_PARSE_ERROR(expectToken((rBraceToken = peekToken()), TokenId::RBrace));

	nextToken();

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseSwitchStmt(AstNodePtr<StmtNode> &stmtOut) {
	peff::Option<SyntaxError> syntaxError;

	nextToken();

	AstNodePtr<SwitchStmtNode> stmt;

	if (!(stmt = makeAstNode<SwitchStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
		return genOutOfMemorySyntaxError();
	}

	stmtOut = stmt.castTo<StmtNode>();

	Token *lParentheseToken = peekToken();

	SLKC_RETURN_IF_PARSE_ERROR(expectToken(lParentheseToken, TokenId::LParenthese));

	nextToken();

	SLKC_RETURN_IF_PARSE_ERROR(parseExpr(0, stmt->condition));

	Token *rParentheseToken = peekToken();

	SLKC_RETURN_IF_PARSE_ERROR(expectToken(rParentheseToken, TokenId::RParenthese));

	nextToken();

	Token *lBraceToken = peekToken();

	SLKC_RETURN_IF_PARSE_ERROR(expectToken(lBraceToken, TokenId::LBrace));

	nextToken();

	AstNodePtr<StmtNode> curStmt;

	while (true) {
		SLKC_RETURN_IF_PARSE_ERROR(expectToken(peekToken()));

		if (peekToken()->tokenId == TokenId::RBrace) {
			break;
		}

		if ((syntaxError = parseStmt(curStmt))) {
			if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
				return genOutOfMemorySyntaxError();
		}

		if (curStmt) {
			// We detect and push case labels in advance to deal with them easier.
			if (curStmt->stmtKind == StmtKind::CaseLabel) {
				if (!stmt->caseOffsets.pushBack(stmt->body.size()))
					return genOutOfMemorySyntaxError();
			}

			if (!stmt->body.pushBack(std::move(curStmt))) {
				return genOutOfMemorySyntaxError();
			}
		}
	}

	Token *rBraceToken;

	SLKC_RETURN_IF_PARSE_ERROR(expectToken((rBraceToken = peekToken()), TokenId::RBrace));

	nextToken();

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseCaseStmt(AstNodePtr<StmtNode> &stmtOut) {
	peff::Option<SyntaxError> syntaxError;

	nextToken();

	AstNodePtr<CaseLabelStmtNode> stmt;

	if (!(stmt = makeAstNode<CaseLabelStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
		return genOutOfMemorySyntaxError();
	}

	stmtOut = stmt.castTo<StmtNode>();

	SLKC_RETURN_IF_PARSE_ERROR(parseExpr(0, stmt->condition));

	SLKC_RETURN_IF_PARSE_ERROR(expectToken(peekToken(), TokenId::Colon));

	nextToken();

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseDefaultStmt(AstNodePtr<StmtNode> &stmtOut) {
	peff::Option<SyntaxError> syntaxError;

	nextToken();

	AstNodePtr<CaseLabelStmtNode> stmt;

	if (!(stmt = makeAstNode<CaseLabelStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
		return genOutOfMemorySyntaxError();
	}

	stmtOut = stmt.castTo<StmtNode>();

	SLKC_RETURN_IF_PARSE_ERROR(expectToken(peekToken(), TokenId::Colon));

	nextToken();

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseExprStmt(AstNodePtr<StmtNode> &stmtOut) {
	peff::Option<SyntaxError> syntaxError;

	AstNodePtr<ExprNode> curExpr;

	AstNodePtr<ExprStmtNode> stmt;

	if (!(stmt = makeAstNode<ExprStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
		return genOutOfMemorySyntaxError();
	}

	stmtOut = stmt.castTo<StmtNode>();

	if ((syntaxError = parseExpr(-10, stmt->expr))) {
		if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
			return genOutOfMemorySyntaxError();
	}

	SLKC_RETURN_IF_PARSE_ERROR(expectToken(peekToken(), TokenId::Semicolon));

	nextToken();

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseStmt(AstNodePtr<StmtNode> &stmtOut) {
	Token *prefixToken;

	peff::Option<SyntaxError> syntaxError;

	if ((syntaxError = expectToken((prefixToken = peekToken()))))
		goto genBadStmt;

	{
		peff::ScopeGuard setTokenRangeGuard([this, prefixToken, &stmtOut]() noexcept {
			stmtOut->tokenRange = TokenRange{ document->mainModule, prefixToken->index, parseContext.idxPrevToken };
		});

		switch (prefixToken->tokenId) {
			case TokenId::IfKeyword:
				if ((syntaxError = parseIfStmt(stmtOut)))
					goto genBadStmt;
				break;
			case TokenId::WithKeyword:
				if ((syntaxError = parseWithStmt(stmtOut)))
					goto genBadStmt;
				break;
			case TokenId::ForKeyword:
				if ((syntaxError = parseForStmt(stmtOut)))
					goto genBadStmt;
				break;
			case TokenId::WhileKeyword:
				if ((syntaxError = parseWhileStmt(stmtOut)))
					goto genBadStmt;
				break;
			case TokenId::DoKeyword:
				if ((syntaxError = parseDoWhileStmt(stmtOut)))
					goto genBadStmt;
				break;
			case TokenId::LetKeyword:
				if ((syntaxError = parseLetStmt(stmtOut)))
					goto genBadStmt;
				break;
			case TokenId::BreakKeyword:
				if ((syntaxError = parseBreakStmt(stmtOut)))
					goto genBadStmt;
				break;
			case TokenId::ContinueKeyword:
				if ((syntaxError = parseContinueStmt(stmtOut)))
					goto genBadStmt;
				break;
			case TokenId::ReturnKeyword:
				if ((syntaxError = parseReturnStmt(stmtOut)))
					goto genBadStmt;
				break;
			case TokenId::YieldKeyword:
				if ((syntaxError = parseYieldStmt(stmtOut)))
					goto genBadStmt;
				break;
			case TokenId::Colon:
				if ((syntaxError = parseLabelStmt(stmtOut)))
					goto genBadStmt;
				break;
			case TokenId::CaseKeyword:
				if ((syntaxError = parseCaseStmt(stmtOut)))
					goto genBadStmt;
				break;
			case TokenId::DefaultKeyword:
				if ((syntaxError = parseDefaultStmt(stmtOut)))
					goto genBadStmt;
				break;
			case TokenId::SwitchKeyword:
				if ((syntaxError = parseSwitchStmt(stmtOut)))
					goto genBadStmt;
				break;
			case TokenId::LBrace:
				if ((syntaxError = parseBlockStmt(stmtOut)))
					goto genBadStmt;
				break;
			default:
				if ((syntaxError = parseExprStmt(stmtOut)))
					goto genBadStmt;
				break;
		}
	}

	return {};

genBadStmt:
	if (!(stmtOut = makeAstNode<BadStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document, stmtOut).castTo<StmtNode>()))
		return genOutOfMemorySyntaxError();
	stmtOut->tokenRange = { document->mainModule, prefixToken->index, parseContext.idxCurrentToken };
	return syntaxError;
}
