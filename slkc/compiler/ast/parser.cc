#include <slkc/compiler/compiler.h>

using namespace slake;
using namespace slake::slkc;

void Parser::_putDefinition(
	Location loc,
	std::string name,
	std::shared_ptr<MemberNode> member) {
	if (curScope->members.count(name))
		throw FatalCompilationError(
			Message(
				loc,
				MessageType::Error,
				"Redefinition of `" + name + "'"));
	curScope->members[name] = member;
	member->parent = (MemberNode *)curScope->owner;
}

void Parser::_putFnDefinition(
	Location loc,
	std::string name,
	std::shared_ptr<FnOverloadingNode> overloading) {
	if (!curScope->members.count(name)) {
		curScope->members[name] = std::make_shared<FnNode>(compiler, name);
		curScope->members[name]->parent = (MemberNode *)curScope->owner;
	}

	if (curScope->members.at(name)->getNodeType() != NodeType::Fn) {
		throw FatalCompilationError(
			Message(
				loc,
				MessageType::Error,
				"Redefinition of `" + name + "'"));
	} else {
		auto fn = std::static_pointer_cast<FnNode>(curScope->members.at(name));
		fn->overloadingRegistries.push_back(overloading);
		overloading->owner = fn.get();
	}
}

std::map<TokenId, Parser::OpRegistry> Parser::prefixOpRegistries = {
	{ TokenId::SubOp,
		{ 131,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<UnaryOpExprNode>(
					opToken.beginLocation,
					UnaryOp::Neg,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::NotOp,
		{ 131,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<UnaryOpExprNode>(
					opToken.beginLocation,
					UnaryOp::Not,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::LNotOp,
		{ 131,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<UnaryOpExprNode>(
					opToken.beginLocation,
					UnaryOp::LNot,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::IncOp,
		{ 131,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<UnaryOpExprNode>(
					opToken.beginLocation,
					UnaryOp::IncF,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::DecOp,
		{ 131,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<UnaryOpExprNode>(
					opToken.beginLocation,
					UnaryOp::DecF,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
};

std::map<TokenId, Parser::OpRegistry> Parser::infixOpRegistries = {
	{ TokenId::LParenthese,
		{ 140,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<CallExprNode>();

				expr->target = lhs;
				expr->idxLParentheseToken = parser->lexer->getTokenIndex(opToken);

				parser->parseArgs(expr->args, expr->idxCommaTokens);

				expr->idxRParentheseToken = parser->lexer->getTokenIndex(
					parser->expectToken(parser->lexer->nextToken(), TokenId::RParenthese));

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::LBracket,
		{ 140,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					lhs->getLocation(),
					BinaryOp::Subscript,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr();

				const auto &closingToken = parser->expectToken(parser->lexer->nextToken(), TokenId::RBracket);
				expr->idxClosingToken = parser->lexer->getTokenIndex(closingToken);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::IncOp,
		{ 140,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<UnaryOpExprNode>(
					lhs->getLocation(),
					UnaryOp::IncB,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::DecOp,
		{ 140,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<UnaryOpExprNode>(
					lhs->getLocation(),
					UnaryOp::DecB,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::Dot,
		{ 140,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<HeadedIdRefExprNode>(
					lhs,
					parser->parseRef());

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::MulOp,
		{ 120,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					lhs->getLocation(),
					BinaryOp::Mul,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(121);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::DivOp,
		{ 120,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					lhs->getLocation(),
					BinaryOp::Div,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(121);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::ModOp,
		{ 120,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					lhs->getLocation(),
					BinaryOp::Mod,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(121);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::AddOp,
		{ 110,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					lhs->getLocation(),
					BinaryOp::Add,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(111);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::SubOp,
		{ 110,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					lhs->getLocation(),
					BinaryOp::Sub,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(111);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::LshOp,
		{ 100,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					lhs->getLocation(),
					BinaryOp::Lsh,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(101);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::RshOp,
		{ 100,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					lhs->getLocation(),
					BinaryOp::Rsh,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(101);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::GtOp,
		{ 90,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					lhs->getLocation(),
					BinaryOp::Gt,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(91);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::GtEqOp,
		{ 90,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					lhs->getLocation(),
					BinaryOp::GtEq,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(91);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::LtOp,
		{ 90,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					lhs->getLocation(),
					BinaryOp::Lt,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(91);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::LtEqOp,
		{ 90,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					lhs->getLocation(),
					BinaryOp::LtEq,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(91);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::EqOp,
		{ 80,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					lhs->getLocation(),
					BinaryOp::Eq,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(81);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
	{ TokenId::NeqOp,
		{ 80,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					lhs->getLocation(),
					BinaryOp::Neq,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(81);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::AndOp,
		{ 70,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					lhs->getLocation(),
					BinaryOp::And,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(71);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::XorOp,
		{ 60,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					lhs->getLocation(),
					BinaryOp::Xor,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(61);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::OrOp,
		{ 50,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					lhs->getLocation(),
					BinaryOp::Or,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(51);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::LAndOp,
		{ 40,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					lhs->getLocation(),
					BinaryOp::LAnd,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(41);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::LOrOp,
		{ 30,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					lhs->getLocation(),
					BinaryOp::LOr,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(31);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::Question,
		{ 21,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<TernaryOpExprNode>(lhs);

				expr->idxQuestionToken = parser->lexer->getTokenIndex(opToken);

				expr->x = parser->parseExpr(20);

				const auto &colonToken = parser->expectToken(parser->lexer->nextToken(), TokenId::Colon);
				expr->idxColonToken = parser->lexer->getTokenIndex(colonToken);

				expr->y = parser->parseExpr(20);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },

	{ TokenId::AssignOp,
		{ 11,
			[](Parser *parser, std::shared_ptr<ExprNode> lhs, const Token &opToken) -> std::shared_ptr<ExprNode> {
				auto expr = std::make_shared<BinaryOpExprNode>(
					lhs->getLocation(),
					BinaryOp::Assign,
					lhs);

				expr->idxOpToken = parser->lexer->getTokenIndex(opToken);

				expr->rhs = parser->parseExpr(10);

				return std::static_pointer_cast<ExprNode>(expr);
			} } },
};

AccessModifier Parser::parseAccessModifier(Location &locationOut) {
	AccessModifier accessModifier = 0;

	while (true) {
		auto &token = lexer->peekToken();

		if (!accessModifier)
			locationOut = token.beginLocation;

		switch (token.tokenId) {
			case TokenId::PubKeyword:
			case TokenId::FinalKeyword:
			case TokenId::OverrideKeyword:
			case TokenId::StaticKeyword:
			case TokenId::NativeKeyword:
				lexer->nextToken();
				switch (token.tokenId) {
					case TokenId::PubKeyword:
						accessModifier |= ACCESS_PUB;
						break;
					case TokenId::FinalKeyword:
						accessModifier |= ACCESS_FINAL;
						break;
					case TokenId::OverrideKeyword:
						accessModifier |= ACCESS_OVERRIDE;
						break;
					case TokenId::StaticKeyword:
						accessModifier |= ACCESS_STATIC;
						break;
					case TokenId::NativeKeyword:
						accessModifier |= ACCESS_NATIVE;
						break;
				}
				break;
			default:
				goto end;
		}
	}

end:
	return accessModifier;
}

std::shared_ptr<TypeNameNode> Parser::parseTypeName(bool forGenericArgs) {
	std::shared_ptr<TypeNameNode> type;

	switch (auto &token = lexer->peekToken(); token.tokenId) {
		case TokenId::I8TypeName:
		case TokenId::I16TypeName:
		case TokenId::I32TypeName:
		case TokenId::I64TypeName:
		case TokenId::U8TypeName:
		case TokenId::U16TypeName:
		case TokenId::U32TypeName:
		case TokenId::U64TypeName:
		case TokenId::F32TypeName:
		case TokenId::F64TypeName:
		case TokenId::StringTypeName:
		case TokenId::BoolTypeName:
		case TokenId::AutoTypeName:
		case TokenId::VoidTypeName:
		case TokenId::AnyTypeName: {
			switch (token.tokenId) {
				case TokenId::I8TypeName:
					type = std::make_shared<I8TypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::I16TypeName:
					type = std::make_shared<I16TypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::I32TypeName:
					type = std::make_shared<I32TypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::I64TypeName:
					type = std::make_shared<I64TypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::U8TypeName:
					type = std::make_shared<U8TypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::U16TypeName:
					type = std::make_shared<U16TypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::U32TypeName:
					type = std::make_shared<U32TypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::U64TypeName:
					type = std::make_shared<U64TypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::F32TypeName:
					type = std::make_shared<F32TypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::F64TypeName:
					type = std::make_shared<F64TypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::StringTypeName:
					type = std::make_shared<StringTypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::BoolTypeName:
					type = std::make_shared<BoolTypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::AutoTypeName:
					type = std::make_shared<AutoTypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::VoidTypeName:
					type = std::make_shared<VoidTypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::AnyTypeName:
					type = std::make_shared<AnyTypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
			}
			lexer->nextToken();

			break;
		}
		case TokenId::Id: {
			LexerContext savedContext = lexer->context;
			auto ref = parseRef(true);
			if (!isCompleteIdRef(ref)) {
				lexer->context = savedContext;
				return std::make_shared<BadTypeNameNode>(
					ref[0].loc,
					ref[0].idxToken,
					lexer->context.curIndex);
			}
			type = std::make_shared<CustomTypeNameNode>(token.beginLocation, ref, compiler, curScope.get());
			break;
		}
		default:
			if (forGenericArgs) {
				compiler->messages.push_back(
					Message(
						token.beginLocation,
						MessageType::Error,
						"Expecting a type name"));
			}
			return std::make_shared<BadTypeNameNode>(
				token.beginLocation,
				lexer->getTokenIndex(token),
				lexer->getTokenIndex(token));
	}

	if (const auto &lBracketToken = lexer->peekToken(); lBracketToken.tokenId == TokenId::LBracket) {
		lexer->nextToken();

		auto t = std::make_shared<ArrayTypeNameNode>(type);
		type = t;
		t->idxLBracketToken = lexer->getTokenIndex(lBracketToken);

		const auto &rBracketToken = lexer->peekToken();
		if (rBracketToken.tokenId != TokenId::RBracket) {
			compiler->messages.push_back(
				Message(
					rBracketToken.beginLocation,
					MessageType::Error,
					"Expecting ]"));
			return t;
		}
		lexer->nextToken();

		t->idxRBracketToken = lexer->getTokenIndex(rBracketToken);
	}

	if (auto &token = lexer->peekToken(); token.tokenId == TokenId::AndOp) {
		lexer->nextToken();

		type->isRef = true;
		type->idxRefIndicatorToken = lexer->getTokenIndex(token);
	}

	if (auto &token = lexer->peekToken(); token.tokenId == TokenId::MulOp) {
		lexer->nextToken();

		auto result = std::make_shared<ContextTypeNameNode>(type);
		result->idxIndicatorToken = lexer->getTokenIndex(token);
		type = result;
	}

	return type;
}

std::deque<std::shared_ptr<TypeNameNode>> Parser::parseGenericArgs(bool forTypeName) {
	LexerContext savedContext = lexer->context;
	std::deque<std::shared_ptr<TypeNameNode>> genericArgs;

	if (auto &token = lexer->nextToken(); token.tokenId != TokenId::LtOp)
		goto fail;

	while (true) {
		if (auto &token = lexer->peekToken(); token.tokenId == TokenId::GtOp)
			break;

		if (auto type = parseTypeName(true); type->getTypeId() != TypeId::Bad)
			genericArgs.push_back(type);
		else {
			if (forTypeName) {
				compiler->messages.push_back(
					Message(
						type->getLocation(),
						MessageType::Error,
						"Expecting a type name"));
				genericArgs.push_back(type);
				return genericArgs;
			} else
				goto fail;
		}

		if (auto &token = lexer->peekToken(); token.tokenId != TokenId::Comma)
			break;

		lexer->nextToken();
	}

	if (auto &token = lexer->peekToken(); token.tokenId != TokenId::GtOp) {
		if (forTypeName) {
			compiler->messages.push_back(
				Message(
					token.beginLocation,
					MessageType::Error,
					"Expecting a type name"));
			return genericArgs;
		} else
			goto fail;
	}
	lexer->nextToken();

	return genericArgs;

fail:
	lexer->context = savedContext;
	return {};
}

IdRef Parser::parseModuleRef() {
	IdRef ref;
	size_t idxPrecedingAccessOp = SIZE_MAX;

	while (true) {
		auto &nameToken = lexer->peekToken();

		auto refEntry = IdRefEntry(nameToken.beginLocation, SIZE_MAX, "");
		refEntry.idxAccessOpToken = idxPrecedingAccessOp;

		if (nameToken.tokenId != TokenId::Id) {
			// Return the bad reference.
			compiler->messages.push_back(Message(
				nameToken.beginLocation,
				MessageType::Error,
				"Expecting an identifier"));
			ref.push_back(refEntry);
			return ref;
		} else {
			// Push current reference scope.
			lexer->nextToken();
			refEntry.name = nameToken.text;
			refEntry.idxToken = lexer->getTokenIndex(nameToken);
			ref.push_back(refEntry);
		}

		if (auto &token = lexer->peekToken(); token.tokenId != TokenId::Dot)
			break;

		const auto &precedingAccessOpToken = lexer->nextToken();
		idxPrecedingAccessOp = lexer->getTokenIndex(precedingAccessOpToken);
	}

	return ref;
}

IdRef Parser::parseRef(bool forTypeName) {
	IdRef ref;
	size_t idxPrecedingAccessOp = SIZE_MAX;

	switch (auto &token = lexer->peekToken(); token.tokenId) {
		case TokenId::ThisKeyword:
		case TokenId::BaseKeyword:
		case TokenId::ScopeOp: {
			auto refEntry = IdRefEntry(token.beginLocation, lexer->getTokenIndex(token), "", {});

			switch (token.tokenId) {
				case TokenId::ThisKeyword:
					refEntry.name = "this";
					ref.push_back(refEntry);
					break;
				case TokenId::BaseKeyword:
					refEntry.name = "base";
					ref.push_back(refEntry);
					break;
				case TokenId::ScopeOp:
					refEntry.name = "";
					ref.push_back(refEntry);
					break;
			}

			lexer->nextToken();
			if (lexer->peekToken().tokenId != TokenId::Dot)
				goto end;

			const auto &precedingAccessOpToken = lexer->nextToken();
			idxPrecedingAccessOp = lexer->getTokenIndex(precedingAccessOpToken);

			break;
		}
	}

	while (true) {
		auto &nameToken = lexer->peekToken();

		auto refEntry = IdRefEntry(nameToken.beginLocation, SIZE_MAX, "");
		refEntry.idxAccessOpToken = idxPrecedingAccessOp;

		if (nameToken.tokenId != TokenId::Id) {
			// Return the bad reference.
			compiler->messages.push_back(Message(
				nameToken.beginLocation,
				MessageType::Error,
				"Expecting an identifier"));
			ref.push_back(refEntry);
			return ref;
		} else {
			// Push current reference scope.
			lexer->nextToken();
			refEntry.name = nameToken.text;
			refEntry.idxToken = lexer->getTokenIndex(nameToken);
			ref.push_back(refEntry);
		}

		if (lexer->peekToken().tokenId == TokenId::LtOp)
			ref.back().genericArgs = parseGenericArgs(forTypeName);

		if (auto &token = lexer->peekToken(); token.tokenId != TokenId::Dot)
			break;

		const auto &precedingAccessOpToken = lexer->nextToken();
		idxPrecedingAccessOp = lexer->getTokenIndex(precedingAccessOpToken);
	}

end:
	return ref;
}

void Parser::parseArgs(
	std::deque<std::shared_ptr<ExprNode>> &argsOut,
	std::deque<size_t> &idxCommaTokensOut) {
	while (true) {
		if (lexer->peekToken().tokenId == TokenId::RParenthese) {
			break;
		}

		argsOut.push_back(parseExpr());

		if (lexer->peekToken().tokenId != TokenId::Comma) {
			break;
		}

		const auto &commaToken = lexer->nextToken();
		idxCommaTokensOut.push_back(lexer->getTokenIndex(commaToken));
	}
}

std::shared_ptr<ExprNode> Parser::parseExpr(int precedence) {
	LexerContext savedContext = lexer->context;

	std::shared_ptr<ExprNode> lhs, rhs;

	const Token &prefixToken = expectToken(lexer->peekToken());

	try {
		if (auto it = prefixOpRegistries.find(prefixToken.tokenId); it != prefixOpRegistries.end()) {
			lexer->nextToken();

			lhs = it->second.parselet(this, parseExpr(it->second.leftPrecedence), prefixToken);
		} else {
			switch (prefixToken.tokenId) {
				case TokenId::ThisKeyword:
				case TokenId::BaseKeyword:
				case TokenId::ScopeOp:
				case TokenId::Id:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<IdRefExprNode>(
							parseRef()));
					break;
				case TokenId::LParenthese:
					lexer->nextToken();
					lhs = parseExpr();
					expectToken(TokenId::RParenthese);
					break;
				case TokenId::NewKeyword: {
					auto expr = std::make_shared<NewExprNode>(prefixToken.beginLocation);
					lhs = expr;

					const auto &newToken = lexer->nextToken();
					expr->idxNewToken = lexer->getTokenIndex(newToken);

					expr->type = parseTypeName();

					const auto &lParentheseToken = expectToken(TokenId::LParenthese);
					expr->idxLParentheseToken = lexer->getTokenIndex(lParentheseToken);

					parseArgs(expr->args, expr->idxCommaTokens);

					const auto &rParentheseToken = expectToken(TokenId::RParenthese);
					expr->idxRParentheseToken = lexer->getTokenIndex(rParentheseToken);
					break;
				}
				case TokenId::IntLiteral:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<I32LiteralExprNode>(
							prefixToken.beginLocation,
							((IntLiteralTokenExtension *)prefixToken.exData.get())->data,
							lexer->getTokenIndex(prefixToken)));
					lexer->nextToken();
					break;
				case TokenId::LongLiteral:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<I64LiteralExprNode>(
							prefixToken.beginLocation,
							((LongLiteralTokenExtension *)prefixToken.exData.get())->data,
							lexer->getTokenIndex(prefixToken)));
					lexer->nextToken();
					break;
				case TokenId::UIntLiteral:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<U32LiteralExprNode>(
							prefixToken.beginLocation,
							((UIntLiteralTokenExtension *)prefixToken.exData.get())->data,
							lexer->getTokenIndex(prefixToken)));
					lexer->nextToken();
					break;
				case TokenId::ULongLiteral:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<U64LiteralExprNode>(
							prefixToken.beginLocation,
							((ULongLiteralTokenExtension *)prefixToken.exData.get())->data,
							lexer->getTokenIndex(prefixToken)));
					lexer->nextToken();
					break;
				case TokenId::StringLiteral:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<StringLiteralExprNode>(
							prefixToken.beginLocation,
							((StringLiteralTokenExtension *)prefixToken.exData.get())->data,
							lexer->getTokenIndex(prefixToken)));
					lexer->nextToken();
					break;
				case TokenId::F32Literal:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<F32LiteralExprNode>(
							prefixToken.beginLocation,
							((F32LiteralTokenExtension *)prefixToken.exData.get())->data,
							lexer->getTokenIndex(prefixToken)));
					lexer->nextToken();
					break;
				case TokenId::F64Literal:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<F64LiteralExprNode>(
							prefixToken.beginLocation,
							((F64LiteralTokenExtension *)prefixToken.exData.get())->data,
							lexer->getTokenIndex(prefixToken)));
					lexer->nextToken();
					break;
				case TokenId::TrueKeyword:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<BoolLiteralExprNode>(
							prefixToken.beginLocation,
							true,
							lexer->getTokenIndex(prefixToken)));
					lexer->nextToken();
					break;
				case TokenId::FalseKeyword:
					lhs = std::static_pointer_cast<ExprNode>(
						std::make_shared<BoolLiteralExprNode>(
							prefixToken.beginLocation,
							false,
							lexer->getTokenIndex(prefixToken)));
					lexer->nextToken();
					break;
				case TokenId::LBrace: {
					lexer->nextToken();

					auto expr = std::make_shared<ArrayExprNode>(prefixToken.beginLocation);

					expr->idxLBraceToken = lexer->getTokenIndex(prefixToken);

					while (true) {
						expr->elements.push_back(parseExpr());

						if (auto& commaToken = lexer->peekToken(); commaToken.tokenId == TokenId::Comma) {
							lexer->nextToken();
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
					throw SyntaxError("Expecting an expression", prefixToken.beginLocation);
			}
		}

		while (true) {
			const Token &infixToken = lexer->peekToken();
			if (infixToken.tokenId == TokenId::End)
				break;

			if (auto it = infixOpRegistries.find(infixToken.tokenId); it != infixOpRegistries.end()) {
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
		lhs = std::make_shared<BadExprNode>(prefixToken.beginLocation, lhs);
	}

	return lhs;
}

void Parser::parseParentSlot(
	std::shared_ptr<TypeNameNode> &typeNameOut,
	size_t &idxLParentheseTokenOut,
	size_t &idxRParentheseTokenOut) {
	if (const auto &lParentheseToken = lexer->peekToken(); lParentheseToken.tokenId == TokenId::LParenthese) {
		lexer->nextToken();

		idxLParentheseTokenOut = lexer->getTokenIndex(lParentheseToken);

		typeNameOut = parseTypeName();

		const auto &rParentheseToken = expectToken(TokenId::RParenthese);
		idxRParentheseTokenOut = lexer->getTokenIndex(rParentheseToken);
	}
}

void Parser::parseImplList(
	std::deque<std::shared_ptr<TypeNameNode>> &implInterfacesOut,
	size_t &idxColonTokenOut,
	std::deque<size_t> &idxCommaTokensOut) {
	if (const auto &colonToken = lexer->peekToken(); colonToken.tokenId == TokenId::Colon) {
		lexer->nextToken();
		idxColonTokenOut = lexer->getTokenIndex(colonToken);

		while (true) {
			implInterfacesOut.push_back(parseTypeName());

			if (lexer->peekToken().tokenId != TokenId::Comma)
				break;

			const auto &commaToken = lexer->nextToken();
			idxCommaTokensOut.push_back(lexer->getTokenIndex(commaToken));
		}
	}
}

std::deque<std::shared_ptr<TypeNameNode>> Parser::parseTraitList() {
	std::deque<std::shared_ptr<TypeNameNode>> inheritedTraits;

	if (lexer->peekToken().tokenId == TokenId::LBracket) {
		lexer->nextToken();

		while (true) {
			inheritedTraits.push_back(parseTypeName());

			if (lexer->peekToken().tokenId != TokenId::Comma)
				break;

			lexer->nextToken();
		}

		expectToken(TokenId::RBracket);
	}

	return inheritedTraits;
}

void Parser::parseVarDefs(std::shared_ptr<VarDefStmtNode> varDefStmtOut) {
	while (true) {
		const auto &nameToken = expectToken(TokenId::Id);

		varDefStmtOut->varDefs[nameToken.text] = VarDefEntry(
			nameToken.beginLocation,
			nameToken.text,
			lexer->getTokenIndex(nameToken));
		VarDefEntry &entry = varDefStmtOut->varDefs[nameToken.text];

		if (const auto &token = lexer->peekToken(); token.tokenId == TokenId::Colon) {
			const auto &colonToken = lexer->nextToken();
			entry.idxColonToken = lexer->getTokenIndex(colonToken);

			entry.type = parseTypeName();
		}

		if (auto &token = lexer->peekToken(); token.tokenId == TokenId::AssignOp) {
			const auto &assignOpToken = lexer->nextToken();
			entry.idxAssignOpToken = lexer->getTokenIndex(assignOpToken);

			entry.initValue = parseExpr();
		}

		if (lexer->peekToken().tokenId != TokenId::Comma)
			break;

		const auto &commaToken = lexer->nextToken();
		entry.idxCommaToken = lexer->getTokenIndex(commaToken);
	}
}

std::shared_ptr<StmtNode> Parser::parseStmt() {
	LexerContext beginContext = lexer->context;
	std::shared_ptr<StmtNode> result;

	try {
		switch (auto &beginToken = lexer->peekToken(); beginToken.tokenId) {
			case TokenId::BreakKeyword: {
				auto stmt = std::make_shared<BreakStmtNode>(beginToken.beginLocation);
				result = std::static_pointer_cast<StmtNode>(stmt);

				const auto &idxBreakToken = lexer->nextToken();
				stmt->idxBreakToken = lexer->getTokenIndex(idxBreakToken);

				const auto &semicolonToken = expectToken(TokenId::Semicolon);
				stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);
				break;
			}
			case TokenId::ContinueKeyword: {
				auto stmt = std::make_shared<BreakStmtNode>(beginToken.beginLocation);
				result = std::static_pointer_cast<StmtNode>(stmt);

				const auto &idxBreakToken = lexer->nextToken();
				stmt->idxBreakToken = lexer->getTokenIndex(idxBreakToken);

				const auto &semicolonToken = expectToken(TokenId::Semicolon);
				stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);

				break;
			}
			case TokenId::ForKeyword: {
				auto stmt = std::make_shared<ForStmtNode>(beginToken.beginLocation);
				result = std::static_pointer_cast<StmtNode>(stmt);

				const auto &forToken = lexer->nextToken();
				stmt->idxForToken = lexer->getTokenIndex(forToken);

				const auto &lParentheseToken = expectToken(TokenId::LParenthese);
				stmt->idxLParentheseToken = lexer->getTokenIndex(lParentheseToken);

				if (const auto &letToken = lexer->peekToken(); letToken.tokenId == TokenId::LetKeyword) {
					stmt->varDefs = std::make_shared<VarDefStmtNode>(letToken.beginLocation);
					stmt->varDefs->idxLetToken = lexer->getTokenIndex(letToken);
					parseVarDefs(stmt->varDefs);
				}
				const auto &firstSemicolonToken = expectToken(TokenId::Semicolon);
				stmt->idxFirstSemicolonToken = lexer->getTokenIndex(firstSemicolonToken);

				stmt->condition = parseExpr();
				const auto &secondSemicolonToken = expectToken(TokenId::Semicolon);
				stmt->idxSecondSemicolonToken = lexer->getTokenIndex(secondSemicolonToken);

				stmt->endExpr = parseExpr();
				const auto &rParentheseToken = expectToken(TokenId::RParenthese);
				stmt->idxRParentheseToken = lexer->getTokenIndex(rParentheseToken);

				stmt->body = parseStmt();
				break;
			}
			case TokenId::WhileKeyword: {
				auto stmt = std::make_shared<WhileStmtNode>(beginToken.beginLocation);
				result = std::static_pointer_cast<StmtNode>(stmt);

				const auto &whileToken = lexer->nextToken();
				stmt->idxWhileToken = lexer->getTokenIndex(whileToken);

				const auto &lParentheseToken = expectToken(TokenId::LParenthese);
				stmt->idxLParentheseToken = lexer->getTokenIndex(lParentheseToken);

				stmt->condition = parseExpr();

				const auto &rParentheseToken = expectToken(TokenId::RParenthese);
				stmt->idxRParentheseToken = lexer->getTokenIndex(rParentheseToken);

				stmt->body = parseStmt();
				break;
			}
			case TokenId::ReturnKeyword: {
				auto stmt = std::make_shared<ReturnStmtNode>(beginToken.beginLocation);
				result = std::static_pointer_cast<StmtNode>(stmt);

				const auto &returnToken = lexer->nextToken();
				stmt->idxReturnToken = lexer->getTokenIndex(returnToken);

				if (auto &token = lexer->peekToken(); token.tokenId == TokenId::Semicolon) {
					lexer->nextToken();
					stmt->idxSemicolonToken = lexer->getTokenIndex(token);
				} else {
					stmt->returnValue = parseExpr();

					const auto &semicolonToken = expectToken(TokenId::Semicolon);
					stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);
				}
				break;
			}
			case TokenId::YieldKeyword: {
				auto stmt = std::make_shared<YieldStmtNode>(beginToken.beginLocation);
				result = std::static_pointer_cast<StmtNode>(stmt);

				const auto &returnToken = lexer->nextToken();
				stmt->idxYieldToken = lexer->getTokenIndex(returnToken);

				if (auto &token = lexer->peekToken(); token.tokenId == TokenId::Semicolon) {
					lexer->nextToken();
					stmt->idxSemicolonToken = lexer->getTokenIndex(token);
				} else {
					stmt->returnValue = parseExpr();

					const auto &semicolonToken = expectToken(TokenId::Semicolon);
					stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);
				}
				break;
			}
			case TokenId::IfKeyword: {
				auto stmt = std::make_shared<IfStmtNode>(beginToken.beginLocation);
				result = std::static_pointer_cast<StmtNode>(stmt);

				const auto &ifToken = lexer->nextToken();
				stmt->idxIfToken = lexer->getTokenIndex(ifToken);

				const auto &lParentheseToken = expectToken(TokenId::LParenthese);
				stmt->idxLParentheseToken = lexer->getTokenIndex(lParentheseToken);

				stmt->condition = parseExpr();

				const auto &rParentheseToken = expectToken(TokenId::RParenthese);
				stmt->idxRParentheseToken = lexer->getTokenIndex(rParentheseToken);

				stmt->body = parseStmt();

				if (const auto &elseToken = lexer->peekToken(); elseToken.tokenId == TokenId::ElseKeyword) {
					lexer->nextToken();
					stmt->idxElseToken = lexer->getTokenIndex(elseToken);
					stmt->elseBranch = parseStmt();
				}
				break;
			}
			case TokenId::TryKeyword: {
				auto stmt = std::make_shared<TryStmtNode>(beginToken.beginLocation);
				result = std::static_pointer_cast<StmtNode>(stmt);

				const auto &tryToken = lexer->nextToken();
				stmt->idxTryToken = lexer->getTokenIndex(tryToken);

				stmt->body = parseStmt();

				while (true) {
					auto &catchToken = lexer->peekToken();
					if (catchToken.tokenId != TokenId::CatchKeyword)
						break;
					lexer->nextToken();

					stmt->catchBlocks.push_back({});
					CatchBlock &catchBlock = stmt->catchBlocks.back();

					catchBlock.idxCatchToken = lexer->getTokenIndex(catchToken);

					const auto &lParentheseToken = expectToken(TokenId::LParenthese);
					catchBlock.idxLParentheseToken = lexer->getTokenIndex(lParentheseToken);

					catchBlock.targetType = parseTypeName();

					std::string exceptionVarName;

					if (auto &nameToken = lexer->peekToken(); nameToken.tokenId == TokenId::Id) {
						lexer->nextToken();
						catchBlock.idxExceptionVarNameToken = lexer->getTokenIndex(nameToken);

						exceptionVarName = nameToken.text;
					}

					const auto &rParentheseToken = expectToken(TokenId::RParenthese);
					catchBlock.idxRParentheseToken = lexer->getTokenIndex(rParentheseToken);

					catchBlock.body = parseStmt();
				}

				if (const auto &finalToken = lexer->peekToken(); finalToken.tokenId == TokenId::FinalKeyword) {
					lexer->nextToken();
					stmt->finalBlock.idxFinalToken = lexer->getTokenIndex(finalToken);

					stmt->finalBlock.loc = finalToken.beginLocation;
					stmt->finalBlock.body = parseStmt();
				}

				break;
			}
			case TokenId::SwitchKeyword: {
				lexer->nextToken();

				const auto stmt = std::make_shared<SwitchStmtNode>(beginToken.beginLocation);
				result = std::static_pointer_cast<StmtNode>(stmt);

				const auto &lParentheseToken = expectToken(TokenId::LParenthese);
				stmt->idxLParentheseToken = lexer->getTokenIndex(lParentheseToken);

				stmt->expr = parseExpr();

				const auto &rParentheseToken = expectToken(TokenId::RParenthese);
				stmt->idxRParentheseToken = lexer->getTokenIndex(rParentheseToken);

				const auto &lBraceToken = expectToken(TokenId::LBrace);
				stmt->idxLBraceToken = lexer->getTokenIndex(lBraceToken);

				while (true) {
					const auto &caseToken = lexer->peekToken();

					if (caseToken.tokenId != TokenId::CaseKeyword)
						break;

					stmt->cases.push_back({});
					SwitchCase &newCase = stmt->cases.back();
					newCase.idxCaseToken = lexer->getTokenIndex(caseToken);

					lexer->nextToken();

					newCase.condition = parseExpr();

					const auto &colonToken = expectToken(TokenId::Colon);
					newCase.idxColonToken = lexer->getTokenIndex(colonToken);

					while (true) {
						if (const auto &token = lexer->peekToken();
							(token.tokenId == TokenId::DefaultKeyword) ||
							(token.tokenId == TokenId::RBrace))
							break;
						newCase.body.push_back(parseStmt());
					}
				}

				if (const auto &defaultToken = lexer->peekToken(); defaultToken.tokenId == TokenId::DefaultKeyword) {
					lexer->nextToken();

					stmt->cases.push_back({});
					SwitchCase &defaultCase = stmt->cases.back();
					defaultCase.idxCaseToken = lexer->getTokenIndex(defaultToken);

					const auto &colonToken = expectToken(TokenId::Colon);
					defaultCase.idxColonToken = lexer->getTokenIndex(colonToken);

					std::deque<std::shared_ptr<StmtNode>> body;

					while (true) {
						if (const auto &token = lexer->peekToken(); token.tokenId == TokenId::RBrace)
							break;
						body.push_back(parseStmt());
					}
				}

				const auto &rBraceToken = expectToken(TokenId::RBrace);
				stmt->idxRBraceToken = lexer->getTokenIndex(rBraceToken);

				break;
			}
			case TokenId::LBrace: {
				const auto stmt = std::make_shared<CodeBlockStmtNode>(CodeBlock{ beginToken.beginLocation, {} });
				result = std::static_pointer_cast<StmtNode>(stmt);

				const auto &lBraceToken = lexer->nextToken();
				stmt->body.idxLBraceToken = lexer->getTokenIndex(lBraceToken);

				while (true) {
					if (lexer->peekToken().tokenId == TokenId::RBrace) {
						const auto &rBraceToken = lexer->nextToken();
						stmt->body.idxRBraceToken = lexer->getTokenIndex(rBraceToken);
						break;
					}

					stmt->body.stmts.push_back(parseStmt());
				}

				break;
			}
			case TokenId::LetKeyword: {
				const auto &letToken = lexer->nextToken();

				auto stmt = std::make_shared<VarDefStmtNode>(letToken.beginLocation);
				result = std::static_pointer_cast<StmtNode>(stmt);

				stmt->idxLetToken = lexer->getTokenIndex(letToken);

				parseVarDefs(stmt);

				const auto &semicolonToken = expectToken(TokenId::Semicolon);
				stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);

				break;
			}
			default: {
				auto stmt = std::make_shared<ExprStmtNode>();
				result = stmt;

				stmt->expr = parseExpr();

				const auto &semicolonToken = expectToken(TokenId::Semicolon);
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
		return std::make_shared<BadStmtNode>(e.location, result);
	}

	return result;
}

void Parser::parseParams(std::deque<std::shared_ptr<ParamNode>> &paramsOut, std::deque<size_t> &idxCommaTokensOut) {
	if (lexer->peekToken().tokenId == TokenId::RParenthese)
		return;

	while (true) {
		if (auto &varArgToken = lexer->peekToken(); varArgToken.tokenId == TokenId::VarArg) {
			auto param = std::make_shared<ParamNode>(
				varArgToken.beginLocation,
				std::make_shared<ArrayTypeNameNode>(
					std::make_shared<AnyTypeNameNode>(
						varArgToken.beginLocation,
						lexer->getTokenIndex(varArgToken))));

			param->name = "...";
			param->idxNameToken = lexer->getTokenIndex(varArgToken);

			paramsOut.push_back(param);
			lexer->nextToken();
			break;
		}

		auto type = parseTypeName();

		auto paramNode = std::make_shared<ParamNode>(type->getLocation(), type);

		auto &nameToken = lexer->peekToken();
		if (nameToken.tokenId != TokenId::Id) {
			compiler->messages.push_back(
				Message(
					nameToken.beginLocation,
					MessageType::Error,
					"Expecting an identifier"));
		} else {
			paramNode->name = nameToken.text;
			paramNode->idxNameToken = lexer->getTokenIndex(nameToken);
			lexer->nextToken();
		}

		paramsOut.push_back(paramNode);

		if (lexer->peekToken().tokenId != TokenId::Comma)
			break;

		const auto &commaToken = lexer->nextToken();
		idxCommaTokensOut.push_back(lexer->getTokenIndex(commaToken));
	}
}

std::shared_ptr<FnOverloadingNode> Parser::parseFnDecl(std::string &nameOut) {
	auto savedScope = curScope;
	curScope = std::make_shared<Scope>();

	auto &fnKeywordToken = expectToken(TokenId::FnKeyword);
	auto overloading = std::make_shared<FnOverloadingNode>(fnKeywordToken.beginLocation, compiler, curScope);

	auto &nameToken = lexer->peekToken();
	switch (nameToken.tokenId) {
		case TokenId::Id:
		case TokenId::NewKeyword:
		case TokenId::DeleteKeyword:
			lexer->nextToken();
			break;
		default:
			throw SyntaxError("Expecting identifier or new or delete", nameToken.beginLocation);
	}
	nameOut = nameToken.text;
	overloading->idxNameToken = lexer->getTokenIndex(nameToken);

	if (lexer->peekToken().tokenId == TokenId::LtOp)
		overloading->setGenericParams(parseGenericParams());

	{
		const auto &paramLParentheseToken = expectToken(TokenId::LParenthese);
		overloading->idxParamLParentheseToken = lexer->getTokenIndex(paramLParentheseToken);
	}

	parseParams(overloading->params, overloading->idxParamCommaTokens);

	{
		const auto &paramRParentheseToken = expectToken(TokenId::RParenthese);
		overloading->idxParamRParentheseToken = lexer->getTokenIndex(paramRParentheseToken);
	}

	if (auto &token = lexer->peekToken(); token.tokenId == TokenId::AsyncKeyword) {
		lexer->nextToken();

		overloading->idxAsyncModifierToken = lexer->getTokenIndex(token);

		overloading->isAsync = true;
	}

	if (auto &token = lexer->peekToken(); token.tokenId == TokenId::Colon) {
		lexer->nextToken();

		overloading->idxReturnTypeColonToken = lexer->getTokenIndex(token);

		overloading->returnType = parseTypeName();
	}

	curScope->parent = savedScope.get();
	curScope = savedScope;

	return overloading;
}

std::shared_ptr<FnOverloadingNode> Parser::parseFnDef(std::string &nameOut) {
	std::shared_ptr<FnOverloadingNode> overloading = parseFnDecl(nameOut);

	if (auto &token = lexer->peekToken(); token.tokenId == TokenId::LBrace) {
		auto savedScope = curScope;
		curScope = overloading->scope;

		lexer->nextToken();

		std::deque<std::shared_ptr<StmtNode>> stmts;

		while (true) {
			if (lexer->peekToken().tokenId == TokenId::RBrace) {
				lexer->nextToken();
				break;
			}

			stmts.push_back(parseStmt());
		}

		overloading->body = std::make_shared<BlockStmtNode>(
			token.beginLocation,
			stmts);

		curScope = savedScope;
	} else
		expectToken(TokenId::Semicolon);

	return overloading;
}

std::shared_ptr<FnOverloadingNode> Parser::parseOperatorDecl(std::string &nameOut) {
	auto savedScope = curScope;
	curScope = std::make_shared<Scope>();

	auto &operatorKeywordToken = expectToken(TokenId::OperatorKeyword);
	auto overloading = std::make_shared<FnOverloadingNode>(operatorKeywordToken.beginLocation, compiler, curScope);

	auto &nameToken = lexer->nextToken();
	std::string name;
	switch (nameToken.tokenId) {
		case TokenId::AddOp:
		case TokenId::SubOp:
		case TokenId::MulOp:
		case TokenId::DivOp:
		case TokenId::ModOp:
		case TokenId::AndOp:
		case TokenId::OrOp:
		case TokenId::XorOp:
		case TokenId::LAndOp:
		case TokenId::LOrOp:
		case TokenId::IncOp:
		case TokenId::DecOp:
		case TokenId::NotOp:
		case TokenId::LNotOp:
		case TokenId::AddAssignOp:
		case TokenId::SubAssignOp:
		case TokenId::MulAssignOp:
		case TokenId::DivAssignOp:
		case TokenId::ModAssignOp:
		case TokenId::AndAssignOp:
		case TokenId::OrAssignOp:
		case TokenId::XorAssignOp:
		case TokenId::EqOp:
		case TokenId::NeqOp:
		case TokenId::GtOp:
		case TokenId::LtOp:
		case TokenId::GtEqOp:
		case TokenId::LtEqOp:
		case TokenId::NewKeyword:
		case TokenId::DeleteKeyword:
			name = "operator" + nameToken.text;
			break;
		case TokenId::LBracket:
			name = "operator[]";
			expectToken(TokenId::RBracket);
			break;
		case TokenId::LParenthese:
			name = "operator()";
			expectToken(TokenId::RParenthese);
			break;
		default:
			throw SyntaxError("Unrecognized operator", nameToken.beginLocation);
	}

	{
		const auto &paramLParentheseToken = expectToken(TokenId::LParenthese);
		overloading->idxParamLParentheseToken = lexer->getTokenIndex(paramLParentheseToken);
	}

	parseParams(overloading->params, overloading->idxParamCommaTokens);

	{
		const auto &paramRParentheseToken = expectToken(TokenId::RParenthese);
		overloading->idxParamRParentheseToken = lexer->getTokenIndex(paramRParentheseToken);
	}

	if (auto &colonToken = lexer->peekToken(); colonToken.tokenId == TokenId::Colon) {
		lexer->nextToken();

		overloading->idxReturnTypeColonToken = lexer->getTokenIndex(colonToken);

		overloading->returnType = parseTypeName();
	}

	nameOut = name;

	curScope = savedScope;

	overloading->idxNameToken = lexer->getTokenIndex(nameToken);

	return overloading;
}

std::shared_ptr<FnOverloadingNode> Parser::parseOperatorDef(std::string &nameOut) {
	std::shared_ptr<FnOverloadingNode> overloading = parseOperatorDecl(nameOut);

	if (auto &token = lexer->peekToken(); token.tokenId == TokenId::LBrace) {
		auto savedScope = curScope;
		curScope = overloading->scope;

		lexer->nextToken();

		std::deque<std::shared_ptr<StmtNode>> stmts;

		while (true) {
			if (lexer->peekToken().tokenId == TokenId::RBrace) {
				lexer->nextToken();
				break;
			}

			stmts.push_back(parseStmt());
		}

		curScope = savedScope;

		overloading->body = std::make_shared<BlockStmtNode>(
			token.beginLocation,
			stmts);
	} else
		expectToken(TokenId::Semicolon);

	return overloading;
}

GenericParamNodeList Parser::parseGenericParams() {
	GenericParamNodeList genericParams;

	expectToken(TokenId::LtOp);

	while (true) {
		auto &nameToken = expectToken(TokenId::Id);

		auto param = std::make_shared<GenericParamNode>(
			nameToken.beginLocation,
			nameToken.text);

		param->idxNameToken = lexer->getTokenIndex(nameToken);

		parseParentSlot(
			param->baseType,
			param->idxParentSlotLParentheseToken,
			param->idxParentSlotRParentheseToken);
		parseImplList(
			param->interfaceTypes,
			param->idxImplInterfacesColonToken,
			param->idxImplInterfacesCommaTokens);

		// TODO: Parse trait types.

		genericParams.push_back(param);

		if (auto &token = lexer->peekToken(); token.tokenId != TokenId::Comma)
			break;

		const auto &token = lexer->nextToken();
		param->idxCommaToken = lexer->getTokenIndex(token);
	}

	expectToken(TokenId::GtOp);

	// stub
	return genericParams;
}

std::shared_ptr<ClassNode> Parser::parseClassDef() {
	auto &beginToken = expectTokens(lexer->nextToken(), TokenId::ClassKeyword);
	auto &nameToken = expectToken(TokenId::Id);

	std::shared_ptr<ClassNode> classNode = std::make_shared<ClassNode>(
		beginToken.beginLocation,
		compiler,
		nameToken.text);
	classNode->idxNameToken = lexer->getTokenIndex(nameToken);

	auto savedScope = curScope;
	try {
		curScope = classNode->scope;
		curScope->parent = savedScope.get();

		if (auto &token = lexer->peekToken(); token.tokenId == TokenId::LtOp) {
			classNode->setGenericParams(parseGenericParams());
		}

		parseParentSlot(
			classNode->parentClass,
			classNode->idxParentSlotLParentheseToken,
			classNode->idxParentSlotRParentheseToken);
		parseImplList(
			classNode->implInterfaces,
			classNode->idxImplInterfacesColonToken,
			classNode->idxImplInterfacesCommaTokens);

		const auto &lBraceToken = expectToken(TokenId::LBrace);
		classNode->idxLBraceToken = lexer->getTokenIndex(lBraceToken);

		while (true) {
			if (auto &token = lexer->peekToken();
				token.tokenId == TokenId::RBrace ||
				token.tokenId == TokenId::End)
				break;
			parseClassStmt();
		}

		const auto &rBraceToken = expectToken(TokenId::RBrace);
		classNode->idxRBraceToken = lexer->getTokenIndex(rBraceToken);
	} catch (SyntaxError e) {
		compiler->messages.push_back(Message(
			e.location,
			MessageType::Error,
			e.what()));
	}
	curScope = savedScope;

	return classNode;
}

void Parser::parseClassStmt() {
	Location loc;
	AccessModifier accessModifier = parseAccessModifier(loc);

	switch (auto &token = lexer->peekToken(); token.tokenId) {
		case TokenId::End:
		case TokenId::RBrace:
			return;
		case TokenId::ClassKeyword: {
			auto def = parseClassDef();

			def->access = accessModifier;

			_putDefinition(
				def->getLocation(),
				def->name,
				def);
			break;
		}
		case TokenId::OperatorKeyword: {
			std::string name;
			auto overloading = parseOperatorDef(name);

			overloading->access = accessModifier;

			_putFnDefinition(token.beginLocation, name, overloading);
			break;
		}
		case TokenId::FnKeyword: {
			std::string name;
			auto overloading = parseFnDef(name);

			overloading->access = accessModifier;

			_putFnDefinition(token.beginLocation, name, overloading);

			break;
		}
		case TokenId::LetKeyword: {
			const auto &letToken = lexer->nextToken();

			auto stmt = std::make_shared<VarDefStmtNode>(letToken.beginLocation);

			stmt->idxLetToken = lexer->getTokenIndex(letToken);

			parseVarDefs(stmt);

			const auto &semicolonToken = expectToken(TokenId::Semicolon);
			stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);

			for (auto &i : stmt->varDefs) {
				_putDefinition(
					i.second.loc,
					i.first,
					std::make_shared<VarNode>(
						i.second.loc,
						compiler,
						accessModifier,
						i.second.type,
						i.first,
						i.second.initValue,
						i.second.idxNameToken,
						i.second.idxColonToken,
						i.second.idxAssignOpToken,
						i.second.idxCommaToken));
			}
			break;
		}
		default:
			throw SyntaxError("Unrecognized token", token.beginLocation);
	}
}

std::shared_ptr<InterfaceNode> Parser::parseInterfaceDef() {
	auto &beginToken = expectTokens(lexer->nextToken(), TokenId::InterfaceKeyword);
	auto &nameToken = expectToken(TokenId::Id);

	std::shared_ptr<InterfaceNode> interfaceNode = std::make_shared<InterfaceNode>(
		beginToken.beginLocation,
		nameToken.text);

	interfaceNode->idxNameToken = lexer->getTokenIndex(nameToken);

	auto savedScope = curScope;
	try {
		curScope = interfaceNode->scope;
		curScope->parent = savedScope.get();

		if (auto &token = lexer->peekToken(); token.tokenId == TokenId::LtOp) {
			interfaceNode->setGenericParams(parseGenericParams());
		}

		parseImplList(
			interfaceNode->parentInterfaces,
			interfaceNode->idxImplInterfacesColonToken,
			interfaceNode->idxImplInterfacesCommaTokens);

		const auto &lBraceToken = expectToken(TokenId::LBrace);
		interfaceNode->idxLBraceToken = lexer->getTokenIndex(lBraceToken);

		while (true) {
			if (auto &token = lexer->peekToken();
				token.tokenId == TokenId::RBrace ||
				token.tokenId == TokenId::End)
				break;
			parseInterfaceStmt();
		}

		const auto &rBraceToken = expectToken(TokenId::RBrace);
		interfaceNode->idxRBraceToken = lexer->getTokenIndex(rBraceToken);
	} catch (SyntaxError e) {
		compiler->messages.push_back(Message(
			e.location,
			MessageType::Error,
			e.what()));
	}
	curScope = savedScope;

	return interfaceNode;
}

void Parser::parseInterfaceStmt() {
	Location loc;
	AccessModifier accessModifier = parseAccessModifier(loc);

	switch (auto &token = lexer->peekToken(); token.tokenId) {
		case TokenId::End:
		case TokenId::RBrace:
			return;
		case TokenId::ClassKeyword: {
			auto def = parseClassDef();

			def->access = accessModifier;

			_putDefinition(
				def->getLocation(),
				def->name,
				def);
			break;
		}
		case TokenId::OperatorKeyword: {
			std::string name;

			auto overloading = parseOperatorDef(name);

			overloading->access = accessModifier;

			_putFnDefinition(token.beginLocation, name, overloading);
			break;
		}
		case TokenId::FnKeyword: {
			std::string name;
			auto overloading = parseFnDef(name);

			overloading->access = accessModifier;

			_putFnDefinition(token.beginLocation, name, overloading);

			break;
		}
		case TokenId::LetKeyword: {
			const auto &letToken = lexer->nextToken();

			auto stmt = std::make_shared<VarDefStmtNode>(letToken.beginLocation);

			stmt->idxLetToken = lexer->getTokenIndex(letToken);

			parseVarDefs(stmt);

			const auto &semicolonToken = expectToken(TokenId::Semicolon);
			stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);

			for (auto &i : stmt->varDefs) {
				_putDefinition(
					i.second.loc,
					i.first,
					std::make_shared<VarNode>(
						i.second.loc,
						compiler,
						accessModifier,
						i.second.type,
						i.first,
						i.second.initValue,
						i.second.idxNameToken,
						i.second.idxColonToken,
						i.second.idxAssignOpToken,
						i.second.idxCommaToken));
			}
			break;
		}
		default:
			throw SyntaxError("Unrecognized token", token.beginLocation);
	}
}

std::shared_ptr<TraitNode> Parser::parseTraitDef() {
	auto &beginToken = expectToken(TokenId::TraitKeyword),
		 &nameToken = expectToken(TokenId::Id);

	std::shared_ptr<TraitNode> traitNode = std::make_shared<TraitNode>(beginToken.beginLocation, nameToken.text);
	traitNode->idxNameToken = lexer->getTokenIndex(nameToken);

	parseImplList(
		traitNode->parentTraits,
		traitNode->idxImplTraitsColonToken,
		traitNode->idxImplTraitsCommaTokens);

	const auto &lBraceToken = expectToken(TokenId::LBrace);
	traitNode->idxLBraceToken = lexer->getTokenIndex(lBraceToken);

	const auto &rBraceToken = expectToken(TokenId::RBrace);
	traitNode->idxRBraceToken = lexer->getTokenIndex(rBraceToken);

	return traitNode;
}

void Parser::parseTraitStmt() {
}

void Parser::parseProgramStmt() {
	Location loc;
	AccessModifier accessModifier = parseAccessModifier(loc);

	switch (auto &token = lexer->peekToken(); token.tokenId) {
		case TokenId::End:
			return;
		case TokenId::ClassKeyword: {
			auto def = parseClassDef();

			def->access = accessModifier;

			_putDefinition(
				def->getLocation(),
				def->name,
				def);
			break;
		}
		case TokenId::FnKeyword: {
			std::string name;
			auto overloading = parseFnDef(name);

			overloading->access = accessModifier;
			overloading->access |= ACCESS_STATIC;

			_putFnDefinition(token.beginLocation, name, overloading);

			break;
		}
		case TokenId::LetKeyword: {
			const auto &letToken = lexer->nextToken();

			auto stmt = std::make_shared<VarDefStmtNode>(letToken.beginLocation);

			stmt->idxLetToken = lexer->getTokenIndex(letToken);

			parseVarDefs(stmt);

			const auto &semicolonToken = expectToken(TokenId::Semicolon);
			stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);

			for (auto &i : stmt->varDefs) {
				auto varNode = std::make_shared<VarNode>(
					i.second.loc,
					compiler,
					0,
					i.second.type,
					i.first,
					i.second.initValue,
					i.second.idxNameToken,
					i.second.idxColonToken,
					i.second.idxAssignOpToken,
					i.second.idxCommaToken);

				varNode->access = accessModifier;
				varNode->access |= ACCESS_STATIC;

				_putDefinition(
					i.second.loc,
					i.first,
					varNode);
			}
			break;
		}
		default:
			throw SyntaxError("Unrecognized token", token.beginLocation);
	}
}

void Parser::parseModuleDecl() {
	if (auto &beginToken = lexer->peekToken(); beginToken.tokenId == TokenId::ModuleKeyword) {
		lexer->nextToken();

		curModule->moduleName = parseModuleRef();

		expectToken(TokenId::Semicolon);
	}
}

void Parser::parseImportList() {
	if (auto &beginToken = lexer->peekToken(); beginToken.tokenId == TokenId::ImportKeyword) {
		lexer->nextToken();

		const auto &lBraceToken = expectToken(TokenId::LBrace);

#if SLKC_WITH_LANGUAGE_SERVER
		compiler->updateTokenInfo(lexer->getTokenIndex(lBraceToken), [](TokenInfo &tokenInfo) {
			tokenInfo.completionContext = CompletionContext::Import;
		});
#endif

		while (true) {
			auto ref = parseModuleRef();

			if (auto &asToken = lexer->peekToken(); asToken.tokenId == TokenId::AsKeyword) {
				lexer->nextToken();

				auto &nameToken = expectToken(TokenId::Id);

				curModule->imports[nameToken.text] = { ref, lexer->getTokenIndex(nameToken) };
			} else
				curModule->unnamedImports.push_back({ ref, SIZE_MAX });

			if (lexer->peekToken().tokenId != TokenId::Comma)
				break;

			lexer->nextToken();
		}

		expectToken(TokenId::RBrace);
	}
}

void Parser::parse(Lexer *lexer, Compiler *compiler) {
	this->compiler = compiler;
	this->lexer = lexer;

	curModule = compiler->_targetModule;
	curScope = curModule->scope;

	parseModuleDecl();
	parseImportList();

	while (lexer->peekToken().tokenId != TokenId::End) {
		parseProgramStmt();
	}
}
