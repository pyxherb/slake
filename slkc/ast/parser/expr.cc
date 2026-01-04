#include "../parser.h"

using namespace slkc;

SLKC_API peff::Option<SyntaxError> Parser::parseExpr(int precedence, AstNodePtr<ExprNode> &exprOut) {
	Token *prefixToken;

	peff::Option<SyntaxError> syntaxError;
	AstNodePtr<ExprNode> lhs, rhs;

	if ((syntaxError = expectToken((prefixToken = peekToken()))))
		goto genBadExpr;

	{
		{
			peff::ScopeGuard setTokenRangeGuard([this, prefixToken, &lhs]() noexcept {
				if (lhs) {
					lhs->tokenRange = TokenRange{ document->mainModule, prefixToken->index, parseContext.idxPrevToken };
				}
			});

			switch (prefixToken->tokenId) {
				case TokenId::ThisKeyword:
				case TokenId::ScopeOp:
				case TokenId::Id: {
					IdRefPtr idRefPtr;
					if ((syntaxError = parseIdRef(idRefPtr)))
						goto genBadExpr;
					if (!(lhs = makeAstNode<IdRefExprNode>(resourceAllocator.get(), resourceAllocator.get(), document, std::move(idRefPtr)).template castTo<ExprNode>()))
						return genOutOfMemoryError();
					break;
				}
				case TokenId::LParenthese: {
					nextToken();

					AstNodePtr<WrapperExprNode> expr;

					if (!(expr = makeAstNode<WrapperExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					lhs = expr.castTo<ExprNode>();

					if ((syntaxError = parseExpr(0, expr->target)))
						goto genBadExpr;

					Token *rParentheseToken;

					if ((syntaxError = expectToken((rParentheseToken = nextToken()), TokenId::RParenthese)))
						goto genBadExpr;
					break;
				}
				case TokenId::AllocaKeyword: {
					nextToken();

					AstNodePtr<AllocaExprNode> expr;

					if (!(expr = makeAstNode<AllocaExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseTypeName(expr->targetType, false)))
						goto genBadExpr;

					Token *lBracketToken;

					if ((lBracketToken = peekToken())->tokenId == TokenId::LBracket) {
						nextToken();

						if ((syntaxError = parseExpr(0, expr->countExpr)))
							goto genBadExpr;

						if ((syntaxError = splitRDBracketsToken()))
							goto genBadExpr;

						Token *rBracketToken;

						if ((syntaxError = expectToken((rBracketToken = peekToken()), TokenId::RBracket)))
							goto genBadExpr;

						nextToken();
					}
					break;
				}
				case TokenId::NewKeyword: {
					nextToken();

					AstNodePtr<NewExprNode> expr;

					if (!(expr = makeAstNode<NewExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					lhs = expr.template castTo<ExprNode>();

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
					if (!(lhs = peff::makeSharedWithControlBlock<I32LiteralExprNode, AstNodeControlBlock<I32LiteralExprNode>>(
							  resourceAllocator.get(), resourceAllocator.get(), document,
							  ((IntTokenExtension *)prefixToken->exData.get())->data)
								.template castTo<ExprNode>()))
						return genOutOfMemoryError();
					break;
				}
				case TokenId::LongLiteral: {
					nextToken();
					if (!(lhs = peff::makeSharedWithControlBlock<I64LiteralExprNode, AstNodeControlBlock<I64LiteralExprNode>>(
							  resourceAllocator.get(), resourceAllocator.get(), document,
							  ((LongTokenExtension *)prefixToken->exData.get())->data)
								.template castTo<ExprNode>()))
						return genOutOfMemoryError();
					break;
				}
				case TokenId::UIntLiteral: {
					nextToken();
					if (!(lhs = peff::makeSharedWithControlBlock<U32LiteralExprNode, AstNodeControlBlock<U32LiteralExprNode>>(
							  resourceAllocator.get(), resourceAllocator.get(), document,
							  ((UIntTokenExtension *)prefixToken->exData.get())->data)
								.template castTo<ExprNode>()))
						return genOutOfMemoryError();
					break;
				}
				case TokenId::ULongLiteral: {
					nextToken();
					if (!(lhs = peff::makeSharedWithControlBlock<U64LiteralExprNode, AstNodeControlBlock<U64LiteralExprNode>>(
							  resourceAllocator.get(), resourceAllocator.get(), document,
							  ((ULongTokenExtension *)prefixToken->exData.get())->data)
								.template castTo<ExprNode>()))
						return genOutOfMemoryError();
					break;
				}
				case TokenId::StringLiteral: {
					nextToken();
					peff::String s(resourceAllocator.get());

					if (!s.build(((StringTokenExtension *)prefixToken->exData.get())->data)) {
						return genOutOfMemoryError();
					}

					if (!(lhs = makeAstNode<StringLiteralExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document,
							  std::move(s))
								.template castTo<ExprNode>()))
						return genOutOfMemoryError();
					break;
				}
				case TokenId::F32Literal: {
					nextToken();
					if (!(lhs = peff::makeSharedWithControlBlock<F32LiteralExprNode, AstNodeControlBlock<F32LiteralExprNode>>(
							  resourceAllocator.get(), resourceAllocator.get(), document,
							  ((F32TokenExtension *)prefixToken->exData.get())->data)
								.template castTo<ExprNode>()))
						return genOutOfMemoryError();
					break;
				}
				case TokenId::F64Literal: {
					nextToken();
					if (!(lhs = peff::makeSharedWithControlBlock<F64LiteralExprNode, AstNodeControlBlock<F64LiteralExprNode>>(
							  resourceAllocator.get(), resourceAllocator.get(), document,
							  ((F64TokenExtension *)prefixToken->exData.get())->data)
								.template castTo<ExprNode>()))
						return genOutOfMemoryError();
					break;
				}
				case TokenId::TrueKeyword: {
					nextToken();
					if (!(lhs = makeAstNode<BoolLiteralExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document,
							  true)
								.template castTo<ExprNode>()))
						return genOutOfMemoryError();
					break;
				}
				case TokenId::FalseKeyword: {
					nextToken();
					if (!(lhs = makeAstNode<BoolLiteralExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document,
							  false)
								.template castTo<ExprNode>()))
						return genOutOfMemoryError();
					break;
				}
				case TokenId::NullKeyword: {
					nextToken();
					if (!(lhs = makeAstNode<NullLiteralExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)
								.template castTo<ExprNode>()))
						return genOutOfMemoryError();
					break;
				}
				case TokenId::VarArg: {
					nextToken();

					AstNodePtr<UnaryExprNode> expr;

					if (!(expr = makeAstNode<UnaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->unaryOp = UnaryOp::Unpacking;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(131, expr->operand))) {
						goto genBadExpr;
					}
					break;
				}
				case TokenId::LBrace: {
					nextToken();

					AstNodePtr<InitializerListExprNode> initializerExpr;

					if (!(initializerExpr = makeAstNode<InitializerListExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					lhs = initializerExpr.template castTo<ExprNode>();

					AstNodePtr<ExprNode> curExpr;

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

					AstNodePtr<UnaryExprNode> expr;

					if (!(expr = makeAstNode<UnaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->unaryOp = UnaryOp::Neg;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(131, expr->operand))) {
						goto genBadExpr;
					}
					break;
				}
				case TokenId::NotOp: {
					nextToken();

					AstNodePtr<UnaryExprNode> expr;

					if (!(expr = makeAstNode<UnaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->unaryOp = UnaryOp::Not;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(131, expr->operand))) {
						goto genBadExpr;
					}
					break;
				}
				case TokenId::LNotOp: {
					nextToken();

					AstNodePtr<UnaryExprNode> expr;

					if (!(expr = makeAstNode<UnaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->unaryOp = UnaryOp::LNot;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(131, expr->operand))) {
						goto genBadExpr;
					}

					break;
				}
				case TokenId::MatchKeyword: {
					nextToken();

					AstNodePtr<MatchExprNode> expr;

					if (!(expr = makeAstNode<MatchExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					lhs = expr.template castTo<ExprNode>();

					if (peekToken()->tokenId == TokenId::ConstKeyword) {
						expr->isConst = true;
						nextToken();
					}

					Token *lParentheseToken;

					if ((syntaxError = expectToken((lParentheseToken = peekToken()), TokenId::LParenthese)))
						goto genBadExpr;

					nextToken();

					if ((syntaxError = parseExpr(0, expr->condition)))
						goto genBadExpr;

					Token *rParentheseToken;

					if ((syntaxError = expectToken((rParentheseToken = peekToken()), TokenId::RParenthese)))
						goto genBadExpr;

					nextToken();

					Token *returnTypeToken;
					if ((returnTypeToken = peekToken())->tokenId == TokenId::ReturnTypeOp) {
						nextToken();
						if ((syntaxError = parseTypeName(expr->returnType))) {
							return syntaxError;
						}
					}

					Token *lBraceToken = peekToken();

					if ((syntaxError = expectToken(lBraceToken, TokenId::LBrace))) {
						goto genBadExpr;
					}

					nextToken();

					while (true) {
						AstNodePtr<ExprNode> conditionExpr, resultExpr;

						if (peekToken()->tokenId == TokenId::DefaultKeyword) {
							nextToken();
						} else {
							if ((syntaxError = parseExpr(0, conditionExpr))) {
								goto genBadExpr;
							}
						}

						Token *colonToken = peekToken();

						if ((syntaxError = expectToken(colonToken, TokenId::Colon))) {
							goto genBadExpr;
						}

						nextToken();

						if ((syntaxError = parseExpr(0, resultExpr))) {
							goto genBadExpr;
						}

						if (!expr->cases.pushBack({ conditionExpr, resultExpr })) {
							return genOutOfMemoryError();
						}

						if (peekToken()->tokenId != TokenId::Comma)
							break;

						Token *commaToken = nextToken();
					}

					Token *rBraceToken = peekToken();

					if ((syntaxError = expectToken(rBraceToken, TokenId::RBrace))) {
						goto genBadExpr;
					}

					nextToken();

					break;
				}
				default:
					nextToken();
					return SyntaxError(
						TokenRange{ document->mainModule, prefixToken->index },
						SyntaxErrorKind::ExpectingExpr);
			}
		}

		Token *infixToken;

		for (;;) {
			peff::ScopeGuard setTokenRangeGuard([this, prefixToken, &lhs]() noexcept {
				if (lhs) {
					lhs->tokenRange = TokenRange{ document->mainModule, prefixToken->index, parseContext.idxPrevToken };
				}
			});

			switch ((infixToken = peekToken())->tokenId) {
				case TokenId::LParenthese: {
					if (precedence > 140)
						goto end;
					nextToken();

					AstNodePtr<CallExprNode> expr;

					if (!(expr = makeAstNode<CallExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document, AstNodePtr<ExprNode>(), peff::DynArray<AstNodePtr<ExprNode>>{ resourceAllocator.get() })))
						return genOutOfMemoryError();

					expr->target = lhs;

					lhs = expr.template castTo<ExprNode>();

					expr->lParentheseTokenIndex = infixToken->index;

					if ((syntaxError = parseArgs(expr->args, expr->idxCommaTokens))) {
						goto genBadExpr;
					}

					Token *rParentheseToken;

					if ((syntaxError = expectToken((rParentheseToken = peekToken()), TokenId::RParenthese)))
						goto genBadExpr;

					nextToken();

					expr->rParentheseTokenIndex = rParentheseToken->index;

					if (peekToken()->tokenId == TokenId::WithKeyword) {
						nextToken();

						if (auto e = parseExpr(121, expr->withObject); e)
							return e;
					}

					break;
				}
				case TokenId::LBracket: {
					if (precedence > 140)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::Subscript;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(0, expr->rhs)))
						goto genBadExpr;

					if ((syntaxError = splitRDBracketsToken()))
						goto genBadExpr;

					Token *rBracketToken;

					if ((syntaxError = expectToken((rBracketToken = nextToken()), TokenId::RBracket)))
						goto genBadExpr;

					break;
				}
				case TokenId::Dot: {
					if (precedence > 140)
						goto end;
					nextToken();

					AstNodePtr<HeadedIdRefExprNode> expr;

					if (!(expr = makeAstNode<HeadedIdRefExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document, lhs, IdRefPtr{})))
						return genOutOfMemoryError();

					expr->head = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseIdRef(expr->idRefPtr)))
						goto genBadExpr;

					break;
				}
				case TokenId::AsKeyword: {
					if (precedence > 130)
						goto end;
					nextToken();

					AstNodePtr<CastExprNode> expr;

					if (!(expr = makeAstNode<CastExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->source = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseTypeName(expr->targetType)))
						goto genBadExpr;

					expr->tokenRange.endIndex = expr->targetType->tokenRange.endIndex;

					break;
				}

				case TokenId::MulOp: {
					if (precedence > 120)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::Mul;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(121, expr->rhs)))
						goto genBadExpr;

					break;
				}
				case TokenId::DivOp: {
					if (precedence > 120)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::Div;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(121, expr->rhs)))
						goto genBadExpr;

					break;
				}
				case TokenId::ModOp: {
					if (precedence > 120)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::Mod;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(121, expr->rhs)))
						goto genBadExpr;

					break;
				}

				case TokenId::AddOp: {
					if (precedence > 110)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::Add;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(111, expr->rhs)))
						goto genBadExpr;

					break;
				}
				case TokenId::SubOp: {
					if (precedence > 110)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::Sub;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(111, expr->rhs)))
						goto genBadExpr;

					break;
				}

				case TokenId::LshOp: {
					if (precedence > 100)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::Shl;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(101, expr->rhs)))
						goto genBadExpr;

					break;
				}
				case TokenId::RshOp: {
					if (precedence > 100)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::Shr;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(101, expr->rhs)))
						goto genBadExpr;

					break;
				}

				case TokenId::CmpOp: {
					if (precedence > 90)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::Cmp;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(91, expr->rhs)))
						goto genBadExpr;

					break;
				}

				case TokenId::GtOp: {
					if (precedence > 80)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::Gt;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(81, expr->rhs)))
						goto genBadExpr;

					break;
				}
				case TokenId::GtEqOp: {
					if (precedence > 80)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::GtEq;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(81, expr->rhs)))
						goto genBadExpr;

					break;
				}
				case TokenId::LtOp: {
					if (precedence > 80)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::Lt;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(81, expr->rhs)))
						goto genBadExpr;

					break;
				}
				case TokenId::LtEqOp: {
					if (precedence > 80)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::LtEq;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(81, expr->rhs)))
						goto genBadExpr;

					break;
				}

				case TokenId::EqOp: {
					if (precedence > 70)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::Eq;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(71, expr->rhs)))
						goto genBadExpr;

					break;
				}
				case TokenId::NeqOp: {
					if (precedence > 70)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::Neq;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(71, expr->rhs)))
						goto genBadExpr;

					break;
				}
				case TokenId::StrictEqOp: {
					if (precedence > 70)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::StrictEq;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(71, expr->rhs)))
						goto genBadExpr;

					break;
				}
				case TokenId::StrictNeqOp: {
					if (precedence > 70)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::StrictNeq;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(71, expr->rhs)))
						goto genBadExpr;

					break;
				}

				case TokenId::AndOp: {
					if (precedence > 60)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::And;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(61, expr->rhs)))
						goto genBadExpr;

					break;
				}

				case TokenId::XorOp: {
					if (precedence > 50)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::Xor;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(51, expr->rhs)))
						goto genBadExpr;

					break;
				}

				case TokenId::OrOp: {
					if (precedence > 40)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::Or;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(41, expr->rhs)))
						goto genBadExpr;

					break;
				}

				case TokenId::LAndOp: {
					if (precedence > 30)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::LAnd;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(31, expr->rhs)))
						goto genBadExpr;

					break;
				}

				case TokenId::LOrOp: {
					if (precedence > 20)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::LOr;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(21, expr->rhs)))
						goto genBadExpr;

					break;
				}

				case TokenId::Question: {
					if (precedence > 10)
						goto end;
					nextToken();

					AstNodePtr<TernaryExprNode> expr;

					if (!(expr = makeAstNode<TernaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(10, expr->lhs)))
						goto genBadExpr;

					expr->tokenRange.endIndex = expr->lhs->tokenRange.endIndex;

					Token *colonToken;
					if ((syntaxError = expectToken((colonToken = nextToken()), TokenId::Colon)))
						goto genBadExpr;

					expr->tokenRange.endIndex = colonToken->index;

					if ((syntaxError = parseExpr(10, expr->rhs)))
						goto genBadExpr;

					break;
				}

				case TokenId::AssignOp: {
					if (precedence > 1)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::Assign;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(0, expr->rhs)))
						goto genBadExpr;

					break;
				}
				case TokenId::AddAssignOp: {
					if (precedence > 1)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::AddAssign;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(0, expr->rhs)))
						goto genBadExpr;

					break;
				}
				case TokenId::SubAssignOp: {
					if (precedence > 1)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::SubAssign;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(0, expr->rhs)))
						goto genBadExpr;

					break;
				}
				case TokenId::MulAssignOp: {
					if (precedence > 1)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::MulAssign;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(0, expr->rhs)))
						goto genBadExpr;

					break;
				}
				case TokenId::DivAssignOp: {
					if (precedence > 1)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::DivAssign;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(0, expr->rhs)))
						goto genBadExpr;

					break;
				}
				case TokenId::AndAssignOp: {
					if (precedence > 1)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::AndAssign;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(0, expr->rhs)))
						goto genBadExpr;

					break;
				}
				case TokenId::OrAssignOp: {
					if (precedence > 1)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::OrAssign;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(0, expr->rhs)))
						goto genBadExpr;

					break;
				}
				case TokenId::XorAssignOp: {
					if (precedence > 1)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::XorAssign;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(0, expr->rhs)))
						goto genBadExpr;

					break;
				}
				case TokenId::LshAssignOp: {
					if (precedence > 1)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::ShlAssign;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(0, expr->rhs)))
						goto genBadExpr;

					break;
				}
				case TokenId::RshAssignOp: {
					if (precedence > 1)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::ShrAssign;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(0, expr->rhs)))
						goto genBadExpr;

					break;
				}
				case TokenId::Comma: {
					if (precedence > -9)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemoryError();

					expr->binaryOp = BinaryOp::Comma;
					expr->lhs = lhs;

					lhs = expr.template castTo<ExprNode>();

					if ((syntaxError = parseExpr(-10, expr->rhs)))
						goto genBadExpr;

					break;
				}
				default:
					goto end;
			}
		}
	}

end:
	exprOut = lhs;

	return {};

genBadExpr:
	if (!(exprOut = makeAstNode<BadExprNode>(resourceAllocator.get(), resourceAllocator.get(), document, lhs).template castTo<ExprNode>()))
		return genOutOfMemoryError();
	exprOut->tokenRange = { document->mainModule, prefixToken->index, parseContext.idxCurrentToken };
	return syntaxError;
}
