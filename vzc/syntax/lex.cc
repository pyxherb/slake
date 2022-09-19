///
/// @file lex.cc
/// @author CodesBuilder (2602783536@qq.com)
/// @brief Implementations for the lexical analyzer.
/// @version 0.1
/// @date 2022-09-19
///
/// @copyright Copyright (c) 2022 Vizard Contributors
///

#include "lex.h"

using namespace Blare;
using namespace std;
using namespace Vzc::Syntax;

enum States {
	STATE_STRING = 1
};

const auto typenameAction = [](Lexer& lexer, const smatch& result, shared_ptr<Sym>& dest) -> bool {
	dest = std::make_shared<TypenameSym>(TOK_TYPENAME, result.str());
	return true;
};

Vzc::Syntax::VzcLexer::VzcLexer() {
	State initialState{
		{ R"(\/\/[^\n]*\n)", TOK_INVALID },

		{ "fn", KW_FN },
		{ "const", KW_CONST },
		{ "if", KW_IF },
		{ "elif", KW_ELIF },
		{ "else", KW_ELSE },
		{ "switch", KW_SWITCH },
		{ "case", KW_CASE },
		{ "while", KW_WHILE },
		{ "for", KW_FOR },
		{ "times", KW_TIMES },
		{ "continue", KW_CONTINUE },
		{ "break", KW_BREAK },
		{ "return", KW_RETURN },
		{ "pub", KW_PUB },
		{ "import", KW_IMPORT },
		{ "null", KW_NULL },

		{ "i8", typenameAction },
		{ "i16", typenameAction },
		{ "i32", typenameAction },
		{ "i64", typenameAction },
		{ "u8", typenameAction },
		{ "u16", typenameAction },
		{ "u32", typenameAction },
		{ "u64", typenameAction },
		{ "string", typenameAction },
		{ "@([a-zA-Z_][a-zA-Z0-9_]+)",
			[](Lexer& lexer, const smatch& result,
				shared_ptr<Sym>& dest) -> bool {
				dest =
					std::make_shared<TypenameSym>(TOK_TYPENAME, result[1]);
				return true;
			} },

		{ R"(::)", OP_SCOPE },
		{ R"(=>)", OP_INLINE_SW },
		{ R"(&&)", OP_LAND },
		{ R"(\|\|)", OP_LOR },
		{ R"(\<\<)", OP_LSHIFT },
		{ R"(\>\>)", OP_RSHIFT },
		{ R"(:)", ':' },
		{ R"(@)", '@' },
		{ R"(#)", '#' },
		{ R"(\$)", '$' },
		{ R"(!)", '!' },
		{ R"(\+)", '+' },
		{ R"(-)", '-' },
		{ R"(\*)", '*' },
		{ R"(\/)", '/' },
		{ R"(%)", '%' },
		{ R"(&)", '&' },
		{ R"(\|)", '|' },
		{ R"(\^)", '^' },
		{ R"(>)", '>' },
		{ R"(<)", '<' },
		{ R"(=)", '=' },
		{ R"(~)", '~' },
		{ R"(\?)", '?' },
		{ R"(\()", '(' },
		{ R"(\))", ')' },
		{ R"(\[)", '[' },
		{ R"(\])", ']' },
		{ R"(\{)", '{' },
		{ R"(\})", '}' },
		{ R"(\.)", '.' },
		{ R"(,)", ',' },
		{ R"(;)", ';' },
		{ R"([ \t\n\r]+)", TOK_INVALID },

		{ R"([-]?[0-9]+)",
			[](Lexer& lexer, const smatch& result,
				shared_ptr<Sym>& dest) -> bool {
				dest = std::make_shared<IntLiteralSym>(
					TOK_INT_L, atoi(result.str().c_str()));
				return true;
			} },
		{ R"([-]?[0-9]+[uU])",
			[](Lexer& lexer, const smatch& result,
				shared_ptr<Sym>& dest) -> bool {
				dest = std::make_shared<UIntLiteralSym>(
					TOK_UINT_L, strtoul(result.str().c_str(), nullptr, 0));
				return true;
			} },
		{ R"([-]?[0-9]+[lL])",
			[](Lexer& lexer, const smatch& result,
				shared_ptr<Sym>& dest) -> bool {
				dest = std::make_shared<LongLiteralSym>(
					TOK_LONG_L, atoi(result.str().c_str()));
				return true;
			} },
		{ R"([-]?[0-9]+(?:(?:[uU][lL])|(?:[lL][uU])))",
			[](Lexer& lexer, const smatch& result,
				shared_ptr<Sym>& dest) -> bool {
				dest = std::make_shared<ULongLiteralSym>(
					TOK_ULONG_L, strtoul(result.str().c_str(), nullptr, 0));
				return true;
			} },
		{ R"([-]?(?:(?:[0-9]+\.[0-9]+)|(?:\.[0-9]+)|(?:[0-9]+\.)|(?:[0-9]+))[fF])",
			[](Lexer& lexer, const smatch& result,
				shared_ptr<Sym>& dest) -> bool {
				dest = std::make_shared<FloatLiteralSym>(
					TOK_FLOAT_L, (float)atof(result.str().c_str()));
				return true;
			} },
		{ R"([-]?(?:(?:[0-9]+\.[0-9]+)|(?:\.[0-9]+)|(?:[0-9]+\.)|(?:[0-9]+)))",
			[](Lexer& lexer, const smatch& result,
				shared_ptr<Sym>& dest) -> bool {
				dest = std::make_shared<DoubleLiteralSym>(
					TOK_DOUBLE_L, atof(result.str().c_str()));
				return true;
			} },
		{ R"([a-zA-Z_][a-zA-Z0-9_]*)",
			[](Lexer& lexer, const smatch& result,
				shared_ptr<Sym>& dest) -> bool {
				dest =
					std::make_shared<IdSym>(TOK_ID, result.str());
				return true;
			} },
		{ R"(\")",
			[](Lexer& lexer, const smatch& result,
				shared_ptr<Sym>& dest) -> bool {
				lexer.setState((StateID)States::STATE_STRING);
				dest = std::make_shared<StringLiteralSym>(TOK_STRING_L, "");
				return false;
			} }
	};

	State stringState{ { R"([^"]+)",
						   [](Lexer& lexer, const smatch& result,
							   shared_ptr<Sym>& dest) -> bool {
							   shared_ptr<StringLiteralSym> sym =
								   dynamic_pointer_cast<StringLiteralSym>(dest);
							   sym->data += result.str();
							   return false;
						   } },
		{ R"(\")",
			[](Lexer& lexer, const smatch& result,
				shared_ptr<Sym>& dest) -> bool {
				lexer.setState(STATE_INITIAL);
				return true;
			} } };

	addState(STATE_INITIAL, initialState);
	addState(STATE_STRING, stringState);
}
