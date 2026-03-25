#define NOMINMAX
#include "../parser.h"

using namespace slkc;

template <typename T>
SLAKE_FORCEINLINE peff::Option<SyntaxError> _parseInt(Parser *parser, Token *token, bool isNegative, const std::string_view &bodyView, T &dataOut) {
	peff::Option<SyntaxError> syntaxError;

	bool overflowWarned = false;
	char c;
	T data = 0;
	size_t i = 0;

	switch (((IntTokenExtension *)token->exData.get())->tokenType) {
		case IntTokenType::Decimal:
			while (i < bodyView.size()) {
				c = bodyView.at(i);
				if (isNegative) {
					if ((!overflowWarned) && (std::numeric_limits<T>::min() / 10 > data)) {
						if ((syntaxError = parser->pushLiteralOverflowedError(token)))
							return syntaxError;
						overflowWarned = true;
					}
					data *= 10;
					data -= c - '0';
				} else {
					if ((!overflowWarned) && (std::numeric_limits<T>::max() / 10 < data)) {
						if ((syntaxError = parser->pushLiteralOverflowedError(token)))
							return syntaxError;
						overflowWarned = true;
					}
					data *= 10;
					data += c - '0';
				}
				++i;
			}
			break;
		case IntTokenType::Hexadecimal:
			while (i < bodyView.size()) {
				char c = bodyView.at(i);
				if (isNegative) {
					if ((!overflowWarned) && (std::numeric_limits<T>::min() / 16 > data)) {
						if ((syntaxError = parser->pushLiteralOverflowedError(token)))
							return syntaxError;
						overflowWarned = true;
					}
					data *= 16;
					switch (c) {
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9':
							data -= c - '0';
							break;
						case 'a':
						case 'b':
						case 'c':
						case 'd':
						case 'e':
						case 'f':
							data -= c - 'a' + 10;
							break;
						case 'A':
						case 'B':
						case 'C':
						case 'D':
						case 'E':
						case 'F':
							data -= c - 'A' + 10;
							break;
					}
				} else {
					if ((!overflowWarned) && (std::numeric_limits<T>::max() / 10 < data)) {
						if ((syntaxError = parser->pushLiteralOverflowedError(token)))
							return syntaxError;
						overflowWarned = true;
					}
					data *= 16;
					switch (c) {
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9':
							data += c - '0';
							break;
						case 'a':
						case 'b':
						case 'c':
						case 'd':
						case 'e':
						case 'f':
							data += c - 'a' + 10;
							break;
						case 'A':
						case 'B':
						case 'C':
						case 'D':
						case 'E':
						case 'F':
							data += c - 'A' + 10;
							break;
					}
				}
				++i;
			}
			break;
		case IntTokenType::Octal:
			while (i < bodyView.size()) {
				c = bodyView.at(i);
				if (isNegative) {
					if ((!overflowWarned) && (std::numeric_limits<T>::min() / 8 > data)) {
						if ((syntaxError = parser->pushLiteralOverflowedError(token)))
							return syntaxError;
						overflowWarned = true;
					}
					data *= 8;
					data -= c - '0';
				} else {
					if ((!overflowWarned) && (std::numeric_limits<T>::max() / 8 < data)) {
						if ((syntaxError = parser->pushLiteralOverflowedError(token)))
							return syntaxError;
						overflowWarned = true;
					}
					data *= 8;
					data += c - '0';
				}
				++i;
			}
			break;
		case IntTokenType::Binary:
			while (i < bodyView.size()) {
				c = bodyView.at(i);
				if (isNegative) {
					if ((!overflowWarned) && ((std::numeric_limits<T>::min() >> 1) > data)) {
						if ((syntaxError = parser->pushLiteralOverflowedError(token)))
							return syntaxError;
						overflowWarned = true;
					}
					data <<= 1;
					data -= c - '0';
				} else {
					if ((!overflowWarned) && ((std::numeric_limits<T>::max() >> 1) < data)) {
						if ((syntaxError = parser->pushLiteralOverflowedError(token)))
							return syntaxError;
						overflowWarned = true;
					}
					data <<= 1;
					data += c - '0';
				}
				++i;
			}
			break;
		default:
			std::terminate();
	}

	dataOut = data;

	return {};
}

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
					if (!(lhs = makeAstNode<IdRefExprNode>(resourceAllocator.get(), resourceAllocator.get(), document, std::move(idRefPtr)).castTo<ExprNode>()))
						return genOutOfMemorySyntaxError();
					break;
				}
				case TokenId::LParenthese: {
					nextToken();

					AstNodePtr<WrapperExprNode> expr;

					if (!(expr = makeAstNode<WrapperExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemorySyntaxError();

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
						return genOutOfMemorySyntaxError();

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

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
				case TokenId::I8Literal: {
					nextToken();

					int8_t data = 0;
					bool isNegative = false;

					if (prefixToken->sourceText.at(0) == '-')
						isNegative = true;

					std::string_view bodyView = prefixToken->sourceText.substr(isNegative ? 1 : 0, prefixToken->sourceText.size() - (isNegative ? 1 : 0) - (sizeof("i8") - 1));

					if ((syntaxError = _parseInt<int8_t>(this, prefixToken, isNegative, bodyView, data)))
						return syntaxError;

					if (!(lhs = makeAstNode<I8LiteralExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document,
							  data)
								.castTo<ExprNode>()))
						return genOutOfMemorySyntaxError();
					break;
				}
				case TokenId::I16Literal: {
					nextToken();

					int16_t data = 0;
					bool isNegative = false;

					if (prefixToken->sourceText.at(0) == '-')
						isNegative = true;

					std::string_view bodyView = prefixToken->sourceText.substr(isNegative ? 1 : 0, prefixToken->sourceText.size() - (isNegative ? 1 : 0) - (sizeof("i16") - 1));

					if ((syntaxError = _parseInt<int16_t>(this, prefixToken, isNegative, bodyView, data)))
						return syntaxError;

					if (!(lhs = makeAstNode<I16LiteralExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document,
							  data)
								.castTo<ExprNode>()))
						return genOutOfMemorySyntaxError();
					break;
				}
				case TokenId::I32Literal: {
					nextToken();

					int32_t data = 0;
					bool isNegative = false;

					if (prefixToken->sourceText.at(0) == '-')
						isNegative = true;

					std::string_view bodyView = prefixToken->sourceText.substr(isNegative ? 1 : 0, prefixToken->sourceText.size() - (isNegative ? 1 : 0));

					if ((syntaxError = _parseInt<int32_t>(this, prefixToken, isNegative, bodyView, data)))
						return syntaxError;

					if (!(lhs = makeAstNode<I32LiteralExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document,
							  data)
								.castTo<ExprNode>()))
						return genOutOfMemorySyntaxError();
					break;
				}
				case TokenId::I64Literal: {
					nextToken();

					int64_t data = 0;
					bool isNegative = false;

					if (prefixToken->sourceText.at(0) == '-')
						isNegative = true;

					std::string_view bodyView = prefixToken->sourceText.substr(isNegative ? 1 : 0, prefixToken->sourceText.size() - (isNegative ? 1 : 0) - (sizeof("L") - 1));

					if ((syntaxError = _parseInt<int64_t>(this, prefixToken, isNegative, bodyView, data)))
						return syntaxError;

					if (!(lhs = makeAstNode<I64LiteralExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document,
							  data)
								.castTo<ExprNode>()))
						return genOutOfMemorySyntaxError();
					break;
				}
				case TokenId::U8Literal: {
					nextToken();

					uint8_t data = 0;
					bool isNegative = false;

					if (prefixToken->sourceText.at(0) == '-')
						isNegative = true;

					std::string_view bodyView = prefixToken->sourceText.substr(isNegative ? 1 : 0, prefixToken->sourceText.size() - (isNegative ? 1 : 0) - (sizeof("Ui8") - 1));

					if ((syntaxError = _parseInt<uint8_t>(this, prefixToken, isNegative, bodyView, data)))
						return syntaxError;

					if (!(lhs = makeAstNode<U8LiteralExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document,
							  data)
								.castTo<ExprNode>()))
						return genOutOfMemorySyntaxError();
					break;
				}
				case TokenId::U16Literal: {
					nextToken();

					uint16_t data = 0;
					bool isNegative = false;

					if (prefixToken->sourceText.at(0) == '-')
						isNegative = true;

					std::string_view bodyView = prefixToken->sourceText.substr(isNegative ? 1 : 0, prefixToken->sourceText.size() - (isNegative ? 1 : 0) - (sizeof("Ui16") - 1));

					if ((syntaxError = _parseInt<uint16_t>(this, prefixToken, isNegative, bodyView, data)))
						return syntaxError;

					if (!(lhs = makeAstNode<U16LiteralExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document,
							  data)
								.castTo<ExprNode>()))
						return genOutOfMemorySyntaxError();
					break;
				}
				case TokenId::U32Literal: {
					nextToken();

					uint32_t data = 0;
					bool isNegative = false;

					if (prefixToken->sourceText.at(0) == '-')
						isNegative = true;

					std::string_view bodyView = prefixToken->sourceText.substr(isNegative ? 1 : 0, prefixToken->sourceText.size() - (isNegative ? 1 : 0) - (sizeof("U") - 1));

					if ((syntaxError = _parseInt<uint32_t>(this, prefixToken, isNegative, bodyView, data)))
						return syntaxError;

					if (!(lhs = makeAstNode<U32LiteralExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document,
							  data)
								.castTo<ExprNode>()))
						return genOutOfMemorySyntaxError();
					break;
				}
				case TokenId::U64Literal: {
					nextToken();

					uint64_t data = 0;
					bool isNegative = false;

					if (prefixToken->sourceText.at(0) == '-')
						isNegative = true;

					std::string_view bodyView = prefixToken->sourceText.substr(isNegative ? 1 : 0, prefixToken->sourceText.size() - (isNegative ? 1 : 0) - (sizeof("UL") - 1));

					if ((syntaxError = _parseInt<uint64_t>(this, prefixToken, isNegative, bodyView, data)))
						return syntaxError;

					if (!(lhs = makeAstNode<U64LiteralExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document,
							  data)
								.castTo<ExprNode>()))
						return genOutOfMemorySyntaxError();
					break;
				}
				case TokenId::StringLiteral: {
					nextToken();
					peff::String s(resourceAllocator.get());

					if (!s.build(((StringTokenExtension *)prefixToken->exData.get())->data)) {
						return genOutOfMemorySyntaxError();
					}

					if (!(lhs = makeAstNode<StringLiteralExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document,
							  std::move(s))
								.castTo<ExprNode>()))
						return genOutOfMemorySyntaxError();
					break;
				}
				case TokenId::F32Literal: {
					nextToken();

					if (!(lhs = peff::makeSharedWithControlBlock<F32LiteralExprNode, AstNodeControlBlock<F32LiteralExprNode>>(
							  resourceAllocator.get(), resourceAllocator.get(), document,
							  strtof(prefixToken->sourceText.data(), nullptr))
								.castTo<ExprNode>()))
						return genOutOfMemorySyntaxError();
					break;
				}
				case TokenId::F64Literal: {
					nextToken();
					if (!(lhs = peff::makeSharedWithControlBlock<F64LiteralExprNode, AstNodeControlBlock<F64LiteralExprNode>>(
							  resourceAllocator.get(), resourceAllocator.get(), document,
							  strtod(prefixToken->sourceText.data(), nullptr))
								.castTo<ExprNode>()))
						return genOutOfMemorySyntaxError();
					break;
				}
				case TokenId::TrueKeyword: {
					nextToken();
					if (!(lhs = makeAstNode<BoolLiteralExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document,
							  true)
								.castTo<ExprNode>()))
						return genOutOfMemorySyntaxError();
					break;
				}
				case TokenId::FalseKeyword: {
					nextToken();
					if (!(lhs = makeAstNode<BoolLiteralExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document,
							  false)
								.castTo<ExprNode>()))
						return genOutOfMemorySyntaxError();
					break;
				}
				case TokenId::NullKeyword: {
					nextToken();
					if (!(lhs = makeAstNode<NullLiteralExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)
								.castTo<ExprNode>()))
						return genOutOfMemorySyntaxError();
					break;
				}
				case TokenId::VarArg: {
					nextToken();

					AstNodePtr<UnaryExprNode> expr;

					if (!(expr = makeAstNode<UnaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemorySyntaxError();

					expr->unaryOp = UnaryOp::Unpacking;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					lhs = initializerExpr.castTo<ExprNode>();

					AstNodePtr<ExprNode> curExpr;

					static TokenId matchingTokens[] = {
						TokenId::RBrace,
						TokenId::Semicolon
					};

					Token *currentToken;

					for (;;) {
						if ((syntaxError = parseExpr(0, curExpr))) {
							if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
								return genOutOfMemorySyntaxError();
							syntaxError.reset();
							goto genBadExpr;
						}

						if (!initializerExpr->elements.pushBack(std::move(curExpr))) {
							return genOutOfMemorySyntaxError();
						}

						currentToken = peekToken();

						if (currentToken->tokenId != TokenId::Comma)
							break;

						nextToken();
					}

					if ((syntaxError = expectToken(currentToken = peekToken(), TokenId::RBrace))) {
						if (!syntaxErrors.pushBack(std::move(syntaxError.value())))
							return genOutOfMemorySyntaxError();
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
						return genOutOfMemorySyntaxError();

					expr->unaryOp = UnaryOp::Neg;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->unaryOp = UnaryOp::Not;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->unaryOp = UnaryOp::LNot;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					lhs = expr.castTo<ExprNode>();

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
						SLKC_RETURN_IF_PARSE_ERROR((parseTypeName(expr->returnType)));
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
							return genOutOfMemorySyntaxError();
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
						return genOutOfMemorySyntaxError();

					expr->target = lhs;

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::Subscript;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->head = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->source = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::Mul;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::Div;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::Mod;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::Add;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::Sub;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

					if ((syntaxError = parseExpr(111, expr->rhs)))
						goto genBadExpr;

					break;
				}

				case TokenId::ShlOp: {
					if (precedence > 100)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::Shl;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

					if ((syntaxError = parseExpr(101, expr->rhs)))
						goto genBadExpr;

					break;
				}
				case TokenId::ShrOp: {
					if (precedence > 100)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::Shr;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::Cmp;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::Gt;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::GtEq;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::Lt;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::LtEq;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::Eq;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::Neq;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::StrictEq;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::StrictNeq;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::And;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::Xor;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::Or;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::LAnd;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::LOr;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::Assign;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::AddAssign;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::SubAssign;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::MulAssign;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::DivAssign;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::AndAssign;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::OrAssign;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::XorAssign;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

					if ((syntaxError = parseExpr(0, expr->rhs)))
						goto genBadExpr;

					break;
				}
				case TokenId::ShlAssignOp: {
					if (precedence > 1)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::ShlAssign;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

					if ((syntaxError = parseExpr(0, expr->rhs)))
						goto genBadExpr;

					break;
				}
				case TokenId::ShrAssignOp: {
					if (precedence > 1)
						goto end;
					nextToken();

					AstNodePtr<BinaryExprNode> expr;

					if (!(expr = makeAstNode<BinaryExprNode>(
							  resourceAllocator.get(), resourceAllocator.get(), document)))
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::ShrAssign;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
						return genOutOfMemorySyntaxError();

					expr->binaryOp = BinaryOp::Comma;
					expr->lhs = lhs;

					lhs = expr.castTo<ExprNode>();

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
	if (!(exprOut = makeAstNode<BadExprNode>(resourceAllocator.get(), resourceAllocator.get(), document, lhs).castTo<ExprNode>()))
		return genOutOfMemorySyntaxError();
	exprOut->tokenRange = { document->mainModule, prefixToken->index, parseContext.idxCurrentToken };
	return syntaxError;
}
