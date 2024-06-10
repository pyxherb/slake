#include <slkc/compiler/compiler.h>

using namespace slake;
using namespace slake::slkc;

void Parser::parseVarDefs(std::shared_ptr<VarDefStmtNode> varDefStmtOut) {
	while (true) {
		Token *nameToken = expectToken(TokenId::Id);

		varDefStmtOut->varDefs[nameToken->text] = VarDefEntry(
			nameToken->location,
			nameToken->text,
			lexer->getTokenIndex(nameToken));
		VarDefEntry &entry = varDefStmtOut->varDefs[nameToken->text];

		if (Token *token = lexer->peekToken(); token->tokenId == TokenId::Colon) {
			Token *colonToken = lexer->nextToken();
			entry.loc.endPosition = colonToken->location.endPosition;
			entry.idxColonToken = lexer->getTokenIndex(colonToken);

			entry.type = parseTypeName();
			entry.loc.endPosition = entry.type->sourceLocation.endPosition;
		}

		if (Token *token = lexer->peekToken(); token->tokenId == TokenId::AssignOp) {
			Token *assignOpToken = lexer->nextToken();
			entry.loc.endPosition = token->location.endPosition;
			entry.idxAssignOpToken = lexer->getTokenIndex(assignOpToken);

			entry.initValue = parseExpr();
			entry.loc.endPosition = entry.initValue->sourceLocation.endPosition;
		}

		if (lexer->peekToken()->tokenId != TokenId::Comma) {
			varDefStmtOut->sourceLocation.endPosition = entry.loc.endPosition;
			break;
		}

		Token *commaToken = lexer->nextToken();
		entry.loc.endPosition = commaToken->location.endPosition;
		entry.idxCommaToken = lexer->getTokenIndex(commaToken);

		varDefStmtOut->sourceLocation.endPosition = entry.loc.endPosition;
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
				stmt->sourceLocation = breakToken->location;
				stmt->idxBreakToken = lexer->getTokenIndex(breakToken);

				Token *semicolonToken = expectToken(TokenId::Semicolon);
				stmt->sourceLocation.endPosition = semicolonToken->location.endPosition;
				stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);
				break;
			}
			case TokenId::ContinueKeyword: {
				auto stmt = std::make_shared<BreakStmtNode>();
				result = std::static_pointer_cast<StmtNode>(stmt);

				Token *breakToken = lexer->nextToken();
				result->sourceLocation = breakToken->location;
				stmt->idxBreakToken = lexer->getTokenIndex(breakToken);

				Token *semicolonToken = expectToken(TokenId::Semicolon);
				stmt->sourceLocation.endPosition = semicolonToken->location.endPosition;
				stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);

				break;
			}
			case TokenId::ForKeyword: {
				auto stmt = std::make_shared<ForStmtNode>();
				result = std::static_pointer_cast<StmtNode>(stmt);

				Token *forToken = lexer->nextToken();
				stmt->sourceLocation = forToken->location;
				stmt->idxForToken = lexer->getTokenIndex(forToken);

				Token *lParentheseToken = expectToken(TokenId::LParenthese);
				stmt->sourceLocation.endPosition = lParentheseToken->location.endPosition;
				stmt->idxLParentheseToken = lexer->getTokenIndex(lParentheseToken);

				if (Token *letToken = lexer->peekToken(); letToken->tokenId == TokenId::LetKeyword) {
					stmt->varDefs = std::make_shared<VarDefStmtNode>();

					stmt->varDefs->sourceLocation = letToken->location;
					stmt->varDefs->idxLetToken = lexer->getTokenIndex(letToken);
					stmt->sourceLocation.endPosition = stmt->varDefs->sourceLocation.endPosition;

					parseVarDefs(stmt->varDefs);
					stmt->sourceLocation.endPosition = stmt->varDefs->sourceLocation.endPosition;
				}

				Token *firstSemicolonToken = expectToken(TokenId::Semicolon);
				stmt->sourceLocation.endPosition = firstSemicolonToken->location.endPosition;
				stmt->idxFirstSemicolonToken = lexer->getTokenIndex(firstSemicolonToken);

				if (Token *token = lexer->peekToken(); token->tokenId != TokenId::Semicolon) {
					stmt->condition = parseExpr();
					stmt->sourceLocation.endPosition = stmt->condition->sourceLocation.endPosition;
				}

				Token *secondSemicolonToken = expectToken(TokenId::Semicolon);
				stmt->sourceLocation.endPosition = secondSemicolonToken->location.endPosition;
				stmt->idxSecondSemicolonToken = lexer->getTokenIndex(secondSemicolonToken);

				if (Token *token = lexer->peekToken(); token->tokenId != TokenId::RParenthese) {
					stmt->endExpr = parseExpr();
					stmt->sourceLocation.endPosition = stmt->endExpr->sourceLocation.endPosition;
				}

				Token *rParentheseToken = expectToken(TokenId::RParenthese);
				stmt->sourceLocation.endPosition = rParentheseToken->location.endPosition;
				stmt->idxRParentheseToken = lexer->getTokenIndex(rParentheseToken);

				stmt->body = parseStmt();
				stmt->sourceLocation.endPosition = stmt->body->sourceLocation.endPosition;
				break;
			}
			case TokenId::WhileKeyword: {
				auto stmt = std::make_shared<WhileStmtNode>();
				result = std::static_pointer_cast<StmtNode>(stmt);

				Token *whileToken = lexer->nextToken();
				stmt->sourceLocation = whileToken->location;
				stmt->idxWhileToken = lexer->getTokenIndex(whileToken);

				Token *lParentheseToken = expectToken(TokenId::LParenthese);
				stmt->sourceLocation.endPosition = lParentheseToken->location.endPosition;
				stmt->idxLParentheseToken = lexer->getTokenIndex(lParentheseToken);

				stmt->condition = parseExpr();
				stmt->sourceLocation.endPosition = stmt->condition->sourceLocation.endPosition;

				Token *rParentheseToken = expectToken(TokenId::RParenthese);
				stmt->sourceLocation.endPosition = rParentheseToken->location.endPosition;
				stmt->idxRParentheseToken = lexer->getTokenIndex(rParentheseToken);

				stmt->body = parseStmt();
				break;
			}
			case TokenId::ReturnKeyword: {
				auto stmt = std::make_shared<ReturnStmtNode>();
				result = std::static_pointer_cast<StmtNode>(stmt);

				Token *returnToken = lexer->nextToken();
				stmt->sourceLocation = returnToken->location;
				stmt->idxReturnToken = lexer->getTokenIndex(returnToken);

				if (Token *token = lexer->peekToken(); token->tokenId == TokenId::Semicolon) {
					lexer->nextToken();
					stmt->sourceLocation.endPosition = token->location.endPosition;
					stmt->idxSemicolonToken = lexer->getTokenIndex(token);
				} else {
					stmt->returnValue = parseExpr();
					stmt->sourceLocation.endPosition = stmt->returnValue->sourceLocation.endPosition;

					Token *semicolonToken = expectToken(TokenId::Semicolon);
					stmt->sourceLocation.endPosition = semicolonToken->location.endPosition;
					stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);
				}
				break;
			}
			case TokenId::YieldKeyword: {
				auto stmt = std::make_shared<YieldStmtNode>();
				result = std::static_pointer_cast<StmtNode>(stmt);

				Token *yieldToken = lexer->nextToken();
				stmt->sourceLocation = yieldToken->location;
				stmt->idxYieldToken = lexer->getTokenIndex(yieldToken);

				if (Token *token = lexer->peekToken(); token->tokenId == TokenId::Semicolon) {
					lexer->nextToken();
					stmt->sourceLocation.endPosition = token->location.endPosition;
					stmt->idxSemicolonToken = lexer->getTokenIndex(token);
				} else {
					stmt->returnValue = parseExpr();
					stmt->sourceLocation.endPosition = stmt->returnValue->sourceLocation.endPosition;

					Token *semicolonToken = expectToken(TokenId::Semicolon);
					stmt->sourceLocation.endPosition = semicolonToken->location.endPosition;
					stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);
				}
				break;
			}
			case TokenId::IfKeyword: {
				auto stmt = std::make_shared<IfStmtNode>();
				result = std::static_pointer_cast<StmtNode>(stmt);

				Token *ifToken = lexer->nextToken();
				stmt->sourceLocation = ifToken->location;
				stmt->idxIfToken = lexer->getTokenIndex(ifToken);

				Token *lParentheseToken = expectToken(TokenId::LParenthese);
				stmt->sourceLocation.endPosition = lParentheseToken->location.endPosition;
				stmt->idxLParentheseToken = lexer->getTokenIndex(lParentheseToken);

				stmt->condition = parseExpr();
				stmt->sourceLocation.endPosition = stmt->condition->sourceLocation.endPosition;

				Token *rParentheseToken = expectToken(TokenId::RParenthese);
				stmt->sourceLocation.endPosition = rParentheseToken->location.endPosition;
				stmt->idxRParentheseToken = lexer->getTokenIndex(rParentheseToken);

				stmt->body = parseStmt();
				stmt->sourceLocation.endPosition = stmt->body->sourceLocation.endPosition;

				if (Token *elseToken = lexer->peekToken(); elseToken->tokenId == TokenId::ElseKeyword) {
					lexer->nextToken();
					stmt->sourceLocation.endPosition = elseToken->location.endPosition;
					stmt->idxElseToken = lexer->getTokenIndex(elseToken);

					stmt->elseBranch = parseStmt();
					stmt->sourceLocation.endPosition = stmt->elseBranch->sourceLocation.endPosition;
				}
				break;
			}
			case TokenId::TryKeyword: {
				auto stmt = std::make_shared<TryStmtNode>();
				result = std::static_pointer_cast<StmtNode>(stmt);

				Token *tryToken = lexer->nextToken();
				stmt->sourceLocation = tryToken->location;
				stmt->idxTryToken = lexer->getTokenIndex(tryToken);

				stmt->body = parseStmt();
				stmt->sourceLocation.endPosition = stmt->body->sourceLocation.endPosition;

				while (true) {
					Token *catchToken = lexer->peekToken();
					if (catchToken->tokenId != TokenId::CatchKeyword)
						break;
					stmt->sourceLocation.endPosition = catchToken->location.endPosition;
					lexer->nextToken();

					stmt->catchBlocks.push_back({});
					CatchBlock &catchBlock = stmt->catchBlocks.back();

					catchBlock.idxCatchToken = lexer->getTokenIndex(catchToken);
					catchBlock.loc = catchToken->location;

					Token *lParentheseToken = expectToken(TokenId::LParenthese);
					catchBlock.loc.endPosition = lParentheseToken->location.endPosition;
					catchBlock.idxLParentheseToken = lexer->getTokenIndex(lParentheseToken);

					catchBlock.targetType = parseTypeName();
					catchBlock.loc.endPosition = catchBlock.targetType->sourceLocation.endPosition;

					std::string exceptionVarName;

					if (Token *nameToken = lexer->peekToken(); nameToken->tokenId == TokenId::Id) {
						lexer->nextToken();
						catchBlock.loc.endPosition = nameToken->location.endPosition;
						catchBlock.idxExceptionVarNameToken = lexer->getTokenIndex(nameToken);

						exceptionVarName = nameToken->text;
					}

					Token *rParentheseToken = expectToken(TokenId::RParenthese);
					catchBlock.loc.endPosition = rParentheseToken->location.endPosition;
					catchBlock.idxRParentheseToken = lexer->getTokenIndex(rParentheseToken);

					catchBlock.body = parseStmt();
					catchBlock.loc.endPosition = catchBlock.body->sourceLocation.endPosition;

					stmt->sourceLocation.endPosition = catchBlock.loc.endPosition;
				}

				if (Token *finalToken = lexer->peekToken(); finalToken->tokenId == TokenId::FinalKeyword) {
					lexer->nextToken();
					stmt->finalBlock.loc = finalToken->location;
					stmt->finalBlock.idxFinalToken = lexer->getTokenIndex(finalToken);

					stmt->finalBlock.body = parseStmt();
					stmt->finalBlock.loc.endPosition = stmt->finalBlock.body->sourceLocation.endPosition;

					stmt->sourceLocation.endPosition = stmt->finalBlock.loc.endPosition;
				}

				break;
			}
			case TokenId::SwitchKeyword: {
				const auto stmt = std::make_shared<SwitchStmtNode>();
				result = std::static_pointer_cast<StmtNode>(stmt);

				Token *switchToken = lexer->nextToken();
				stmt->sourceLocation = switchToken->location;
				stmt->idxSwitchToken = lexer->getTokenIndex(switchToken);

				Token *lParentheseToken = expectToken(TokenId::LParenthese);
				stmt->sourceLocation.endPosition = lParentheseToken->location.endPosition;
				stmt->idxLParentheseToken = lexer->getTokenIndex(lParentheseToken);

				stmt->expr = parseExpr();
				stmt->sourceLocation.endPosition = stmt->expr->sourceLocation.endPosition;

				Token *rParentheseToken = expectToken(TokenId::RParenthese);
				stmt->sourceLocation.endPosition = rParentheseToken->location.endPosition;
				stmt->idxRParentheseToken = lexer->getTokenIndex(rParentheseToken);

				Token *lBraceToken = expectToken(TokenId::LBrace);
				stmt->sourceLocation.endPosition = lBraceToken->location.endPosition;
				stmt->idxLBraceToken = lexer->getTokenIndex(lBraceToken);

				while (true) {
					Token *caseToken = lexer->peekToken();

					if (caseToken->tokenId != TokenId::CaseKeyword)
						break;

					lexer->nextToken();

					stmt->cases.push_back({});
					SwitchCase &newCase = stmt->cases.back();

					newCase.loc = caseToken->location;
					newCase.idxCaseToken = lexer->getTokenIndex(caseToken);
					;

					newCase.condition = parseExpr();
					newCase.loc.endPosition = newCase.condition->sourceLocation.endPosition;

					Token *colonToken = expectToken(TokenId::Colon);
					newCase.loc.endPosition = colonToken->location.endPosition;
					newCase.idxColonToken = lexer->getTokenIndex(colonToken);

					while (true) {
						if (Token *token = lexer->peekToken();
							(token->tokenId == TokenId::DefaultKeyword) ||
							(token->tokenId == TokenId::RBrace))
							break;

						auto newCaseStmt = parseStmt();
						newCase.body.push_back(newCaseStmt);
						newCase.loc.endPosition = newCaseStmt->sourceLocation.endPosition;

						stmt->sourceLocation.endPosition = newCase.loc.endPosition;
					}
				}

				if (Token *defaultToken = lexer->peekToken(); defaultToken->tokenId == TokenId::DefaultKeyword) {
					lexer->nextToken();

					stmt->cases.push_back({});
					SwitchCase &defaultCase = stmt->cases.back();

					defaultCase.loc = defaultToken->location;
					defaultCase.idxCaseToken = lexer->getTokenIndex(defaultToken);

					Token *colonToken = expectToken(TokenId::Colon);
					defaultCase.loc.endPosition = colonToken->location.endPosition;
					defaultCase.idxColonToken = lexer->getTokenIndex(colonToken);

					while (true) {
						if (Token *token = lexer->peekToken(); token->tokenId == TokenId::RBrace)
							break;

						auto newDefaultCaseStmt = parseStmt();
						defaultCase.body.push_back(newDefaultCaseStmt);
						defaultCase.loc.endPosition = newDefaultCaseStmt->sourceLocation.endPosition;

						stmt->sourceLocation.endPosition = defaultCase.loc.endPosition;
					}
				}

				Token *rBraceToken = expectToken(TokenId::RBrace);
				stmt->sourceLocation.endPosition = rBraceToken->location.endPosition;
				stmt->idxRBraceToken = lexer->getTokenIndex(rBraceToken);

				break;
			}
			case TokenId::LBrace: {
				auto stmt = std::make_shared<CodeBlockStmtNode>(CodeBlock{ SourceLocation(), {} });
				result = std::static_pointer_cast<StmtNode>(stmt);

				Token *lBraceToken = lexer->nextToken();
				stmt->body.loc = lBraceToken->location;
				stmt->body.idxLBraceToken = lexer->getTokenIndex(lBraceToken);
				stmt->sourceLocation = stmt->body.loc;

				while (true) {
					if (lexer->peekToken()->tokenId == TokenId::RBrace) {
						Token *rBraceToken = lexer->nextToken();
						stmt->body.loc = rBraceToken->location;
						stmt->body.idxRBraceToken = lexer->getTokenIndex(rBraceToken);
						break;
					}

					auto newStmt = parseStmt();
					stmt->body.stmts.push_back(newStmt);
					stmt->body.loc.endPosition = newStmt->sourceLocation.endPosition;
					stmt->sourceLocation = stmt->body.loc;
				}

				stmt->sourceLocation = stmt->body.loc;

				break;
			}
			case TokenId::LetKeyword: {
				Token *letToken = lexer->nextToken();

				auto stmt = std::make_shared<VarDefStmtNode>();
				result = std::static_pointer_cast<StmtNode>(stmt);

				stmt->sourceLocation.endPosition = letToken->location.endPosition;
				stmt->idxLetToken = lexer->getTokenIndex(letToken);

				parseVarDefs(stmt);

				Token *semicolonToken = expectToken(TokenId::Semicolon);
				stmt->sourceLocation.endPosition = semicolonToken->location.endPosition;
				stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);

				break;
			}
			default: {
				auto stmt = std::make_shared<ExprStmtNode>();
				result = stmt;

				stmt->expr = parseExpr();
				stmt->sourceLocation = stmt->expr->sourceLocation;

				Token *semicolonToken = expectToken(TokenId::Semicolon);
				stmt->sourceLocation.endPosition = semicolonToken->location.endPosition;
				stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);

				break;
			}
		}
	} catch (SyntaxError e) {
		compiler->messages.push_back(
			Message(
				e.location,
				MessageType::Error,
				e.what()));

		auto badStmt = std::make_shared<BadStmtNode>(result);
		badStmt->sourceLocation = SourceLocation{ beginToken->location.beginPosition, e.location.endPosition };

		return badStmt;
	}

	return result;
}
