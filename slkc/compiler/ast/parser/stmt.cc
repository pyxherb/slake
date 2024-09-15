#include <slkc/compiler/compiler.h>

using namespace slake;
using namespace slake::slkc;

void Parser::parseVarDefs(std::shared_ptr<VarDefStmtNode> varDefStmtOut) {
	while (true) {
		Token *nameToken = expectToken(TokenId::Id);

		varDefStmtOut->varDefs[nameToken->text] = VarDefEntry(
			lexer->getTokenIndex(nameToken),
			nameToken->text,
			lexer->getTokenIndex(nameToken));
		VarDefEntry &entry = varDefStmtOut->varDefs[nameToken->text];

		if (Token *token = lexer->peekToken(); token->tokenId == TokenId::Colon) {
			Token *colonToken = lexer->nextToken();
			entry.tokenRange.endIndex = lexer->getTokenIndex(colonToken);
			entry.idxColonToken = lexer->getTokenIndex(colonToken);

			entry.type = parseTypeName();
			entry.tokenRange.endIndex = entry.type->tokenRange.endIndex;
		}

		if (Token *token = lexer->peekToken(); token->tokenId == TokenId::AssignOp) {
			Token *assignOpToken = lexer->nextToken();
			entry.tokenRange.endIndex = lexer->getTokenIndex(token);
			entry.idxAssignOpToken = lexer->getTokenIndex(assignOpToken);

			entry.initValue = parseExpr();
			entry.tokenRange.endIndex = entry.initValue->tokenRange.endIndex;
		}

		if (lexer->peekToken()->tokenId != TokenId::Comma) {
			varDefStmtOut->tokenRange.endIndex = entry.tokenRange.endIndex;
			break;
		}

		Token *commaToken = lexer->nextToken();
		entry.tokenRange.endIndex = lexer->getTokenIndex(commaToken);
		entry.idxCommaToken = lexer->getTokenIndex(commaToken);

		varDefStmtOut->tokenRange.endIndex = entry.tokenRange.endIndex;
	}
}

std::shared_ptr<StmtNode> Parser::parseStmt() {
	LexerContext beginContext = lexer->context;
	std::shared_ptr<StmtNode> result;

	Token *beginToken = lexer->peekToken();

	try {
		switch (beginToken->tokenId) {
			case TokenId::BreakKeyword: {
				auto stmt = std::make_shared<BreakStmtNode>();
				result = std::static_pointer_cast<StmtNode>(stmt);

				Token *breakToken = lexer->nextToken();
				stmt->tokenRange = lexer->getTokenIndex(breakToken);
				stmt->idxBreakToken = lexer->getTokenIndex(breakToken);

				Token *semicolonToken = expectToken(TokenId::Semicolon);
				stmt->tokenRange.endIndex = lexer->getTokenIndex(semicolonToken);
				stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);
				break;
			}
			case TokenId::ContinueKeyword: {
				auto stmt = std::make_shared<BreakStmtNode>();
				result = std::static_pointer_cast<StmtNode>(stmt);

				Token *breakToken = lexer->nextToken();
				result->tokenRange = lexer->getTokenIndex(breakToken);
				stmt->idxBreakToken = lexer->getTokenIndex(breakToken);

				Token *semicolonToken = expectToken(TokenId::Semicolon);
				stmt->tokenRange.endIndex = lexer->getTokenIndex(semicolonToken);
				stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);

				break;
			}
			case TokenId::ForKeyword: {
				auto stmt = std::make_shared<ForStmtNode>();
				result = std::static_pointer_cast<StmtNode>(stmt);

				Token *forToken = lexer->nextToken();
				stmt->tokenRange = lexer->getTokenIndex(forToken);
				stmt->idxForToken = lexer->getTokenIndex(forToken);

				Token *lParentheseToken = expectToken(TokenId::LParenthese);
				stmt->tokenRange.endIndex = lexer->getTokenIndex(lParentheseToken);
				stmt->idxLParentheseToken = lexer->getTokenIndex(lParentheseToken);

				if (Token *letToken = lexer->peekToken(); letToken->tokenId == TokenId::LetKeyword) {
					stmt->varDefs = std::make_shared<VarDefStmtNode>();

					stmt->varDefs->tokenRange = lexer->getTokenIndex(letToken);
					stmt->varDefs->idxLetToken = lexer->getTokenIndex(letToken);
					stmt->tokenRange.endIndex = stmt->varDefs->tokenRange.endIndex;

					parseVarDefs(stmt->varDefs);
					stmt->tokenRange.endIndex = stmt->varDefs->tokenRange.endIndex;
				}

				Token *firstSemicolonToken = expectToken(TokenId::Semicolon);
				stmt->tokenRange.endIndex = lexer->getTokenIndex(firstSemicolonToken);
				stmt->idxFirstSemicolonToken = lexer->getTokenIndex(firstSemicolonToken);

				if (Token *token = lexer->peekToken(); token->tokenId != TokenId::Semicolon) {
					stmt->condition = parseExpr();
					stmt->tokenRange.endIndex = stmt->condition->tokenRange.endIndex;
				}

				Token *secondSemicolonToken = expectToken(TokenId::Semicolon);
				stmt->tokenRange.endIndex = lexer->getTokenIndex(secondSemicolonToken);
				stmt->idxSecondSemicolonToken = lexer->getTokenIndex(secondSemicolonToken);

				if (Token *token = lexer->peekToken(); token->tokenId != TokenId::RParenthese) {
					stmt->endExpr = parseExpr();
					stmt->tokenRange.endIndex = stmt->endExpr->tokenRange.endIndex;
				}

				Token *rParentheseToken = expectToken(TokenId::RParenthese);
				stmt->tokenRange.endIndex = lexer->getTokenIndex(rParentheseToken);
				stmt->idxRParentheseToken = lexer->getTokenIndex(rParentheseToken);

				stmt->body = parseStmt();
				stmt->tokenRange.endIndex = stmt->body->tokenRange.endIndex;
				break;
			}
			case TokenId::WhileKeyword: {
				auto stmt = std::make_shared<WhileStmtNode>();
				result = std::static_pointer_cast<StmtNode>(stmt);

				Token *whileToken = lexer->nextToken();
				stmt->tokenRange = lexer->getTokenIndex(whileToken);
				stmt->idxWhileToken = lexer->getTokenIndex(whileToken);

				Token *lParentheseToken = expectToken(TokenId::LParenthese);
				stmt->tokenRange.endIndex = lexer->getTokenIndex(lParentheseToken);
				stmt->idxLParentheseToken = lexer->getTokenIndex(lParentheseToken);

				stmt->condition = parseExpr();
				stmt->tokenRange.endIndex = stmt->condition->tokenRange.endIndex;

				Token *rParentheseToken = expectToken(TokenId::RParenthese);
				stmt->tokenRange.endIndex = lexer->getTokenIndex(rParentheseToken);
				stmt->idxRParentheseToken = lexer->getTokenIndex(rParentheseToken);

				stmt->body = parseStmt();
				break;
			}
			case TokenId::ReturnKeyword: {
				auto stmt = std::make_shared<ReturnStmtNode>();
				result = std::static_pointer_cast<StmtNode>(stmt);

				Token *returnToken = lexer->nextToken();
				stmt->tokenRange = lexer->getTokenIndex(returnToken);
				stmt->idxReturnToken = lexer->getTokenIndex(returnToken);

				if (Token *token = lexer->peekToken(); token->tokenId == TokenId::Semicolon) {
					lexer->nextToken();
					stmt->tokenRange.endIndex = lexer->getTokenIndex(token);
					stmt->idxSemicolonToken = lexer->getTokenIndex(token);
				} else {
					stmt->returnValue = parseExpr();
					stmt->tokenRange.endIndex = stmt->returnValue->tokenRange.endIndex;

					Token *semicolonToken = expectToken(TokenId::Semicolon);
					stmt->tokenRange.endIndex = lexer->getTokenIndex(semicolonToken);
					stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);
				}
				break;
			}
			case TokenId::YieldKeyword: {
				auto stmt = std::make_shared<YieldStmtNode>();
				result = std::static_pointer_cast<StmtNode>(stmt);

				Token *yieldToken = lexer->nextToken();
				stmt->tokenRange = lexer->getTokenIndex(yieldToken);
				stmt->idxYieldToken = lexer->getTokenIndex(yieldToken);

				if (Token *token = lexer->peekToken(); token->tokenId == TokenId::Semicolon) {
					lexer->nextToken();
					stmt->tokenRange.endIndex = lexer->getTokenIndex(token);
					stmt->idxSemicolonToken = lexer->getTokenIndex(token);
				} else {
					stmt->returnValue = parseExpr();
					stmt->tokenRange.endIndex = stmt->returnValue->tokenRange.endIndex;

					Token *semicolonToken = expectToken(TokenId::Semicolon);
					stmt->tokenRange.endIndex = lexer->getTokenIndex(semicolonToken);
					stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);
				}
				break;
			}
			case TokenId::IfKeyword: {
				auto stmt = std::make_shared<IfStmtNode>();
				result = std::static_pointer_cast<StmtNode>(stmt);

				Token *ifToken = lexer->nextToken();
				stmt->tokenRange = lexer->getTokenIndex(ifToken);
				stmt->idxIfToken = lexer->getTokenIndex(ifToken);

				Token *lParentheseToken = expectToken(TokenId::LParenthese);
				stmt->tokenRange.endIndex = lexer->getTokenIndex(lParentheseToken);
				stmt->idxLParentheseToken = lexer->getTokenIndex(lParentheseToken);

				stmt->condition = parseExpr();
				stmt->tokenRange.endIndex = stmt->condition->tokenRange.endIndex;

				Token *rParentheseToken = expectToken(TokenId::RParenthese);
				stmt->tokenRange.endIndex = lexer->getTokenIndex(rParentheseToken);
				stmt->idxRParentheseToken = lexer->getTokenIndex(rParentheseToken);

				stmt->body = parseStmt();
				stmt->tokenRange.endIndex = stmt->body->tokenRange.endIndex;

				if (Token *elseToken = lexer->peekToken(); elseToken->tokenId == TokenId::ElseKeyword) {
					lexer->nextToken();
					stmt->tokenRange.endIndex = lexer->getTokenIndex(elseToken);
					stmt->idxElseToken = lexer->getTokenIndex(elseToken);

					stmt->elseBranch = parseStmt();
					stmt->tokenRange.endIndex = stmt->elseBranch->tokenRange.endIndex;
				}
				break;
			}
			case TokenId::TryKeyword: {
				auto stmt = std::make_shared<TryStmtNode>();
				result = std::static_pointer_cast<StmtNode>(stmt);

				Token *tryToken = lexer->nextToken();
				stmt->tokenRange = lexer->getTokenIndex(tryToken);
				stmt->idxTryToken = lexer->getTokenIndex(tryToken);

				stmt->body = parseStmt();
				stmt->tokenRange.endIndex = stmt->body->tokenRange.endIndex;

				while (true) {
					Token *catchToken = lexer->peekToken();
					if (catchToken->tokenId != TokenId::CatchKeyword)
						break;
					stmt->tokenRange.endIndex = lexer->getTokenIndex(catchToken);
					lexer->nextToken();

					stmt->catchBlocks.push_back({});
					CatchBlock &catchBlock = stmt->catchBlocks.back();

					catchBlock.idxCatchToken = lexer->getTokenIndex(catchToken);
					catchBlock.tokenRange = lexer->getTokenIndex(catchToken);

					Token *lParentheseToken = expectToken(TokenId::LParenthese);
					catchBlock.tokenRange.endIndex = lexer->getTokenIndex(lParentheseToken);
					catchBlock.idxLParentheseToken = lexer->getTokenIndex(lParentheseToken);

					catchBlock.targetType = parseTypeName();
					catchBlock.tokenRange.endIndex = catchBlock.targetType->tokenRange.endIndex;

					std::string exceptionVarName;

					if (Token *nameToken = lexer->peekToken(); nameToken->tokenId == TokenId::Id) {
						lexer->nextToken();
						catchBlock.tokenRange.endIndex = lexer->getTokenIndex(nameToken);
						catchBlock.idxExceptionVarNameToken = lexer->getTokenIndex(nameToken);

						exceptionVarName = nameToken->text;
					}

					Token *rParentheseToken = expectToken(TokenId::RParenthese);
					catchBlock.tokenRange.endIndex = lexer->getTokenIndex(rParentheseToken);
					catchBlock.idxRParentheseToken = lexer->getTokenIndex(rParentheseToken);

					catchBlock.body = parseStmt();
					catchBlock.tokenRange.endIndex = catchBlock.body->tokenRange.endIndex;

					stmt->tokenRange.endIndex = catchBlock.tokenRange.endIndex;
				}

				if (Token *finalToken = lexer->peekToken(); finalToken->tokenId == TokenId::FinalKeyword) {
					lexer->nextToken();
					stmt->finalBlock.tokenRange = lexer->getTokenIndex(finalToken);
					stmt->finalBlock.idxFinalToken = lexer->getTokenIndex(finalToken);

					stmt->finalBlock.body = parseStmt();
					stmt->finalBlock.tokenRange.endIndex = stmt->finalBlock.body->tokenRange.endIndex;

					stmt->tokenRange.endIndex = stmt->finalBlock.tokenRange.endIndex;
				}

				break;
			}
			case TokenId::SwitchKeyword: {
				const auto stmt = std::make_shared<SwitchStmtNode>();
				result = std::static_pointer_cast<StmtNode>(stmt);

				Token *switchToken = lexer->nextToken();
				stmt->tokenRange = lexer->getTokenIndex(switchToken);
				stmt->idxSwitchToken = lexer->getTokenIndex(switchToken);

				Token *lParentheseToken = expectToken(TokenId::LParenthese);
				stmt->tokenRange.endIndex = lexer->getTokenIndex(lParentheseToken);
				stmt->idxLParentheseToken = lexer->getTokenIndex(lParentheseToken);

				stmt->expr = parseExpr();
				stmt->tokenRange.endIndex = stmt->expr->tokenRange.endIndex;

				Token *rParentheseToken = expectToken(TokenId::RParenthese);
				stmt->tokenRange.endIndex = lexer->getTokenIndex(rParentheseToken);
				stmt->idxRParentheseToken = lexer->getTokenIndex(rParentheseToken);

				Token *lBraceToken = expectToken(TokenId::LBrace);
				stmt->tokenRange.endIndex = lexer->getTokenIndex(lBraceToken);
				stmt->idxLBraceToken = lexer->getTokenIndex(lBraceToken);

				while (true) {
					Token *caseToken = lexer->peekToken();

					if (caseToken->tokenId != TokenId::CaseKeyword)
						break;

					lexer->nextToken();

					stmt->cases.push_back({});
					SwitchCase &newCase = stmt->cases.back();

					newCase.tokenRange = lexer->getTokenIndex(caseToken);
					newCase.idxCaseToken = lexer->getTokenIndex(caseToken);
					;

					newCase.condition = parseExpr();
					newCase.tokenRange.endIndex = newCase.condition->tokenRange.endIndex;

					Token *colonToken = expectToken(TokenId::Colon);
					newCase.tokenRange.endIndex = lexer->getTokenIndex(colonToken);
					newCase.idxColonToken = lexer->getTokenIndex(colonToken);

					while (true) {
						if (Token *token = lexer->peekToken();
							(token->tokenId == TokenId::DefaultKeyword) ||
							(token->tokenId == TokenId::RBrace))
							break;

						auto newCaseStmt = parseStmt();
						newCase.body.push_back(newCaseStmt);
						newCase.tokenRange.endIndex = newCaseStmt->tokenRange.endIndex;

						stmt->tokenRange.endIndex = newCase.tokenRange.endIndex;
					}
				}

				if (Token *defaultToken = lexer->peekToken(); defaultToken->tokenId == TokenId::DefaultKeyword) {
					lexer->nextToken();

					stmt->cases.push_back({});
					SwitchCase &defaultCase = stmt->cases.back();

					defaultCase.tokenRange = lexer->getTokenIndex(defaultToken);
					defaultCase.idxCaseToken = lexer->getTokenIndex(defaultToken);

					Token *colonToken = expectToken(TokenId::Colon);
					defaultCase.tokenRange.endIndex = lexer->getTokenIndex(colonToken);
					defaultCase.idxColonToken = lexer->getTokenIndex(colonToken);

					while (true) {
						if (Token *token = lexer->peekToken(); token->tokenId == TokenId::RBrace)
							break;

						auto newDefaultCaseStmt = parseStmt();
						defaultCase.body.push_back(newDefaultCaseStmt);
						defaultCase.tokenRange.endIndex = newDefaultCaseStmt->tokenRange.endIndex;

						stmt->tokenRange.endIndex = defaultCase.tokenRange.endIndex;
					}
				}

				Token *rBraceToken = expectToken(TokenId::RBrace);
				stmt->tokenRange.endIndex = lexer->getTokenIndex(rBraceToken);
				stmt->idxRBraceToken = lexer->getTokenIndex(rBraceToken);

				break;
			}
			case TokenId::LBrace: {
				auto stmt = std::make_shared<CodeBlockStmtNode>(CodeBlock{ TokenRange(), {} });
				result = std::static_pointer_cast<StmtNode>(stmt);

				Token *lBraceToken = lexer->nextToken();
				stmt->body.tokenRange = lexer->getTokenIndex(lBraceToken);
				stmt->body.idxLBraceToken = lexer->getTokenIndex(lBraceToken);
				stmt->tokenRange = stmt->body.tokenRange;

				while (true) {
					if (lexer->peekToken()->tokenId == TokenId::RBrace) {
						Token *rBraceToken = lexer->nextToken();
						stmt->body.tokenRange = lexer->getTokenIndex(rBraceToken);
						stmt->body.idxRBraceToken = lexer->getTokenIndex(rBraceToken);
						break;
					}

					auto newStmt = parseStmt();
					stmt->body.stmts.push_back(newStmt);
					stmt->body.tokenRange.endIndex = newStmt->tokenRange.endIndex;
					stmt->tokenRange = stmt->body.tokenRange;
				}

				stmt->tokenRange = stmt->body.tokenRange;

				break;
			}
			case TokenId::LetKeyword: {
				Token *letToken = lexer->nextToken();

				auto stmt = std::make_shared<VarDefStmtNode>();
				result = std::static_pointer_cast<StmtNode>(stmt);

				stmt->tokenRange.endIndex = lexer->getTokenIndex(letToken);
				stmt->idxLetToken = lexer->getTokenIndex(letToken);

				parseVarDefs(stmt);

				Token *semicolonToken = expectToken(TokenId::Semicolon);
				stmt->tokenRange.endIndex = lexer->getTokenIndex(semicolonToken);
				stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);

				break;
			}
			default: {
				auto stmt = std::make_shared<ExprStmtNode>();
				result = stmt;

				stmt->expr = parseExpr();
				stmt->tokenRange = stmt->expr->tokenRange;

				Token *semicolonToken = expectToken(TokenId::Semicolon);
				stmt->tokenRange.endIndex = lexer->getTokenIndex(semicolonToken);
				stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);

				break;
			}
		}
	} catch (SyntaxError e) {
		compiler->messages.push_back(
			Message(
				compiler->tokenRangeToSourceLocation(e.tokenRange),
				MessageType::Error,
				e.what()));

		auto badStmt = std::make_shared<BadStmtNode>(result);
		badStmt->tokenRange = TokenRange{ lexer->getTokenIndex(beginToken), e.tokenRange.endIndex };

		return badStmt;
	}

	return result;
}
