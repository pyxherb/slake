#include "../parser.h"

using namespace slkc;

SLKC_API std::optional<SyntaxError> Parser::parseVarDefs(peff::DynArray<VarDefEntryPtr> &varDefEntries) {
	Token *currentToken;
	std::optional<SyntaxError> syntaxError;
	peff::SharedPtr<TypeNameNode> type;
	peff::SharedPtr<ExprNode> initialValue;

	for (;;) {
		bool isSealed = false;

		peff::DynArray<peff::SharedPtr<AttributeNode>> attributes(resourceAllocator.get());

		if ((syntaxError = parseAttributes(attributes))) {
			return syntaxError;
		}

		if ((syntaxError = expectToken((currentToken = nextToken()), TokenId::Id))) {
			if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
				return genOutOfMemoryError();
			syntaxError.reset();
			if (!syntaxErrors.pushBack(SyntaxError(currentToken->index, SyntaxErrorKind::ExpectingId)))
				return genOutOfMemoryError();
		}

		peff::String copiedName(resourceAllocator.get());
		if (!copiedName.build(currentToken->sourceText)) {
			return genOutOfMemoryError();
		}

		if (peekToken()->tokenId == TokenId::FinalKeyword) {
			Token *finalToken = nextToken();

			isSealed = true;
		}

		if ((currentToken = peekToken())->tokenId == TokenId::Colon) {
			nextToken();

			if ((syntaxError = parseTypeName(type))) {
				if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
					return genOutOfMemoryError();
				syntaxError.reset();
			}
		}

		if ((currentToken = peekToken())->tokenId == TokenId::AssignOp) {
			nextToken();

			if ((syntaxError = parseExpr(0, initialValue))) {
				if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
					return genOutOfMemoryError();
				syntaxError.reset();
			}
		}

		VarDefEntryPtr entry(
			peff::allocAndConstruct<VarDefEntry>(
				resourceAllocator.get(),
				ASTNODE_ALIGNMENT,
				resourceAllocator.get(),
				std::move(copiedName),
				type,
				initialValue,
				isSealed));

		if (!entry) {
			return genOutOfMemoryError();
		}

		if (!varDefEntries.pushBack(std::move(entry))) {
			return genOutOfMemoryError();
		}

		if ((currentToken = peekToken())->tokenId != TokenId::Comma) {
			break;
		}

		nextToken();
	}

	return {};
}

SLKC_API std::optional<SyntaxError> Parser::parseStmt(peff::SharedPtr<StmtNode> &stmtOut) {
	Token *prefixToken;

	std::optional<SyntaxError> syntaxError;

	if ((syntaxError = expectToken((prefixToken = peekToken()))))
		goto genBadStmt;

	{
		peff::ScopeGuard setTokenRangeGuard([this, prefixToken, &stmtOut]() noexcept {
			stmtOut->tokenRange = TokenRange{ prefixToken->index, parseContext.idxPrevToken };
		});

		switch (prefixToken->tokenId) {
			case TokenId::IfKeyword: {
				nextToken();

				peff::SharedPtr<IfStmtNode> ifStmt;

				if (!(ifStmt = peff::makeShared<IfStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
					return genOutOfMemoryError();
				}

				stmtOut = ifStmt.castTo<StmtNode>();

				Token *lParentheseToken = peekToken();

				if ((syntaxError = expectToken(lParentheseToken, TokenId::LParenthese))) {
					goto genBadStmt;
				}

				nextToken();

				{
					static TokenId skippingTerminativeToken[] = {
						TokenId::RParenthese,
						TokenId::Semicolon,
						TokenId::RBrace
					};

					if ((syntaxError = parseExpr(0, ifStmt->cond))) {
						if ((syntaxError = lookaheadUntil(std::size(skippingTerminativeToken), skippingTerminativeToken))) {
							goto genBadStmt;
						}
						goto genBadStmt;
					}
				}

				Token *rParentheseToken = peekToken();

				if ((syntaxError = expectToken(rParentheseToken, TokenId::RParenthese))) {
					goto genBadStmt;
				}

				nextToken();

				if ((syntaxError = parseStmt(ifStmt->trueBody))) {
					goto genBadStmt;
				}

				Token *elseToken = peekToken();

				if (elseToken->tokenId == TokenId::ElseKeyword) {
					nextToken();

					if ((syntaxError = parseStmt(ifStmt->falseBody))) {
						goto genBadStmt;
					}
				}

				break;
			}
			case TokenId::WithKeyword: {
				nextToken();

				peff::SharedPtr<WithStmtNode> withStmt;

				if (!(withStmt = peff::makeShared<WithStmtNode>(
						  resourceAllocator.get(),
						  resourceAllocator.get(),
						  document))) {
					return genOutOfMemoryError();
				}

				stmtOut = withStmt.castTo<StmtNode>();

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

				if ((syntaxError = parseStmt(withStmt->trueBody))) {
					goto genBadStmt;
				}

				Token *elseToken = peekToken();

				if (elseToken->tokenId == TokenId::ElseKeyword) {
					nextToken();

					if ((syntaxError = parseStmt(withStmt->falseBody))) {
						goto genBadStmt;
					}
				}

				break;
			}
			case TokenId::ForKeyword: {
				nextToken();

				peff::SharedPtr<ForStmtNode> forStmt;

				if (!(forStmt = peff::makeShared<ForStmtNode>(
						  resourceAllocator.get(),
						  resourceAllocator.get(),
						  document))) {
					return genOutOfMemoryError();
				}

				stmtOut = forStmt.castTo<StmtNode>();

				Token *lParentheseToken = peekToken();

				if ((syntaxError = expectToken(lParentheseToken, TokenId::LParenthese))) {
					goto genBadStmt;
				}

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
						if ((syntaxError = expectToken((letToken = peekToken()), TokenId::LetKeyword))) {
							goto genBadStmt;
						}
						nextToken();

						if ((syntaxError = parseVarDefs(forStmt->varDefEntries))) {
							goto genBadStmt;
						}

						if ((syntaxError = expectToken((varDefSeparatorToken = peekToken()), TokenId::Semicolon))) {
							goto genBadStmt;
						}
					} else {
						nextToken();
					}

					if ((condSeparatorToken = peekToken())->tokenId != TokenId::Semicolon) {
						if ((syntaxError = parseExpr(0, forStmt->cond))) {
							if ((syntaxError = lookaheadUntil(std::size(skippingTerminativeToken), skippingTerminativeToken))) {
								goto genBadStmt;
							}
							goto genBadStmt;
						}

						if ((syntaxError = expectToken((condSeparatorToken = peekToken()), TokenId::Semicolon))) {
							goto genBadStmt;
						}
					} else {
						nextToken();
					}

					if ((rParentheseToken = peekToken())->tokenId != TokenId::RParenthese) {
						if ((syntaxError = parseExpr(-10, forStmt->step))) {
							if ((syntaxError = lookaheadUntil(std::size(skippingTerminativeToken), skippingTerminativeToken))) {
								goto genBadStmt;
							}
							goto genBadStmt;
						}

						if ((syntaxError = expectToken((rParentheseToken = peekToken()), TokenId::RParenthese))) {
							goto genBadStmt;
						}
					} else {
						nextToken();
					}
				}

				nextToken();

				if ((syntaxError = parseStmt(forStmt->body))) {
					goto genBadStmt;
				}

				break;
			}
			case TokenId::WhileKeyword: {
				nextToken();

				peff::SharedPtr<WhileStmtNode> whileStmt;

				if (!(whileStmt = peff::makeShared<WhileStmtNode>(
						  resourceAllocator.get(),
						  resourceAllocator.get(),
						  document))) {
					return genOutOfMemoryError();
				}

				stmtOut = whileStmt.castTo<StmtNode>();

				Token *lParentheseToken = peekToken();

				if ((syntaxError = expectToken(lParentheseToken, TokenId::LParenthese))) {
					goto genBadStmt;
				}

				nextToken();

				Token *rParentheseToken;
				{
					static TokenId skippingTerminativeToken[] = {
						TokenId::RParenthese,
						TokenId::Semicolon,
						TokenId::RBrace
					};

					if ((syntaxError = parseExpr(0, whileStmt->cond))) {
						if ((syntaxError = lookaheadUntil(std::size(skippingTerminativeToken), skippingTerminativeToken))) {
							goto genBadStmt;
						}
						goto genBadStmt;
					}

					if ((syntaxError = expectToken((rParentheseToken = peekToken()), TokenId::RParenthese))) {
						goto genBadStmt;
					}

					nextToken();
				}

				if ((syntaxError = parseStmt(whileStmt->body))) {
					goto genBadStmt;
				}

				break;
			}
			case TokenId::DoKeyword: {
				nextToken();

				peff::SharedPtr<WhileStmtNode> whileStmt;

				if (!(whileStmt = peff::makeShared<WhileStmtNode>(
						  resourceAllocator.get(),
						  resourceAllocator.get(),
						  document))) {
					return genOutOfMemoryError();
				}

				stmtOut = whileStmt.castTo<StmtNode>();

				whileStmt->isDoWhile = true;

				if ((syntaxError = parseStmt(whileStmt->body))) {
					goto genBadStmt;
				}

				Token *whileToken = peekToken();

				if ((syntaxError = expectToken(whileToken, TokenId::WhileKeyword))) {
					goto genBadStmt;
				}

				nextToken();

				Token *lParentheseToken = peekToken();

				if ((syntaxError = expectToken(lParentheseToken, TokenId::LParenthese))) {
					goto genBadStmt;
				}

				nextToken();

				Token *rParentheseToken;
				{
					static TokenId skippingTerminativeToken[] = {
						TokenId::RParenthese,
						TokenId::Semicolon,
						TokenId::RBrace
					};

					if ((syntaxError = parseExpr(0, whileStmt->cond))) {
						if ((syntaxError = lookaheadUntil(std::size(skippingTerminativeToken), skippingTerminativeToken))) {
							goto genBadStmt;
						}
						goto genBadStmt;
					}

					if ((syntaxError = expectToken((rParentheseToken = peekToken()), TokenId::RParenthese))) {
						goto genBadStmt;
					}

					nextToken();
				}

				break;
			}
			case TokenId::LetKeyword: {
				nextToken();

				peff::SharedPtr<VarDefStmtNode> stmt;

				if (!(stmt = peff::makeShared<VarDefStmtNode>(
						  resourceAllocator.get(),
						  resourceAllocator.get(),
						  document,
						  peff::DynArray<VarDefEntryPtr>(resourceAllocator.get())))) {
					return genOutOfMemoryError();
				}

				stmtOut = stmt.castTo<StmtNode>();

				if ((syntaxError = parseVarDefs(stmt->varDefEntries))) {
					goto genBadStmt;
				}

				Token *semicolonToken;

				if ((syntaxError = expectToken((semicolonToken = peekToken()), TokenId::Semicolon))) {
					goto genBadStmt;
				}

				nextToken();

				break;
			}
			case TokenId::BreakKeyword: {
				nextToken();

				peff::SharedPtr<BreakStmtNode> stmt;

				if (!(stmt = peff::makeShared<BreakStmtNode>(
						  resourceAllocator.get(),
						  resourceAllocator.get(),
						  document))) {
					return genOutOfMemoryError();
				}

				stmtOut = stmt.castTo<StmtNode>();

				Token *semicolonToken;

				if ((syntaxError = expectToken((semicolonToken = peekToken()), TokenId::Semicolon))) {
					goto genBadStmt;
				}

				nextToken();

				break;
			}
			case TokenId::ContinueKeyword: {
				nextToken();

				peff::SharedPtr<ContinueStmtNode> stmt;

				if (!(stmt = peff::makeShared<ContinueStmtNode>(
						  resourceAllocator.get(),
						  resourceAllocator.get(),
						  document))) {
					return genOutOfMemoryError();
				}

				stmtOut = stmt.castTo<StmtNode>();

				Token *semicolonToken;

				if ((syntaxError = expectToken((semicolonToken = peekToken()), TokenId::Semicolon))) {
					goto genBadStmt;
				}

				nextToken();

				break;
			}
			case TokenId::ReturnKeyword: {
				nextToken();

				peff::SharedPtr<ReturnStmtNode> stmt;

				if (!(stmt = peff::makeShared<ReturnStmtNode>(
						  resourceAllocator.get(),
						  resourceAllocator.get(),
						  document,
						  peff::SharedPtr<ExprNode>()))) {
					return genOutOfMemoryError();
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
							if ((syntaxError = lookaheadUntil(std::size(skippingTerminativeToken), skippingTerminativeToken))) {
								goto genBadStmt;
							}
							goto genBadStmt;
						}

						if ((syntaxError = expectToken(peekToken(), TokenId::Semicolon))) {
							goto genBadStmt;
						}

						nextToken();
						break;
				}

				break;
			}
			case TokenId::YieldKeyword: {
				nextToken();

				peff::SharedPtr<YieldStmtNode> stmt;

				if (!(stmt = peff::makeShared<YieldStmtNode>(
						  resourceAllocator.get(),
						  resourceAllocator.get(),
						  document,
						  peff::SharedPtr<ExprNode>()))) {
					return genOutOfMemoryError();
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
							if ((syntaxError = lookaheadUntil(std::size(skippingTerminativeToken), skippingTerminativeToken))) {
								goto genBadStmt;
							}
							goto genBadStmt;
						}

						if ((syntaxError = expectToken(peekToken(), TokenId::Semicolon))) {
							goto genBadStmt;
						}

						nextToken();
						break;
				}

				break;
			}
			case TokenId::Colon: {
				nextToken();

				peff::SharedPtr<LabelStmtNode> stmt;

				if (!(stmt = peff::makeShared<LabelStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
					return genOutOfMemoryError();
				}

				stmtOut = stmt.castTo<StmtNode>();

				if ((syntaxError = expectToken(peekToken(), TokenId::Id))) {
					return syntaxError;
				}

				Token *nameToken = nextToken();

				if (!stmt->name.build(nameToken->sourceText)) {
					return genOutOfMemoryError();
				}

				break;
			}
			case TokenId::CaseKeyword: {
				nextToken();

				peff::SharedPtr<CaseLabelStmtNode> stmt;

				if (!(stmt = peff::makeShared<CaseLabelStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
					return genOutOfMemoryError();
				}

				stmtOut = stmt.castTo<StmtNode>();

				if ((syntaxError = parseExpr(0, stmt->condition))) {
					return syntaxError;
				}

				if ((syntaxError = expectToken(peekToken(), TokenId::Colon))) {
					return syntaxError;
				}

				nextToken();

				break;
			}
			case TokenId::DefaultKeyword: {
				nextToken();

				peff::SharedPtr<CaseLabelStmtNode> stmt;

				if (!(stmt = peff::makeShared<CaseLabelStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
					return genOutOfMemoryError();
				}

				stmtOut = stmt.castTo<StmtNode>();

				if ((syntaxError = expectToken(peekToken(), TokenId::Colon))) {
					return syntaxError;
				}

				nextToken();

				break;
			}
			case TokenId::SwitchKeyword: {
				nextToken();

				peff::SharedPtr<SwitchStmtNode> stmt;

				if (!(stmt = peff::makeShared<SwitchStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
					return genOutOfMemoryError();
				}

				stmtOut = stmt.castTo<StmtNode>();

				Token *lParentheseToken = peekToken();

				if ((syntaxError = expectToken(lParentheseToken, TokenId::LParenthese))) {
					goto genBadStmt;
				}

				nextToken();

				if ((syntaxError = parseExpr(0, stmt->condition))) {
					return syntaxError;
				}

				Token *rParentheseToken = peekToken();

				if ((syntaxError = expectToken(rParentheseToken, TokenId::RParenthese))) {
					goto genBadStmt;
				}

				nextToken();

				Token *lBraceToken = peekToken();

				if ((syntaxError = expectToken(lBraceToken, TokenId::LBrace))) {
					goto genBadStmt;
				}

				nextToken();

				peff::SharedPtr<StmtNode> curStmt;

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

				break;
			}
			case TokenId::LBrace: {
				nextToken();

				peff::SharedPtr<CodeBlockStmtNode> stmt;
				peff::SharedPtr<StmtNode> curStmt;

				if (!(stmt = peff::makeShared<CodeBlockStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
					return genOutOfMemoryError();
				}

				stmtOut = stmt.castTo<StmtNode>();

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

				break;
			}
			default: {
				peff::SharedPtr<ExprNode> curExpr;

				peff::SharedPtr<ExprStmtNode> stmt;

				if (!(stmt = peff::makeShared<ExprStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
					return genOutOfMemoryError();
				}

				stmtOut = stmt.castTo<StmtNode>();

				if ((syntaxError = parseExpr(-10, stmt->expr))) {
					if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
						return genOutOfMemoryError();
					syntaxError.reset();
					goto genBadStmt;
				}

				if ((syntaxError = expectToken(peekToken(), TokenId::Semicolon))) {
					goto genBadStmt;
				}

				nextToken();

				break;
			}
		}
	}

	return {};

genBadStmt:
	if (!(stmtOut = peff::makeShared<BadStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document, stmtOut).castTo<StmtNode>()))
		return genOutOfMemoryError();
	stmtOut->tokenRange = { prefixToken->index, parseContext.idxCurrentToken };
	return syntaxError;
}
