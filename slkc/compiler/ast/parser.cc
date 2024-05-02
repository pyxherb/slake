#include <slkc/compiler/compiler.h>

using namespace slake;
using namespace slake::slkc;

void Parser::_putDefinition(
	Location loc,
	string name,
	shared_ptr<MemberNode> member) {
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
	string name,
	shared_ptr<FnOverloadingNode> overloading) {
	if (!curScope->members.count(name)) {
		curScope->members[name] = make_shared<FnNode>(compiler, name);
		curScope->members[name]->parent = (MemberNode *)curScope->owner;
	}

	if (curScope->members.at(name)->getNodeType() != NodeType::Fn) {
		throw FatalCompilationError(
			Message(
				loc,
				MessageType::Error,
				"Redefinition of `" + name + "'"));
	} else {
		auto fn = static_pointer_cast<FnNode>(curScope->members.at(name));
		fn->overloadingRegistries.push_back(overloading);
		overloading->owner = fn.get();
	}
}

std::map<TokenId, Parser::OpRegistry> Parser::prefixOpRegistries = {
	{ TokenId::SubOp,
		{ 131,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				return static_pointer_cast<ExprNode>(
					make_shared<UnaryOpExprNode>(
						lhs->getLocation(),
						UnaryOp::Neg,
						lhs));
			} } },
	{ TokenId::RevOp,
		{ 131,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				return static_pointer_cast<ExprNode>(
					make_shared<UnaryOpExprNode>(
						lhs->getLocation(),
						UnaryOp::Not,
						lhs));
			} } },
	{ TokenId::NotOp,
		{ 131,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				return static_pointer_cast<ExprNode>(
					make_shared<UnaryOpExprNode>(
						lhs->getLocation(),
						UnaryOp::LNot,
						lhs));
			} } },
	{ TokenId::IncOp,
		{ 131,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				return static_pointer_cast<ExprNode>(
					make_shared<UnaryOpExprNode>(
						lhs->getLocation(),
						UnaryOp::IncF,
						lhs));
			} } },
	{ TokenId::DecOp,
		{ 131,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				return static_pointer_cast<ExprNode>(
					make_shared<UnaryOpExprNode>(
						lhs->getLocation(),
						UnaryOp::DecF,
						lhs));
			} } },
};

std::map<TokenId, Parser::OpRegistry> Parser::infixOpRegistries = {
	{ TokenId::LParenthese,
		{ 140,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				auto args = parser->parseArgs();

				parser->expectToken(parser->lexer->nextToken(), TokenId::RParenthese);

				return static_pointer_cast<ExprNode>(
					make_shared<CallExprNode>(
						lhs,
						args));
			} } },
	{ TokenId::LBracket,
		{ 140,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				auto rhs = parser->parseExpr();

				parser->expectToken(parser->lexer->nextToken(), TokenId::RBracket);

				return static_pointer_cast<ExprNode>(
					make_shared<BinaryOpExprNode>(
						lhs->getLocation(),
						BinaryOp::Subscript,
						lhs,
						rhs));
			} } },
	{ TokenId::IncOp,
		{ 140,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				return static_pointer_cast<ExprNode>(
					make_shared<UnaryOpExprNode>(
						lhs->getLocation(),
						UnaryOp::IncB,
						lhs));
			} } },
	{ TokenId::DecOp,
		{ 140,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				return static_pointer_cast<ExprNode>(
					make_shared<UnaryOpExprNode>(
						lhs->getLocation(),
						UnaryOp::DecB,
						lhs));
			} } },
	{ TokenId::Dot,
		{ 140,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				return static_pointer_cast<ExprNode>(
					make_shared<HeadedRefExprNode>(
						lhs,
						parser->parseRef()));
			} } },

	{ TokenId::MulOp,
		{ 120,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				auto rhs = parser->parseExpr(121);

				return static_pointer_cast<ExprNode>(
					make_shared<BinaryOpExprNode>(
						lhs->getLocation(),
						BinaryOp::Mul,
						lhs,
						rhs));
			} } },
	{ TokenId::DivOp,
		{ 120,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				auto rhs = parser->parseExpr(121);

				return static_pointer_cast<ExprNode>(
					make_shared<BinaryOpExprNode>(
						lhs->getLocation(),
						BinaryOp::Div,
						lhs,
						rhs));
			} } },
	{ TokenId::ModOp,
		{ 120,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				auto rhs = parser->parseExpr(121);

				return static_pointer_cast<ExprNode>(
					make_shared<BinaryOpExprNode>(
						lhs->getLocation(),
						BinaryOp::Mod,
						lhs,
						rhs));
			} } },

	{ TokenId::AddOp,
		{ 110,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				auto rhs = parser->parseExpr(111);

				return static_pointer_cast<ExprNode>(
					make_shared<BinaryOpExprNode>(
						lhs->getLocation(),
						BinaryOp::Add,
						lhs,
						rhs));
			} } },
	{ TokenId::SubOp,
		{ 110,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				auto rhs = parser->parseExpr(111);

				return static_pointer_cast<ExprNode>(
					make_shared<BinaryOpExprNode>(
						lhs->getLocation(),
						BinaryOp::Sub,
						lhs,
						rhs));
			} } },

	{ TokenId::LshOp,
		{ 100,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				auto rhs = parser->parseExpr(101);

				return static_pointer_cast<ExprNode>(
					make_shared<BinaryOpExprNode>(
						lhs->getLocation(),
						BinaryOp::Lsh,
						lhs,
						rhs));
			} } },
	{ TokenId::RshOp,
		{ 100,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				auto rhs = parser->parseExpr(101);

				return static_pointer_cast<ExprNode>(
					make_shared<BinaryOpExprNode>(
						lhs->getLocation(),
						BinaryOp::Rsh,
						lhs,
						rhs));
			} } },

	{ TokenId::GtOp,
		{ 90,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				auto rhs = parser->parseExpr(91);

				return static_pointer_cast<ExprNode>(
					make_shared<BinaryOpExprNode>(
						lhs->getLocation(),
						BinaryOp::Gt,
						lhs,
						rhs));
			} } },
	{ TokenId::GtEqOp,
		{ 90,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				auto rhs = parser->parseExpr(91);

				return static_pointer_cast<ExprNode>(
					make_shared<BinaryOpExprNode>(
						lhs->getLocation(),
						BinaryOp::GtEq,
						lhs,
						rhs));
			} } },
	{ TokenId::LtOp,
		{ 90,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				auto rhs = parser->parseExpr(91);

				return static_pointer_cast<ExprNode>(
					make_shared<BinaryOpExprNode>(
						lhs->getLocation(),
						BinaryOp::Lt,
						lhs,
						rhs));
			} } },
	{ TokenId::LtEqOp,
		{ 90,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				auto rhs = parser->parseExpr(91);

				return static_pointer_cast<ExprNode>(
					make_shared<BinaryOpExprNode>(
						lhs->getLocation(),
						BinaryOp::LtEq,
						lhs,
						rhs));
			} } },

	{ TokenId::EqOp,
		{ 80,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				auto rhs = parser->parseExpr(81);

				return static_pointer_cast<ExprNode>(
					make_shared<BinaryOpExprNode>(
						lhs->getLocation(),
						BinaryOp::Eq,
						lhs,
						rhs));
			} } },
	{ TokenId::NeqOp,
		{ 80,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				auto rhs = parser->parseExpr(81);

				return static_pointer_cast<ExprNode>(
					make_shared<BinaryOpExprNode>(
						lhs->getLocation(),
						BinaryOp::Neq,
						lhs,
						rhs));
			} } },

	{ TokenId::AndOp,
		{ 70,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				auto rhs = parser->parseExpr(71);

				return static_pointer_cast<ExprNode>(
					make_shared<BinaryOpExprNode>(
						lhs->getLocation(),
						BinaryOp::And,
						lhs,
						rhs));
			} } },

	{ TokenId::XorOp,
		{ 60,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				auto rhs = parser->parseExpr(61);

				return static_pointer_cast<ExprNode>(
					make_shared<BinaryOpExprNode>(
						lhs->getLocation(),
						BinaryOp::Xor,
						lhs,
						rhs));
			} } },

	{ TokenId::OrOp,
		{ 50,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				auto rhs = parser->parseExpr(51);

				return static_pointer_cast<ExprNode>(
					make_shared<BinaryOpExprNode>(
						lhs->getLocation(),
						BinaryOp::Or,
						lhs,
						rhs));
			} } },

	{ TokenId::LAndOp,
		{ 40,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				auto rhs = parser->parseExpr(41);

				return static_pointer_cast<ExprNode>(
					make_shared<BinaryOpExprNode>(
						lhs->getLocation(),
						BinaryOp::LAnd,
						lhs,
						rhs));
			} } },

	{ TokenId::LOrOp,
		{ 30,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				auto rhs = parser->parseExpr(31);

				return static_pointer_cast<ExprNode>(
					make_shared<BinaryOpExprNode>(
						lhs->getLocation(),
						BinaryOp::LOr,
						lhs,
						rhs));
			} } },

	{ TokenId::Question,
		{ 21,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				auto trueBranch = parser->parseExpr(20);

				parser->expectToken(parser->lexer->nextToken(), TokenId::Colon);

				auto falseBranch = parser->parseExpr(20);

				return static_pointer_cast<ExprNode>(
					make_shared<TernaryOpExprNode>(
						lhs,
						trueBranch,
						falseBranch));
			} } },

	{ TokenId::AssignOp,
		{ 11,
			[](Parser *parser, shared_ptr<ExprNode> lhs) -> shared_ptr<ExprNode> {
				auto rhs = parser->parseExpr(10);

				return static_pointer_cast<ExprNode>(
					make_shared<BinaryOpExprNode>(
						lhs->getLocation(),
						BinaryOp::Assign,
						lhs,
						rhs));
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
			case TokenId::ConstKeyword:
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
					case TokenId::ConstKeyword:
						accessModifier |= ACCESS_CONST;
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

shared_ptr<TypeNameNode> Parser::parseTypeName() {
	shared_ptr<TypeNameNode> type;

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
					type = make_shared<I8TypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::I16TypeName:
					type = make_shared<I16TypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::I32TypeName:
					type = make_shared<I32TypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::I64TypeName:
					type = make_shared<I64TypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::U8TypeName:
					type = make_shared<U8TypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::U16TypeName:
					type = make_shared<U16TypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::U32TypeName:
					type = make_shared<U32TypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::U64TypeName:
					type = make_shared<U64TypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::F32TypeName:
					type = make_shared<F32TypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::F64TypeName:
					type = make_shared<F64TypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::StringTypeName:
					type = make_shared<StringTypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::BoolTypeName:
					type = make_shared<BoolTypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::AutoTypeName:
					type = make_shared<AutoTypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::VoidTypeName:
					type = make_shared<VoidTypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
				case TokenId::AnyTypeName:
					type = make_shared<AnyTypeNameNode>(token.beginLocation, lexer->getTokenIndex(token));
					break;
			}
			lexer->nextToken();

			break;
		}
		case TokenId::Id: {
			LexerContext savedContext = lexer->context;
			auto ref = parseRef();
			if (!isCompleteRef(ref)) {
				lexer->context = savedContext;
				return make_shared<BadTypeNameNode>(
					ref[0].loc,
					ref[0].idxToken,
					lexer->context.curIndex);
			}
			type = make_shared<CustomTypeNameNode>(token.beginLocation, ref, compiler, curScope.get());
			break;
		}
		default:
			compiler->messages.push_back(
				Message(
					token.beginLocation,
					MessageType::Error,
					"Expecting a type name"));
			return make_shared<BadTypeNameNode>(
				token.beginLocation,
				lexer->getTokenIndex(token),
				lexer->getTokenIndex(token));
	}

	if (auto &token = lexer->peekToken(); token.tokenId == TokenId::LBracket) {
		lexer->nextToken();

		LexerContext savedContext = lexer->context;

		auto valueType = parseTypeName();

		type = make_shared<MapTypeNameNode>(type, valueType);

		if (!type)
			return {};

		expectToken(TokenId::RBracket);
	}

	if (auto &token = lexer->peekToken(); token.tokenId == TokenId::AndOp) {
		lexer->nextToken();

		type = make_shared<RefTypeNameNode>(type);
	}

	return type;
}

deque<shared_ptr<TypeNameNode>> Parser::parseGenericArgs() {
	LexerContext savedContext = lexer->context;
	deque<shared_ptr<TypeNameNode>> genericArgs;

	if (auto &token = lexer->nextToken(); token.tokenId != TokenId::LtOp)
		goto fail;

	while (true) {
		if (auto &token = lexer->peekToken(); token.tokenId == TokenId::GtOp)
			break;

		if (auto type = parseTypeName(); type)
			genericArgs.push_back(type);
		else
			goto fail;

		if (auto &token = lexer->peekToken(); token.tokenId != TokenId::Comma)
			break;

		lexer->nextToken();
	}

	if (auto &token = lexer->nextToken(); token.tokenId != TokenId::GtOp)
		goto fail;

	return genericArgs;

fail:
	lexer->context = savedContext;
	return {};
}

Ref Parser::parseModuleRef() {
	Ref ref;

	while (true) {
		auto &nameToken = expectToken(TokenId::Id);
		auto nameTokenIndex = lexer->getTokenIndex(nameToken);

		ref.push_back(RefEntry(nameToken.beginLocation, nameTokenIndex, nameToken.text));

		if (auto &token = lexer->peekToken(); token.tokenId != TokenId::Dot)
			break;

		lexer->nextToken();
	}

	return ref;
}

Ref Parser::parseRef() {
	Ref ref;
	size_t idxPrecedingAccessOp = SIZE_MAX;

	switch (auto &token = lexer->peekToken(); token.tokenId) {
		case TokenId::ThisKeyword:
		case TokenId::BaseKeyword:
		case TokenId::ScopeOp: {
			auto refEntry = RefEntry(token.beginLocation, lexer->getTokenIndex(token), "", {});

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
		auto nameTokenIndex = lexer->getTokenIndex(nameToken);

		auto refEntry = RefEntry(nameToken.beginLocation, SIZE_MAX, "");
		refEntry.idxAccessOpToken = idxPrecedingAccessOp;

		if (nameToken.tokenId != TokenId::Id) {
			ref.push_back(refEntry);
			return ref;
		} else {
			lexer->nextToken();
			refEntry.name = nameToken.text;
			refEntry.idxToken = nameTokenIndex;
			ref.push_back(refEntry);
		}

		ref.back().genericArgs = parseGenericArgs();

		if (auto &token = lexer->peekToken(); token.tokenId != TokenId::Dot)
			break;

		const auto &precedingAccessOpToken = lexer->nextToken();
		idxPrecedingAccessOp = lexer->getTokenIndex(precedingAccessOpToken);
	}

end:
	return ref;
}

deque<shared_ptr<ExprNode>> Parser::parseArgs() {
	deque<shared_ptr<ExprNode>> args;

	while (true) {
		if (lexer->peekToken().tokenId == TokenId::RParenthese) {
			break;
		}

		args.push_back(parseExpr());

		if (lexer->peekToken().tokenId != TokenId::Comma) {
			break;
		}

		lexer->nextToken();
	}

	return args;
}

shared_ptr<ExprNode> Parser::parseExpr(int precedence) {
	LexerContext savedContext = lexer->context;

	std::shared_ptr<ExprNode> lhs, rhs;

	const Token &prefixToken = expectToken(lexer->peekToken());

	if (auto it = prefixOpRegistries.find(prefixToken.tokenId); it != prefixOpRegistries.end()) {
		lexer->nextToken();

		lhs = it->second.parselet(this, parseExpr(it->second.leftPrecedence));
	} else {
		switch (prefixToken.tokenId) {
			case TokenId::ThisKeyword:
			case TokenId::BaseKeyword:
			case TokenId::ScopeOp:
			case TokenId::Id:
				lhs = static_pointer_cast<ExprNode>(
					make_shared<RefExprNode>(
						parseRef()));
				break;
			case TokenId::LParenthese:
				lexer->nextToken();
				parseExpr(precedence);
				expectToken(TokenId::RParenthese);
				break;
			case TokenId::NewKeyword: {
				lexer->nextToken();

				auto typeName = parseTypeName();

				expectToken(TokenId::LParenthese);
				auto args = parseArgs();
				expectToken(TokenId::RParenthese);

				lhs = make_shared<NewExprNode>(prefixToken.beginLocation, typeName, args);

				break;
			}
			case TokenId::IntLiteral:
				lhs = static_pointer_cast<ExprNode>(
					make_shared<I32LiteralExprNode>(
						prefixToken.beginLocation,
						((IntLiteralTokenExtension *)prefixToken.exData.get())->data,
						lexer->getTokenIndex(prefixToken)));
				lexer->nextToken();
				break;
			case TokenId::LongLiteral:
				lhs = static_pointer_cast<ExprNode>(
					make_shared<I64LiteralExprNode>(
						prefixToken.beginLocation,
						((LongLiteralTokenExtension *)prefixToken.exData.get())->data,
						lexer->getTokenIndex(prefixToken)));
				lexer->nextToken();
				break;
			case TokenId::UIntLiteral:
				lhs = static_pointer_cast<ExprNode>(
					make_shared<U32LiteralExprNode>(
						prefixToken.beginLocation,
						((UIntLiteralTokenExtension *)prefixToken.exData.get())->data,
						lexer->getTokenIndex(prefixToken)));
				lexer->nextToken();
				break;
			case TokenId::ULongLiteral:
				lhs = static_pointer_cast<ExprNode>(
					make_shared<U64LiteralExprNode>(
						prefixToken.beginLocation,
						((ULongLiteralTokenExtension *)prefixToken.exData.get())->data,
						lexer->getTokenIndex(prefixToken)));
				lexer->nextToken();
				break;
			case TokenId::StringLiteral:
				lhs = static_pointer_cast<ExprNode>(
					make_shared<StringLiteralExprNode>(
						prefixToken.beginLocation,
						((StringLiteralTokenExtension *)prefixToken.exData.get())->data,
						lexer->getTokenIndex(prefixToken)));
				lexer->nextToken();
				break;
			case TokenId::F32Literal:
				lhs = static_pointer_cast<ExprNode>(
					make_shared<F32LiteralExprNode>(
						prefixToken.beginLocation,
						((F32LiteralTokenExtension *)prefixToken.exData.get())->data,
						lexer->getTokenIndex(prefixToken)));
				lexer->nextToken();
				break;
			case TokenId::F64Literal:
				lhs = static_pointer_cast<ExprNode>(
					make_shared<F64LiteralExprNode>(
						prefixToken.beginLocation,
						((F64LiteralTokenExtension *)prefixToken.exData.get())->data,
						lexer->getTokenIndex(prefixToken)));
				lexer->nextToken();
				break;
			case TokenId::TrueKeyword:
				lhs = static_pointer_cast<ExprNode>(
					make_shared<BoolLiteralExprNode>(
						prefixToken.beginLocation,
						true,
						lexer->getTokenIndex(prefixToken)));
				lexer->nextToken();
				break;
			case TokenId::FalseKeyword:
				lhs = static_pointer_cast<ExprNode>(
					make_shared<BoolLiteralExprNode>(
						prefixToken.beginLocation,
						false,
						lexer->getTokenIndex(prefixToken)));
				lexer->nextToken();
				break;
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

			lhs = it->second.parselet(this, lhs);
		} else
			break;
	}

	return lhs;
}

void Parser::parseParentSlot(
	shared_ptr<TypeNameNode> &typeNameOut,
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
	deque<shared_ptr<TypeNameNode>> &implInterfacesOut,
	size_t &idxColonTokenOut,
	deque<size_t> &idxCommaTokensOut) {
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

deque<shared_ptr<TypeNameNode>> Parser::parseTraitList() {
	deque<shared_ptr<TypeNameNode>> inheritedTraits;

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

void Parser::parseVarDefs(shared_ptr<VarDefStmtNode> varDefStmtOut) {
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

shared_ptr<StmtNode> Parser::parseStmt() {
	LexerContext beginContext = lexer->context;
	shared_ptr<StmtNode> result;

	try {
		switch (auto &beginToken = lexer->peekToken(); beginToken.tokenId) {
			case TokenId::BreakKeyword: {
				auto stmt = make_shared<BreakStmtNode>(beginToken.beginLocation);
				result = static_pointer_cast<StmtNode>(stmt);

				const auto &idxBreakToken = lexer->nextToken();
				stmt->idxBreakToken = lexer->getTokenIndex(idxBreakToken);

				const auto &semicolonToken = expectToken(TokenId::Semicolon);
				stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);
				break;
			}
			case TokenId::ContinueKeyword: {
				auto stmt = make_shared<BreakStmtNode>(beginToken.beginLocation);
				result = static_pointer_cast<StmtNode>(stmt);

				const auto &idxBreakToken = lexer->nextToken();
				stmt->idxBreakToken = lexer->getTokenIndex(idxBreakToken);

				const auto &semicolonToken = expectToken(TokenId::Semicolon);
				stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);

				break;
			}
			case TokenId::ForKeyword: {
				auto stmt = make_shared<ForStmtNode>(beginToken.beginLocation);
				result = static_pointer_cast<StmtNode>(stmt);

				const auto &forToken = lexer->nextToken();
				stmt->idxForToken = lexer->getTokenIndex(forToken);

				const auto &lParentheseToken = expectToken(TokenId::LParenthese);
				stmt->idxLParentheseToken = lexer->getTokenIndex(lParentheseToken);

				if (const auto &letToken = lexer->peekToken(); letToken.tokenId == TokenId::LetKeyword) {
					stmt->varDefs = make_shared<VarDefStmtNode>(letToken.beginLocation);
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
				auto stmt = make_shared<WhileStmtNode>(beginToken.beginLocation);
				result = static_pointer_cast<StmtNode>(stmt);

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
				auto stmt = make_shared<ReturnStmtNode>(beginToken.beginLocation);
				result = static_pointer_cast<StmtNode>(stmt);

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
				auto stmt = make_shared<YieldStmtNode>(beginToken.beginLocation);
				result = static_pointer_cast<StmtNode>(stmt);

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
				auto stmt = make_shared<IfStmtNode>(beginToken.beginLocation);
				result = static_pointer_cast<StmtNode>(stmt);

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
				auto stmt = make_shared<TryStmtNode>(beginToken.beginLocation);
				result = static_pointer_cast<StmtNode>(stmt);

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

				const auto stmt = make_shared<SwitchStmtNode>(beginToken.beginLocation);
				result = static_pointer_cast<StmtNode>(stmt);

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

					deque<shared_ptr<StmtNode>> body;

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
				const auto stmt = make_shared<CodeBlockStmtNode>(CodeBlock{ beginToken.beginLocation, {} });
				result = static_pointer_cast<StmtNode>(stmt);

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

				auto stmt = make_shared<VarDefStmtNode>(letToken.beginLocation);
				result = static_pointer_cast<StmtNode>(stmt);

				stmt->idxLetToken = lexer->getTokenIndex(letToken);

				parseVarDefs(stmt);

				const auto &semicolonToken = expectToken(TokenId::Semicolon);
				stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);

				break;
			}
			default: {
				auto stmt = make_shared<ExprStmtNode>();
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
		return make_shared<BadStmtNode>(e.location, result);
	}

	return result;
}

deque<shared_ptr<ParamNode>> Parser::parseParams() {
	deque<shared_ptr<ParamNode>> params;

	while (true) {
		if (lexer->peekToken().tokenId == TokenId::RParenthese)
			break;

		if (auto &token = lexer->peekToken(); token.tokenId == TokenId::VarArg) {
			params.push_back(
				make_shared<ParamNode>(token.beginLocation,
					make_shared<ArrayTypeNameNode>(
						make_shared<AnyTypeNameNode>(
							token.beginLocation,
							lexer->getTokenIndex(token))),
					"...",
					lexer->getTokenIndex(token)));
			lexer->nextToken();
			break;
		}

		auto type = parseTypeName();
		auto &nameToken = expectToken(TokenId::Id);
		size_t idxNameToken = lexer->getTokenIndex(nameToken);

		params.push_back(make_shared<ParamNode>(type->getLocation(), type, nameToken.text, idxNameToken));

		if (lexer->peekToken().tokenId != TokenId::Comma)
			break;

		lexer->nextToken();
	}

	return params;
}

shared_ptr<FnOverloadingNode> Parser::parseFnDecl(string &nameOut) {
	auto &fnKeywordToken = expectToken(TokenId::FnKeyword);

	auto &nameToken = expectToken(TokenId::Id);
	size_t idxNameToken = lexer->getTokenIndex(nameToken);

	auto savedScope = curScope;
	auto newScope = (curScope = make_shared<Scope>());

	GenericParamNodeList genericParams;

	if (lexer->peekToken().tokenId == TokenId::LtOp)
		genericParams = parseGenericParams();

	expectToken(TokenId::LParenthese);
	auto params = parseParams();
	expectToken(TokenId::RParenthese);

	nameOut = nameToken.text;

	shared_ptr<TypeNameNode> returnType;

	if (auto &token = lexer->peekToken(); token.tokenId == TokenId::Colon) {
		lexer->nextToken();

#if SLKC_WITH_LANGUAGE_SERVER
		// Update corresponding semantic information.
		auto &tokenInfo = compiler->tokenInfos[lexer->getTokenIndex(token)];
		tokenInfo.tokenContext.curScope = curScope;
		tokenInfo.completionContext = CompletionContext::Type;
		tokenInfo.semanticInfo.isTopLevelRef = true;
#endif

		returnType = parseTypeName();
	}

	auto overloading = make_shared<FnOverloadingNode>(fnKeywordToken.beginLocation, compiler, returnType, genericParams, params, newScope, idxNameToken);

	curScope->parent = savedScope.get();
	curScope = savedScope;

	return overloading;
}

shared_ptr<FnOverloadingNode> Parser::parseFnDef(string &nameOut) {
	shared_ptr<FnOverloadingNode> overloading = parseFnDecl(nameOut);

	if (auto &token = lexer->peekToken(); token.tokenId == TokenId::LBrace) {
		auto savedScope = curScope;
		curScope = overloading->scope;

		lexer->nextToken();

		deque<shared_ptr<StmtNode>> stmts;

		while (true) {
			if (lexer->peekToken().tokenId == TokenId::RBrace) {
				lexer->nextToken();
				break;
			}

			stmts.push_back(parseStmt());
		}

		overloading->body = make_shared<BlockStmtNode>(
			token.beginLocation,
			stmts);

		curScope = savedScope;
	} else
		expectToken(TokenId::Semicolon);

	return overloading;
}

shared_ptr<FnOverloadingNode> Parser::parseOperatorDecl(string &nameOut) {
	auto &operatorKeywordToken = expectToken(TokenId::OperatorKeyword);

	auto savedScope = curScope;
	auto newScope = (curScope = make_shared<Scope>());

	auto &nameToken = lexer->nextToken();
	size_t idxNameToken = lexer->getTokenIndex(nameToken);
	string name;
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
		case TokenId::RevOp:
		case TokenId::NotOp:
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
			name = nameToken.text;
			break;
		case TokenId::LBracket:
			name = "[]";
			expectToken(TokenId::RBracket);
			break;
		case TokenId::LParenthese:
			name = "()";
			expectToken(TokenId::RParenthese);
			break;
		default:
			throw SyntaxError("Unrecognized operator", nameToken.beginLocation);
	}

	expectToken(TokenId::LParenthese);
	auto params = parseParams();
	expectToken(TokenId::RParenthese);

	shared_ptr<TypeNameNode> returnType;

	if (auto &token = lexer->peekToken(); token.tokenId == TokenId::Colon) {
		lexer->nextToken();

		returnType = parseTypeName();
	}

	nameOut = name;

	curScope = savedScope;

	return make_shared<FnOverloadingNode>(operatorKeywordToken.beginLocation, compiler, returnType, GenericParamNodeList{}, params, newScope, idxNameToken);
}

shared_ptr<FnOverloadingNode> Parser::parseOperatorDef(string &nameOut) {
	shared_ptr<FnOverloadingNode> overloading = parseOperatorDecl(nameOut);

	if (auto &token = lexer->peekToken(); token.tokenId == TokenId::LBrace) {
		auto savedScope = curScope;
		curScope = overloading->scope;

		lexer->nextToken();

		deque<shared_ptr<StmtNode>> stmts;

		while (true) {
			if (lexer->peekToken().tokenId == TokenId::RBrace) {
				lexer->nextToken();
				break;
			}

			stmts.push_back(parseStmt());
		}

		curScope = savedScope;

		overloading->body = make_shared<BlockStmtNode>(
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

		auto param = make_shared<GenericParamNode>(
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

shared_ptr<ClassNode> Parser::parseClassDef() {
	auto &beginToken = expectTokens(lexer->nextToken(), TokenId::ClassKeyword);
	auto &nameToken = expectToken(TokenId::Id);

	shared_ptr<ClassNode> classNode = make_shared<ClassNode>(
		beginToken.beginLocation,
		compiler,
		nameToken.text);
	classNode->idxNameToken = lexer->getTokenIndex(nameToken);

	auto savedScope = curScope;
	try {
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

		curScope = classNode->scope;
		curScope->parent = savedScope.get();

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
			_putDefinition(
				def->getLocation(),
				def->name,
				def);
			break;
		}
		case TokenId::OperatorKeyword: {
			string name;

			auto overloading = parseOperatorDef(name);
			_putFnDefinition(token.beginLocation, name, overloading);
			break;
		}
		case TokenId::FnKeyword: {
			string name;
			auto overloading = parseFnDef(name);
			_putFnDefinition(token.beginLocation, name, overloading);

			break;
		}
		case TokenId::LetKeyword: {
			const auto &letToken = lexer->nextToken();

			auto stmt = make_shared<VarDefStmtNode>(letToken.beginLocation);

			stmt->idxLetToken = lexer->getTokenIndex(letToken);

			parseVarDefs(stmt);

			const auto &semicolonToken = expectToken(TokenId::Semicolon);
			stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);

			for (auto &i : stmt->varDefs) {
				_putDefinition(
					i.second.loc,
					i.first,
					make_shared<VarNode>(
						i.second.loc,
						compiler,
						0,
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

shared_ptr<InterfaceNode> Parser::parseInterfaceDef() {
	auto &beginToken = expectTokens(lexer->nextToken(), TokenId::InterfaceKeyword);
	auto &nameToken = expectToken(TokenId::Id);

	shared_ptr<InterfaceNode> interfaceNode = make_shared<InterfaceNode>(
		beginToken.beginLocation,
		nameToken.text);

	interfaceNode->idxNameToken = lexer->getTokenIndex(nameToken);

	auto savedScope = curScope;
	try {
		GenericParamNodeList genericParams;

		if (auto &token = lexer->peekToken(); token.tokenId == TokenId::LtOp) {
			interfaceNode->setGenericParams(parseGenericParams());
		}

		parseImplList(
			interfaceNode->parentInterfaces,
			interfaceNode->idxImplInterfacesColonToken,
			interfaceNode->idxImplInterfacesCommaTokens);

		curScope = interfaceNode->scope;
		curScope->parent = savedScope.get();

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
			_putDefinition(
				def->getLocation(),
				def->name,
				def);
			break;
		}
		case TokenId::OperatorKeyword: {
			string name;

			auto overloading = parseOperatorDef(name);
			_putFnDefinition(token.beginLocation, name, overloading);
			break;
		}
		case TokenId::FnKeyword: {
			string name;
			auto overloading = parseFnDef(name);
			_putFnDefinition(token.beginLocation, name, overloading);

			break;
		}
		case TokenId::LetKeyword: {
			const auto &letToken = lexer->nextToken();

			auto stmt = make_shared<VarDefStmtNode>(letToken.beginLocation);

			stmt->idxLetToken = lexer->getTokenIndex(letToken);

			parseVarDefs(stmt);

			const auto &semicolonToken = expectToken(TokenId::Semicolon);
			stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);

			for (auto &i : stmt->varDefs) {
				_putDefinition(
					i.second.loc,
					i.first,
					make_shared<VarNode>(
						i.second.loc,
						compiler,
						0,
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

shared_ptr<TraitNode> Parser::parseTraitDef() {
	auto &beginToken = expectToken(TokenId::TraitKeyword),
		 &nameToken = expectToken(TokenId::Id);

	shared_ptr<TraitNode> traitNode = make_shared<TraitNode>(beginToken.beginLocation, nameToken.text);
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
			_putDefinition(
				def->getLocation(),
				def->name,
				def);
			break;
		}
		case TokenId::FnKeyword: {
			string name;
			auto overloading = parseFnDef(name);
			_putFnDefinition(token.beginLocation, name, overloading);

			break;
		}
		case TokenId::LetKeyword: {
			const auto &letToken = lexer->nextToken();

			auto stmt = make_shared<VarDefStmtNode>(letToken.beginLocation);

			stmt->idxLetToken = lexer->getTokenIndex(letToken);

			parseVarDefs(stmt);

			const auto &semicolonToken = expectToken(TokenId::Semicolon);
			stmt->idxSemicolonToken = lexer->getTokenIndex(semicolonToken);

			for (auto &i : stmt->varDefs) {
				_putDefinition(
					i.second.loc,
					i.first,
					make_shared<VarNode>(
						i.second.loc,
						compiler,
						0,
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

void Parser::parseModuleDecl() {
	if (auto &beginToken = lexer->peekToken(); beginToken.tokenId == TokenId::ModuleKeyword) {
		lexer->nextToken();

		curModule->moduleName = parseModuleRef();

		expectToken(TokenId::Semicolon);
	}
}

void Parser::parseImportList() {
	if (auto &beginToken = lexer->peekToken(); beginToken.tokenId == TokenId::UseKeyword) {
		lexer->nextToken();

		expectToken(TokenId::LBrace);

		while (true) {
			auto &nameToken = expectToken(TokenId::Id);
			size_t idxNameToken = lexer->getTokenIndex(nameToken);

			expectToken(TokenId::AssignOp);

			auto ref = parseModuleRef();

			curModule->imports[nameToken.text] = { ref, idxNameToken };

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

	compiler->_targetModule = (curModule = make_shared<ModuleNode>(compiler, Location()));
	curScope = curModule->scope;

	parseModuleDecl();
	parseImportList();

	while (lexer->peekToken().tokenId != TokenId::End) {
		parseProgramStmt();
	}
}
