///
/// @file bccparse.yy
/// @author Pyxherb (codesbuilder@163.com)
/// @brief Parser for Slake bytecode assembly
/// @version 0.1
/// @date 2022-11-21
///
/// @copyright Copyright (C) 2022 Slake Project
///
/// SPDX-License-Identifier: LGPL-3.0-only
///
%require "3.2"

%{
#include <bcparse.hh>
#include <bclex.h>

YY_DECL;
using namespace Slake::Assembler;

%}

%code requires {
#include <slake/util/debug.h>
#include <slkbcc.h>

#pragma push_macro("new")

#undef new
}

%code provides {
extern Slake::Assembler::parser::location_type yylloc;
extern std::shared_ptr<Slake::Assembler::parser> yyparser;
extern Slake::AccessModifier curAccess;
extern std::unordered_map<std::string, uint32_t> curLabels;
extern std::deque<std::shared_ptr<Slake::Assembler::Scope>> savedScopes;

#pragma pop_macro("new")
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

%token <string> T_ID "identifier"
%token T_VARARG "..."

%token <int8_t> L_I8 "i8 literal"
%token <int16_t> L_I16 "i16 literal"
%token <int32_t> L_I32 "i32 literal"
%token <int64_t> L_I64 "i64 literal"
%token <uint8_t> L_U8 "u8 literal"
%token <uint16_t> L_U16 "u16 literal"
%token <uint32_t> L_U32 "u32 literal"
%token <uint64_t> L_U64 "u64 literal"
%token <float> L_F32 "f32 literal"
%token <double> L_F64 "f64 literal"
%token <string> L_STRING "string literal"

%token KW_CONST "const"
%token KW_END "end"
%token KW_FALSE "false"
%token KW_FINAL "final"
%token KW_NATIVE "native"
%token KW_NULL "null"
%token KW_OVERRIDE "override"
%token KW_OPERATOR "operator"
%token KW_PUB "pub"
%token KW_STATIC "static"
%token KW_TRUE "true"

%token D_ACCESS ".access"
%token D_CLASS ".class"
%token D_INTERFACE ".interface"
%token D_TRAIT ".trait"
%token D_STRUCT ".struct"
%token D_FN ".fn"
%token D_FNDECL ".fndecl"
%token D_END ".end"
%token D_MODULE ".module"
%token D_IMPORT ".import"
%token D_ALIAS ".alias"
%token D_VAR ".var"
%token D_EXTENDS ".extends"
%token D_IMPLEMENTS ".implements"
%token D_COMPLIES ".complies"

%token TN_I8 "i8"
%token TN_I16 "i16"
%token TN_I32 "i32"
%token TN_I64 "i64"
%token TN_ISIZE "isize"
%token TN_U8 "u8"
%token TN_U16 "u16"
%token TN_U32 "u32"
%token TN_U64 "u64"
%token TN_USIZE "usize"
%token TN_F32 "f32"
%token TN_F64 "f64"
%token TN_STRING "string"
%token TN_BOOL "bool"
%token TN_AUTO "auto"
%token TN_ANY "any"
%token TN_VOID "void"

%token OP_ADD "+"

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

%type <shared_ptr<Operand>> Literal Operand
%type <shared_ptr<ArrayOperand>> Array
%type <shared_ptr<MapOperand>> Map
%type <shared_ptr<Ref>> Ref InheritSlot
%type <string> OperatorName
%type <uint16_t> AccessModifier
%type <deque<shared_ptr<TypeName>>> GenericArgs TypeNameList
%type <shared_ptr<TypeName>> TypeName LiteralTypeName CustomTypeName FnTypeName ArrayTypeName MapTypeName
%type <ParamDeclList> Params _Params
%type <deque<shared_ptr<Instruction>>> Instructions
%type <shared_ptr<Instruction>> Instruction
%type <deque<shared_ptr<Operand>>> Operands _Operands

%expect 0

%%

Prog:
Stmts
| %empty
;

Stmts:
Stmt Stmts
| Stmt
;

Stmt:
ModuleDecl
| ImportBlock
| FnDef
| FnDecl
| VarDef
| Access
| ClassDef
| InterfaceDef
| TraitDef
;

// Class

ClassDef:
".class" T_ID InheritSlot ImplList {
	auto curClass = make_shared<Class>(@1, curAccess, $3);
	curScope->classes[$2] = curClass;
	curAccess = 0;

	savedScopes.push_back(curScope);
	curScope = curClass->scope;
} ClassStmts ".end" {
	curScope = savedScopes.back();
	savedScopes.pop_back();
}
;

InheritSlot:
%empty {}
| ".extends" Ref { $$ = $2; }
;

ImplList:
%empty
| ".implements" _ImplList {}
;

_ImplList:
Ref "," _ImplList {}
| Ref {}
;

ComplyDecls:
ComplyDecl "," ComplyDecls {}
| ComplyDecl {}
;

ComplyDecl:
".complies" Ref {}
;

ClassStmts:
ClassStmt ClassStmts
| ClassStmt
;

ClassStmt:
FnDef
| FnDecl
| VarDef
| Access
;

// Interface

InterfaceDef:
".interface" T_ID InheritSlot {
	curAccess = 0;
} InterfaceStmts ".end" {
}
;

InterfaceStmts:
InterfaceStmt InterfaceStmts
| InterfaceStmt
;

InterfaceStmt:
FnDecl
| Access
;

// Trait

TraitDef:
".trait" T_ID InheritSlot ImplList {
	curAccess = 0;
} TraitStmts ".end" {
}
;

TraitStmts:
TraitStmt TraitStmts
| TraitStmt
;

TraitStmt:
FnDecl
| VarDef
| Access
;

// Miscellaneous

VarDef:
".var" TypeName T_ID {
	curScope->vars[$3] = make_shared<Var>(@1, curAccess, $2);
	curAccess = 0;
}
| ".var" TypeName T_ID Literal {
	curScope->vars[$3] = make_shared<Var>(@1, curAccess, $2, $4);
	curAccess = 0;
}
;

Access:
".access" AccessModifier { curAccess = $2; }
;

AccessModifier:
"pub" AccessModifier { $$ |= ACCESS_PUB; }
| "static" AccessModifier { $$ |= ACCESS_STATIC; }
| "final" AccessModifier { $$ |= ACCESS_FINAL; }
| "const" AccessModifier { $$ |= ACCESS_CONST; }
| "native" AccessModifier { $$ |= ACCESS_NATIVE; }
| "override" AccessModifier { $$ |= ACCESS_OVERRIDE; }
| %empty {}
;

FnDef:
".fn" TypeName T_ID Params Instructions ".end" {
	if(curScope->funcs.count($3))
		error(@3, "Dulplicated function entry `" + $3 + "'");
	curScope->funcs[$3] = make_shared<Fn>(@1, curAccess, $2, $4, $5);
	curAccess = 0;
}
| ".fn" TypeName "operator" OperatorName Params Instructions ".end" {
	if(curScope->funcs.count($4))
		error(@4, "Dulplicated function entry `" + $4 + "'");
	curScope->funcs[$4] = make_shared<Fn>(@1, curAccess, $2, $5, $6);
	curAccess = 0;
}
;

FnDecl:
".fndecl" TypeName T_ID Params {
	if(curScope->funcs.count($3))
		error(@3, "Dulplicated function entry `" + $3 + "'");
	curScope->funcs[$3] = make_shared<Fn>(@1, curAccess, $2, $4);
	curAccess = 0;
}
| ".fndecl" TypeName "operator" OperatorName Params {
	if(curScope->funcs.count($4))
		error(@3, "Dulplicated function entry `" + $4 + "'");
	curScope->funcs[$4] = make_shared<Fn>(@1, curAccess, $2, $5);
	curAccess = 0;
}
;

Params:
%empty {}
| _Params { $$ = $1; }
;

_Params:
TypeName "," _Params { $$.swap($3), $$.push_front($1); }
| TypeName { $$.push_back($1); }
| "..." { $$.isVariadic = true; }
;

Instructions:
Instructions Instruction ";" { $$ = $1, $$.push_back($2); }
| Instructions T_ID ":" Instruction ";" {
	if (curLabels.count($2))
		error(@2, "Dulplicated label `" + $2 + "'");
	curLabels[$2] = $$.size();
}
| T_ID ":" {
	if (curLabels.count($1))
		error(@1, "Dulplicated label `" + $1 + "'");
	curLabels[$1] = $$.size();
}
| Instruction ";" { $$.push_back($1); }
;

Instruction:
L_I32 Operands { $$ = make_shared<Instruction>(@1, (Opcode)$1, $2); }
| L_U32 Operands { $$ = make_shared<Instruction>(@1, (Opcode)$1, $2); }
| T_ID Operands {
	if(mnemonics.count($1))
		$$ = make_shared<Instruction>(@1, mnemonics.at($1), $2);
	else
		error(@1, "Invalid mnemonic `" + $1 + "'");
}
;

Operands:
%empty {}
| _Operands { $$ = $1; }
;

_Operands:
Operand "," _Operands { $$.push_back($1); }
| Operand { $$.push_back($1); }
;

Operand:
Ref { $$ = make_shared<RefOperand>(@1, $1); }
| Literal { $$ = $1; }
| "$" T_ID { $$ = make_shared<LabelOperand>(@1, $2); }
| Array { $$ = $1; }
| Map { $$ = $1; }
;

//
// Import
//
ImportBlock:
".import" T_ID Ref
;

//
// Module declaration
//
ModuleDecl:
".module" Ref { moduleName = $2; }
;

OperatorName:
"+" { $$ = "+"; }
| "-" { $$ = "-"; }
| "*" { $$ = "*"; }
| "/" { $$ = "/"; }
| "%" { $$ = "%"; }
| "&" { $$ = "&"; }
| "|" { $$ = "|"; }
| "^" { $$ = "^"; }
| "&&" { $$ = "&&"; }
| "||" { $$ = "||"; }
| "~" { $$ = "~"; }
| "!" { $$ = "!"; }
| "=" { $$ = "="; }
| "+=" { $$ = "+="; }
| "-=" { $$ = "-="; }
| "*=" { $$ = "*="; }
| "/=" { $$ = "/="; }
| "%=" { $$ = "%="; }
| "&=" { $$ = "&="; }
| "|=" { $$ = "|="; }
| "^=" { $$ = "^="; }
| "~=" { $$ = "~="; }
| "==" { $$ = "=="; }
| "!=" { $$ = "!="; }
| ">" { $$ = ">"; }
| "<" { $$ = "<"; }
| ">=" { $$ = ">="; }
| "<=" { $$ = "<="; }
| "[" "]" { $$ = "[]"; }
| "(" ")" { $$ = "()"; }
;

Array:
"{" "}" {}
| "{" ArrayElements "}" {}
;

ArrayElements:
Literal "," Literal
| Literal
;

Map:
"[" PairList "]" {
}
;

PairList:
%empty { }
| Pairs { }
;

Pairs:
Pair {
}
| Pairs "," Pair {
}
;

Pair:
Literal ":" Literal {
}
;

GenericArgs:
%empty {}
|"<" TypeNameList ">" { $$ = $2; }
;

TypeNameList:
TypeName { $$.push_back($1); }
| TypeNameList "," TypeName {
	$$.swap($1);
	$$.push_back($3);
}
;

TypeName:
LiteralTypeName { $$ = $1; }
| CustomTypeName { $$ = $1; }
| FnTypeName  { $$ = $1; }
| ArrayTypeName { $$ = $1; }
| MapTypeName { $$ = $1; }
;

LiteralTypeName:
"i8" { $$ = make_shared<TypeName>(@1, TYPE_I8); }
|"i16" { $$ = make_shared<TypeName>(@1, TYPE_I16); }
|"i32" { $$ = make_shared<TypeName>(@1, TYPE_I32); }
|"i64" { $$ = make_shared<TypeName>(@1, TYPE_I64); }
|"isize" { $$ = make_shared<TypeName>(@1, TYPE_ISIZE); }
|"u8" { $$ = make_shared<TypeName>(@1, TYPE_U8); }
|"u16" { $$ = make_shared<TypeName>(@1, TYPE_U16); }
|"u32" { $$ = make_shared<TypeName>(@1, TYPE_U32); }
|"u64" { $$ = make_shared<TypeName>(@1, TYPE_U64); }
|"usize" { $$ = make_shared<TypeName>(@1, TYPE_USIZE); }
|"f32" { $$ = make_shared<TypeName>(@1, TYPE_F32); }
|"f64" { $$ = make_shared<TypeName>(@1, TYPE_F64); }
|"string" { $$ = make_shared<TypeName>(@1, TYPE_STRING); }
|"bool" { $$ = make_shared<TypeName>(@1, TYPE_BOOL); }
|"void" { $$ = make_shared<TypeName>(@1, TYPE_VOID); }
|"any" { $$ = make_shared<TypeName>(@1, TYPE_ANY); }
;

CustomTypeName:
"@" Ref { $$ = make_shared<CustomTypeName>(@1, $2); }
;

FnTypeName:
TypeName "->" "(" Params ")" {
}
;

ArrayTypeName:
TypeName "[" "]" { $$ = make_shared<ArrayTypeName>(@1, $1); }
;

MapTypeName:
TypeName "[" TypeName "]" { $$ = make_shared<MapTypeName>(@1, $1, $3); }
;

Literal:
L_I8 { $$ = make_shared<I8Operand>(@1, $1); }
| L_I16 { $$ = make_shared<I16Operand>(@1, $1); }
| L_I32 { $$ = make_shared<I32Operand>(@1, $1); }
| L_I64 { $$ = make_shared<I64Operand>(@1, $1); }
| L_U8 { $$ = make_shared<U8Operand>(@1, $1); }
| L_U16 { $$ = make_shared<U16Operand>(@1, $1); }
| L_U32 { $$ = make_shared<U32Operand>(@1, $1); }
| L_U64 { $$ = make_shared<U64Operand>(@1, $1); }
| L_F32 { $$ = make_shared<F32Operand>(@1, $1); }
| L_F64 { $$ = make_shared<F64Operand>(@1, $1); }
| L_STRING { $$ = make_shared<StringOperand>(@1, $1); }
| "true" { $$ = make_shared<BoolOperand>(@1, true); }
| "false" { $$ = make_shared<BoolOperand>(@1, false); }
| "null" { $$ = make_shared<NullOperand>(@1); }
;

Ref:
T_ID GenericArgs {
	$$ = make_shared<Ref>();
	$$->scopes.push_back(make_shared<RefScope>(@1, $1, $2));
}
| Ref "." T_ID GenericArgs {
	$$ = $1;
	$$->scopes.push_back(make_shared<RefScope>(@3, $3, $4));
}
;

%%

Slake::Assembler::parser::location_type yylloc;
shared_ptr<Slake::Assembler::parser> yyparser;
Slake::AccessModifier curAccess = 0;
unordered_map<string, uint32_t> curLabels;
deque<shared_ptr<Scope>> savedScopes;
