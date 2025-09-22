#include "../parser.h"

using namespace slkc;

SLKC_API std::optional<SyntaxError> Parser::parseVarDefs(peff::DynArray<VarDefEntryPtr> &varDefEntries) {
	Token *currentToken;
	std::optional<SyntaxError> syntaxError;
	AstNodePtr<TypeNameNode> type;
	AstNodePtr<ExprNode> initialValue;

	for (;;) {
		bool isSealed = false;

		peff::DynArray<AstNodePtr<AttributeNode>> attributes(resourceAllocator.get());

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

SLKC_API std::optional<SyntaxError> Parser::parseStmt(AstNodePtr<StmtNode> &stmtOut) {
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

				AstNodePtr<IfStmtNode> ifStmt;

				if (!(ifStmt = makeAstNode<IfStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
					return genOutOfMemoryError();
				}

				stmtOut = ifStmt.template castTo<StmtNode>();

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

				AstNodePtr<ForStmtNode> forStmt;

				if (!(forStmt = makeAstNode<ForStmtNode>(
						  resourceAllocator.get(),
						  resourceAllocator.get(),
						  document))) {
					return genOutOfMemoryError();
				}

				stmtOut = forStmt.template castTo<StmtNode>();

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

				AstNodePtr<WhileStmtNode> whileStmt;

				if (!(whileStmt = makeAstNode<WhileStmtNode>(
						  resourceAllocator.get(),
						  resourceAllocator.get(),
						  document))) {
					return genOutOfMemoryError();
				}

				stmtOut = whileStmt.template castTo<StmtNode>();

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

				AstNodePtr<WhileStmtNode> whileStmt;

				if (!(whileStmt = makeAstNode<WhileStmtNode>(
						  resourceAllocator.get(),
						  resourceAllocator.get(),
						  document))) {
					return genOutOfMemoryError();
				}

				stmtOut = whileStmt.template castTo<StmtNode>();

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

				AstNodePtr<VarDefStmtNode> stmt;

				if (!(stmt = makeAstNode<VarDefStmtNode>(
						  resourceAllocator.get(),
						  resourceAllocator.get(),
						  document,
						  peff::DynArray<VarDefEntryPtr>(resourceAllocator.get())))) {
					return genOutOfMemoryError();
				}

				stmtOut = stmt.template castTo<StmtNode>();

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

				AstNodePtr<BreakStmtNode> stmt;

				if (!(stmt = makeAstNode<BreakStmtNode>(
						  resourceAllocator.get(),
						  resourceAllocator.get(),
						  document))) {
					return genOutOfMemoryError();
				}

				stmtOut = stmt.template castTo<StmtNode>();

				Token *semicolonToken;

				if ((syntaxError = expectToken((semicolonToken = peekToken()), TokenId::Semicolon))) {
					goto genBadStmt;
				}

				nextToken();

				break;
			}
			case TokenId::ContinueKeyword: {
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

				if ((syntaxError = expectToken((semicolonToken = peekToken()), TokenId::Semicolon))) {
					goto genBadStmt;
				}

				nextToken();

				break;
			}
			case TokenId::ReturnKeyword: {
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

				break;
			}
			case TokenId::CaseKeyword: {
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

				break;
			}
			case TokenId::DefaultKeyword: {
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

				break;
			}
			case TokenId::SwitchKeyword: {
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

				break;
			}
			case TokenId::LBrace: {
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

				break;
			}
			default: {
				AstNodePtr<ExprNode> curExpr;

				AstNodePtr<ExprStmtNode> stmt;

				if (!(stmt = makeAstNode<ExprStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document))) {
					return genOutOfMemoryError();
				}

				stmtOut = stmt.template castTo<StmtNode>();

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
	if (!(stmtOut = makeAstNode<BadStmtNode>(resourceAllocator.get(), resourceAllocator.get(), document, stmtOut).template castTo<StmtNode>()))
		return genOutOfMemoryError();
	stmtOut->tokenRange = { prefixToken->index, parseContext.idxCurrentToken };
	return syntaxError;
}
