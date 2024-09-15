#include <slkc/compiler/compiler.h>

using namespace slake;
using namespace slake::slkc;

std::map<TokenId, Parser::OpRegistry> Parser::prefixOpRegistries = {
	{ TokenId::SubOp,
		{ 131,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<UnaryOpExprNode>(
					UnaryOp::Neg,
					lhs);

				expr->tokenRange =
					TokenRange{ lhs->tokenRange.beginIndex, parser->lexer->getTokenIndex(opToken) };
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::NotOp,
		{ 131,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<UnaryOpExprNode>(
					UnaryOp::Not,
					lhs);

				expr->tokenRange =
					TokenRange{ lhs->tokenRange.beginIndex, parser->lexer->getTokenIndex(opToken) };
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::LNotOp,
		{ 131,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<UnaryOpExprNode>(
					UnaryOp::LNot,
					lhs);

				expr->tokenRange =
					TokenRange{ lhs->tokenRange.beginIndex, parser->lexer->getTokenIndex(opToken) };
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
};

std::map<TokenId, Parser::OpRegistry> Parser::infixOpRegistries = {
	{ TokenId::LParenthese,
		{ 140,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<CallExprNode>();

				expr->target = lhs;
				expr->tokenRange = TokenRange{ lhs->tokenRange.beginIndex, parser->lexer->getTokenIndex(opToken) };
				expr->idxLParentheseToken = parser->lexer->getTokenIndex(opToken);

				parser->parseArgs(expr->args, expr->idxCommaTokens);
				if (expr->args.size())
					expr->tokenRange.endIndex = expr->args.back()->tokenRange.endIndex;

				Token *lParentheseToken = parser->lexer->nextToken();
				expr->idxRParentheseToken = parser->lexer->getTokenIndex(
					parser->expectToken(lParentheseToken, TokenId::RParenthese));
				expr->tokenRange.endIndex = parser->lexer->getTokenIndex(lParentheseToken);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::LBracket,
		{ 140,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Subscript,
					lhs);

				expr->tokenRange = TokenRange{ lhs->tokenRange.beginIndex, parser->lexer->getTokenIndex(opToken) };
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr();
				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				Token *closingToken = parser->expectToken(parser->lexer->nextToken(), TokenId::RBracket);
				expr->idxClosingToken = parser->lexer->getTokenIndex(closingToken);
				expr->tokenRange.endIndex = parser->lexer->getTokenIndex(closingToken);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::Dot,
		{ 140,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<HeadedIdRefExprNode>(
					lhs,
					parser->parseRef());

				expr->tokenRange = TokenRange{ lhs->tokenRange.beginIndex, expr->ref.back().tokenRange.endIndex };
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::MulOp,
		{ 120,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Mul,
					lhs);

				expr->tokenRange = lhs->tokenRange;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(121);
				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::DivOp,
		{ 120,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Div,
					lhs);

				expr->tokenRange = lhs->tokenRange;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(121);
				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::ModOp,
		{ 120,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Mod,
					lhs);

				expr->tokenRange = lhs->tokenRange;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(121);
				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::AddOp,
		{ 110,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Add,
					lhs);

				expr->tokenRange = lhs->tokenRange;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(111);
				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::SubOp,
		{ 110,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Sub,
					lhs);

				expr->tokenRange = lhs->tokenRange;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(111);
				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::LshOp,
		{ 100,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Lsh,
					lhs);

				expr->tokenRange = lhs->tokenRange;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(101);
				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::RshOp,
		{ 100,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Rsh,
					lhs);

				expr->tokenRange = lhs->tokenRange;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(101);
				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::GtOp,
		{ 90,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Gt,
					lhs);

				expr->tokenRange = lhs->tokenRange;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(91);
				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::GtEqOp,
		{ 90,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::GtEq,
					lhs);

				expr->tokenRange = lhs->tokenRange;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(91);
				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::LtOp,
		{ 90,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Lt,
					lhs);

				expr->tokenRange = lhs->tokenRange;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(91);
				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::LtEqOp,
		{ 90,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::LtEq,
					lhs);

				expr->tokenRange = lhs->tokenRange;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(91);
				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::EqOp,
		{ 80,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Eq,
					lhs);

				expr->tokenRange = lhs->tokenRange;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(81);
				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::NeqOp,
		{ 80,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Neq,
					lhs);

				expr->tokenRange = lhs->tokenRange;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(81);
				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::StrictEqOp,
		{ 80,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::StrictEq,
					lhs);

				expr->tokenRange = lhs->tokenRange;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(81);
				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::StrictNeqOp,
		{ 80,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::StrictNeq,
					lhs);

				expr->tokenRange = lhs->tokenRange;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(81);
				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::AndOp,
		{ 70,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::And,
					lhs);

				expr->tokenRange = lhs->tokenRange;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(71);
				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::XorOp,
		{ 60,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Xor,
					lhs);

				expr->tokenRange = lhs->tokenRange;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(61);
				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::OrOp,
		{ 50,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Or,
					lhs);

				expr->tokenRange = lhs->tokenRange;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(51);
				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::LAndOp,
		{ 40,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::LAnd,
					lhs);

				expr->tokenRange = lhs->tokenRange;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(41);
				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::LOrOp,
		{ 30,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::LOr,
					lhs);

				expr->tokenRange = lhs->tokenRange;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(31);
				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::Question,
		{ 21,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<TernaryOpExprNode>(lhs);

				expr->tokenRange = TokenRange{ lhs->tokenRange.beginIndex, parser->lexer->getTokenIndex(opToken) };
				expr->idxQuestionToken = parser->lexer->getTokenIndex(opToken);

				expr->x = parser->parseExpr(20);
				expr->tokenRange.endIndex = expr->x->tokenRange.endIndex;

				Token *colonToken = parser->expectToken(parser->lexer->nextToken(), TokenId::Colon);
				expr->idxColonToken = parser->lexer->getTokenIndex(colonToken);
				expr->tokenRange.endIndex = parser->lexer->getTokenIndex(colonToken);

				expr->y = parser->parseExpr(20);
				expr->tokenRange.endIndex = expr->y->tokenRange.endIndex;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::AssignOp,
		{ 11,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Assign,
					lhs);

				expr->tokenRange = lhs->tokenRange;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(10);
				expr->tokenRange.endIndex = expr->rhs->tokenRange.endIndex;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
};

void Parser::parseArgs(
	std::deque<std::shared_ptr<ExprNode>> &argsOut,
	std::deque<size_t> &idxCommaTokensOut) {
	while (true) {
		if (lexer->peekToken()->tokenId == TokenId::RParenthese) {
			break;
		}

		argsOut.push_back(parseExpr());

		if (lexer->peekToken()->tokenId != TokenId::Comma) {
			break;
		}

		Token *commaToken = lexer->nextToken();
		idxCommaTokensOut.push_back(lexer->getTokenIndex(commaToken));
	}
}

std::shared_ptr<ExprNode> Parser::parseExpr(int precedence) {
	LexerContext savedContext = lexer->context;

	std::shared_ptr<ExprNode> lhs, rhs;

	Token *prefixToken = expectToken(lexer->peekToken());

	try {
		if (auto it = prefixOpRegistries.find(prefixToken->tokenId); it != prefixOpRegistries.end()) {
			lexer->nextToken();

			lhs = it->second.parselet(this, parseExpr(it->second.leftPrecedence), prefixToken);
		} else {
			switch (prefixToken->tokenId) {
				case TokenId::ThisKeyword:
				case TokenId::BaseKeyword:
				case TokenId::ScopeOp:
				case TokenId::Id: {
					auto ref = parseRef();
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<IdRefExprNode>(ref));
					lhs->tokenRange = TokenRange{ ref[0].tokenRange.beginIndex, ref.back().tokenRange.endIndex };
					break;
				}
				case TokenId::LParenthese: {
					lexer->nextToken();

					lhs = parseExpr();
					lhs->tokenRange.beginIndex = lexer->getTokenIndex(prefixToken);

					Token *rParentheseToken = expectToken(TokenId::RParenthese);
					lhs->tokenRange.endIndex = lexer->getTokenIndex(rParentheseToken);
					break;
				}
				case TokenId::NewKeyword: {
					auto expr = std::make_shared<NewExprNode>();

					Token *newToken = lexer->nextToken();
					expr->tokenRange = lexer->getTokenIndex(prefixToken);
					expr->idxNewToken = lexer->getTokenIndex(newToken);

					lhs = expr;

					expr->type = parseTypeName();

					Token *lParentheseToken = expectToken(TokenId::LParenthese);
					expr->tokenRange.endIndex = lexer->getTokenIndex(lParentheseToken);
					expr->idxLParentheseToken = lexer->getTokenIndex(lParentheseToken);

					parseArgs(expr->args, expr->idxCommaTokens);

					if (expr->args.size())
						expr->tokenRange.endIndex = expr->args.back()->tokenRange.endIndex;

					Token *rParentheseToken = expectToken(TokenId::RParenthese);
					expr->tokenRange.endIndex = lexer->getTokenIndex(rParentheseToken);
					expr->idxRParentheseToken = lexer->getTokenIndex(rParentheseToken);
					break;
				}
				case TokenId::IntLiteral:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<I32LiteralExprNode>(
							((IntLiteralTokenExtension *)prefixToken->exData.get())->data,
							lexer->getTokenIndex(prefixToken)));
					lhs->tokenRange = lexer->getTokenIndex(prefixToken);
					lexer->nextToken();
					break;
				case TokenId::LongLiteral:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<I64LiteralExprNode>(
							((LongLiteralTokenExtension *)prefixToken->exData.get())->data,
							lexer->getTokenIndex(prefixToken)));
					lhs->tokenRange = lexer->getTokenIndex(prefixToken);
					lexer->nextToken();
					break;
				case TokenId::UIntLiteral:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<U32LiteralExprNode>(
							((UIntLiteralTokenExtension *)prefixToken->exData.get())->data,
							lexer->getTokenIndex(prefixToken)));
					lhs->tokenRange = lexer->getTokenIndex(prefixToken);
					lexer->nextToken();
					break;
				case TokenId::ULongLiteral:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<U64LiteralExprNode>(
							((ULongLiteralTokenExtension *)prefixToken->exData.get())->data,
							lexer->getTokenIndex(prefixToken)));
					lhs->tokenRange = lexer->getTokenIndex(prefixToken);
					lexer->nextToken();
					break;
				case TokenId::StringLiteral:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<StringLiteralExprNode>(
							((StringLiteralTokenExtension *)prefixToken->exData.get())->data,
							lexer->getTokenIndex(prefixToken)));
					lhs->tokenRange = lexer->getTokenIndex(prefixToken);
					lexer->nextToken();
					break;
				case TokenId::F32Literal:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<F32LiteralExprNode>(
							((F32LiteralTokenExtension *)prefixToken->exData.get())->data,
							lexer->getTokenIndex(prefixToken)));
					lhs->tokenRange = lexer->getTokenIndex(prefixToken);
					lexer->nextToken();
					break;
				case TokenId::F64Literal:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<F64LiteralExprNode>(
							((F64LiteralTokenExtension *)prefixToken->exData.get())->data,
							lexer->getTokenIndex(prefixToken)));
					lhs->tokenRange = lexer->getTokenIndex(prefixToken);
					lexer->nextToken();
					break;
				case TokenId::TrueKeyword:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<BoolLiteralExprNode>(
							true,
							lexer->getTokenIndex(prefixToken)));
					lhs->tokenRange = lexer->getTokenIndex(prefixToken);
					lexer->nextToken();
					break;
				case TokenId::FalseKeyword:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<BoolLiteralExprNode>(
							false,
							lexer->getTokenIndex(prefixToken)));
					lhs->tokenRange = lexer->getTokenIndex(prefixToken);
					lexer->nextToken();
					break;
				case TokenId::NullKeyword:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<NullLiteralExprNode>(
							lexer->getTokenIndex(prefixToken)));
					lhs->tokenRange = lexer->getTokenIndex(prefixToken);
					lexer->nextToken();
					break;
				case TokenId::LBrace: {
					lexer->nextToken();

					auto expr = std::make_shared<ArrayExprNode>();

					expr->tokenRange = lexer->getTokenIndex(prefixToken);
					expr->idxLBraceToken = lexer->getTokenIndex(prefixToken);

					while (true) {
						auto element = parseExpr();

						expr->tokenRange.endIndex = element->tokenRange.endIndex;
						expr->elements.push_back(element);

						if (Token *commaToken = lexer->peekToken(); commaToken->tokenId == TokenId::Comma) {
							lexer->nextToken();
							expr->tokenRange.endIndex = lexer->getTokenIndex(commaToken);
							expr->idxCommaTokens.push_back(lexer->getTokenIndex(commaToken));
						} else
							break;
					}

					lhs = std::static_pointer_cast<ExprNode>(expr);

					expectToken(TokenId::RBrace);
					break;
				}
				default:
					lexer->nextToken();
					throw SyntaxError("Expecting an expression", lexer->getTokenIndex(prefixToken));
			}
		}

		while (true) {
			Token *infixToken = lexer->peekToken();
			if (infixToken->tokenId == TokenId::End)
				break;

			if (auto it = infixOpRegistries.find(infixToken->tokenId); it != infixOpRegistries.end()) {
				if (it->second.leftPrecedence < precedence)
					break;

				lexer->nextToken();

				lhs = it->second.parselet(this, lhs, infixToken);
			} else
				break;
		}
	} catch (SyntaxError e) {
		compiler->messages.push_back(
			Message(
				compiler->tokenRangeToSourceLocation(e.tokenRange),
				MessageType::Error,
				e.what()));
		lhs = std::make_shared<BadExprNode>(lhs);
		lhs->tokenRange = TokenRange{ lexer->getTokenIndex(prefixToken), e.tokenRange.endIndex };
	}

	return lhs;
}
