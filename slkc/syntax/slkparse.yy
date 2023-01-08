///
/// @file slkparse.yy
/// @author CodesBuilder (2602783536@qq.com)
/// @brief Parser for Slake
/// @version 0.1
/// @date 2022-11-21
///
/// @copyright Copyright (C) 2022 Slake Project
///
%require "3.2"

%{
#include <slkparse.hh>

Slake::Compiler::parser::symbol_type yylex();

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
extern Slake::Compiler::parser::location_type yylloc;
extern std::shared_ptr<Slake::Compiler::parser> yyparser;
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
%token <unsigned long long> L_ULONG "unsigned long long integer literal"
%token <float> L_FLOAT "single precision FP literal"
%token <double> L_DOUBLE "double precision FP literal"
%token <std::string> L_STRING

%token KW_ASYNC "async"
%token KW_AWAIT "await"
%token KW_BREAK "break"
%token KW_CASE "case"
%token KW_CATCH "catch"
%token KW_CLASS "class"
%token KW_CONST "const"
%token KW_CONTINUE "continue"
%token KW_DEFAULT "default"
%token KW_ELSE "else"
%token KW_ENUM "enum"
%token KW_FINAL "final"
%token KW_FN "fn"
%token KW_FOR "for"
%token KW_IF "if"
%token KW_IMPORT "import"
%token KW_INTERFACE "interface"
%token KW_NEW "new"
%token KW_NULL "null"
%token KW_OPERATOR "operator"
%token KW_PUB "pub"
%token KW_RETURN "return"
%token KW_SELF "self"
%token KW_SWITCH "switch"
%token KW_THROW "throw"
%token KW_TIMES "times"
%token KW_TRAIT "trait"
%token KW_TRY "try"
%token KW_WHILE "while"

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
%token OP_EQ "=="
%token OP_NEQ "!="
%token OP_LTEQ "<="
%token OP_GTEQ ">="
%token OP_LAND "&&"
%token OP_LOR "||"
%token OP_INC "++"
%token OP_DEC "--"
%token OP_INLINE_SW "=>"
%token OP_WRAP "->"

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

%precedence Call
%precedence Await

%expect 0

%type <AccessModifier> AccessModifier
%type <StorageModifier> StorageModifier
%type <std::shared_ptr<TypeName>> TypeName
%type <std::shared_ptr<TypeName>> InheritSlot
%type <std::shared_ptr<ImplList>> ImplementTypeNameList
%type <std::shared_ptr<ImplList>> ImplList
%type <std::shared_ptr<ParamDecl>> ParamDecl
%type <std::shared_ptr<ParamDeclList>> ParamDecls
%type <std::shared_ptr<ParamDeclList>> ParamDeclList
%type <std::shared_ptr<VarDefStmt>> VarDef
%type <std::shared_ptr<VarDeclList>> VarDecls
%type <std::shared_ptr<VarDecl>> VarDecl
%type <std::shared_ptr<CodeBlock>> Stmts
%type <std::shared_ptr<Stmt>> Stmt
%type <std::shared_ptr<ExprStmt>> ExprStmt
%type <std::shared_ptr<IfStmt>> IfBlock
%type <std::shared_ptr<SwitchStmt>> SwitchStmt
%type <std::shared_ptr<SwitchCase>> SwitchCase
%type <std::shared_ptr<SwitchCaseList>> SwitchCases
%type <std::shared_ptr<ForStmt>> ForBlock
%type <std::shared_ptr<VarDefStmt>> OptionalVarDef
%type <std::shared_ptr<Expr>> OptionalExpr
%type <std::shared_ptr<ExprStmt>> OptionalExprStmt
%type <std::shared_ptr<WhileStmt>> WhileBlock
%type <std::shared_ptr<TimesStmt>> TimesBlock
%type <std::shared_ptr<Stmt>> ElseBlock
%type <std::shared_ptr<CodeBlock>> CodeBlock
%type <std::shared_ptr<Expr>> Expr
%type <std::shared_ptr<RefExpr>> Ref
%type <std::shared_ptr<MapExpr>> MapExpr
%type <std::shared_ptr<PairList>> PairList
%type <std::shared_ptr<PairList>> Pairs
%type <std::shared_ptr<ExprPair>> Pair
%type <std::shared_ptr<UnaryOpExpr>> UnaryOpExpr
%type <std::shared_ptr<BinaryOpExpr>> BinaryOpExpr
%type <std::shared_ptr<TernaryOpExpr>> TernaryOpExpr
%type <std::shared_ptr<InlineSwitchExpr>> InlineSwitchExpr
%type <std::shared_ptr<InlineSwitchCase>> InlineSwitchCase
%type <std::shared_ptr<InlineSwitchCaseList>> InlineSwitchCases
%type <std::shared_ptr<CallExpr>> CallExpr
%type <std::shared_ptr<AwaitExpr>> AwaitExpr
%type <std::shared_ptr<ArgList>> Args
%type <std::shared_ptr<ArgList>> ArgList
%type <std::shared_ptr<LiteralExpr>> Literal

%%

//
// Program
//
Program:
ProgramStmt Program
| ProgramStmt
;

ProgramStmt:
FnDef
| ImportBlock
| VarDef ";"
| Class
| Interface
| Enum
;

//
// Modifiers
//
AccessModifier:
"pub" AccessModifier { $$ |= ACCESS_PUB; }
| "final" AccessModifier { $$ |= ACCESS_FINAL; }
| %empty {}
;

StorageModifier:
"const" StorageModifier { $$ |= STORAGE_CONST; }
| %empty {}
;

//
// Enumeration
//
Enum:
AccessModifier "enum" T_ID ":" TypeName "{" {
	if(currentScope->types.count($3))
		this->error(yylloc, "Redefinition of type `" + $3 + "`");
	else {
		currentEnum = make_shared<EnumType>($1, $3, $5);
		currentScope->types[$3] = currentEnum;
	}
} EnumPairs "}" {
	currentEnum = shared_ptr<EnumType>();
}
;

EnumPairs:
EnumPair "," EnumPairs
| EnumPair
;
EnumPair:
T_ID {
	if(currentEnum->pairs.count($1))
		this->error(yylloc, "Redefinition of enumeration constant `" + $1 + "`");
	else {
		if(currentEnum->pairs.size()) {
			currentEnum->pairs[$1]=calcConstExpr(
				make_shared<UnaryOpExpr>(
					(currentEnum->pairs.rbegin())->second->getLocation(),
					UnaryOp::INC_F,
					(currentEnum->pairs.rbegin())->second
				)
			);
		}
		else
			currentEnum->pairs[$1] = make_shared<IntLiteralExpr>(@1, 0);
	}
}
| T_ID "=" Expr {
	if(currentEnum->pairs.count($1))
		this->error(yylloc, "Redefinition of enumeration constant `" + $1 + "`");
	else
		currentEnum->pairs[$1] = $3;
}
;

//
// Class
//
Class:
AccessModifier "class" T_ID InheritSlot ImplList {
	auto curClass = make_shared<ClassType>($1, make_shared<Scope>(currentScope), $4, $3, $5);

	if(currentScope->types.count($3))
		this->error(yylloc, "Redefinition of type `" + $3 + "`");

	currentScope->types[$3] = curClass;
	currentScope = curClass->scope;
} "{" Program "}" {
	currentScope = currentScope->parent.lock();
}
;

InheritSlot:
%empty { $$ = shared_ptr<TypeName>(); }
| "(" TypeName ")" { $$ = $2; }
;

ImplList:
%empty { $$ = shared_ptr<ImplList>(); }
| ":" ImplementTypeNameList { $$ = $2; }
;

ImplementTypeNameList:
TypeName {
	$$ = make_shared<ImplList>();
	$$->impls.push_back($1);
}
| ImplementTypeNameList "," TypeName {
	$$ = $1;
	$$->impls.push_back($3);
}
;

//
// Interface
//
Interface:
AccessModifier "interface" T_ID InheritSlot {
	auto curType = make_shared<InterfaceType>(
		$1,
		make_shared<Scope>(currentScope),
		$4,
		$3
	);

	if(currentScope->types.count($3))
		this->error(yylloc, "Redefinition of type `" + $3 + "`");

	currentScope->types[$3] = curType;
	currentScope = curType->scope;
} "{" InterfaceStmts "}" {
	currentScope = currentScope->parent.lock();
}
;

InterfaceStmts:
InterfaceStmt InterfaceStmts
| InterfaceStmt
;

InterfaceStmt:
FnDecl
;

FnDecl:
TypeName T_ID "(" ParamDecls ")" ";" {
	if(currentScope->fnDefs.count($2))
		this->error(yylloc, "Redefinition of function `" + $2 + "`");
	else
		currentScope->fnDefs[$2] = make_shared<FnDef>($1->getLocation(), ACCESS_PUB, $4, $1);
}
| TypeName "operator" OperatorName "(" ParamDecls ")" ';' {
}
| "operator" TypeName ';' {
}
;

//
// Import
//
ImportBlock:
"import" "{" Imports "}"
;

Imports:
Import "," Imports
| Import
;

Import:
T_ID "=" L_STRING {
	// Redefinition check
	if(currentScope->imports.count($3))
		this->error(yylloc, "Redefinition of import item `" + $1 + "`");
	else
		currentScope->imports[$1] = make_shared<ImportItem>(@1, $3);
}
| T_ID "=" "@" L_STRING {
	// Redefinition check
	if(currentScope->imports.count($4))
		this->error(yylloc, "Redefinition of import item `" + $1 + "`");
	else
		currentScope->imports[$1] = make_shared<ImportItem>(@1, $4);
}
;

//
// Function
//
FnDef:
AccessModifier TypeName T_ID "(" ParamDecls ")" CodeBlock {
	if(currentScope->fnDefs.count($3))
		this->error(yylloc, "Redefinition of function `" + $3 + "`");
	else
		currentScope->fnDefs[$3] = make_shared<FnDef>(@1, $1, $5, $2, $7, $3);
}
| AccessModifier "new" "(" ParamDecls ")" CodeBlock {
	if(currentScope->fnDefs.count("new"))
		this->error(yylloc, "Redefinition of constructor");
	else
		currentScope->fnDefs["new"] = make_shared<FnDef>(@1, $1, $4, shared_ptr<TypeName>(), $6, "new");
}
| AccessModifier "~" "(" ParamDecls ")" CodeBlock {
	if(currentScope->fnDefs.count("delete"))
		this->error(yylloc, "Redefinition of constructor");
	else
		currentScope->fnDefs["delete"] = make_shared<FnDef>(@1, $1, $4, shared_ptr<TypeName>(), $6, "delete");
}
| AccessModifier TypeName "operator" OperatorName "(" ParamDecls ")" CodeBlock {
}
| AccessModifier "operator" TypeName CodeBlock {
}
;

OperatorName:
"+" {}
| "-" {}
| "*" {}
| "/" {}
| "%" {}
| "&" {}
| "|" {}
| "^" {}
| "&&" {}
| "||" {}
| "~" {}
| "!" {}
| "=" {}
| "+=" {}
| "-=" {}
| "*=" {}
| "/=" {}
| "%=" {}
| "&=" {}
| "|=" {}
| "^=" {}
| "~=" {}
| "==" {}
| "!=" {}
| ">" {}
| "<" {}
| ">=" {}
| "<=" {}
;

//
// Parameters
//
ParamDecls:
%empty { $$ = std::make_shared<ParamDeclList>(); }
| ParamDeclList { $$ = $1; }
;

ParamDeclList:
ParamDecl {
	$$ = make_shared<ParamDeclList>();
	$$->decls.push_back($1);
}
| ParamDeclList "," ParamDecl {
	$$ = $1;
	if ((*$$)[$3->name])
		this->error(yylloc, "Redefinition of parameter `" + $3->name + "`");
	else
		$$->decls.push_back($3);
}
;

ParamDecl:
TypeName T_ID {
	$$ = make_shared<ParamDecl>($1->getLocation(), $2, $1);
}
| TypeName T_ID "=" Expr {
	$$ = make_shared<ParamDecl>($1->getLocation(), $2, $1, $4);
}
;

//
// Variable
//
VarDef:
AccessModifier TypeName VarDecls {
	$$ = make_shared<VarDefStmt>($1, $2, $3);
}
;
VarDecls:
VarDecl {
	$$ = make_shared<VarDeclList>();
	$$->decls.push_back($1);
}
| VarDecls "," VarDecl {
	$$ = $1;
	if ((*$$)[$3->name])
		this->error(yylloc, "Redefinition of variable `" + $3->name + "`");
	else
		$$->decls.push_back($3);
}
;
VarDecl:
T_ID { $$ = make_shared<VarDecl>($1); }
| T_ID "=" Expr { $$ = make_shared<VarDecl>($1, $3); }
;

//
// Statement
//
Stmts:
Stmt {
	$$ = make_shared<CodeBlock>();
	$$->ins.push_back($1);
}
| Stmts Stmt {
	$$ = $1;
	$$->ins.push_back($2);
}
;

Stmt:
ExprStmt ";" { $$ = $1; }
| "return" ";" { $$ = make_shared<ReturnStmt>(); }
| "return" Expr ";" { $$ = make_shared<ReturnStmt>($2); }
| "continue" ";" { $$ = make_shared<ContinueStmt>(); }
| "break" ";" { $$ = make_shared<BreakStmt>(); }
| IfBlock { $$ = $1; }
| ForBlock { $$ = $1; }
| WhileBlock { $$ = $1; }
| TimesBlock { $$ = $1; }
| VarDef ";" { $$ = $1; }
| SwitchStmt { $$ = $1; }
;

TryBlock:
"try" CodeBlock CatchList FinalBlock {
}
;

CatchList:
CatchBlock {}
| CatchList CatchBlock {
}
;

CatchBlock:
"catch" "(" TypeName T_ID ")" CodeBlock {
}
;

FinalBlock:
"final" CodeBlock {
}
| %empty {}
;

ExprStmt:
Expr "," ExprStmt { $$ = make_shared<ExprStmt>($1, $3); }
| Expr { $$ = make_shared<ExprStmt>($1); }
;

IfBlock:
"if" "(" Expr ")" CodeBlock {
	$$ = make_shared<IfStmt>($3, $5, shared_ptr<Stmt>());
}
| "if" "(" Expr ")" CodeBlock ElseBlock {
	$$ = make_shared<IfStmt>($3, $5, $6);
}
;

ElseBlock:
"else" IfBlock { $$ = $2; }
| "else" CodeBlock { $$ = $2; }
;

CodeBlock:
"{" Stmts "}" { $$ = $2; }
;

SwitchStmt:
"switch" "(" Expr ")" "{" SwitchCases "}" {
	$$ = make_shared<SwitchStmt>($3, $6);
}
;

SwitchCases:
SwitchCase {
	$$ = make_shared<SwitchCaseList>();
	$$->push_back($1);
}
| SwitchCases SwitchCase {
	$$ = $1;
	$$->push_back($2);
}
;
SwitchCase:
"case" Expr ":" Stmt { $$ = make_shared<SwitchCase>($2, $4); }
| "default" ":" Stmt { $$ = make_shared<SwitchCase>(shared_ptr<Expr>(), $3); }
;

ForBlock:
"for" "(" OptionalVarDef ";" OptionalExpr ";" OptionalExprStmt ")" CodeBlock { $$ = make_shared<ForStmt>($3, $5, $7, $9); }
;

OptionalVarDef:
%empty { $$ = shared_ptr<VarDefStmt>(); }
| VarDef { $$ = $1; }
;
OptionalExpr:
%empty { $$ = shared_ptr<Expr>(); }
| Expr { $$ = $1; }
;
OptionalExprStmt:
%empty { $$ = shared_ptr<ExprStmt>(); }
| ExprStmt { $$ = $1; }
;

WhileBlock:
"while" "(" Expr ")" CodeBlock { $$ = make_shared<WhileStmt>($3, $5); }
;

TimesBlock:
"times" "(" Expr ")" CodeBlock { $$ = make_shared<TimesStmt>($3, $5); }

//
// Expression
//
Expr:
"(" Expr ")" { $$ = $2; }
| Literal { $$ = $1; }
| UnaryOpExpr { $$ = $1; }
| BinaryOpExpr { $$ = $1; }
| TernaryOpExpr { $$ = $1; }
| InlineSwitchExpr { $$ = $1; }
| Ref { $$ = $1; }
| CallExpr { $$ = $1; }
| AwaitExpr { $$ = $1; }
| MapExpr { $$ = $1; }
;

MapExpr:
"[" PairList "]" {
	$$ = make_shared<MapExpr>(@1, $2);
}
;

PairList:
%empty { $$ = shared_ptr<PairList>(); }
| Pairs { $$ = $1; }
;

Pairs:
Pair {
	$$ = make_shared<PairList>();
	$$->push_back($1);
}
| Pairs "," Pair {
	$$ = $1;
	$$->push_back($3);
}
;

Pair:
Expr ":" Expr {
	$$ = make_shared<ExprPair>($1->getLocation(), $1, $3);
}
;

TypeName:
"i8" { $$ = make_shared<TypeName>(@1, EvalType::I8); }
|"i16" { $$ = make_shared<TypeName>(@1, EvalType::I16); }
|"i32" { $$ = make_shared<TypeName>(@1, EvalType::I32); }
|"i64" { $$ = make_shared<TypeName>(@1, EvalType::I64); }
|"u8" { $$ = make_shared<TypeName>(@1, EvalType::U8); }
|"u16" { $$ = make_shared<TypeName>(@1, EvalType::U16); }
|"u32" { $$ = make_shared<TypeName>(@1, EvalType::U32); }
|"u64" { $$ = make_shared<TypeName>(@1, EvalType::U64); }
|"float" { $$ = make_shared<TypeName>(@1, EvalType::FLOAT); }
|"double" { $$ = make_shared<TypeName>(@1, EvalType::DOUBLE); }
|"string" { $$ = make_shared<TypeName>(@1, EvalType::STRING); }
|"auto" { $$ = make_shared<TypeName>(@1, EvalType::AUTO); }
|"void" { $$ = make_shared<TypeName>(@1, EvalType::NONE); }
|"@" Ref { $$ = make_shared<CustomTypeName>(@1, $2); }
| TypeName "[" "]" {}
;

UnaryOpExpr:
"!" Expr %prec "!" { $$ = make_shared<UnaryOpExpr>(@1, UnaryOp::NOT, $2);}
| "-" Expr %prec OP_NEG { $$ = make_shared<UnaryOpExpr>(@1, UnaryOp::NEG, $2);}
| "++" Expr { $$ = make_shared<UnaryOpExpr>(@1, UnaryOp::INC_F, $2);}
| "--" Expr { $$ = make_shared<UnaryOpExpr>(@1, UnaryOp::DEC_F, $2);}
;

BinaryOpExpr:
Expr "+" Expr { $$ = make_shared<BinaryOpExpr>($1->getLocation(), BinaryOp::ADD, $1, $3); }
| Expr "-" Expr { $$ = make_shared<BinaryOpExpr>($1->getLocation(), BinaryOp::SUB, $1, $3); }
| Expr "*" Expr { $$ = make_shared<BinaryOpExpr>($1->getLocation(), BinaryOp::MUL, $1, $3); }
| Expr "/" Expr { $$ = make_shared<BinaryOpExpr>($1->getLocation(), BinaryOp::DIV, $1, $3); }
| Expr "%" Expr { $$ = make_shared<BinaryOpExpr>($1->getLocation(), BinaryOp::MOD, $1, $3); }
| Expr "&" Expr { $$ = make_shared<BinaryOpExpr>($1->getLocation(), BinaryOp::AND, $1, $3); }
| Expr "|" Expr { $$ = make_shared<BinaryOpExpr>($1->getLocation(), BinaryOp::OR, $1, $3); }
| Expr "^" Expr { $$ = make_shared<BinaryOpExpr>($1->getLocation(), BinaryOp::XOR, $1, $3); }
| Expr "&&" Expr { $$ = make_shared<BinaryOpExpr>($1->getLocation(), BinaryOp::LAND, $1, $3); }
| Expr "||" Expr { $$ = make_shared<BinaryOpExpr>($1->getLocation(), BinaryOp::LOR, $1, $3); }
| Expr "==" Expr { $$ = make_shared<BinaryOpExpr>($1->getLocation(), BinaryOp::EQ, $1, $3); }
| Expr "!=" Expr { $$ = make_shared<BinaryOpExpr>($1->getLocation(), BinaryOp::NEQ, $1, $3); }
| Expr "<=" Expr { $$ = make_shared<BinaryOpExpr>($1->getLocation(), BinaryOp::LTEQ, $1, $3); }
| Expr ">=" Expr { $$ = make_shared<BinaryOpExpr>($1->getLocation(), BinaryOp::GTEQ, $1, $3); }
| Expr "=" Expr { $$ = make_shared<BinaryOpExpr>($1->getLocation(), BinaryOp::ASSIGN, $1, $3); }
| Expr "+=" Expr { $$ = make_shared<BinaryOpExpr>($1->getLocation(), BinaryOp::ADD_ASSIGN, $1, $3); }
| Expr "-=" Expr { $$ = make_shared<BinaryOpExpr>($1->getLocation(), BinaryOp::SUB_ASSIGN, $1, $3); }
| Expr "*=" Expr { $$ = make_shared<BinaryOpExpr>($1->getLocation(), BinaryOp::MUL_ASSIGN, $1, $3); }
| Expr "/=" Expr { $$ = make_shared<BinaryOpExpr>($1->getLocation(), BinaryOp::DIV_ASSIGN, $1, $3); }
| Expr "%=" Expr { $$ = make_shared<BinaryOpExpr>($1->getLocation(), BinaryOp::MOD_ASSIGN, $1, $3); }
| Expr "&=" Expr { $$ = make_shared<BinaryOpExpr>($1->getLocation(), BinaryOp::AND_ASSIGN, $1, $3); }
| Expr "|=" Expr { $$ = make_shared<BinaryOpExpr>($1->getLocation(), BinaryOp::OR_ASSIGN, $1, $3); }
| Expr "^=" Expr { $$ = make_shared<BinaryOpExpr>($1->getLocation(), BinaryOp::XOR_ASSIGN, $1, $3); }
;

TernaryOpExpr:
Expr "?" Expr ":" Expr %prec "?" { $$ = make_shared<TernaryOpExpr>($1->getLocation(), $1, $3, $5); }
;

InlineSwitchExpr:
Expr "=>" "{" InlineSwitchCases "}" {
	$$ = make_shared<InlineSwitchExpr>($1->getLocation(), $1, $4);
}
;
InlineSwitchCases:
InlineSwitchCase {
	$$ = make_shared<InlineSwitchCaseList>();
	$$->push_back($1);
}
| InlineSwitchCases "," InlineSwitchCase {
	$$ = $1;
	$$->push_back($3);
}
;
InlineSwitchCase:
Expr ":" Expr { $$ = make_shared<InlineSwitchCase>($1->getLocation(), $1, $3); }
| "default" ":" Expr { $$ = make_shared<InlineSwitchCase>(@1, shared_ptr<Expr>(), $3); }
;

CallExpr:
Expr "(" Args ")" %prec Call { $$ = make_shared<CallExpr>($1->getLocation(), $1, $3); }
| Expr "(" Args ")" "async" %prec Call { $$ = make_shared<CallExpr>($1->getLocation(), $1, $3, true); }
;

AwaitExpr:
"await" Expr %prec Await { $$ = make_shared<AwaitExpr>(@1, $2); }
;

Args:
%empty { $$ = shared_ptr<ArgList>(); }
| ArgList { $$ = $1; }
;

ArgList:
Expr {
	$$ = make_shared<ArgList>();
	$$->push_back($1);
}
| Args "," Expr {
	$$ = $1;
	$$->push_back($3);
}
;

Literal:
L_INT { $$ = make_shared<IntLiteralExpr>(@1, $1); }
| L_UINT { $$ = make_shared<UIntLiteralExpr>(@1, $1); }
| L_LONG { $$ = make_shared<LongLiteralExpr>(@1, $1); }
| L_ULONG { $$ = make_shared<ULongLiteralExpr>(@1, $1); }
| L_FLOAT { $$ = make_shared<FloatLiteralExpr>(@1, $1); }
| L_DOUBLE { $$ = make_shared<DoubleLiteralExpr>(@1, $1); }
| "null" { $$ = make_shared<NullLiteralExpr>(@1); }
| L_STRING { $$ = make_shared<StringLiteralExpr>(@1, $1); }
;

Ref:
"self" "." Ref {
	$$ = make_shared<RefExpr>(@1, "", $3);
}
| T_ID "." Ref {
	auto name = $1;
	$$ = make_shared<RefExpr>(@1, name, $3);
}
| "self" {
	$$ = make_shared<RefExpr>(@1, "");
}
| T_ID {
	auto name = $1;
	$$ = make_shared<RefExpr>(@1, name);
};
