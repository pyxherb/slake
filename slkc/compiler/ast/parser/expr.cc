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

				expr->sourceLocation =
					SourceLocation{ lhs->sourceLocation.beginPosition, opToken->location.endPosition };
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::NotOp,
		{ 131,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<UnaryOpExprNode>(
					UnaryOp::Not,
					lhs);

				expr->sourceLocation =
					SourceLocation{ lhs->sourceLocation.beginPosition, opToken->location.endPosition };
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::LNotOp,
		{ 131,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<UnaryOpExprNode>(
					UnaryOp::LNot,
					lhs);

				expr->sourceLocation =
					SourceLocation{ lhs->sourceLocation.beginPosition, opToken->location.endPosition };
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
				expr->sourceLocation = SourceLocation{ lhs->sourceLocation.beginPosition, opToken->location.endPosition };
				expr->idxLParentheseToken = parser->lexer->getTokenIndex(opToken);

				parser->parseArgs(expr->args, expr->idxCommaTokens);
				if (expr->args.size())
					expr->sourceLocation.endPosition = expr->args.back()->sourceLocation.endPosition;

				Token *lParentheseToken = parser->lexer->nextToken();
				expr->idxRParentheseToken = parser->lexer->getTokenIndex(
					parser->expectToken(lParentheseToken, TokenId::RParenthese));
				expr->sourceLocation.endPosition = lParentheseToken->location.endPosition;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::LBracket,
		{ 140,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Subscript,
					lhs);

				expr->sourceLocation = SourceLocation{ lhs->sourceLocation.beginPosition, opToken->location.endPosition };
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr();
				expr->sourceLocation.endPosition = expr->rhs->sourceLocation.endPosition;

				Token *closingToken = parser->expectToken(parser->lexer->nextToken(), TokenId::RBracket);
				expr->idxClosingToken = parser->lexer->getTokenIndex(closingToken);
				expr->sourceLocation.endPosition = closingToken->location.endPosition;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::Dot,
		{ 140,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<HeadedIdRefExprNode>(
					lhs,
					parser->parseRef());

				expr->sourceLocation = SourceLocation{ lhs->sourceLocation.beginPosition, expr->ref.back().loc.endPosition };
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::MulOp,
		{ 120,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Mul,
					lhs);

				expr->sourceLocation = lhs->sourceLocation;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(121);
				expr->sourceLocation.endPosition = expr->rhs->sourceLocation.endPosition;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::DivOp,
		{ 120,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Div,
					lhs);

				expr->sourceLocation = lhs->sourceLocation;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(121);
				expr->sourceLocation.endPosition = expr->rhs->sourceLocation.endPosition;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::ModOp,
		{ 120,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Mod,
					lhs);

				expr->sourceLocation = lhs->sourceLocation;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(121);
				expr->sourceLocation.endPosition = expr->rhs->sourceLocation.endPosition;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::AddOp,
		{ 110,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Add,
					lhs);

				expr->sourceLocation = lhs->sourceLocation;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(111);
				expr->sourceLocation.endPosition = expr->rhs->sourceLocation.endPosition;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::SubOp,
		{ 110,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Sub,
					lhs);

				expr->sourceLocation = lhs->sourceLocation;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(111);
				expr->sourceLocation.endPosition = expr->rhs->sourceLocation.endPosition;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::LshOp,
		{ 100,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Lsh,
					lhs);

				expr->sourceLocation = lhs->sourceLocation;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(101);
				expr->sourceLocation.endPosition = expr->rhs->sourceLocation.endPosition;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::RshOp,
		{ 100,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Rsh,
					lhs);

				expr->sourceLocation = lhs->sourceLocation;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(101);
				expr->sourceLocation.endPosition = expr->rhs->sourceLocation.endPosition;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::GtOp,
		{ 90,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Gt,
					lhs);

				expr->sourceLocation = lhs->sourceLocation;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(91);
				expr->sourceLocation.endPosition = expr->rhs->sourceLocation.endPosition;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::GtEqOp,
		{ 90,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::GtEq,
					lhs);

				expr->sourceLocation = lhs->sourceLocation;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(91);
				expr->sourceLocation.endPosition = expr->rhs->sourceLocation.endPosition;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::LtOp,
		{ 90,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Lt,
					lhs);

				expr->sourceLocation = lhs->sourceLocation;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(91);
				expr->sourceLocation.endPosition = expr->rhs->sourceLocation.endPosition;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::LtEqOp,
		{ 90,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::LtEq,
					lhs);

				expr->sourceLocation = lhs->sourceLocation;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(91);
				expr->sourceLocation.endPosition = expr->rhs->sourceLocation.endPosition;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::EqOp,
		{ 80,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Eq,
					lhs);

				expr->sourceLocation = lhs->sourceLocation;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(81);
				expr->sourceLocation.endPosition = expr->rhs->sourceLocation.endPosition;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::NeqOp,
		{ 80,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Neq,
					lhs);

				expr->sourceLocation = lhs->sourceLocation;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(81);
				expr->sourceLocation.endPosition = expr->rhs->sourceLocation.endPosition;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::StrictEqOp,
		{ 80,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::StrictEq,
					lhs);

				expr->sourceLocation = lhs->sourceLocation;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(81);
				expr->sourceLocation.endPosition = expr->rhs->sourceLocation.endPosition;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::StrictNeqOp,
		{ 80,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::StrictNeq,
					lhs);

				expr->sourceLocation = lhs->sourceLocation;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(81);
				expr->sourceLocation.endPosition = expr->rhs->sourceLocation.endPosition;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::AndOp,
		{ 70,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::And,
					lhs);

				expr->sourceLocation = lhs->sourceLocation;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(71);
				expr->sourceLocation.endPosition = expr->rhs->sourceLocation.endPosition;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::XorOp,
		{ 60,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Xor,
					lhs);

				expr->sourceLocation = lhs->sourceLocation;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(61);
				expr->sourceLocation.endPosition = expr->rhs->sourceLocation.endPosition;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::OrOp,
		{ 50,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Or,
					lhs);

				expr->sourceLocation = lhs->sourceLocation;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(51);
				expr->sourceLocation.endPosition = expr->rhs->sourceLocation.endPosition;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::LAndOp,
		{ 40,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::LAnd,
					lhs);

				expr->sourceLocation = lhs->sourceLocation;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(41);
				expr->sourceLocation.endPosition = expr->rhs->sourceLocation.endPosition;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::LOrOp,
		{ 30,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::LOr,
					lhs);

				expr->sourceLocation = lhs->sourceLocation;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(31);
				expr->sourceLocation.endPosition = expr->rhs->sourceLocation.endPosition;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::Question,
		{ 21,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<TernaryOpExprNode>(lhs);

				expr->sourceLocation = SourceLocation{ lhs->sourceLocation.beginPosition, opToken->location.endPosition };
				expr->idxQuestionToken = parser->lexer->getTokenIndex(opToken);

				expr->x = parser->parseExpr(20);
				expr->sourceLocation.endPosition = expr->x->sourceLocation.endPosition;

				Token *colonToken = parser->expectToken(parser->lexer->nextToken(), TokenId::Colon);
				expr->idxColonToken = parser->lexer->getTokenIndex(colonToken);
				expr->sourceLocation.endPosition = colonToken->location.endPosition;

				expr->y = parser->parseExpr(20);
				expr->sourceLocation.endPosition = expr->y->sourceLocation.endPosition;

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::AssignOp,
		{ 11,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, Token *opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					BinaryOp::Assign,
					lhs);

				expr->sourceLocation = lhs->sourceLocation;
				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(10);
				expr->sourceLocation.endPosition = expr->rhs->sourceLocation.endPosition;

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
					lhs->sourceLocation = SourceLocation{ ref[0].loc.beginPosition, ref.back().loc.endPosition };
					break;
				}
				case TokenId::LParenthese: {
					lexer->nextToken();

					lhs = parseExpr();
					lhs->sourceLocation.beginPosition = prefixToken->location.beginPosition;

					Token *rParentheseToken = expectToken(TokenId::RParenthese);
					lhs->sourceLocation.endPosition = rParentheseToken->location.endPosition;
					break;
				}
				case TokenId::NewKeyword: {
					auto expr = std::make_shared<NewExprNode>();

					Token *newToken = lexer->nextToken();
					expr->sourceLocation = prefixToken->location;
					expr->idxNewToken = lexer->getTokenIndex(newToken);

					lhs = expr;

					expr->type = parseTypeName();

					Token *lParentheseToken = expectToken(TokenId::LParenthese);
					expr->sourceLocation.endPosition = lParentheseToken->location.endPosition;
					expr->idxLParentheseToken = lexer->getTokenIndex(lParentheseToken);

					parseArgs(expr->args, expr->idxCommaTokens);

					if (expr->args.size())
						expr->sourceLocation.endPosition = expr->args.back()->sourceLocation.endPosition;

					Token *rParentheseToken = expectToken(TokenId::RParenthese);
					expr->sourceLocation.endPosition = rParentheseToken->location.endPosition;
					expr->idxRParentheseToken = lexer->getTokenIndex(rParentheseToken);
					break;
				}
				case TokenId::IntLiteral:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<I32LiteralExprNode>(
							((IntLiteralTokenExtension *)prefixToken->exData.get())->data,
							lexer->getTokenIndex(prefixToken)));
					lhs->sourceLocation = prefixToken->location;
					lexer->nextToken();
					break;
				case TokenId::LongLiteral:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<I64LiteralExprNode>(
							((LongLiteralTokenExtension *)prefixToken->exData.get())->data,
							lexer->getTokenIndex(prefixToken)));
					lhs->sourceLocation = prefixToken->location;
					lexer->nextToken();
					break;
				case TokenId::UIntLiteral:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<U32LiteralExprNode>(
							((UIntLiteralTokenExtension *)prefixToken->exData.get())->data,
							lexer->getTokenIndex(prefixToken)));
					lhs->sourceLocation = prefixToken->location;
					lexer->nextToken();
					break;
				case TokenId::ULongLiteral:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<U64LiteralExprNode>(
							((ULongLiteralTokenExtension *)prefixToken->exData.get())->data,
							lexer->getTokenIndex(prefixToken)));
					lhs->sourceLocation = prefixToken->location;
					lexer->nextToken();
					break;
				case TokenId::StringLiteral:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<StringLiteralExprNode>(
							((StringLiteralTokenExtension *)prefixToken->exData.get())->data,
							lexer->getTokenIndex(prefixToken)));
					lhs->sourceLocation = prefixToken->location;
					lexer->nextToken();
					break;
				case TokenId::F32Literal:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<F32LiteralExprNode>(
							((F32LiteralTokenExtension *)prefixToken->exData.get())->data,
							lexer->getTokenIndex(prefixToken)));
					lhs->sourceLocation = prefixToken->location;
					lexer->nextToken();
					break;
				case TokenId::F64Literal:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<F64LiteralExprNode>(
							((F64LiteralTokenExtension *)prefixToken->exData.get())->data,
							lexer->getTokenIndex(prefixToken)));
					lhs->sourceLocation = prefixToken->location;
					lexer->nextToken();
					break;
				case TokenId::TrueKeyword:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<BoolLiteralExprNode>(
							true,
							lexer->getTokenIndex(prefixToken)));
					lhs->sourceLocation = prefixToken->location;
					lexer->nextToken();
					break;
				case TokenId::FalseKeyword:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<BoolLiteralExprNode>(
							false,
							lexer->getTokenIndex(prefixToken)));
					lhs->sourceLocation = prefixToken->location;
					lexer->nextToken();
					break;
				case TokenId::NullKeyword:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<NullLiteralExprNode>(
							lexer->getTokenIndex(prefixToken)));
					lhs->sourceLocation = prefixToken->location;
					lexer->nextToken();
					break;
				case TokenId::LBrace: {
					lexer->nextToken();

					auto expr = std::make_shared<ArrayExprNode>();

					expr->sourceLocation = prefixToken->location;
					expr->idxLBraceToken = lexer->getTokenIndex(prefixToken);

					while (true) {
						auto element = parseExpr();

						expr->sourceLocation.endPosition = element->sourceLocation.endPosition;
						expr->elements.push_back(element);

						if (Token *commaToken = lexer->peekToken(); commaToken->tokenId == TokenId::Comma) {
							lexer->nextToken();
							expr->sourceLocation.endPosition = commaToken->location.endPosition;
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
					throw SyntaxError("Expecting an expression", prefixToken->location);
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
				e.location,
				MessageType::Error,
				e.what()));
		lhs = std::make_shared<BadExprNode>(lhs);
		lhs->sourceLocation = SourceLocation{ prefixToken->location.beginPosition, e.location.endPosition };
	}

	return lhs;
}
