///
/// @file slkparse.yy
/// @author CodesBuilder (2602783536@qq.com)
/// @brief Parser for Slake
/// @version 0.1
/// @date 2022-11-21
///
/// @copyright Copyright (C) 2022 Slake Project
///
/// SPDX-License-Identifier: LGPL-3.0-only
///
%require "3.2"

%{
#include <slkparse.hh>
#include <slklex.h>

YY_DECL;

%}

%code requires {
#include <compiler/compiler.hh>
#include <cstdarg>

#pragma push_macro("new")

#undef new
}

%code provides {
extern Slake::Compiler::parser::location_type yylloc;
extern std::shared_ptr<Slake::Compiler::parser> yyparser;

#pragma pop_macro("new")
}

%locations
%language "c++"
%define api.namespace {Slake::Compiler}
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
%token <unsigned long long> L_ULONG "unsigned long integer literal"
%token <float> L_FLOAT "single precision FP literal"
%token <double> L_DOUBLE "double precision FP literal"
%token <std::string> L_STRING "string literal"

%token KW_ASYNC "async"
%token KW_AWAIT "await"
%token KW_BASE "base"
%token KW_BREAK "break"
%token KW_CASE "case"
%token KW_CATCH "catch"
%token KW_CLASS "class"
%token KW_CONST "const"
%token KW_CONTINUE "continue"
%token KW_DEFAULT "default"
%token KW_DELETE "delete"
%token KW_ELSE "else"
%token KW_ENUM "enum"
%token KW_FALSE "false"
%token KW_FINAL "final"
%token KW_FINALLY "finally"
%token KW_FN "fn"
%token KW_FOR "for"
%token KW_IF "if"
%token KW_INTERFACE "interface"
%token KW_NATIVE "native"
%token KW_NEW "new"
%token KW_NULL "null"
%token KW_OVERRIDE "override"
%token KW_OPERATOR "operator"
%token KW_PUB "pub"
%token KW_RETURN "return"
%token KW_STATIC "static"
%token KW_STRUCT "struct"
%token KW_SWITCH "switch"
%token KW_THIS "this"
%token KW_THROW "throw"
%token KW_TIMES "times"
%token KW_TRAIT "trait"
%token KW_TRUE "true"
%token KW_TRY "try"
%token KW_USE "use"
%token KW_VAR "var"
%token KW_WHILE "while"
%token KW_YIELD "yield"

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
%token TN_FLOAT "float"
%token TN_DOUBLE "double"
%token TN_STRING "string"
%token TN_BOOL "bool"
%token TN_AUTO "auto"
%token TN_ANY "any"
%token TN_VOID "void"

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
%token OP_ASSIGN "="
%token OP_ASSIGN_ADD "+="
%token OP_ASSIGN_SUB "-="
%token OP_ASSIGN_MUL "*="
%token OP_ASSIGN_DIV "/="
%token OP_ASSIGN_MOD "%="
%token OP_ASSIGN_AND "&="
%token OP_ASSIGN_OR "|="
%token OP_ASSIGN_XOR "^="
%token OP_ASSIGN_REV "~="
%token OP_ASSIGN_LSH ">>="
%token OP_ASSIGN_RSH "<<="
%token OP_ASSIGN_ACCESS ".="
%token OP_XCHG "<=>"
%token OP_EQ "=="
%token OP_NEQ "!="
%token OP_STRICTEQ "==="
%token OP_STRICTNEQ "!=="
%token OP_LT "<"
%token OP_GT ">"
%token OP_LTEQ "<="
%token OP_GTEQ ">="
%token OP_RSH ">>"
%token OP_LSH "<<"
%token OP_LAND "&&"
%token OP_LOR "||"
%token OP_INC "++"
%token OP_DEC "--"
%token OP_MATCH "=>"
%token OP_WRAP "->"
%token OP_SCOPE "::"
%token OP_DOLLAR "$"
%token OP_DIRECTIVE "#"

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

%right "=" "+=" "-=" "*=" "/=" "%=" "&=" "|=" "^=" "~=" "<<=" ">>="
%right "?" "=>"
%left "||"
%left "&&"
%left "|"
%left "^"
%left "&"
%left "==" "!="
%left "<<" ">>"
%left "<" "<=" ">" ">="
%left "+" "-"
%left "*" "/" "%"
%precedence "!" NegatePrec
%precedence "++" "--"
%nonassoc "(" ")" "[" "]"
%left "."

%precedence AwaitPrec
%precedence CallPrec

%precedence ForwardUnaryOpPrec
%precedence BackwardUnaryOpPrec

%precedence StaticRefPrec
%precedence RefPrec

%expect 0

%type <AccessModifier> AccessModifier
%type <std::shared_ptr<TypeName>> TypeName InheritSlot LiteralTypeName CustomTypeName FnTypeName ArrayTypeName MapTypeName
%type <std::shared_ptr<ImplList>> ImplementTypeNameList ImplList
%type <std::shared_ptr<ParamDecl>> ParamDecl
%type <std::shared_ptr<ParamDeclList>> ParamDecls ParamDeclList
%type <std::shared_ptr<VarDefStmt>> VarDef
%type <VarDeclList> VarDecls NativeVarDecls Aliases
%type <std::shared_ptr<VarDecl>> VarDecl NativeVarDecl
%type <std::shared_ptr<CodeBlock>> Stmts CodeBlock FinallyBlock
%type <std::shared_ptr<Stmt>> Stmt ElseBlock
%type <std::shared_ptr<ExprStmt>> ExprStmt OptionalExprStmt
%type <std::shared_ptr<IfStmt>> IfBlock
%type <std::shared_ptr<SwitchStmt>> SwitchStmt
%type <std::shared_ptr<SwitchCase>> SwitchCase
%type <std::shared_ptr<SwitchCaseList>> SwitchCases
%type <std::shared_ptr<ForStmt>> ForBlock
%type <std::shared_ptr<VarDefStmt>> OptionalVarDef
%type <std::shared_ptr<Expr>> Expr OptionalExpr
%type <std::shared_ptr<WhileStmt>> WhileBlock
%type <std::shared_ptr<TimesStmt>> TimesBlock
%type <std::shared_ptr<CastExpr>> CastExpr
%type <std::shared_ptr<NewExpr>> NewExpr
%type <std::shared_ptr<RefExpr>> Ref RefBody StaticRef StaticRefBody StaticRefTail
%type <std::shared_ptr<MapExpr>> MapExpr
%type <std::shared_ptr<PairList>> PairList Pairs
%type <std::shared_ptr<ExprPair>> Pair
%type <std::shared_ptr<SubscriptOpExpr>> SubscriptOpExpr
%type <std::shared_ptr<UnaryOpExpr>> UnaryOpExpr
%type <std::shared_ptr<BinaryOpExpr>> BinaryOpExpr
%type <std::shared_ptr<TernaryOpExpr>> TernaryOpExpr
%type <std::shared_ptr<MatchExpr>> MatchExpr
%type <std::shared_ptr<ArrayExpr>> ArrayExpr
%type <std::vector<std::shared_ptr<Expr>>> Exprs ExprList
%type <MatchCase> MatchCase
%type <MatchCaseList> MatchCases
%type <std::shared_ptr<CallExpr>> CallExpr
%type <std::shared_ptr<AwaitExpr>> AwaitExpr
%type <std::shared_ptr<ArgList>> Args ArgList
%type <std::shared_ptr<LiteralExpr>> Literal
%type <std::string> OperatorName GenericParam
%type <std::vector<std::string>> GenericParamList GenericParams
%type <std::vector<std::shared_ptr<CatchBlock>>> CatchList
%type <std::shared_ptr<TryStmt>> TryStmt
%type <std::shared_ptr<CatchBlock>> CatchBlock

%%

//
// Program
//
Program:
ProgramStmts
| %empty
;

ProgramStmts:
ProgramStmt ProgramStmts
| ProgramStmt
;

ProgramStmt:
FnDef
| ImportBlock
| VarDef ";" {
	currentScope->defineVars($1);
}
| Class
| Struct
| Interface
| Trait
| Enum
| AliasDef ";"
;

//
// Modifiers
//
AccessModifier:
"pub" AccessModifier {
	if($$ & ACCESS_PUB)
		this->error(@1, "Duplicated modifier");
	$$ |= ACCESS_PUB;
}
| "final" AccessModifier {
	if($$ & ACCESS_FINAL)
		this->error(@1, "Duplicated modifier");
	$$ |= ACCESS_FINAL;
}
| "const" AccessModifier {
	if($$ & ACCESS_CONST)
		this->error(@1, "Duplicated modifier");
	$$ |= ACCESS_CONST;
}
| "volatile" AccessModifier {
	if($$ & ACCESS_VOLATILE)
		this->error(@1, "Duplicated modifier");
	$$ |= ACCESS_VOLATILE;
}
| "override" AccessModifier {
	if($$ & ACCESS_OVERRIDE)
		this->error(@1, "Duplicated modifier");
	$$ |= ACCESS_OVERRIDE;
}
| "static" AccessModifier {
	if($$ & ACCESS_STATIC)
		this->error(@1, "Duplicated modifier");
	$$ |= ACCESS_STATIC;
}
| %empty {}
;

//
// Enumeration
//
Enum:
AccessModifier "enum" T_ID ":" TypeName "{" {
	if(currentScope->types.count($3))
		this->error(@3, "Redefinition of type `" + $3 + "`");
	else {
		currentEnum = std::make_shared<EnumType>(@1, $1, $3, $5);
		currentScope->types[$3] = currentEnum;
	}
} EnumPairs "}" {
	currentEnum.reset();
}
;

EnumPairs:
EnumPair "," EnumPairs
| EnumPair
;
EnumPair:
T_ID {
	if(currentEnum->pairs.count($1))
		this->error(@1, "Redefinition of enumeration constant `" + $1 + "`");
	currentEnum->pairs[$1] = std::make_shared<IntLiteralExpr>(@1, 0);
}
| T_ID "=" Expr {
	if(currentEnum->pairs.count($1))
		this->error(@1, "Redefinition of enumeration constant `" + $1 + "`");
	else
		currentEnum->pairs[$1] = $3;
}
;

//
// Class
//
Class:
AccessModifier "class" T_ID GenericParamList InheritSlot ImplList {
	auto curClass = std::make_shared<ClassType>(@1, $1, std::make_shared<Scope>($3, currentScope), $5, $3, $4, $6);

	if(currentScope->types.count($3))
		this->error(@3, "Redefinition of type `" + $3 + "`");

	currentScope->types[$3] = curClass;
	currentScope = curClass->scope;
} "{" Program "}" {
	currentScope = currentScope->parent.lock();
}
;

InheritSlot:
%empty { }
| "(" TypeName ")" { $$ = $2; }
;

ImplList:
%empty { $$ = std::make_shared<ImplList>(); }
| ":" ImplementTypeNameList { $$ = $2; }
;

ImplementTypeNameList:
TypeName {
	$$ = std::make_shared<ImplList>();
	$$->impls.push_back($1);
}
| ImplementTypeNameList "," TypeName {
	$$ = $1;
	$$->impls.push_back($3);
}
;

GenericParamList:
"<" GenericParams ">" { $$ = $2; }
| %empty {}
;

GenericParams:
GenericParam { $$.push_back($1); }
| GenericParams GenericParam {
	if(std::any_of($1.begin(), $1.end(), [this](decltype($2) &x)->bool { return x == $2; }))
		this->error(@2, "Redefinition of generic parameter `" + $2 + "`");
	$$ = $1;
	$$.push_back($2);
}
;

GenericParam:
T_ID { $$ = $1; }
;

//
// Structure
//
Struct:
AccessModifier "struct" T_ID "{" {
	if(currentScope->types.count($3))
		this->error(@3, "Redefinition of type `" + $3 + "`");
	else {
		currentStruct = std::make_shared<StructType>(@1, $1, $3);
		currentScope->types[$3] = currentStruct;
	}
} StructStmts "}" {
	currentStruct.reset();
}
| AccessModifier "struct" T_ID "{" "}" {
	this->error(@4, "Missing member definitions");
}
;

StructStmts:
StructStmt StructStmts
| StructStmt
;

StructStmt:
VarDef ";" {
	currentStruct->addMembers($1);
}
;

//
// Interface
//
Interface:
AccessModifier "interface" T_ID GenericParamList InheritSlot {
	auto curType = std::make_shared<InterfaceType>(
		@1,
		$1,
		std::make_shared<Scope>($3, currentScope),
		$5,
		$3,
		$4
	);

	if(currentScope->types.count($3))
		this->error(@3, "Redefinition of type `" + $3 + "`");

	currentScope->types[$3] = curType;
	currentScope = curType->scope;
} "{" InterfaceProgram "}" {
	currentScope = currentScope->parent.lock();
}
;

InterfaceProgram:
InterfaceStmts
| %empty
;

InterfaceStmts:
InterfaceStmt InterfaceStmts
| InterfaceStmt
;

InterfaceStmt:
FnDecl
;

//
// Trait
//
Trait:
AccessModifier "trait" T_ID GenericParamList InheritSlot {
}
;

FnDecl:
TypeName T_ID GenericParamList "(" ParamDecls ")" ";" {
	if(currentScope->fnDefs.count($2))
		this->error(@2, "Redefinition of function `" + $2 + "`");
	else
		currentScope->fnDefs[$2] = std::make_shared<FnDef>(@1, ACCESS_PUB, $5, $1, std::shared_ptr<CodeBlock>(), $2, $3);
}
| TypeName "operator" OperatorName GenericParamList "(" ParamDecls ")" ";" {
	if(currentScope->fnDefs.count($3)) {
		this->error(@2, "Redefinition of operator " + $3);
	}
	else
		currentScope->fnDefs[$3] = std::make_shared<FnDef>(@1, ACCESS_PUB, $6, $1, std::shared_ptr<CodeBlock>(), "operator" + $3, $4);
}
| "operator" TypeName ";" {
}
| NativeFnDecl {}
;

NativeFnDecl:
AccessModifier "native" TypeName T_ID GenericParamList "(" ParamDecls ")" ";" {
	if(currentScope->parent.expired())
		$1 |= ACCESS_STATIC;

	if(currentScope->fnDefs.count($4)) {
		this->error(@4, "Redefinition of function `" + $4 + "'");
	}
	else
		currentScope->fnDefs[$4] = std::make_shared<FnDef>(@1, $1 | ACCESS_NATIVE, $7, $3, std::shared_ptr<CodeBlock>(), $4, $5);
}
| AccessModifier "native" TypeName "operator" OperatorName GenericParamList "(" ParamDecls ")" ";" {
	if(currentScope->fnDefs.count($5)) {
		this->error(@5, "Redefinition of operator " + $5);
	}
	else
		currentScope->fnDefs[$5] = std::make_shared<FnDef>(@1, $1 | ACCESS_NATIVE, $8, $3, std::shared_ptr<CodeBlock>(), "operator" + $5, $6);
}
| AccessModifier "native" "operator" "new" "(" ParamDecls ")" ";" {
	if(currentScope->fnDefs.count("new")) {
		this->error(@1, "Redefinition of constructor");
	}
	else
		currentScope->fnDefs["new"] = std::make_shared<FnDef>(@1, $1 | ACCESS_NATIVE, $6, std::make_shared<TypeName>(@1, TypeNameKind::NONE), std::shared_ptr<CodeBlock>(), "operator new");
}
| AccessModifier "native" "operator" "delete" "(" ParamDecls ")" ";" {
	if(currentScope->fnDefs.count("delete")) {
		this->error(@1, "Redefinition of destructor");
	}
	else
		currentScope->fnDefs["delete"] = std::make_shared<FnDef>(@1, $1 | ACCESS_NATIVE, $6, std::make_shared<TypeName>(@1, TypeNameKind::NONE), std::shared_ptr<CodeBlock>(), "operator delete");
}
;

//
// Import
//
ImportBlock:
"use" "{" ImportAliases "}"
;

//
// Function
//
FnDef:
AccessModifier TypeName T_ID GenericParamList "(" ParamDecls ")" CodeBlock {
	if(currentScope->fnDefs.count($3))
		this->error(@3, "Redefinition of function `" + $3 + "`");
	else
		currentScope->fnDefs[$3] = std::make_shared<FnDef>(@1, $1, $6, $2, $8, $3, $4);
}
| AccessModifier TypeName "operator" OperatorName GenericParamList "(" ParamDecls ")" CodeBlock {
	if(currentScope->fnDefs.count($4)) {
		this->error(@4, "Redefinition of operator " + $4);
	}
	else
		currentScope->fnDefs[$4] = std::make_shared<FnDef>(@1, $1, $7, std::shared_ptr<TypeName>(), $9, "operator" + $4, $5);
}
| AccessModifier "operator" "new" "(" ParamDecls ")" CodeBlock {
	if(currentScope->fnDefs.count("new"))
		this->error(@1, "Redefinition of constructor");
	else
		currentScope->fnDefs["new"] = std::make_shared<FnDef>(@1, $1, $5, std::make_shared<TypeName>(@1, TypeNameKind::NONE), $7, "operator new");
}
| AccessModifier "operator" "delete" "(" ")" CodeBlock {
	if(currentScope->fnDefs.count("delete"))
		this->error(@1, "Redefinition of destructor");
	else
		currentScope->fnDefs["delete"] = std::make_shared<FnDef>(@1, $1, std::make_shared<ParamDeclList>(), std::make_shared<TypeName>(@1, TypeNameKind::NONE), $6, "operator delete");
}
| AccessModifier "operator" TypeName CodeBlock {
}
| NativeFnDecl {}
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

//
// Parameters
//
ParamDecls:
%empty { $$ = std::make_shared<ParamDeclList>(); }
| ParamDeclList { $$ = $1; }
| ParamDeclList "," "..." {
	$$ = $1;
	$$->push_back(std::make_shared<ParamDecl>(@3, "...", std::make_shared<ArrayTypeName>(@3, std::make_shared<TypeName>(@3, TypeNameKind::ANY))));
}
| "..." {
	$$ = std::make_shared<ParamDeclList>();
	$$->push_back(std::make_shared<ParamDecl>(@1, "...", std::make_shared<ArrayTypeName>(@1, std::make_shared<TypeName>(@1, TypeNameKind::ANY))));
}
;

ParamDeclList:
ParamDecl {
	$$ = std::make_shared<ParamDeclList>();
	$$->push_back($1);
}
| ParamDeclList "," ParamDecl {
	$$ = $1;
	if ((*$$)[$3->name])
		this->error(@3, "Redefinition of parameter `" + $3->name + "`");
	else
		$$->push_back($3);
}
;

ParamDecl:
TypeName T_ID {
	$$ = std::make_shared<ParamDecl>(@1, $2, $1);
}
;

//
// Variable
//
VarDef:
AccessModifier TypeName VarDecls {
	$$ = std::make_shared<VarDefStmt>($1, $2, $3);
}
| AccessModifier "native" TypeName NativeVarDecls {
	$$ = std::make_shared<VarDefStmt>($1, $3, $4);
	$$->accessModifier |= ACCESS_NATIVE;
}
;
VarDecls:
VarDecl { $$.push_back($1); }
| VarDecls "," VarDecl {
	$$ = $1;
	if ($$[$3->name])
		this->error(@3, "Redefinition of variable `" + $3->name + "`");
	else
		$$.push_back($3);
}
;
VarDecl:
T_ID { $$ = std::make_shared<VarDecl>(@1, $1); }
| T_ID "=" Expr { $$ = std::make_shared<VarDecl>(@1, $1, $3); }
| T_ID "[" Expr "]" { $$ = std::make_shared<VarDecl>(@1, $1); }
| T_ID "[" Expr "]" "=" Expr { $$ = std::make_shared<VarDecl>(@1, $1, $6); }
;
NativeVarDecls:
NativeVarDecl { $$.push_back($1); }
| NativeVarDecls NativeVarDecl {
	$$ = $1;
	$$.push_back($2);
}
;
NativeVarDecl:
T_ID { $$ = std::make_shared<VarDecl>(@1, $1, std::shared_ptr<Expr>()); }
;

AliasDef:
"pub" "use" Aliases {
}
| "use" Aliases {
}
;

Aliases:
T_ID "=" StaticRef "," Aliases {}
| T_ID "=" StaticRef {}
;

ImportAliases:
T_ID "=" StaticRef "," ImportAliases {
	if(currentScope->imports.count($1))
		this->error(@1, "Redefinition of import item `" + $3->name + "`");
	currentScope->imports[$1] = $3;
}
| T_ID "=" StaticRef {
	if(currentScope->imports.count($1))
		this->error(@1, "Redefinition of import item `" + $3->name + "`");
	currentScope->imports[$1] = $3;
}
;

//
// Statement
//
Stmts:
Stmt {
	$$ = std::make_shared<CodeBlock>(@1);
	$$->ins.push_back($1);
}
| Stmts Stmt {
	$$ = $1;
	$$->ins.push_back($2);
}
;

Stmt:
ExprStmt ";" { $$ = $1; }
| "return" ";" { $$ = std::make_shared<ReturnStmt>(@1); }
| "return" Expr ";" { $$ = std::make_shared<ReturnStmt>(@1, $2); }
| "continue" ";" { $$ = std::make_shared<ContinueStmt>(@1); }
| "break" ";" { $$ = std::make_shared<BreakStmt>(@1); }
| IfBlock { $$ = $1; }
| ForBlock { $$ = $1; }
| WhileBlock { $$ = $1; }
| TimesBlock { $$ = $1; }
| VarDef ";" { $$ = $1; }
| SwitchStmt { $$ = $1; }
| TryStmt {}
;

TryStmt:
"try" CodeBlock CatchList FinallyBlock { $$ = std::make_shared<TryStmt>(@1, $2, $3, $4); }
;
CatchList:
CatchBlock { $$.push_back($1); }
| CatchList CatchBlock { $$ = $1, $$.push_back($2); }
;
CatchBlock:
"catch" "(" TypeName T_ID ")" CodeBlock { $$=std::make_shared<CatchBlock>(@1, $6, $4, $3); }
| "catch" "(" "..." ")" CodeBlock { $$=std::make_shared<CatchBlock>(@1, $5); }
;
FinallyBlock:
"finally" CodeBlock { $$ = $2; }
| %empty {}
;

ExprStmt:
Expr "," ExprStmt { $$ = std::make_shared<ExprStmt>($1, $3); }
| Expr { $$ = std::make_shared<ExprStmt>($1); }
;

IfBlock:
"if" "(" Expr ")" CodeBlock {
	$$ = std::make_shared<IfStmt>(@1, $3, $5, std::shared_ptr<Stmt>());
}
| "if" "(" Expr ")" CodeBlock ElseBlock {
	$$ = std::make_shared<IfStmt>(@1, $3, $5, $6);
}
;

ElseBlock:
"else" IfBlock { $$ = $2; }
| "else" CodeBlock { $$ = $2; }
;

CodeBlock:
"{" Stmts "}" { $$ = $2; }
| "{" Stmts Expr "}" {  }
| "{" "}" { $$ = std::make_shared<CodeBlock>(@1); }
;

SwitchStmt:
"switch" "(" Expr ")" "{" SwitchCases "}" {
	$$ = std::make_shared<SwitchStmt>(@1, $3, $6);
}
;

SwitchCases:
SwitchCase {
	$$ = std::make_shared<SwitchCaseList>();
	$$->push_back($1);
}
| SwitchCases SwitchCase {
	$$ = $1;
	$$->push_back($2);
}
;
SwitchCase:
"case" Expr ":" Stmt { $$ = std::make_shared<SwitchCase>(@1, $2, $4); }
| "default" ":" Stmt { $$ = std::make_shared<SwitchCase>(@1, std::shared_ptr<Expr>(), $3); }
;

ForBlock:
"for" "(" OptionalVarDef ";" OptionalExpr ";" OptionalExprStmt ")" CodeBlock { $$ = std::make_shared<ForStmt>(@1, $3, $5, $7, $9); }
;

OptionalVarDef:
%empty { }
| VarDef { $$ = $1; }
;
OptionalExpr:
%empty { }
| Expr { $$ = $1; }
;
OptionalExprStmt:
%empty { }
| ExprStmt { $$ = $1; }
;

WhileBlock:
"while" "(" Expr ")" CodeBlock { $$ = std::make_shared<WhileStmt>(@1, $3, $5); }
;

TimesBlock:
"times" "(" Expr ")" CodeBlock { $$ = std::make_shared<TimesStmt>(@1, $3, $5); }

//
// Expression
//
Expr:
Ref { $$ = $1; }
| Literal { $$ = $1; }
| ArrayExpr { $$ = $1; }
| MapExpr { $$ = $1; }
| NewExpr { $$ = $1; }
| CastExpr { $$ = $1; }
| SubscriptOpExpr { $$ = $1; }
| "(" Expr ")" { $$ = $2; }
| UnaryOpExpr { $$ = $1; }
| BinaryOpExpr { $$ = $1; }
| TernaryOpExpr { $$ = $1; }
| MatchExpr { $$ = $1; }
| CallExpr { $$ = $1; }
| AwaitExpr { $$ = $1; }
| ClosureExpr {}
| "..." { $$ = std::make_shared<RefExpr>(@1, "...", false); }
;

ArrayExpr:
"{" Exprs "}" { $$ = std::make_shared<ArrayExpr>(@1, $2); }
;

Exprs:
ExprList { $$ = $1; }
| %empty {}
;

ExprList:
Expr { $$.push_back($1); }
| ExprList "," Expr {
	$$ = $1;
	$$.push_back($3);
}
;

CastExpr:
"<" TypeName ">" "(" Expr ")" { $$ = std::make_shared<CastExpr>(@1, $2, $5); }
;

NewExpr:
"new" TypeName "(" Args ")" { $$ = std::make_shared<NewExpr>(@1, $2, $4); }
;

MapExpr:
"[" PairList "]" {
	$$ = std::make_shared<MapExpr>(@1, $2);
}
;

ClosureExpr:
"fn" "(" ParamDecls ")" "->" TypeName CodeBlock {
}
| "fn" "(" ParamDecls ")" "async" "->" TypeName CodeBlock {
}
;

PairList:
%empty { }
| Pairs { $$ = $1; }
;

Pairs:
Pair {
	$$ = std::make_shared<PairList>();
	$$->push_back($1);
}
| Pairs "," Pair {
	$$ = $1;
	$$->push_back($3);
}
;

Pair:
Expr ":" Expr {
	$$ = std::make_shared<ExprPair>(@1, $1, $3);
}
;

GenericArgs:
"<" TypeNameList ">" {}
;

TypeNameList:
TypeName {}
| TypeNameList "," TypeName {}
;

TypeName:
LiteralTypeName { $$ = $1; }
| CustomTypeName { $$ = $1; }
| FnTypeName  { $$ = $1; }
| ArrayTypeName { $$ = $1; }
| MapTypeName { $$ = $1; }
;

LiteralTypeName:
"i8" { $$ = std::make_shared<TypeName>(@1, TypeNameKind::I8); }
|"i16" { $$ = std::make_shared<TypeName>(@1, TypeNameKind::I16); }
|"i32" { $$ = std::make_shared<TypeName>(@1, TypeNameKind::I32); }
|"i64" { $$ = std::make_shared<TypeName>(@1, TypeNameKind::I64); }
|"isize" { $$ = std::make_shared<TypeName>(@1, TypeNameKind::ISIZE); }
|"u8" { $$ = std::make_shared<TypeName>(@1, TypeNameKind::U8); }
|"u16" { $$ = std::make_shared<TypeName>(@1, TypeNameKind::U16); }
|"u32" { $$ = std::make_shared<TypeName>(@1, TypeNameKind::U32); }
|"u64" { $$ = std::make_shared<TypeName>(@1, TypeNameKind::U64); }
|"usize" { $$ = std::make_shared<TypeName>(@1, TypeNameKind::USIZE); }
|"float" { $$ = std::make_shared<TypeName>(@1, TypeNameKind::FLOAT); }
|"double" { $$ = std::make_shared<TypeName>(@1, TypeNameKind::DOUBLE); }
|"string" { $$ = std::make_shared<TypeName>(@1, TypeNameKind::STRING); }
|"auto" { $$ = std::make_shared<TypeName>(@1, TypeNameKind::AUTO); }
|"bool" { $$ = std::make_shared<TypeName>(@1, TypeNameKind::BOOL); }
|"void" { $$ = std::make_shared<TypeName>(@1, TypeNameKind::NONE); }
|"any" { $$ = std::make_shared<TypeName>(@1, TypeNameKind::ANY); }
;

CustomTypeName:
"@" StaticRef { $$ = std::make_shared<CustomTypeName>(@1, $2, currentScope); }
| "@" T_ID { $$ = std::make_shared<CustomTypeName>(@1, std::make_shared<RefExpr>(@2, $2, true), currentScope); }
;

FnTypeName:
TypeName "->" "(" ParamDecls ")" {
	auto typeName = std::make_shared<FnTypeName>(@1, $1, true);
	$$ = typeName;
	if ($4)
		for (auto &i : *$4)
			typeName->argTypes.push_back(i->typeName);
}
;

ArrayTypeName:
TypeName "[" "]" { $$ = std::make_shared<ArrayTypeName>(@1, $1); }
;

MapTypeName:
TypeName "[" TypeName "]" { $$ = std::make_shared<MapTypeName>(@1, $3, $1); }
;

SubscriptOpExpr:
Expr "[" Expr "]" { $$ = std::make_shared<SubscriptOpExpr>(@1, $1, $3); }
;

UnaryOpExpr:
"!" Expr %prec "!" { $$ = std::make_shared<UnaryOpExpr>(@1, UnaryOp::NOT, $2);}
| "-" Expr %prec NegatePrec { $$ = std::make_shared<UnaryOpExpr>(@1, UnaryOp::NEG, $2);}
| "++" Expr %prec ForwardUnaryOpPrec { $$ = std::make_shared<UnaryOpExpr>(@1, UnaryOp::INC_F, $2);}
| "--" Expr %prec ForwardUnaryOpPrec { $$ = std::make_shared<UnaryOpExpr>(@1, UnaryOp::DEC_F, $2);}
| Expr "++" %prec BackwardUnaryOpPrec { $$ = std::make_shared<UnaryOpExpr>(@1, UnaryOp::INC_B, $1);}
| Expr "--" %prec BackwardUnaryOpPrec { $$ = std::make_shared<UnaryOpExpr>(@1, UnaryOp::DEC_B, $1);}
;

BinaryOpExpr:
Expr "+" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::ADD, $1, $3); }
| Expr "-" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::SUB, $1, $3); }
| Expr "*" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::MUL, $1, $3); }
| Expr "/" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::DIV, $1, $3); }
| Expr "%" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::MOD, $1, $3); }
| Expr "&" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::AND, $1, $3); }
| Expr "|" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::OR, $1, $3); }
| Expr "^" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::XOR, $1, $3); }
| Expr "&&" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::LAND, $1, $3); }
| Expr "||" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::LOR, $1, $3); }
| Expr "==" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::EQ, $1, $3); }
| Expr "!=" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::NEQ, $1, $3); }
| Expr "<<" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::LSH, $1, $3); }
| Expr ">>" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::RSH, $1, $3); }
| Expr "<" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::LT, $1, $3); }
| Expr ">" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::GT, $1, $3); }
| Expr "<=" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::LTEQ, $1, $3); }
| Expr ">=" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::GTEQ, $1, $3); }
| Expr "=" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::ASSIGN, $1, $3); }
| Expr "+=" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::ADD_ASSIGN, $1, $3); }
| Expr "-=" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::SUB_ASSIGN, $1, $3); }
| Expr "*=" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::MUL_ASSIGN, $1, $3); }
| Expr "/=" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::DIV_ASSIGN, $1, $3); }
| Expr "%=" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::MOD_ASSIGN, $1, $3); }
| Expr "&=" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::AND_ASSIGN, $1, $3); }
| Expr "|=" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::OR_ASSIGN, $1, $3); }
| Expr "^=" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::XOR_ASSIGN, $1, $3); }
| Expr "<<=" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::LSH_ASSIGN, $1, $3); }
| Expr ">>=" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::RSH_ASSIGN, $1, $3); }
;

TernaryOpExpr:
Expr "?" Expr ":" Expr %prec "?" { $$ = std::make_shared<TernaryOpExpr>(@1, $1, $3, $5); }
;

MatchExpr:
Expr "=>" "{" MatchCases "}" {
	$$ = std::make_shared<MatchExpr>(@1, $1, $4);
}
;
MatchCases:
MatchCase {
	$$.push_back($1);
}
| MatchCases "," MatchCase {
	$$ = $1;
	$$.push_back($3);
}
;
MatchCase:
Expr ":" Expr { $$.first = $1, $$.second = $3; }
| "default" ":" Expr { $$.second = $3; }
;

CallExpr:
Expr "(" Args ")" %prec CallPrec { $$ = std::make_shared<CallExpr>(@1, $1, $3); }
| Expr "(" Args ")" "async" %prec CallPrec { $$ = std::make_shared<CallExpr>(@1, $1, $3, true); }
;

AwaitExpr:
"await" Expr %prec AwaitPrec { $$ = std::make_shared<AwaitExpr>(@1, $2); }
;

Args:
%empty { $$ = std::make_shared<ArgList>(); }
| ArgList { $$ = $1; }
;

ArgList:
Expr {
	$$ = std::make_shared<ArgList>();
	$$->push_back($1);
}
| Args "," Expr {
	$$ = $1;
	$$->push_back($3);
}
;

Literal:
L_INT { $$ = std::make_shared<IntLiteralExpr>(@1, $1); }
| L_UINT { $$ = std::make_shared<UIntLiteralExpr>(@1, $1); }
| L_LONG { $$ = std::make_shared<LongLiteralExpr>(@1, $1); }
| L_ULONG { $$ = std::make_shared<ULongLiteralExpr>(@1, $1); }
| L_FLOAT { $$ = std::make_shared<FloatLiteralExpr>(@1, $1); }
| L_DOUBLE { $$ = std::make_shared<DoubleLiteralExpr>(@1, $1); }
| L_STRING { $$ = std::make_shared<StringLiteralExpr>(@1, $1); }
| "true" { $$ = std::make_shared<IntLiteralExpr>(@1, 1); }
| "false" { $$ = std::make_shared<IntLiteralExpr>(@1, 0); }
| "null" { $$ = std::make_shared<NullLiteralExpr>(@1); }
;

RefBody:
T_ID "." RefBody { $$ = std::make_shared<RefExpr>(@1, $1, false, $3); }
| T_ID %prec RefPrec { $$ = std::make_shared<RefExpr>(@1, $1, false); }
;

StaticRefTail:
"::" T_ID { $$ = std::make_shared<RefExpr>(@1, $2, true); }
| "::" T_ID GenericArgs { $$ = std::make_shared<RefExpr>(@1, $2, true); }
;

StaticRefBody:
T_ID "::" StaticRefBody { $$ = std::make_shared<RefExpr>(@1, $1, true, $3); }
| T_ID GenericArgs "::" StaticRefBody { $$ = std::make_shared<RefExpr>(@1, $1, true, $4); }
| T_ID StaticRefTail { $$ = std::make_shared<RefExpr>(@1, $1, true, $2); }
| T_ID GenericArgs StaticRefTail { $$ = std::make_shared<RefExpr>(@1, $1, true, $3); }
;

StaticRef:
"::" StaticRefBody { $$ = std::make_shared<RefExpr>(@1, "", true, $2); }
| "base" "::" StaticRefBody { $$ = std::make_shared<RefExpr>(@1, "base", true, $3); }
| StaticRefBody { $$ = $1; }
;

Ref:
"this" "." RefBody { $$ = std::make_shared<RefExpr>(@1, "this", false, $3); }
| "this" { $$ = std::make_shared<RefExpr>(@1, "this", false); }
| "base" "." RefBody { $$ = std::make_shared<RefExpr>(@1, "base", false, $3); }
| StaticRef "." RefBody { $$ = $1, $$->next = $3; }
| RefBody { $$ = $1; }
;

%%

Slake::Compiler::parser::location_type yylloc;
std::shared_ptr<Slake::Compiler::parser> yyparser;
