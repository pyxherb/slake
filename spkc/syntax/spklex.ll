%option yylineno noinput nounput noyywrap nounistd never-interactive

%top {
#include <spkparse.hh>

#define YY_INPUT(buf, szRead, szMax) \
	int c;\
	if((c = getc(yyin)) == EOF) { \
		szRead = 0;\
	} else {\
		if (c == '\n') {\
			yylloc.last_line = ++(yylloc.first_line);\
			yylloc.first_column = 0;\
		} else\
			yylloc.first_column++;\
		\
		yylloc.last_column = yylloc.first_column;\
		buf[0] = c;\
		szRead = 1;\
	}

using namespace SpkC::Syntax;
}

%x COMMENT LINE_COMMENT STRING ESCAPE

%%

","			return ',';
"?"			return '?';
":"			return ':';
";"			return ';';
"["			return '[';
"]"			return ']';
"{"			return '{';
"}"			return '}';
"("			return '(';
")"			return ')';
"@"			return '@';
"."			return '.';
"..."		return T_VARARG;

"+"			return '+';
"-"			return '-';
"*"			return '*';
"/"			return '/';
"%"			return '%';
"&"			return '&';
"|"			return '|';
"^"			return '^';
"!"			return '!';
"~"			return '~';
"="			return '=';
"+="		return OP_ASSIGN_ADD;
"-="		return OP_ASSIGN_SUB;
"*="		return OP_ASSIGN_MUL;
"/="		return OP_ASSIGN_DIV;
"%="		return OP_ASSIGN_MOD;
"&="		return OP_ASSIGN_AND;
"|="		return OP_ASSIGN_OR;
"^="		return OP_ASSIGN_XOR;
"~="		return OP_ASSIGN_REV;
"=="		return OP_EQ;
"!="		return OP_NEQ;
"<="		return OP_LTEQ;
">="		return OP_GTEQ;
"&&"		return OP_LAND;
"||"		return OP_LOR;
"++"		return OP_INC;
"--"		return OP_DEC;
"=>"		return OP_INLINE_SW;
"->"		return OP_WRAP;

"async"		return KW_ASYNC;
"await"		return KW_AWAIT;
"break"		return KW_BREAK;
"case"		return KW_CASE;
"class"		return KW_CLASS;
"const"		return KW_CONST;
"continue"	return KW_CONTINUE;
"default"	return KW_DEFAULT;
"else"		return KW_ELSE;
"enum"		return KW_ENUM;
"fn"		return KW_FN;
"for"		return KW_FOR;
"if"		return KW_IF;
"import"	return KW_IMPORT;
"interface"	return KW_INTERFACE;
"null"		return KW_NULL;
"pub"		return KW_PUB;
"return"	return KW_RETURN;
"self"		return KW_SELF;
"switch"	return KW_SWITCH;
"times"		return KW_TIMES;
"while"		return KW_WHILE;

"i8"		return TN_I8;
"i16"		return TN_I16;
"i32"		return TN_I32;
"i64"		return TN_I64;
"u8"		return TN_U8;
"u16"		return TN_U16;
"u32"		return TN_U32;
"u64"		return TN_U64;
"float"		return TN_FLOAT;
"double"	return TN_DOUBLE;
"string"	return TN_STRING;
"auto"		return TN_AUTO;

"/*"				BEGIN(COMMENT);
<COMMENT>"*/"		BEGIN(INITIAL);
<COMMENT>.*	;

"//"				BEGIN(LINE_COMMENT);
<LINE_COMMENT>\n	BEGIN(INITIAL);
<LINE_COMMENT>.*	;

[a-zA-Z_][a-zA-Z0-9_]* {
	yylval = std::make_shared<Token<std::string>>(yytext);
	return T_ID;
}

[-]?[0-9]+ {
	yylval = std::make_shared<Token<int>>(strtol(yytext, nullptr, 10));
	return L_INT;
}

[-]?[0-9]+[uU] {
	yylval = std::make_shared<Token<unsigned int>>(strtoul(yytext, nullptr, 10));
	return L_UINT;
}

[-]?[0-9]+[lL] {
	yylval = std::make_shared<Token<long long>>(strtoll(yytext, nullptr, 10));
	return L_LONG;
}

[-]?[0-9]+(([uU][lL])|([lL][uU])) {
	yylval = std::make_shared<Token<unsigned long long>>(strtoull(yytext, nullptr, 10));
	return L_ULONG;
}

[-]?[0-9]+(\.([0-9]*f)|f) {
	yylval = std::make_shared<Token<float>>(strtof(yytext, nullptr));
	return L_FLOAT;
}

[-]?[0-9]+(\.[0-9]*) {
	yylval = std::make_shared<Token<double>>(strtod(yytext, nullptr));
	return L_DOUBLE;
}

-0[xX][0-9]+ {
	yylval = std::make_shared<Token<int>>(strtol(yytext, nullptr, 16));
	return L_INT;
}

0[xX][0-9]+ {
	yylval = std::make_shared<Token<unsigned int>>(strtoul(yytext, nullptr, 16));
	return L_UINT;
}

-0[xX][0-9]+[lL] {
	yylval = std::make_shared<Token<long long>>(strtoll(yytext, nullptr, 16));
	return L_INT;
}

0[xX][0-9]+[lL] {
	yylval = std::make_shared<Token<unsigned long long>>(strtoull(yytext, nullptr, 16));
	return L_UINT;
}

\" {
	BEGIN(STRING);
	yylval = std::make_shared<Token<std::string>>(yytext);
}
<STRING>[^\"\n\\]+ {
	std::static_pointer_cast<Token<std::string>>(yylval)->data += yytext;
}
<STRING>\n {
	yyerror(nullptr, "unterminated string literal");
	BEGIN(INITIAL);
	yyterminate();
}
<STRING>\" {
	BEGIN(INITIAL);
	return L_STRING;
}
<STRING>\\ {
	BEGIN(ESCAPE);
}
<ESCAPE>\\ {
	std::static_pointer_cast<Token<std::string>>(yylval)->data += "\\";
	BEGIN(STRING);
}
<ESCAPE>n {
	std::static_pointer_cast<Token<std::string>>(yylval)->data += "\n";
	BEGIN(STRING);
}
<ESCAPE>r {
	std::static_pointer_cast<Token<std::string>>(yylval)->data += "\r";
	BEGIN(STRING);
}
<ESCAPE>\" {
	std::static_pointer_cast<Token<std::string>>(yylval)->data += "\"";
	BEGIN(STRING);
}
<ESCAPE>0 {
	std::static_pointer_cast<Token<std::string>>(yylval)->data += "\0";
	BEGIN(STRING);
}
<ESCAPE>x[0-9a-fA-F]{2} {
	char s[2] = { (char)strtoul(yytext, nullptr, 16), '\0' };
	std::static_pointer_cast<Token<std::string>>(yylval)->data += s;
	BEGIN(STRING);
}
<ESCAPE>[0-7]{3} {
	char s[2] = { (char)strtoul(yytext, nullptr, 8), '\0' };
	std::static_pointer_cast<Token<std::string>>(yylval)->data += s;
	BEGIN(STRING);
}

[\t\n\r ]+ ;

%%
