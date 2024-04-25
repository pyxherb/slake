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
			compiler->tokenInfos[lexer->context.curIndex].semanticType = SemanticType::Type;

			switch (token.tokenId) {
				case TokenId::I8TypeName:
					type = make_shared<I8TypeNameNode>(token.beginLocation, lexer->context.curIndex);
					break;
				case TokenId::I16TypeName:
					type = make_shared<I16TypeNameNode>(token.beginLocation, lexer->context.curIndex);
					break;
				case TokenId::I32TypeName:
					type = make_shared<I32TypeNameNode>(token.beginLocation, lexer->context.curIndex);
					break;
				case TokenId::I64TypeName:
					type = make_shared<I64TypeNameNode>(token.beginLocation, lexer->context.curIndex);
					break;
				case TokenId::U8TypeName:
					type = make_shared<U8TypeNameNode>(token.beginLocation, lexer->context.curIndex);
					break;
				case TokenId::U16TypeName:
					type = make_shared<U16TypeNameNode>(token.beginLocation, lexer->context.curIndex);
					break;
				case TokenId::U32TypeName:
					type = make_shared<U32TypeNameNode>(token.beginLocation, lexer->context.curIndex);
					break;
				case TokenId::U64TypeName:
					type = make_shared<U64TypeNameNode>(token.beginLocation, lexer->context.curIndex);
					break;
				case TokenId::F32TypeName:
					type = make_shared<F32TypeNameNode>(token.beginLocation, lexer->context.curIndex);
					break;
				case TokenId::F64TypeName:
					type = make_shared<F64TypeNameNode>(token.beginLocation, lexer->context.curIndex);
					break;
				case TokenId::StringTypeName:
					type = make_shared<StringTypeNameNode>(token.beginLocation, lexer->context.curIndex);
					break;
				case TokenId::BoolTypeName:
					type = make_shared<BoolTypeNameNode>(token.beginLocation, lexer->context.curIndex);
					break;
				case TokenId::AutoTypeName:
					type = make_shared<AutoTypeNameNode>(token.beginLocation, lexer->context.curIndex);
					break;
				case TokenId::VoidTypeName:
					type = make_shared<VoidTypeNameNode>(token.beginLocation, lexer->context.curIndex);
					break;
				case TokenId::AnyTypeName:
					type = make_shared<AnyTypeNameNode>(token.beginLocation, lexer->context.curIndex);
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
				return {};
			}
			type = make_shared<CustomTypeNameNode>(token.beginLocation, ref, compiler, curScope.get());
			break;
		}
		default:
			return {};
	}

	if (auto &token = lexer->peekToken(); token.tokenId == TokenId::LBracket) {
		lexer->nextToken();

		LexerContext savedContext = lexer->context;

		auto valueType = parseTypeName();

		type = make_shared<MapTypeNameNode>(type, valueType);

		if (!type)
			return {};

		expectToken(lexer->nextToken(), TokenId::RBracket);
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
		auto nameTokenIndex = lexer->context.curIndex;
		auto &nameToken = expectToken(lexer->nextToken(), TokenId::Id);

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
			auto refEntry = RefEntry(token.beginLocation, lexer->context.curIndex, "", {});

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

			idxPrecedingAccessOp = lexer->context.curIndex;
			lexer->nextToken();

			break;
		}
	}

	while (true) {
		auto nameTokenIndex = lexer->context.curIndex;
		auto &nameToken = lexer->peekToken();

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

		idxPrecedingAccessOp = lexer->context.curIndex;

		lexer->nextToken();
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
				expectToken(lexer->nextToken(), TokenId::RParenthese);
				break;
			case TokenId::NewKeyword: {
				lexer->nextToken();

				auto typeName = parseTypeName();

				expectToken(lexer->nextToken(), TokenId::LParenthese);
				auto args = parseArgs();
				expectToken(lexer->nextToken(), TokenId::RParenthese);

				lhs = make_shared<NewExprNode>(prefixToken.beginLocation, typeName, args);

				break;
			}
			case TokenId::IntLiteral:
				lexer->nextToken();

				lhs = static_pointer_cast<ExprNode>(
					make_shared<I32LiteralExprNode>(
						prefixToken.beginLocation,
						((IntLiteralTokenExtension *)prefixToken.exData.get())->data));
				break;
			case TokenId::LongLiteral:
				lexer->nextToken();

				lhs = static_pointer_cast<ExprNode>(
					make_shared<I64LiteralExprNode>(
						prefixToken.beginLocation,
						((LongLiteralTokenExtension *)prefixToken.exData.get())->data));
				break;
			case TokenId::UIntLiteral:
				lexer->nextToken();

				lhs = static_pointer_cast<ExprNode>(
					make_shared<U32LiteralExprNode>(
						prefixToken.beginLocation,
						((UIntLiteralTokenExtension *)prefixToken.exData.get())->data));
				break;
			case TokenId::ULongLiteral:
				lexer->nextToken();

				lhs = static_pointer_cast<ExprNode>(
					make_shared<U64LiteralExprNode>(
						prefixToken.beginLocation,
						((ULongLiteralTokenExtension *)prefixToken.exData.get())->data));
				break;
			case TokenId::StringLiteral:
				lexer->nextToken();

				lhs = static_pointer_cast<ExprNode>(
					make_shared<StringLiteralExprNode>(
						prefixToken.beginLocation,
						((StringLiteralTokenExtension *)prefixToken.exData.get())->data));
				break;
			case TokenId::F32Literal:
				lexer->nextToken();

				lhs = static_pointer_cast<ExprNode>(
					make_shared<F32LiteralExprNode>(
						prefixToken.beginLocation,
						((F32LiteralTokenExtension *)prefixToken.exData.get())->data));
				break;
			case TokenId::F64Literal:
				lexer->nextToken();

				lhs = static_pointer_cast<ExprNode>(
					make_shared<F64LiteralExprNode>(
						prefixToken.beginLocation,
						((F64LiteralTokenExtension *)prefixToken.exData.get())->data));
				break;
			case TokenId::TrueKeyword:
				lexer->nextToken();

				lhs = static_pointer_cast<ExprNode>(
					make_shared<BoolLiteralExprNode>(
						prefixToken.beginLocation,
						true));
				break;
			case TokenId::FalseKeyword:
				lexer->nextToken();

				lhs = static_pointer_cast<ExprNode>(
					make_shared<BoolLiteralExprNode>(
						prefixToken.beginLocation,
						false));
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

shared_ptr<TypeNameNode> Parser::parseParentSlot() {
	shared_ptr<TypeNameNode> baseClass;

	if (lexer->peekToken().tokenId == TokenId::LParenthese) {
		lexer->nextToken();

		baseClass = parseTypeName();

		expectToken(lexer->nextToken(), TokenId::RParenthese);
	}

	return baseClass;
}

deque<shared_ptr<TypeNameNode>> Parser::parseImplList() {
	deque<shared_ptr<TypeNameNode>> implInterfaces;

	if (lexer->peekToken().tokenId == TokenId::Colon) {
		lexer->nextToken();

		while (true) {
			implInterfaces.push_back(parseTypeName());

			if (lexer->peekToken().tokenId != TokenId::Comma)
				break;

			lexer->nextToken();
		}
	}

	return implInterfaces;
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

		expectToken(lexer->nextToken(), TokenId::RBracket);
	}

	return inheritedTraits;
}

shared_ptr<VarDefStmtNode> Parser::parseVarDefs(shared_ptr<TypeNameNode> type) {
	shared_ptr<VarDefStmtNode> varDefStmt = make_shared<VarDefStmtNode>(type->getLocation(), type);

	while (true) {
		auto &nameToken = expectToken(lexer->nextToken(), TokenId::Id);
		shared_ptr<ExprNode> initValue;

		if (auto &token = lexer->peekToken(); token.tokenId == TokenId::AssignOp) {
			lexer->nextToken();
			initValue = parseExpr();
		}

		varDefStmt->varDefs[nameToken.text] = VarDefEntry(nameToken.beginLocation, nameToken.text, initValue);

		if (lexer->peekToken().tokenId != TokenId::Comma)
			break;

		lexer->nextToken();
	}

	return varDefStmt;
}

shared_ptr<StmtNode> Parser::parseStmt() {
	LexerContext beginContext = lexer->context;

	try {
		switch (auto &beginToken = lexer->peekToken(); beginToken.tokenId) {
			case TokenId::BreakKeyword:
				lexer->nextToken();
				expectToken(lexer->nextToken(), TokenId::Semicolon);
				return static_pointer_cast<StmtNode>(
					make_shared<BreakStmtNode>(beginToken.beginLocation));
			case TokenId::ContinueKeyword:
				lexer->nextToken();
				expectToken(lexer->nextToken(), TokenId::Semicolon);
				return static_pointer_cast<StmtNode>(
					make_shared<ContinueStmtNode>(beginToken.beginLocation));
			case TokenId::ForKeyword: {
				lexer->nextToken();

				expectToken(lexer->nextToken(), TokenId::LParenthese);

				auto varDefs = parseVarDefs(parseTypeName());
				expectToken(lexer->nextToken(), TokenId::Semicolon);

				auto condition = parseExpr();
				expectToken(lexer->nextToken(), TokenId::Semicolon);

				auto endExpr = parseExpr();
				expectToken(lexer->nextToken(), TokenId::RParenthese);

				auto body = parseStmt();

				return static_pointer_cast<StmtNode>(
					make_shared<ForStmtNode>(
						beginToken.beginLocation,
						varDefs,
						condition,
						endExpr,
						body));
			}
			case TokenId::WhileKeyword: {
				lexer->nextToken();

				expectToken(lexer->nextToken(), TokenId::LParenthese);
				auto condition = parseExpr();
				expectToken(lexer->nextToken(), TokenId::RParenthese);

				auto body = parseStmt();

				return static_pointer_cast<StmtNode>(
					make_shared<WhileStmtNode>(beginToken.beginLocation, condition, body));
			}
			case TokenId::ReturnKeyword: {
				lexer->nextToken();

				if (auto &token = lexer->peekToken(); token.tokenId == TokenId::Semicolon) {
					lexer->nextToken();
					return static_pointer_cast<StmtNode>(
						make_shared<ReturnStmtNode>(beginToken.beginLocation, shared_ptr<ExprNode>()));
				} else {
					auto stmt = static_pointer_cast<StmtNode>(
						make_shared<ReturnStmtNode>(beginToken.beginLocation, parseExpr()));
					expectToken(lexer->nextToken(), TokenId::Semicolon);
					return stmt;
				}
			}
			case TokenId::YieldKeyword: {
				lexer->nextToken();

				if (auto &token = lexer->peekToken(); token.tokenId == TokenId::Semicolon) {
					lexer->nextToken();
					return static_pointer_cast<StmtNode>(
						make_shared<YieldStmtNode>(beginToken.beginLocation, shared_ptr<ExprNode>()));
				} else {
					auto stmt = static_pointer_cast<StmtNode>(
						make_shared<YieldStmtNode>(beginToken.beginLocation, parseExpr()));
					expectToken(lexer->nextToken(), TokenId::Semicolon);
					return stmt;
				}
			}
			case TokenId::IfKeyword: {
				lexer->nextToken();

				shared_ptr<VarDefStmtNode> varDefs;

				expectToken(lexer->nextToken(), TokenId::LParenthese);

				{
					LexerContext savedContext = lexer->context;
					try {
						varDefs = parseVarDefs(parseTypeName());
						if (varDefs)
							expectToken(lexer->nextToken(), TokenId::Semicolon);
					} catch (SyntaxError e) {
						lexer->context = savedContext;
					}
				}

				auto condition = parseExpr();

				if (!condition)
					throw SyntaxError("Expecting an expression", lexer->tokens[lexer->context.curIndex].beginLocation);

				expectToken(lexer->nextToken(), TokenId::RParenthese);

				shared_ptr<StmtNode> body = parseStmt(), elseBranch;

				if (lexer->peekToken().tokenId == TokenId::ElseKeyword) {
					lexer->nextToken();
					elseBranch = parseStmt();
				}

				return static_pointer_cast<StmtNode>(
					make_shared<IfStmtNode>(
						beginToken.beginLocation,
						varDefs,
						condition,
						body,
						elseBranch));
			}
			case TokenId::TryKeyword: {
				lexer->nextToken();

				auto body = parseStmt();
				deque<CatchBlock> catchBlocks;
				FinalBlock finalBlock;

				while (true) {
					auto &catchToken = lexer->peekToken();
					if (catchToken.tokenId != TokenId::CatchKeyword)
						break;

					lexer->nextToken();

					expectToken(lexer->nextToken(), TokenId::LParenthese);

					auto targetType = parseTypeName();
					std::string exceptionVarName;

					if (auto &nameToken = lexer->peekToken(); nameToken.tokenId == TokenId::Id) {
						lexer->nextToken();
						exceptionVarName = nameToken.text;
					}

					expectToken(lexer->nextToken(), TokenId::RParenthese);

					auto body = parseStmt();

					catchBlocks.push_back({ catchToken.beginLocation,
						targetType,
						exceptionVarName,
						body });
				}

				if (auto &finalToken = lexer->peekToken(); finalToken.tokenId == TokenId::FinalKeyword) {
					lexer->nextToken();
					finalBlock = { finalToken.beginLocation, parseStmt() };
				}

				return static_pointer_cast<StmtNode>(
					make_shared<TryStmtNode>(
						beginToken.beginLocation,
						body,
						catchBlocks,
						finalBlock));
			}
			case TokenId::SwitchKeyword: {
				lexer->nextToken();

				expectToken(lexer->nextToken(), TokenId::LParenthese);

				shared_ptr<ExprNode> expr = parseExpr();
				deque<SwitchCase> cases;

				expectToken(lexer->nextToken(), TokenId::RParenthese);

				expectToken(lexer->nextToken(), TokenId::LBrace);

				while (true) {
					auto &caseToken = lexer->peekToken();

					if (caseToken.tokenId != TokenId::CaseKeyword)
						break;

					lexer->nextToken();

					shared_ptr<ExprNode> condition = parseExpr();
					deque<shared_ptr<StmtNode>> body;

					while (true) {
						if (auto &token = lexer->peekToken();
							(token.tokenId == TokenId::DefaultKeyword) ||
							(token.tokenId == TokenId::RBrace))
							break;
						body.push_back(parseStmt());
					}

					cases.push_back({ caseToken.beginLocation, body, condition });
				}

				if (auto &defaultToken = lexer->peekToken(); defaultToken.tokenId == TokenId::DefaultKeyword) {
					lexer->nextToken();

					deque<shared_ptr<StmtNode>> body;

					while (true) {
						if (auto &token = lexer->peekToken(); token.tokenId == TokenId::RBrace)
							break;
						body.push_back(parseStmt());
					}

					cases.push_back({ defaultToken.beginLocation, body });
				}

				expectToken(lexer->nextToken(), TokenId::RBrace);

				return static_pointer_cast<StmtNode>(make_shared<SwitchStmtNode>(beginToken.beginLocation, expr, cases));
			}
			case TokenId::LBrace: {
				lexer->nextToken();

				CodeBlock codeBlock = { beginToken.beginLocation, {} };

				while (true) {
					if (lexer->peekToken().tokenId == TokenId::RBrace) {
						lexer->nextToken();
						break;
					}

					codeBlock.stmts.push_back(parseStmt());
				}

				return static_pointer_cast<StmtNode>(make_shared<CodeBlockStmtNode>(codeBlock));
			}
		}

		LexerContext savedContext = lexer->context;

		if (auto type = parseTypeName(); type) {
			try {
				auto varDefStmt = parseVarDefs(type);
				expectToken(lexer->nextToken(), TokenId::Semicolon);
				return varDefStmt;
			} catch (SyntaxError e) {
				lexer->context = savedContext;
			}
		}
		auto expr = parseExpr();
		expectToken(lexer->nextToken(), TokenId::Semicolon);
		return make_shared<ExprStmtNode>(expr);
	} catch (SyntaxError e) {
		compiler->messages.push_back(
			Message(
				e.location,
				MessageType::Error,
				e.what()));
		return make_shared<BadStmtNode>(e.location, beginContext.curIndex, lexer->context.curIndex);
	}
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
							lexer->context.curIndex)),
					"..."));
			lexer->nextToken();
			break;
		}

		auto type = parseTypeName();
		auto &nameToken = expectToken(lexer->nextToken(), TokenId::Id);

		params.push_back(make_shared<ParamNode>(type->getLocation(), type, nameToken.text));

		if (lexer->peekToken().tokenId != TokenId::Comma)
			break;

		lexer->nextToken();
	}

	return params;
}

shared_ptr<FnOverloadingNode> Parser::parseFnDecl(shared_ptr<TypeNameNode> returnType, string &nameOut) {
	auto &nameToken = expectToken(lexer->nextToken(), TokenId::Id);

	auto savedScope = curScope;
	auto newScope = (curScope = make_shared<Scope>());

	GenericParamNodeList genericParams;

	if (lexer->peekToken().tokenId == TokenId::LtOp)
		genericParams = parseGenericParams();

	expectToken(lexer->nextToken(), TokenId::LParenthese);
	auto params = parseParams();
	expectToken(lexer->nextToken(), TokenId::RParenthese);

	nameOut = nameToken.text;

	auto overloading = make_shared<FnOverloadingNode>(returnType->getLocation(), compiler, returnType, genericParams, params, newScope);

	curScope = savedScope;

	return overloading;
}

shared_ptr<FnOverloadingNode> Parser::parseFnDef(shared_ptr<TypeNameNode> returnType, string &nameOut) {
	shared_ptr<FnOverloadingNode> overloading = parseFnDecl(returnType, nameOut);

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
			stmts.empty() ? token.beginLocation : stmts[0]->getLocation(),
			stmts);

		curScope = savedScope;
	} else
		expectToken(lexer->nextToken(), TokenId::Semicolon);

	return overloading;
}

shared_ptr<FnOverloadingNode> Parser::parseOperatorDecl(shared_ptr<TypeNameNode> returnType, string &nameOut) {
	expectToken(lexer->nextToken(), TokenId::OperatorKeyword);

	auto savedScope = curScope;
	auto newScope = (curScope = make_shared<Scope>());

	auto &nameToken = lexer->nextToken();
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
			name = nameToken.text;
			break;
		case TokenId::LBracket:
			name = "[]";
			expectToken(lexer->nextToken(), TokenId::RBracket);
			break;
		case TokenId::LParenthese:
			name = "()";
			expectToken(lexer->nextToken(), TokenId::RParenthese);
			break;
		default:
			throw SyntaxError("Unrecognized operator", nameToken.beginLocation);
	}

	expectToken(lexer->nextToken(), TokenId::LParenthese);
	auto params = parseParams();
	expectToken(lexer->nextToken(), TokenId::RParenthese);

	nameOut = name;

	curScope = savedScope;

	return make_shared<FnOverloadingNode>(returnType->getLocation(), compiler, returnType, GenericParamNodeList{}, params, newScope);
}

shared_ptr<FnOverloadingNode> Parser::parseOperatorDef(shared_ptr<TypeNameNode> returnType, string &nameOut) {
	shared_ptr<FnOverloadingNode> overloading = parseOperatorDecl(returnType, nameOut);

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
			stmts.empty() ? token.beginLocation : stmts[0]->getLocation(),
			stmts);
	} else
		expectToken(lexer->nextToken(), TokenId::Semicolon);

	return overloading;
}

shared_ptr<FnOverloadingNode> Parser::parseConstructorDecl() {
	auto &beginToken = expectToken(lexer->nextToken(), TokenId::NewKeyword);

	expectToken(lexer->nextToken(), TokenId::LParenthese);
	auto params = parseParams();
	expectToken(lexer->nextToken(), TokenId::RParenthese);

	return make_shared<FnOverloadingNode>(beginToken.beginLocation, compiler, make_shared<VoidTypeNameNode>(beginToken.beginLocation, SIZE_MAX), GenericParamNodeList{}, params);
}

shared_ptr<FnOverloadingNode> Parser::parseConstructorDef() {
	shared_ptr<FnOverloadingNode> overloading = parseConstructorDecl();

	if (auto &token = lexer->peekToken(); token.tokenId == TokenId::LBrace) {
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
			stmts.empty() ? token.beginLocation : stmts[0]->getLocation(),
			stmts);
	} else
		expectToken(lexer->nextToken(), TokenId::Semicolon);

	return overloading;
}

shared_ptr<FnOverloadingNode> Parser::parseDestructorDecl() {
	auto &beginToken = expectToken(lexer->nextToken(), TokenId::DeleteKeyword);

	expectToken(lexer->nextToken(), TokenId::LParenthese);
	expectToken(lexer->nextToken(), TokenId::RParenthese);

	return make_shared<FnOverloadingNode>(beginToken.beginLocation, compiler, make_shared<VoidTypeNameNode>(beginToken.beginLocation, SIZE_MAX), GenericParamNodeList{}, deque<shared_ptr<ParamNode>>{});
}

shared_ptr<FnOverloadingNode> Parser::parseDestructorDef() {
	shared_ptr<FnOverloadingNode> overloading = parseDestructorDecl();

	if (auto &token = lexer->peekToken(); token.tokenId == TokenId::LBrace) {
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
			stmts.empty() ? token.beginLocation : stmts[0]->getLocation(),
			stmts);
	} else
		expectToken(lexer->nextToken(), TokenId::Semicolon);

	return overloading;
}

GenericParamNodeList Parser::parseGenericParams() {
	GenericParamNodeList genericParams;

	expectToken(lexer->nextToken(), TokenId::LtOp);

	while (true) {
		auto &nameToken = expectToken(lexer->nextToken(), TokenId::Id);

		shared_ptr<TypeNameNode> baseType = parseParentSlot();
		deque<shared_ptr<TypeNameNode>> interfaceTypes = parseImplList();
		// TODO: Parse trait types.

		auto param = make_shared<GenericParamNode>(nameToken.beginLocation, nameToken.text);
		param->baseType = baseType;
		param->interfaceTypes = interfaceTypes;
		// TODO: Add support for trait types.

		genericParams.push_back(param);

		if (auto &token = lexer->peekToken(); token.tokenId != TokenId::Comma)
			break;

		lexer->nextToken();
	}

	expectToken(lexer->nextToken(), TokenId::GtOp);

	// stub
	return genericParams;
}

shared_ptr<ClassNode> Parser::parseClassDef() {
	auto &beginToken = expectTokens(lexer->nextToken(), TokenId::ClassKeyword),
		 &nameToken = expectToken(lexer->nextToken(), TokenId::Id);

	GenericParamNodeList genericParams;

	if (auto &token = lexer->peekToken(); token.tokenId == TokenId::LtOp) {
		genericParams = parseGenericParams();
	}

	shared_ptr<TypeNameNode> baseClass = parseParentSlot();
	deque<shared_ptr<TypeNameNode>> implInterfaces = parseImplList();

	shared_ptr<ClassNode> classNode = make_shared<ClassNode>(
		beginToken.beginLocation,
		compiler,
		nameToken.text,
		baseClass,
		implInterfaces,
		genericParams);

	auto savedScope = curScope;
	curScope = classNode->scope;
	curScope->parent = savedScope.get();

	expectToken(lexer->nextToken(), TokenId::LBrace);

	while (true) {
		if (auto &token = lexer->peekToken();
			token.tokenId == TokenId::RBrace ||
			token.tokenId == TokenId::End)
			break;
		parseClassStmt();
	}

	lexer->nextToken();

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
		case TokenId::OperatorKeyword:
			lexer->nextToken();
			switch (auto &nextToken = lexer->peekToken(); nextToken.tokenId) {
				case TokenId::NewKeyword: {
					auto def = parseConstructorDef();
					_putFnDefinition(def->loc, "new", def);
					break;
				}
				case TokenId::DeleteKeyword: {
					auto def = parseDestructorDef();
					_putFnDefinition(def->loc, "delete", def);
					break;
				}
			}
			break;
		default: {
			if (auto type = parseTypeName(); type) {
				auto savedContext = lexer->context;

				switch (auto &symbol = lexer->peekToken(); symbol.tokenId) {
					case TokenId::Id:
						lexer->nextToken();

						switch (auto &nextSymbol = lexer->peekToken(); nextSymbol.tokenId) {
							case TokenId::AssignOp:
							case TokenId::Semicolon: {
								lexer->context = savedContext;

								auto stmt = parseVarDefs(type);
								expectToken(lexer->nextToken(), TokenId::Semicolon);

								for (auto &i : stmt->varDefs) {
									_putDefinition(i.second.loc, i.first, make_shared<VarNode>(i.second.loc, compiler, 0, stmt->type, i.first, i.second.initValue));
								}
								return;
							}
							default: {
								string name;
								lexer->context = savedContext;

								auto overloading = parseFnDef(type, name);
								_putFnDefinition(symbol.beginLocation, name, overloading);

								return;
							}
						}
						break;
					case TokenId::OperatorKeyword: {
						string name;

						lexer->context = savedContext;
						auto overloading = parseOperatorDef(type, name);
						_putFnDefinition(symbol.beginLocation, name, overloading);
						return;
					}
				}
			}

			throw SyntaxError("Unrecognized token", lexer->tokens[lexer->context.curIndex].beginLocation);
		}
	}
}

shared_ptr<InterfaceNode> Parser::parseInterfaceDef() {
	auto &beginToken = expectTokens(lexer->nextToken(), TokenId::InterfaceKeyword),
		 &nameToken = expectToken(lexer->nextToken(), TokenId::Id);

	GenericParamNodeList genericParams;

	if (auto &token = lexer->peekToken(); token.tokenId == TokenId::LtOp) {
		genericParams = parseGenericParams();
	}

	deque<shared_ptr<TypeNameNode>> parentInterfaces = parseImplList();

	shared_ptr<InterfaceNode> interfaceNode = make_shared<InterfaceNode>(
		beginToken.beginLocation,
		nameToken.text,
		parentInterfaces,
		genericParams);

	auto savedScope = curScope;
	curScope = interfaceNode->scope;
	curScope->parent = savedScope.get();

	expectToken(lexer->nextToken(), TokenId::LBrace);

	while (true) {
		if (auto &token = lexer->peekToken();
			token.tokenId == TokenId::RBrace ||
			token.tokenId == TokenId::End)
			break;
		parseInterfaceStmt();
	}

	lexer->nextToken();

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
		case TokenId::OperatorKeyword:
			lexer->nextToken();
			switch (auto &nextToken = lexer->peekToken(); nextToken.tokenId) {
				case TokenId::NewKeyword: {
					auto def = parseConstructorDef();
					_putFnDefinition(def->loc, "new", def);
					break;
				}
				case TokenId::DeleteKeyword: {
					auto def = parseDestructorDef();
					_putFnDefinition(def->loc, "delete", def);
					break;
				}
			}
			break;
		default: {
			if (auto type = parseTypeName(); type) {
				auto savedContext = lexer->context;

				switch (auto &symbol = lexer->peekToken(); symbol.tokenId) {
					case TokenId::Id:
						lexer->nextToken();

						switch (auto &nextSymbol = lexer->peekToken(); nextSymbol.tokenId) {
							case TokenId::AssignOp:
							case TokenId::Semicolon: {
								lexer->context = savedContext;

								auto stmt = parseVarDefs(type);
								expectToken(lexer->nextToken(), TokenId::Semicolon);

								for (auto &i : stmt->varDefs) {
									_putDefinition(i.second.loc, i.first, make_shared<VarNode>(i.second.loc, compiler, 0, stmt->type, i.first, i.second.initValue));
								}
								return;
							}
							default: {
								string name;
								lexer->context = savedContext;

								auto overloading = parseFnDef(type, name);
								_putFnDefinition(symbol.beginLocation, name, overloading);

								return;
							}
						}
						break;
					case TokenId::OperatorKeyword: {
						string name;

						lexer->context = savedContext;
						auto overloading = parseOperatorDef(type, name);
						_putFnDefinition(symbol.beginLocation, name, overloading);
						return;
					}
				}
			}

			throw SyntaxError("Unrecognized token", lexer->tokens[lexer->context.curIndex].beginLocation);
		}
	}
}

shared_ptr<InterfaceNode> Parser::parseTraitDef() {
	auto &beginToken = expectToken(lexer->nextToken(), TokenId::TraitKeyword),
		 &nameToken = expectToken(lexer->nextToken(), TokenId::Id);

	deque<shared_ptr<TypeNameNode>> inheritedTraits = parseImplList();

	expectToken(lexer->nextToken(), TokenId::LBrace);

	expectToken(lexer->nextToken(), TokenId::RBrace);

	return {};
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
		default: {
			if (auto type = parseTypeName(); type) {
				auto savedContext = lexer->context;

				switch (auto &symbol = lexer->peekToken(); symbol.tokenId) {
					case TokenId::Id:
						lexer->nextToken();

						switch (auto &nextSymbol = lexer->peekToken(); nextSymbol.tokenId) {
							case TokenId::AssignOp:
							case TokenId::Semicolon: {
								lexer->context = savedContext;

								auto stmt = parseVarDefs(type);
								expectToken(lexer->nextToken(), TokenId::Semicolon);

								for (auto &i : stmt->varDefs) {
									_putDefinition(i.second.loc, i.first, make_shared<VarNode>(i.second.loc, compiler, 0, stmt->type, i.first, i.second.initValue));
								}
								return;
							}
							default: {
								string name;
								lexer->context = savedContext;

								auto overloading = parseFnDef(type, name);
								_putFnDefinition(symbol.beginLocation, name, overloading);

								return;
							}
						}
						break;
					case TokenId::OperatorKeyword: {
						string name;

						lexer->context = savedContext;
						auto overloading = parseOperatorDef(type, name);
						_putFnDefinition(symbol.beginLocation, name, overloading);
						return;
					}
				}
			}

			throw SyntaxError("Unrecognized token", lexer->tokens[lexer->context.curIndex].beginLocation);
		}
	}
}

void Parser::parseModuleDecl() {
	if (auto &beginToken = lexer->peekToken(); beginToken.tokenId == TokenId::ModuleKeyword) {
		lexer->nextToken();

		curModule->moduleName = parseModuleRef();

		expectToken(lexer->nextToken(), TokenId::Semicolon);
	}
}

void Parser::parseImportList() {
	if (auto &beginToken = lexer->peekToken(); beginToken.tokenId == TokenId::UseKeyword) {
		lexer->nextToken();

		expectToken(lexer->nextToken(), TokenId::LBrace);

		while (true) {
			auto &nameToken = expectToken(lexer->nextToken(), TokenId::Id);

			expectToken(lexer->nextToken(), TokenId::AssignOp);

			auto ref = parseModuleRef();

			curModule->imports[nameToken.text] = ref;

			if (lexer->peekToken().tokenId != TokenId::Comma)
				break;

			lexer->nextToken();
		}

		expectToken(lexer->nextToken(), TokenId::RBrace);
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
