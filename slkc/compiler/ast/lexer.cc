#include <slkc/compiler/ast/lexer.h>
#include <algorithm>

using namespace slake::slkc;

const char *slake::slkc::getTokenName(slake::slkc::TokenId tokenId) {
	switch (tokenId) {
		case TokenId::End:
			return "end of file";
		case TokenId::Comma:
			return ",";
		case TokenId::Question:
			return "?";
		case TokenId::Colon:
			return ":";
		case TokenId::Semicolon:
			return ";";
		case TokenId::LBracket:
			return "[";
		case TokenId::RBracket:
			return "]";
		case TokenId::LBrace:
			return "{";
		case TokenId::RBrace:
			return "}";
		case TokenId::LParenthese:
			return "(";
		case TokenId::RParenthese:
			return ")";
		case TokenId::At:
			return "@";
		case TokenId::Dot:
			return ".";
		case TokenId::VarArg:
			return "...";
		case TokenId::AddOp:
			return "+";
		case TokenId::SubOp:
			return "-";
		case TokenId::MulOp:
			return "*";
		case TokenId::DivOp:
			return "/";
		case TokenId::ModOp:
			return "%";
		case TokenId::AndOp:
			return "&";
		case TokenId::OrOp:
			return "|";
		case TokenId::XorOp:
			return "^";
		case TokenId::LNotOp:
			return "!";
		case TokenId::NotOp:
			return "~";
		case TokenId::AssignOp:
			return "=";
		case TokenId::AddAssignOp:
			return "+=";
		case TokenId::SubAssignOp:
			return "-=";
		case TokenId::MulAssignOp:
			return "*=";
		case TokenId::DivAssignOp:
			return "/=";
		case TokenId::ModAssignOp:
			return "%=";
		case TokenId::AndAssignOp:
			return "&=";
		case TokenId::OrAssignOp:
			return "|=";
		case TokenId::XorAssignOp:
			return "^=";
		case TokenId::NotAssignOp:
			return "~=";
		case TokenId::LshAssignOp:
			return "<<=";
		case TokenId::RshAssignOp:
			return ">>=";
		case TokenId::StrictEqOp:
			return "===";
		case TokenId::StrictNeqOp:
			return "!==";
		case TokenId::EqOp:
			return "==";
		case TokenId::NeqOp:
			return "!=";
		case TokenId::LshOp:
			return "<<";
		case TokenId::RshOp:
			return ">>";
		case TokenId::LtOp:
			return "<";
		case TokenId::GtOp:
			return ">";
		case TokenId::LtEqOp:
			return "<=";
		case TokenId::GtEqOp:
			return ">=";
		case TokenId::LAndOp:
			return "&&";
		case TokenId::LOrOp:
			return "||";
		case TokenId::IncOp:
			return "++";
		case TokenId::DecOp:
			return "--";
		case TokenId::MatchOp:
			return "=>";
		case TokenId::WrapOp:
			return "->";
		case TokenId::DollarOp:
			return "$";
		case TokenId::AsKeyword:
			return "as";
		case TokenId::AsyncKeyword:
			return "async";
		case TokenId::AwaitKeyword:
			return "await";
		case TokenId::BaseKeyword:
			return "base";
		case TokenId::BreakKeyword:
			return "break";
		case TokenId::CaseKeyword:
			return "case";
		case TokenId::CatchKeyword:
			return "catch";
		case TokenId::ClassKeyword:
			return "class";
		case TokenId::ContinueKeyword:
			return "continue";
		case TokenId::DeleteKeyword:
			return "delete";
		case TokenId::DefaultKeyword:
			return "default";
		case TokenId::ElseKeyword:
			return "else";
		case TokenId::EnumKeyword:
			return "enum";
		case TokenId::FalseKeyword:
			return "false";
		case TokenId::FnKeyword:
			return "fn";
		case TokenId::ForKeyword:
			return "for";
		case TokenId::FinalKeyword:
			return "final";
		case TokenId::IfKeyword:
			return "if";
		case TokenId::ImportKeyword:
			return "import";
		case TokenId::LetKeyword:
			return "let";
		case TokenId::ModuleKeyword:
			return "module";
		case TokenId::NativeKeyword:
			return "native";
		case TokenId::NewKeyword:
			return "new";
		case TokenId::NullKeyword:
			return "null";
		case TokenId::OverrideKeyword:
			return "override";
		case TokenId::OperatorKeyword:
			return "operator";
		case TokenId::PubKeyword:
			return "pub";
		case TokenId::ReturnKeyword:
			return "return";
		case TokenId::StaticKeyword:
			return "static";
		case TokenId::StructKeyword:
			return "struct";
		case TokenId::SwitchKeyword:
			return "switch";
		case TokenId::ThisKeyword:
			return "this";
		case TokenId::ThrowKeyword:
			return "throw";
		case TokenId::TraitKeyword:
			return "trait";
		case TokenId::TypeofKeyword:
			return "typeof";
		case TokenId::InterfaceKeyword:
			return "interface";
		case TokenId::TrueKeyword:
			return "true";
		case TokenId::TryKeyword:
			return "try";
		case TokenId::UseKeyword:
			return "use";
		case TokenId::WhileKeyword:
			return "while";
		case TokenId::YieldKeyword:
			return "yield";
		case TokenId::I8TypeName:
			return "i8";
		case TokenId::I16TypeName:
			return "i16";
		case TokenId::I32TypeName:
			return "i32";
		case TokenId::I64TypeName:
			return "i64";
		case TokenId::U8TypeName:
			return "u8";
		case TokenId::U16TypeName:
			return "u16";
		case TokenId::U32TypeName:
			return "u32";
		case TokenId::U64TypeName:
			return "u64";
		case TokenId::F32TypeName:
			return "f32";
		case TokenId::F64TypeName:
			return "f64";
		case TokenId::StringTypeName:
			return "string";
		case TokenId::BoolTypeName:
			return "bool";
		case TokenId::AutoTypeName:
			return "auto";
		case TokenId::VoidTypeName:
			return "void";
		case TokenId::AnyTypeName:
			return "any";
		case TokenId::IntLiteral:
			return "integer literal";
		case TokenId::LongLiteral:
			return "long literal";
		case TokenId::UIntLiteral:
			return "unsigned integer literal";
		case TokenId::ULongLiteral:
			return "unsigned long literal";
		case TokenId::F32Literal:
			return "32-bit floating-point number literal";
		case TokenId::F64Literal:
			return "64-bit floating-point number literal";
		case TokenId::StringLiteral:
			return "string literal";
		case TokenId::RawStringLiteral:
			return "raw string literal";
		case TokenId::Id:
			return "identifier";
	}

	return "<unknown tokenId>";
}

Token &Lexer::nextToken(bool keepNewLine, bool keepWhitespace, bool keepComment) {
	size_t &i = context.curIndex;

	while (i < tokens.size()) {
		switch (tokens[i].tokenId) {
			case TokenId::NewLine:
				if (keepNewLine) {
					context.prevIndex = context.curIndex;
					return tokens[i++];
				}
				break;
			case TokenId::Whitespace:
				if (keepWhitespace) {
					context.prevIndex = context.curIndex;
					return tokens[i++];
				}
				break;
			case TokenId::Comment:
				if (keepComment) {
					context.prevIndex = context.curIndex;
					return tokens[i++];
				}
				break;
			default:
				context.prevIndex = context.curIndex;
				return tokens[i++];
		}

		++i;
	}

	return _endToken;
}

Token &Lexer::peekToken(bool keepNewLine, bool keepWhitespace, bool keepComment) {
	size_t i = context.curIndex;

	while (i < tokens.size()) {
		switch (tokens[i].tokenId) {
			case TokenId::NewLine:
				if (keepNewLine)
					return tokens[i];
				break;
			case TokenId::Whitespace:
				if (keepWhitespace)
					return tokens[i];
				break;
			case TokenId::Comment:
				if (keepComment)
					return tokens[i];
				break;
			default:
				return tokens[i];
		}

		++i;
	}

	return _endToken;
}
