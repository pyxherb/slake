#include <slkc/compiler/compiler.h>

using namespace slake;
using namespace slake::slkc;

void Parser::_putDefinition(
	std::string name,
	std::shared_ptr<MemberNode> member) {
	if (curScope->members.count(name)) {
		compiler->pushMessage(
			compiler->curDocName,
			Message(
				compiler->tokenRangeToSourceLocation(member->tokenRange),
				MessageType::Error,
				"Redefinition of `" + name + "'"));
		return;
	}
	curScope->members[name] = member;
	member->parent = (MemberNode *)curScope->owner;
}

void Parser::_putFnDefinition(
	std::string name,
	std::shared_ptr<FnOverloadingNode> overloading) {
	if (!curScope->members.count(name)) {
		curScope->members[name] = std::make_shared<FnNode>(compiler, name);
		curScope->members[name]->parent = (MemberNode *)curScope->owner;
	}

	if (curScope->members.at(name)->getNodeType() != NodeType::Fn) {
		compiler->pushMessage(
			compiler->curDocName,
			Message(
				compiler->tokenRangeToSourceLocation(overloading->tokenRange),
				MessageType::Error,
				"Redefinition of `" + name + "'"));
		return;
	} else {
		auto fn = std::static_pointer_cast<FnNode>(curScope->members.at(name));
		fn->overloadingRegistries.push_back(overloading);
		overloading->owner = fn.get();
	}
}

AccessModifier Parser::parseAccessModifier(TokenRange &tokenRangeOut, std::deque<size_t> idxAccessModifierTokensOut) {
	AccessModifier accessModifier = 0;

	while (true) {
		Token *token = lexer->peekToken();

		if (!accessModifier)
			tokenRangeOut = { curDoc, lexer->getTokenIndex(token) };

		switch (token->tokenId) {
			case TokenId::PubKeyword:
			case TokenId::FinalKeyword:
			case TokenId::OverrideKeyword:
			case TokenId::StaticKeyword:
			case TokenId::NativeKeyword:
				lexer->nextToken();
				tokenRangeOut.endIndex = lexer->getTokenIndex(token);
				switch (token->tokenId) {
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
					default:
						std::terminate();
				}
				idxAccessModifierTokensOut.push_back(lexer->getTokenIndex(token));
				break;
			default:
				goto end;
		}
	}

end:
	return accessModifier;
}

void Parser::splitRshOpToken() {
	switch (Token *token = lexer->peekToken(); token->tokenId) {
		case TokenId::RshOp: {
			token->tokenId = TokenId::GtOp;
			token->text = ">";
			token->location.endPosition.column -= 1;

			std::unique_ptr<Token> extraClosingToken = std::make_unique<Token>();
			*(extraClosingToken.get()) = {
				SIZE_MAX,
				TokenId::GtOp,
				SourceLocation{
					SourcePosition{ token->location.beginPosition.line, token->location.beginPosition.column + 1 },
					token->location.endPosition },
				">",
				nullptr
			};

			lexer->tokens.insert(std::next(lexer->tokens.begin(), lexer->context.curIndex + 1), std::move(extraClosingToken));

			break;
		}
		default:;
	}
}
