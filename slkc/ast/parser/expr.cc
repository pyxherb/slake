#include "../parser.h"

using namespace slkc;

SLKC_API std::optional<SyntaxError> Parser::parseExpr(int precedence, peff::SharedPtr<ExprNode> &exprOut) {
	Token *prefixToken;

	std::optional<SyntaxError> syntaxError;
	peff::SharedPtr<ExprNode> lhs, rhs;

	if ((syntaxError = expectToken((prefixToken = peekToken()))))
		goto genBadExpr;

	switch (prefixToken->tokenId) {
		case TokenId::ThisKeyword:
		case TokenId::ScopeOp:
		case TokenId::Id: {
			IdRefPtr idRefPtr;
			if ((syntaxError = parseIdRef(idRefPtr)))
				goto genBadExpr;
			if (!(lhs = peff::makeShared<IdRefExprNode>(resourceAllocator.get(), resourceAllocator.get(), document, std::move(idRefPtr)).castTo<ExprNode>()))
				return genOutOfMemoryError();
			break;
		}
		case TokenId::LParenthese: {
			nextToken();

			if ((syntaxError = parseExpr(0, lhs)))
				goto genBadExpr;

			Token *rParentheseToken;

			if ((syntaxError = expectToken((rParentheseToken = nextToken()), TokenId::RParenthese)))
				goto genBadExpr;
			break;
		}
		case TokenId::NewKeyword: {
			nextToken();

			peff::SharedPtr<NewExprNode> expr;

			if (!(expr = peff::makeShared<NewExprNode>(
					  resourceAllocator.get(), resourceAllocator.get(), document)))
				return genOutOfMemoryError();

			lhs = expr.castTo<ExprNode>();

			if ((syntaxError = parseTypeName(expr->targetType)))
				goto genBadExpr;

			Token *lParentheseToken;

			if ((syntaxError = expectToken((lParentheseToken = peekToken()), TokenId::LParenthese)))
				goto genBadExpr;

			nextToken();

			if ((syntaxError = parseArgs(expr->args, expr->idxCommaTokens)))
				goto genBadExpr;

			Token *rParentheseToken;

			if ((syntaxError = expectToken((rParentheseToken = peekToken()), TokenId::RParenthese)))
				goto genBadExpr;

			nextToken();
			break;
		}
		case TokenId::IntLiteral: {
			nextToken();
			if (!(lhs = peff::makeShared<I32LiteralExprNode>(
					  resourceAllocator.get(), resourceAllocator.get(), document,
					  ((IntTokenExtension *)prefixToken->exData.get())->data)
							.castTo<ExprNode>()))
				return genOutOfMemoryError();
			break;
		}
		case TokenId::LongLiteral: {
			nextToken();
			if (!(lhs = peff::makeShared<I64LiteralExprNode>(
					  resourceAllocator.get(), resourceAllocator.get(), document,
					  ((LongTokenExtension *)prefixToken->exData.get())->data)
							.castTo<ExprNode>()))
				return genOutOfMemoryError();
			break;
		}
		case TokenId::UIntLiteral: {
			nextToken();
			if (!(lhs = peff::makeShared<U32LiteralExprNode>(
					  resourceAllocator.get(), resourceAllocator.get(), document,
					  ((UIntTokenExtension *)prefixToken->exData.get())->data)
							.castTo<ExprNode>()))
				return genOutOfMemoryError();
			break;
		}
		case TokenId::ULongLiteral: {
			nextToken();
			if (!(lhs = peff::makeShared<U64LiteralExprNode>(
					  resourceAllocator.get(), resourceAllocator.get(), document,
					  ((ULongTokenExtension *)prefixToken->exData.get())->data)
							.castTo<ExprNode>()))
				return genOutOfMemoryError();
			break;
		}
		case TokenId::StringLiteral: {
			nextToken();
			peff::String s(resourceAllocator.get());

			if (!peff::copyAssign(s, ((StringTokenExtension *)prefixToken->exData.get())->data))
				return genOutOfMemoryError();

			if (!(lhs = peff::makeShared<StringLiteralExprNode>(
					  resourceAllocator.get(), resourceAllocator.get(), document,
					  std::move(s))
							.castTo<ExprNode>()))
				return genOutOfMemoryError();
			break;
		}
		case TokenId::F32Literal: {
			nextToken();
			if (!(lhs = peff::makeShared<F32LiteralExprNode>(
					  resourceAllocator.get(), resourceAllocator.get(), document,
					  ((F32TokenExtension *)prefixToken->exData.get())->data)
							.castTo<ExprNode>()))
				return genOutOfMemoryError();
			break;
		}
		case TokenId::F64Literal: {
			nextToken();
			if (!(lhs = peff::makeShared<F64LiteralExprNode>(
					  resourceAllocator.get(), resourceAllocator.get(), document,
					  ((F64TokenExtension *)prefixToken->exData.get())->data)
							.castTo<ExprNode>()))
				return genOutOfMemoryError();
			break;
		}
		case TokenId::TrueKeyword: {
			nextToken();
			if (!(lhs = peff::makeShared<BoolLiteralExprNode>(
					  resourceAllocator.get(), resourceAllocator.get(), document,
					  true)
							.castTo<ExprNode>()))
				return genOutOfMemoryError();
			break;
		}
		case TokenId::FalseKeyword: {
			nextToken();
			if (!(lhs = peff::makeShared<BoolLiteralExprNode>(
					  resourceAllocator.get(), resourceAllocator.get(), document,
					  false)
							.castTo<ExprNode>()))
				return genOutOfMemoryError();
			break;
		}
		case TokenId::NullKeyword: {
			nextToken();
			if (!(lhs = peff::makeShared<NullLiteralExprNode>(
					  resourceAllocator.get(), resourceAllocator.get(), document)
							.castTo<ExprNode>()))
				return genOutOfMemoryError();
			break;
		}
		case TokenId::VarArg: {
			nextToken();
			if (!(lhs = peff::makeShared<VarArgExprNode>(
					  resourceAllocator.get(), resourceAllocator.get(), document)
						.castTo<ExprNode>()))
				return genOutOfMemoryError();
			break;
		}
		case TokenId::LBrace: {
			nextToken();

			peff::SharedPtr<InitializerListExprNode> initializerExpr;

			if (!(initializerExpr = peff::makeShared<InitializerListExprNode>(
					  resourceAllocator.get(), resourceAllocator.get(), document)))
				return genOutOfMemoryError();

			lhs = initializerExpr.castTo<ExprNode>();

			peff::SharedPtr<ExprNode> curExpr;

			static TokenId matchingTokens[] = {
				TokenId::RBrace,
				TokenId::Semicolon
			};

			Token *currentToken;

			for (;;) {
				if ((syntaxError = parseExpr(0, curExpr))) {
					if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
						return genOutOfMemoryError();
					syntaxError.reset();
					goto genBadExpr;
				}

				if (!initializerExpr->elements.pushBack(std::move(curExpr))) {
					return genOutOfMemoryError();
				}

				currentToken = peekToken();

				if (currentToken->tokenId != TokenId::Comma)
					break;

				nextToken();
			}

			if ((syntaxError = expectToken(currentToken = peekToken(), TokenId::RBrace))) {
				if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
					return genOutOfMemoryError();
				syntaxError.reset();
				if ((syntaxError = lookaheadUntil(std::size(matchingTokens), matchingTokens)))
					goto genBadExpr;
			}

			nextToken();

			break;
		}
		case TokenId::SubOp: {
			nextToken();

			peff::SharedPtr<UnaryExprNode> expr;

			if (!(expr = peff::makeShared<UnaryExprNode>(
					  resourceAllocator.get(), resourceAllocator.get(), document, UnaryOp::Neg, peff::SharedPtr<ExprNode>())))
				return genOutOfMemoryError();

			lhs = expr.castTo<ExprNode>();

			if ((syntaxError = parseExpr(131, expr->operand))) {
				goto genBadExpr;
			}
			break;
		}
		case TokenId::NotOp: {
			nextToken();

			peff::SharedPtr<UnaryExprNode> expr;

			if (!(expr = peff::makeShared<UnaryExprNode>(
					  resourceAllocator.get(), resourceAllocator.get(), document, UnaryOp::Neg, peff::SharedPtr<ExprNode>())))
				return genOutOfMemoryError();

			lhs = expr.castTo<ExprNode>();

			if ((syntaxError = parseExpr(131, expr->operand))) {
				goto genBadExpr;
			}
			break;
		}
		case TokenId::LNotOp: {
			nextToken();

			peff::SharedPtr<UnaryExprNode> expr;

			if (!(expr = peff::makeShared<UnaryExprNode>(
					  resourceAllocator.get(), resourceAllocator.get(), document, UnaryOp::Neg, peff::SharedPtr<ExprNode>())))
				return genOutOfMemoryError();

			lhs = expr.castTo<ExprNode>();

			if ((syntaxError = parseExpr(131, expr->operand))) {
				goto genBadExpr;
			}

			break;
		}
		default:
			nextToken();
			return SyntaxError(
				TokenRange{ prefixToken->index },
				SyntaxErrorKind::ExpectingExpr);
	}

	Token *infixToken;

	for (;;) {
		switch ((infixToken = peekToken())->tokenId) {
			case TokenId::LParenthese: {
				if (precedence > 140)
					goto end;
				nextToken();

				peff::SharedPtr<CallExprNode> expr;

				if (!(expr = peff::makeShared<CallExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, peff::SharedPtr<ExprNode>(), peff::DynArray<peff::SharedPtr<ExprNode>>{ resourceAllocator.get() })))
					return genOutOfMemoryError();

				expr->target = lhs;
				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				lhs = expr.castTo<ExprNode>();

				expr->lParentheseTokenIndex = infixToken->index;

				if ((syntaxError = parseArgs(expr->args, expr->idxCommaTokens))) {
					goto genBadExpr;
				}

				Token *rParentheseToken;

				if ((syntaxError = expectToken((rParentheseToken = peekToken()), TokenId::RParenthese)))
					goto genBadExpr;

				nextToken();

				expr->rParentheseTokenIndex = rParentheseToken->index;

				break;
			}
			case TokenId::LBracket: {
				if (precedence > 140)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::Add, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(0, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				Token *rBracketToken;

				if ((syntaxError = expectToken((rBracketToken = nextToken()), TokenId::RBracket)))
					goto genBadExpr;

				break;
			}
			case TokenId::Dot: {
				if (precedence > 140)
					goto end;
				nextToken();

				peff::SharedPtr<HeadedIdRefExprNode> expr;

				if (!(expr = peff::makeShared<HeadedIdRefExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, lhs, IdRefPtr{})))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseIdRef(expr->idRefPtr)))
					goto genBadExpr;

				break;
			}
			case TokenId::AsKeyword: {
				if (precedence > 130)
					goto end;
				nextToken();

				peff::SharedPtr<CastExprNode> expr;

				if (!(expr = peff::makeShared<CastExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, peff::SharedPtr<TypeNameNode>(), lhs)))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseTypeName(expr->targetType)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->targetType->tokenRange.endIndex;

				break;
			}

			case TokenId::MulOp: {
				if (precedence > 120)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::Mul, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(121, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}
			case TokenId::DivOp: {
				if (precedence > 120)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::Div, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

					lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(121, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}
			case TokenId::ModOp: {
				if (precedence > 120)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::Mod, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

					lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(121, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}

			case TokenId::AddOp: {
				if (precedence > 110)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::Add, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(111, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}
			case TokenId::SubOp: {
				if (precedence > 110)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::Sub, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(111, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}

			case TokenId::LshOp: {
				if (precedence > 100)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::Shl, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(101, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}
			case TokenId::RshOp: {
				if (precedence > 100)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::Shr, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(101, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}

			case TokenId::CmpOp: {
				if (precedence > 90)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::Cmp, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(91, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}

			case TokenId::GtOp: {
				if (precedence > 80)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::Gt, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(81, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}
			case TokenId::GtEqOp: {
				if (precedence > 80)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::GtEq, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(81, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}
			case TokenId::LtOp: {
				if (precedence > 80)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::Lt, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(81, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}
			case TokenId::LtEqOp: {
				if (precedence > 80)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::LtEq, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(81, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}

			case TokenId::EqOp: {
				if (precedence > 70)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::Eq, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(71, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}
			case TokenId::NeqOp: {
				if (precedence > 70)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::Neq, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(71, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}
			case TokenId::StrictEqOp: {
				if (precedence > 70)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::StrictEq, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(71, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}
			case TokenId::StrictNeqOp: {
				if (precedence > 70)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::StrictNeq, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(71, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}

			case TokenId::AndOp: {
				if (precedence > 60)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::And, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(61, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}

			case TokenId::XorOp: {
				if (precedence > 50)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::Xor, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(51, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}

			case TokenId::OrOp: {
				if (precedence > 40)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::Or, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(41, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}

			case TokenId::LAndOp: {
				if (precedence > 30)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::Eq, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(31, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}

			case TokenId::LOrOp: {
				if (precedence > 20)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::LOr, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(21, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}

			case TokenId::Question: {
				if (precedence > 10)
					goto end;
				nextToken();

				peff::SharedPtr<TernaryExprNode> expr;

				if (!(expr = peff::makeShared<TernaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, lhs, peff::SharedPtr<ExprNode>(), peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(10, expr->lhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->lhs->tokenRange.endIndex;

				Token *colonToken;
				if ((syntaxError = expectToken((colonToken = nextToken()), TokenId::Colon)))
					goto genBadExpr;

				expr->tokenRange.endIndex = colonToken->index;

				if ((syntaxError = parseExpr(10, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}

			case TokenId::AssignOp: {
				if (precedence > 1)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::Assign, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(0, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}
			case TokenId::AddAssignOp: {
				if (precedence > 1)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::AddAssign, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(0, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}
			case TokenId::SubAssignOp: {
				if (precedence > 1)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::SubAssign, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(0, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}
			case TokenId::MulAssignOp: {
				if (precedence > 1)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::MulAssign, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(0, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}
			case TokenId::DivAssignOp: {
				if (precedence > 1)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::DivAssign, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(0, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}
			case TokenId::AndAssignOp: {
				if (precedence > 1)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::AndAssign, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(0, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}
			case TokenId::OrAssignOp: {
				if (precedence > 1)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::OrAssign, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(0, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}
			case TokenId::XorAssignOp: {
				if (precedence > 1)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::XorAssign, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(0, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}
			case TokenId::LshAssignOp: {
				if (precedence > 1)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::ShlAssign, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(0, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}
			case TokenId::RshAssignOp: {
				if (precedence > 1)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::ShrAssign, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(0, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}
			case TokenId::Comma: {
				if (precedence > -9)
					goto end;
				nextToken();

				peff::SharedPtr<BinaryExprNode> expr;

				if (!(expr = peff::makeShared<BinaryExprNode>(
						  resourceAllocator.get(), resourceAllocator.get(), document, BinaryOp::Comma, lhs, peff::SharedPtr<ExprNode>())))
					return genOutOfMemoryError();

				lhs = expr.castTo<ExprNode>();

				expr->tokenRange = lhs->tokenRange;
				expr->tokenRange.endIndex = infixToken->index;

				if ((syntaxError = parseExpr(-10, expr->rhs)))
					goto genBadExpr;

				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				break;
			}
			default:
				goto end;
		}
	}

end:
	exprOut = lhs;

	return {};

genBadExpr:
	if (!(exprOut = peff::makeShared<BadExprNode>(resourceAllocator.get(), resourceAllocator.get(), document, lhs).castTo<ExprNode>()))
		return genOutOfMemoryError();
	exprOut->tokenRange = { prefixToken->index, parseContext.idxCurrentToken };
	return syntaxError;
}
