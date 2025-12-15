#include "../parser.h"

using namespace slkc;

SLKC_API peff::Option<SyntaxError> Parser::parseVarDefs(peff::DynArray<VarDefEntryPtr> &varDefEntries) {
	Token *currentToken;
	peff::Option<SyntaxError> syntaxError;

	for (;;) {
		peff::DynArray<AstNodePtr<AttributeNode>> attributes(resourceAllocator.get());

		if ((syntaxError = parseAttributes(attributes))) {
			return syntaxError;
		}

		if ((syntaxError = expectToken((currentToken = nextToken()), TokenId::Id))) {
			if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
				return genOutOfMemoryError();
			syntaxError.reset();
			if (!syntaxErrors.pushBack(SyntaxError({ document->mainModule, currentToken->index }, SyntaxErrorKind::ExpectingId)))
				return genOutOfMemoryError();
		}

		VarDefEntryPtr entryPtr(
			peff::allocAndConstruct<VarDefEntry>(
				resourceAllocator.get(),
				ASTNODE_ALIGNMENT,
				resourceAllocator.get()));
		if (!entryPtr) {
			return genOutOfMemoryError();
		}

		VarDefEntry *entry = entryPtr.get();

		if (!varDefEntries.pushBack(std::move(entryPtr))) {
			return genOutOfMemoryError();
		}

		if (!entry->name.build(currentToken->sourceText)) {
			return genOutOfMemoryError();
		}

		if ((currentToken = peekToken())->tokenId == TokenId::Colon) {
			nextToken();

			if ((syntaxError = parseTypeName(entry->type))) {
				if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
					return genOutOfMemoryError();
				syntaxError.reset();
			}
		}

		if ((currentToken = peekToken())->tokenId == TokenId::AssignOp) {
			nextToken();

			if ((syntaxError = parseExpr(0, entry->initialValue))) {
				if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
					return genOutOfMemoryError();
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
		return genOutOfMemoryError();
	}

	stmtOut = ifStmt.template castTo<StmtNode>();

	Token *lParentheseToken = peekToken();

	if ((syntaxError = expectToken(lParentheseToken, TokenId::LParenthese)))
		return syntaxError;

	nextToken();

	{
		static TokenId skippingTerminativeToken[] = {
			TokenId::RParenthese,
			TokenId::Semicolon,
			TokenId::RBrace
		};

		if ((syntaxError = parseExpr(0, ifStmt->cond))) {
			if ((syntaxError = lookaheadUntil(std::size(skippingTerminativeToken), skippingTerminativeToken)))
				return syntaxError;
			return syntaxError;
		}
	}

	Token *rParentheseToken = peekToken();

	if ((syntaxError = expectToken(rParentheseToken, TokenId::RParenthese)))
		return syntaxError;

	nextToken();

	if ((syntaxError = parseStmt(ifStmt->trueBody)))
		return syntaxError;

	Token *elseToken = peekToken();

	if (elseToken->tokenId == TokenId::ElseKeyword) {
		nextToken();

		if ((syntaxError = parseStmt(ifStmt->falseBody)))
			return syntaxError;
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
		return genOutOfMemoryError();
	}

	stmtOut = withStmt.template castTo<StmtNode>();

	WithConstraintEntryPtr entry;

	while (true) {
		if (!(entry = WithConstraintEntryPtr(peff::allocAndConstruct<WithConstraintEntry>(resourceAllocator.get(), alignof(WithConstraintEntry), resourceAllocator.get())))) {
			return genOutOfMemoryError();
		}

		Token *nameToken;

		if ((syntaxError = expectToken((nameToken = peekToken()), TokenId::Id))) {
			return syntaxError;
		}

		if (!entry->genericParamName.build(nameToken->sourceText))
			return genOutOfMemoryError();

		nextToken();

		if ((syntaxError = parseGenericConstraint(entry->constraint))) {
			return syntaxError;
		}

		if (!withStmt->constraints.pushBack(std::move(entry)))
			return genOutOfMemoryError();

		if (peekToken()->tokenId != TokenId::Comma) {
			break;
		}

		Token *commaToken = nextToken();

		/*
		if (!idxCommaTokensOut.pushBack(+commaToken->index))
			return genOutOfMemoryError();*/
	}

	if ((syntaxError = parseStmt(withStmt->trueBody)))
		return syntaxError;

	Token *elseToken = peekToken();

	if (elseToken->tokenId == TokenId::ElseKeyword) {
		nextToken();

		if ((syntaxError = parseStmt(withStmt->falseBody)))
			return syntaxError;
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
		return genOutOfMemoryError();
	}

	stmtOut = forStmt.template castTo<StmtNode>();

	Token *lParentheseToken = peekToken();

	if ((syntaxError = expectToken(lParentheseToken, TokenId::LParenthese)))
		return syntaxError;

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
			Token *letToken;
			if ((syntaxError = expectToken((letToken = peekToken()), TokenId::LetKeyword)))
				return syntaxError;
			nextToken();

			if ((syntaxError = parseVarDefs(forStmt->varDefEntries)))
				return syntaxError;

			if ((syntaxError = expectToken((varDefSeparatorToken = peekToken()), TokenId::Semicolon)))
				return syntaxError;
		} else {
			nextToken();
		}

		if ((condSeparatorToken = peekToken())->tokenId != TokenId::Semicolon) {
			if ((syntaxError = parseExpr(0, forStmt->cond))) {
				if ((syntaxError = lookaheadUntil(std::size(skippingTerminativeToken), skippingTerminativeToken)))
					return syntaxError;
				return syntaxError;
			}

			if ((syntaxError = expectToken((condSeparatorToken = peekToken()), TokenId::Semicolon)))
				return syntaxError;
		} else {
			nextToken();
		}

		if ((rParentheseToken = peekToken())->tokenId != TokenId::RParenthese) {
			if ((syntaxError = parseExpr(-10, forStmt->step))) {
				if ((syntaxError = lookaheadUntil(std::size(skippingTerminativeToken), skippingTerminativeToken)))
					return syntaxError;
				return syntaxError;
			}

			if ((syntaxError = expectToken((rParentheseToken = peekToken()), TokenId::RParenthese)))
				return syntaxError;
		} else {
			nextToken();
		}
	}

	nextToken();

	if ((syntaxError = parseStmt(forStmt->body)))
		return syntaxError;

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
		return genOutOfMemoryError();
	}

	stmtOut = whileStmt.template castTo<StmtNode>();

	Token *lParentheseToken = peekToken();

	if ((syntaxError = expectToken(lParentheseToken, TokenId::LParenthese)))
		return syntaxError;

	nextToken();

	Token *rParentheseToken;
	{
		static TokenId skippingTerminativeToken[] = {
			TokenId::RParenthese,
			TokenId::Semicolon,
			TokenId::RBrace
		};

		if ((syntaxError = parseExpr(0, whileStmt->cond))) {
			if ((syntaxError = lookaheadUntil(std::size(skippingTerminativeToken), skippingTerminativeToken)))
				return syntaxError;
			return syntaxError;
		}

		if ((syntaxError = expectToken((rParentheseToken = peekToken()), TokenId::RParenthese))) {
			return syntaxError;
		}

		nextToken();
	}

	if ((syntaxError = parseStmt(whileStmt->body)))
		return syntaxError;

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseDoWhileStmt(AstNodePtr<StmtNode> &stmtOut) {
	peff::Option<SyntaxError> syntaxError;

	nextToken();

	AstNodePtr<WhileStmtNode> whileStmt;

	if (!(whileStmt = makeAstNode<WhileStmtNode>(
			  resourceAllocator.get(),
			  resourceAllocator.get(),
			  document))) {
		return genOutOfMemoryError();
	}

	stmtOut = whileStmt.template castTo<StmtNode>();

	whileStmt->isDoWhile = true;

	if ((syntaxError = parseStmt(whileStmt->body)))
		return syntaxError;

	Token *whileToken = peekToken();

	if ((syntaxError = expectToken(whileToken, TokenId::WhileKeyword)))
		return syntaxError;

	nextToken();

	Token *lParentheseToken = peekToken();

	if ((syntaxError = expectToken(lParentheseToken, TokenId::LParenthese)))
		return syntaxError;

	nextToken();

	Token *rParentheseToken;
	{
		static TokenId skippingTerminativeToken[] = {
			TokenId::RParenthese,
			TokenId::Semicolon,
			TokenId::RBrace
		};

		if ((syntaxError = parseExpr(0, whileStmt->cond))) {
			if ((syntaxError = lookaheadUntil(std::size(skippingTerminativeToken), skippingTerminativeToken)))
				return syntaxError;
			return syntaxError;
		}

		if ((syntaxError = expectToken((rParentheseToken = peekToken()), TokenId::RParenthese)))
			return syntaxError;

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
		return genOutOfMemoryError();
	}

	stmtOut = stmt.template castTo<StmtNode>();

	if ((syntaxError = parseVarDefs(stmt->varDefEntries)))
		return syntaxError;

	Token *semicolonToken;

	if ((syntaxError = expectToken((semicolonToken = peekToken()), TokenId::Semicolon)))
		return syntaxError;

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
		return genOutOfMemoryError();
	}

	stmtOut = stmt.template castTo<StmtNode>();

	Token *semicolonToken;

	if ((syntaxError = expectToken((semicolonToken = peekToken()), TokenId::Semicolon)))
		return syntaxError;

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
		return genOutOfMemoryError();
	}

	stmtOut = stmt.template castTo<StmtNode>();

	Token *semicolonToken;

	if ((syntaxError = expectToken((semicolonToken = peekToken()), TokenId::Semicolon)))
		return syntaxError;

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
		return genOutOfMemoryError();
	}

	stmtOut = stmt.template castTo<StmtNode>();

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
				if ((syntaxError = lookaheadUntil(std::size(skippingTerminativeToken), skippingTerminativeToken)))
					return syntaxError;
				return syntaxError;
			}

			if ((syntaxError = expectToken(peekToken(), TokenId::Semicolon)))
				return syntaxError;

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
		return genOutOfMemoryError();
	}

	stmtOut = stmt.template castTo<StmtNode>();

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
				if ((syntaxError = lookaheadUntil(std::size(skippingTerminativeToken), skippingTerminativeToken)))
					return syntaxError;
				return syntaxError;
			}

			if ((syntaxError = expectToken(peekToken(), TokenId::Semicolon)))
				return syntaxError;

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
		return genOutOfMemoryError();
	}

	stmtOut = stmt.template castTo<StmtNode>();

	if ((syntaxError = expectToken(peekToken(), TokenId::Id))) {
		return syntaxError;
	}

	Token *nameToken = nextToken();

	if (!stmt->name.build(nameToken->sourceText)) {
		return genOutOfMemoryError();
	}

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseBlockStmt(AstNodePtr<StmtNode> &stmtOut) {
	peff::Option<SyntaxError> syntaxError;

	nextToken();

	AstNodePtr<CodeBlockStmtNode> stmt;
	AstNodePtr<StmtNode> curStmt;

	if (!(stmt = makeAstNode<CodeBlockStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
		return genOutOfMemoryError();
	}

	stmtOut = stmt.template castTo<StmtNode>();

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
			if (!stmt->body.pushBack(std::move(curStmt))) {
				return genOutOfMemoryError();
			}
		}
	}

	Token *rBraceToken;

	if ((syntaxError = expectToken((rBraceToken = peekToken()), TokenId::RBrace))) {
		return syntaxError;
	}

	nextToken();

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseSwitchStmt(AstNodePtr<StmtNode> &stmtOut) {
	peff::Option<SyntaxError> syntaxError;

	nextToken();

	AstNodePtr<SwitchStmtNode> stmt;

	if (!(stmt = makeAstNode<SwitchStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
		return genOutOfMemoryError();
	}

	stmtOut = stmt.template castTo<StmtNode>();

	if (peekToken()->tokenId == TokenId::ConstKeyword) {
		stmt->isConst = true;
		nextToken();
	}

	Token *lParentheseToken = peekToken();

	if ((syntaxError = expectToken(lParentheseToken, TokenId::LParenthese)))
		return syntaxError;

	nextToken();

	if ((syntaxError = parseExpr(0, stmt->condition)))
		return syntaxError;

	Token *rParentheseToken = peekToken();

	if ((syntaxError = expectToken(rParentheseToken, TokenId::RParenthese)))
		return syntaxError;

	nextToken();

	Token *lBraceToken = peekToken();

	if ((syntaxError = expectToken(lBraceToken, TokenId::LBrace)))
		return syntaxError;

	nextToken();

	AstNodePtr<StmtNode> curStmt;

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
			// We detect and push case labels in advance to deal with them easier.
			if (curStmt->stmtKind == StmtKind::CaseLabel) {
				if (!stmt->caseOffsets.pushBack(stmt->body.size()))
					return genOutOfMemoryError();
			}

			if (!stmt->body.pushBack(std::move(curStmt))) {
				return genOutOfMemoryError();
			}
		}
	}

	Token *rBraceToken;

	if ((syntaxError = expectToken((rBraceToken = peekToken()), TokenId::RBrace))) {
		return syntaxError;
	}

	nextToken();

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseCaseStmt(AstNodePtr<StmtNode> &stmtOut) {
	peff::Option<SyntaxError> syntaxError;

	nextToken();

	AstNodePtr<CaseLabelStmtNode> stmt;

	if (!(stmt = makeAstNode<CaseLabelStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
		return genOutOfMemoryError();
	}

	stmtOut = stmt.template castTo<StmtNode>();

	if ((syntaxError = parseExpr(0, stmt->condition))) {
		return syntaxError;
	}

	if ((syntaxError = expectToken(peekToken(), TokenId::Colon))) {
		return syntaxError;
	}

	nextToken();

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseDefaultStmt(AstNodePtr<StmtNode> &stmtOut) {
	peff::Option<SyntaxError> syntaxError;

	nextToken();

	AstNodePtr<CaseLabelStmtNode> stmt;

	if (!(stmt = makeAstNode<CaseLabelStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
		return genOutOfMemoryError();
	}

	stmtOut = stmt.template castTo<StmtNode>();

	if ((syntaxError = expectToken(peekToken(), TokenId::Colon))) {
		return syntaxError;
	}

	nextToken();

	return {};
}

SLKC_API peff::Option<SyntaxError> Parser::parseExprStmt(AstNodePtr<StmtNode> &stmtOut) {
	peff::Option<SyntaxError> syntaxError;

	AstNodePtr<ExprNode> curExpr;

	AstNodePtr<ExprStmtNode> stmt;

	if (!(stmt = makeAstNode<ExprStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
		return genOutOfMemoryError();
	}

	stmtOut = stmt.template castTo<StmtNode>();

	if ((syntaxError = parseExpr(-10, stmt->expr))) {
		if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
			return genOutOfMemoryError();
	}

	if ((syntaxError = expectToken(peekToken(), TokenId::Semicolon)))
		return syntaxError;

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
				if (syntaxError = parseIfStmt(stmtOut))
					goto genBadStmt;
				break;
			case TokenId::WithKeyword:
				if (syntaxError = parseWithStmt(stmtOut))
					goto genBadStmt;
				break;
			case TokenId::ForKeyword:
				if (syntaxError = parseForStmt(stmtOut))
					goto genBadStmt;
				break;
			case TokenId::WhileKeyword:
				if (syntaxError = parseWhileStmt(stmtOut))
					goto genBadStmt;
				break;
			case TokenId::DoKeyword:
				if (syntaxError = parseDoWhileStmt(stmtOut))
					goto genBadStmt;
				break;
			case TokenId::LetKeyword:
				if (syntaxError = parseLetStmt(stmtOut))
					goto genBadStmt;
				break;
			case TokenId::BreakKeyword:
				if (syntaxError = parseBreakStmt(stmtOut))
					goto genBadStmt;
				break;
			case TokenId::ContinueKeyword:
				if (syntaxError = parseContinueStmt(stmtOut))
					goto genBadStmt;
				break;
			case TokenId::ReturnKeyword:
				if (syntaxError = parseReturnStmt(stmtOut))
					goto genBadStmt;
				break;
			case TokenId::YieldKeyword:
				if (syntaxError = parseYieldStmt(stmtOut))
					goto genBadStmt;
				break;
			case TokenId::Colon:
				if (syntaxError = parseLabelStmt(stmtOut))
					goto genBadStmt;
				break;
			case TokenId::CaseKeyword:
				if (syntaxError = parseCaseStmt(stmtOut))
					goto genBadStmt;
				break;
			case TokenId::DefaultKeyword:
				if (syntaxError = parseDefaultStmt(stmtOut))
					goto genBadStmt;
				break;
			case TokenId::SwitchKeyword:
				if (syntaxError = parseSwitchStmt(stmtOut))
					goto genBadStmt;
				break;
			case TokenId::LBrace:
				if (syntaxError = parseBlockStmt(stmtOut))
					goto genBadStmt;
				break;
			default:
				if (syntaxError = parseExprStmt(stmtOut))
					goto genBadStmt;
				break;
		}
	}

	return {};

genBadStmt:
	if (!(stmtOut = makeAstNode<BadStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document, stmtOut).template castTo<StmtNode>()))
		return genOutOfMemoryError();
	stmtOut->tokenRange = { document->mainModule, prefixToken->index, parseContext.idxCurrentToken };
	return syntaxError;
}
