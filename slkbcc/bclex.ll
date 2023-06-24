%option noinput nounput noyywrap nounistd never-interactive

%top {
#include <bcparse.hh>

#define YY_USER_ACTION yylloc.columns(yyleng);
#define YY_DECL Slake::Assembler::parser::symbol_type yylex()

using namespace Slake::Assembler;
}

%{
Slake::Assembler::parser::symbol_type yylval;
%}

%x LINE_COMMENT STRING ESCAPE

%%

%{
yylloc.step();
%}

","			return parser::make_T_COMMA(yylloc);
":"			return parser::make_T_COLON(yylloc);
";"			return parser::make_T_SEMICOLON(yylloc);
"["			return parser::make_T_LBRACKET(yylloc);
"]"			return parser::make_T_RBRACKET(yylloc);
"{"			return parser::make_T_LBRACE(yylloc);
"}"			return parser::make_T_RBRACE(yylloc);
"("			return parser::make_T_LPARENTHESE(yylloc);
")"			return parser::make_T_RPARENTHESE(yylloc);
"@"			return parser::make_T_AT(yylloc);
"."			return parser::make_T_DOT(yylloc);
"..."		return parser::make_T_VARARG(yylloc);

"base"		return parser::make_KW_BASE(yylloc);
"const"		return parser::make_KW_CONST(yylloc);
"false"		return parser::make_KW_FALSE(yylloc);
"final"		return parser::make_KW_FINAL(yylloc);
"native"	return parser::make_KW_NATIVE(yylloc);
"null"		return parser::make_KW_NULL(yylloc);
"override"	return parser::make_KW_OVERRIDE(yylloc);
"operator"	return parser::make_KW_OPERATOR(yylloc);
"pub"		return parser::make_KW_PUB(yylloc);
"static"	return parser::make_KW_STATIC(yylloc);
"true"		return parser::make_KW_TRUE(yylloc);

".access"		return parser::make_D_ACCESS(yylloc);
".class"		return parser::make_D_CLASS(yylloc);
".interface"	return parser::make_D_INTERFACE(yylloc);
".trait"		return parser::make_D_TRAIT(yylloc);
".struct"		return parser::make_D_STRUCT(yylloc);
".fn"			return parser::make_D_FN(yylloc);
".fndecl"		return parser::make_D_FNDECL(yylloc);
".end"			return parser::make_D_END(yylloc);
".module"		return parser::make_D_MODULE(yylloc);
".import"		return parser::make_D_IMPORT(yylloc);
".alias"		return parser::make_D_ALIAS(yylloc);
".var"			return parser::make_D_VAR(yylloc);
".extends"		return parser::make_D_EXTENDS(yylloc);
".implements"	return parser::make_D_IMPLEMENTS(yylloc);
".complies"		return parser::make_D_COMPLIES(yylloc);

"i8"		return parser::make_TN_I8(yylloc);
"i16"		return parser::make_TN_I16(yylloc);
"i32"		return parser::make_TN_I32(yylloc);
"i64"		return parser::make_TN_I64(yylloc);
"isize"		return parser::make_TN_ISIZE(yylloc);
"u8"		return parser::make_TN_U8(yylloc);
"u16"		return parser::make_TN_U16(yylloc);
"u32"		return parser::make_TN_U32(yylloc);
"u64"		return parser::make_TN_U64(yylloc);
"usize"		return parser::make_TN_USIZE(yylloc);
"f32"		return parser::make_TN_F32(yylloc);
"f64"		return parser::make_TN_F64(yylloc);
"string"	return parser::make_TN_STRING(yylloc);
"bool"		return parser::make_TN_BOOL(yylloc);
"auto"		return parser::make_TN_AUTO(yylloc);
"void"		return parser::make_TN_VOID(yylloc);
"any"		return parser::make_TN_ANY(yylloc);

"#"					BEGIN(LINE_COMMENT); yylloc.step();
<LINE_COMMENT>\n	BEGIN(INITIAL); yylloc.lines(yyleng); yylloc.step();
<LINE_COMMENT>.*	yylloc.step();

[a-zA-Z_][a-zA-Z0-9_]* {
	if (std::strlen(yytext) > 255) {
		yyparser->error(yylloc, "identifier is too long");
		return parser::make_YYerror(yylloc);
	}
	return parser::make_T_ID(yytext, yylloc);
}

[-]?[0-9]+[iI]8 {
	std::string s = yytext;
	s.resize(s.size() - 2);
	return parser::make_L_I8((int8_t)strtol(s.c_str(), nullptr, 10), yylloc);
}
[-]?[0-9]+[uU]8 {
	std::string s = yytext;
	s.resize(s.size() - 2);
	return parser::make_L_U8((uint8_t)strtoul(s.c_str(), nullptr, 10), yylloc);
}

[-]?[0-9]+[iI]16 {
	std::string s = yytext;
	s.resize(s.size() - 3);
	return parser::make_L_I16((int16_t)strtol(s.c_str(), nullptr, 10), yylloc);
}
[-]?[0-9]+[uU]16 {
	std::string s = yytext;
	s.resize(s.size() - 3);
	return parser::make_L_U16((uint16_t)strtoul(s.c_str(), nullptr, 10), yylloc);
}

[-]?[0-9]+ return parser::make_L_I32(strtol(yytext, nullptr, 10), yylloc);
[-]?[0-9]+[uU] return parser::make_L_U32(strtoul(yytext, nullptr, 10), yylloc);

[-]?[0-9]+[lL] return parser::make_L_I64(strtoll(yytext, nullptr, 10), yylloc);
[-]?[0-9]+(([uU][lL])|([lL][uU])) return parser::make_L_U64(strtoull(yytext, nullptr, 10), yylloc);

[-]?[0-9]+(\.([0-9]*f)|f) return parser::make_L_F32(strtof(yytext, nullptr), yylloc);
[-]?[0-9]+(\.[0-9]*) return parser::make_L_F64(strtod(yytext, nullptr), yylloc);

-0[xX][0-9a-fA-F]+[iI]8 {
	std::string s = yytext;
	s.resize(s.size() - 2);
	return parser::make_L_I32((int32_t)strtol(s.c_str(), nullptr, 16), yylloc);
}
0[xX][0-9a-fA-F]+[iI]8 {
	std::string s = yytext;
	s.resize(s.size() - 2);
	return parser::make_L_U32((uint32_t)strtoul(s.c_str(), nullptr, 16), yylloc);
}

-0[xX][0-9a-fA-F]+[iI]16 {
	std::string s = yytext;
	s.resize(s.size() - 3);
	return parser::make_L_I16((int16_t)strtol(s.c_str(), nullptr, 16), yylloc);
}
0[xX][0-9a-fA-F]+[iI]16 {
	std::string s = yytext;
	s.resize(s.size() - 3);
	return parser::make_L_U16((uint16_t)strtoul(s.c_str(), nullptr, 16), yylloc);
}

-0[xX][0-9a-fA-F]+ return parser::make_L_I32(strtol(yytext, nullptr, 16), yylloc);
0[xX][0-9a-fA-F]+ return parser::make_L_U32(strtoul(yytext, nullptr, 16), yylloc);

-0[xX][0-9a-fA-F]+[lL] return parser::make_L_I64(strtoll(yytext, nullptr, 16), yylloc);
0[xX][0-9a-fA-F]+[lL] return parser::make_L_U64(strtoull(yytext, nullptr, 16), yylloc);

\" {
	BEGIN(STRING);
	auto tmpSymbol = parser::make_L_STRING("", yylloc);
	yylval.move(tmpSymbol);
	yylloc.step();
}
<STRING>[^\"\n\\]+ {
	yylval.value.as<std::string>() += yytext;
	yylloc.step();
}
<STRING>\n {
	yylloc.step();
	yyparser->error(yylloc, "unterminated string literal");
	BEGIN(INITIAL);
	return parser::make_YYerror(yylloc);
}
<STRING>\" {
	Slake::Assembler::parser::symbol_type value;
	value.move(yylval);

	BEGIN(INITIAL);
	return value;
}
<STRING>\\ {
	BEGIN(ESCAPE);
	yylloc.step();
}
<ESCAPE>\\ {
	yylval.value.as<std::string>() += "\\";
	BEGIN(STRING);
	yylloc.step();
}
<ESCAPE>n {
	yylval.value.as<std::string>() += "\n";
	BEGIN(STRING);
	yylloc.step();
}
<ESCAPE>r {
	yylval.value.as<std::string>() += "\r";
	BEGIN(STRING);
	yylloc.step();
}
<ESCAPE>\" {
	yylval.value.as<std::string>() += "\"";
	BEGIN(STRING);
	yylloc.step();
}
<ESCAPE>0 {
	yylval.value.as<std::string>() += "\0";
	BEGIN(STRING);
	yylloc.step();
}
<ESCAPE>x[0-9a-fA-F]{2} {
	char s[2] = { (char)strtoul(yytext, nullptr, 16), '\0' };
	yylval.value.as<std::string>() += s;
	BEGIN(STRING);
	yylloc.step();
}
<ESCAPE>[0-7]{3} {
	char s[2] = { (char)strtoul(yytext, nullptr, 8), '\0' };
	yylval.value.as<std::string>() += s;
	BEGIN(STRING);
	yylloc.step();
}

[\t\r ]+ yylloc.step();
\n yylloc.lines(yyleng); yylloc.step();
<<EOF>> return parser::make_YYEOF(yylloc);

%%
