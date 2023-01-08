///
/// @file bcparse.yy
/// @author CodesBuilder (2602783536@qq.com)
/// @brief Parser for Slake Byte Code Compiler
/// @version 0.1
/// @date 2022-11-21
///
/// @copyright Copyright (C) 2022 Slake Project
///
%require "3.2"

%{
#include <slkparse.hh>

Slake::Assembler::parser::symbol_type yylex();

using std::static_pointer_cast;
using std::make_shared;
using std::shared_ptr;

%}

%code requires {
#include <compiler/compiler.hh>
#include <cstdio>
#include <cstdarg>

}
%code provides {
extern Slake::Assembler::parser::location_type yylloc;
extern std::shared_ptr<Slake::Assembler::parser> yyparser;
}

%locations
%language "c++"
%define api.namespace {Slake::Assembler}
%define api.token.constructor
%define api.value.type variant

%define parse.assert
%define parse.trace
%define parse.error detailed
%define parse.lac full

%token <std::string> T_ID "identifier"
%token T_VARARG "..."

%token <int> L_INT "integer literal"
%token <unsigned int> L_UINT "unsigned integer literal"
%token <long long> L_LONG "long integer literal"
%token <unsigned long long> L_ULONG "unsigned long long integer literal"
%token <float> L_FLOAT "single precision FP literal"
%token <double> L_DOUBLE "double precision FP literal"
%token <std::string> L_STRING

%token TN_I8 "i8"
%token TN_I16 "i16"
%token TN_I32 "i32"
%token TN_I64 "i64"
%token TN_U8 "u8"
%token TN_U16 "u16"
%token TN_U32 "u32"
%token TN_U64 "u64"
%token TN_FLOAT "float"
%token TN_DOUBLE "double"
%token TN_STRING "string"
%token TN_AUTO "auto"

%token OP_ADD "+"
%token OP_SUB "-"
%token OP_MUL "*"
%token OP_DIV "/"
%token OP_MOD "%"
%token OP_AND "&"
%token OP_OR "|"
%token OP_XOR "^"
%token OP_REV "~"
%token OP_NOT "!"
%token OP_TERNARY "?"
%token OP_EQ "=="
%token OP_NEQ "!="
%token OP_LTEQ "<="
%token OP_GTEQ ">="
%token OP_LAND "&&"
%token OP_LOR "||"

%token T_AT "@"
%token T_LPARENTHESE "("
%token T_RPARENTHESE ")"
%token T_LBRACKET "["
%token T_RBRACKET "]"
%token T_LBRACE "{"
%token T_RBRACE "}"
%token T_COMMA ","
%token T_COLON ":"
%token T_SEMICOLON ";"
%token T_DOT "."
%token T_SECTION "%%"
%token T_EOL "end of line"

%right "=" "+=" "-=" "*=" "/=" "%=" "&=" "|=" "^=" "~="
%right "?" "=>"
%left "||"
%left "&&"
%left "|"
%left "^"
%left "&"
%left "==" "!="
%left "<" "<=" ">" ">="
%left "+" "-"
%left "*" "/" "%"
%precedence "!" OP_NEG "++" "--"
%nonassoc "(" ")"

%expect 0

%%

Program:
Directives "%%"
;

Directives:
Directive {}
| Directives Directive {}
;

Directive:
"@" T_ID T_EOL {}
| "@" T_ID "(" DirectiveArgs ")" T_EOL {}
;

DirectiveArgs:
Literal {}
| DirectiveArgs ',' Literal {}
;

Instructions:
Instruction {}
| Instructions Instruction {}
;

Instruction:
L_INT Operands {}
T_ID Operands {}
;

Operands:
Operands {}
| Operands ',' Operand {}
;

Operand:
Literal {}
| VarRef {}
| TypeRef {}
;

Literal:
L_INT { $$ = make_shared<IntLiteralExpr>($1); }
| L_UINT { $$ = make_shared<UIntLiteralExpr>($1); }
| L_LONG { $$ = make_shared<LongLiteralExpr>($1); }
| L_ULONG { $$ = make_shared<ULongLiteralExpr>($1); }
| L_FLOAT { $$ = make_shared<FloatLiteralExpr>($1); }
| L_DOUBLE { $$ = make_shared<DoubleLiteralExpr>($1); }
| "null" { $$ = make_shared<NullLiteralExpr>(); }
| L_STRING { $$ = make_shared<StringLiteralExpr>($1); }
;

VarRef:
"$" T_ID {}
;

TypeRef:
"@" T_ID {}
;
