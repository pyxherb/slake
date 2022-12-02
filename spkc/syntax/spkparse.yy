///
/// @file spkparse.yy
/// @author CodesBuilder (2602783536@qq.com)
/// @brief Parser for SpkC
/// @version 0.1
/// @date 2022-11-21
///
/// @copyright Copyright (C) 2022 Swampeak Project
///
%{
#include <spkparse.hh>
#include <compiler/common.hh>
#include <cstdio>
#include <cstdarg>

SpkC::Syntax::parser::symbol_type yylex();

using std::static_pointer_cast;
using std::make_shared;
using std::shared_ptr;
%}

%code requires {
#include <compiler/common.hh>
#include <cstdio>
#include <cstdarg>

using namespace SpkC::Syntax;
}
%code provides {
extern SpkC::Syntax::parser::location_type yylloc;
extern std::shared_ptr<SpkC::Syntax::parser> yyparser;
}

%locations
%language "c++"
%define api.namespace {SpkC::Syntax}
%define api.token.constructor
%define api.value.type variant

%define parse.assert
%define parse.trace
%define parse.error detailed
%define parse.lac full

%token <std::string> T_ID
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
%token KW_NULL "null"
%token KW_PUB "pub"
%token KW_RETURN "return"
%token KW_SELF "self"
%token KW_SWITCH "switch"
%token KW_TIMES "times"
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
%type <std::shared_ptr<TypeName>> ReturnType
%type <std::shared_ptr<TypeName>> InheritSlot
%type <std::shared_ptr<ImplList>> ImplementTypeNameList
%type <std::shared_ptr<ImplList>> ImplList
%type <std::shared_ptr<ParamDecl>> ParamDecl
%type <std::shared_ptr<ParamDeclList>> ParamDecls
%type <std::shared_ptr<ParamDeclList>> ParamDeclList
%type <std::shared_ptr<VarDefInstruction>> VarDef
%type <std::shared_ptr<VarDeclList>> VarDecls
%type <std::shared_ptr<VarDecl>> VarDecl
%type <std::shared_ptr<CodeBlock>> Instructions
%type <std::shared_ptr<Instruction>> Instruction
%type <std::shared_ptr<IfInstruction>> IfBlock
%type <std::shared_ptr<WhileInstruction>> WhileBlock
%type <std::shared_ptr<Instruction>> ElseBlock
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
// Statement
//
Stmts:
Stmt Stmts
| Stmt
;

Stmt:
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
"pub" AccessModifier {
	$$ |= ACCESS_PUB;
}
| "final" AccessModifier {
	$$ |= ACCESS_FINAL;
}
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
	auto name = $3;

	if(currentScope->getEnum(name))
		this->error(yylloc, "Redefinition of enumeration `" + name + "`");
	else {
		currentEnum = make_shared<Enum>(
			$1,
			$5
		);
		currentScope->enums[name] = currentEnum;
	}
} EnumPairs "}" {
	currentEnum = shared_ptr<Enum>();
}
;

EnumPairs:
EnumPair "," EnumPairs
| EnumPair
;
EnumPair:
T_ID {
	auto name = $1;
	if(currentEnum->pairs.count(name))
		this->error(yylloc, "Redefinition of enumeration constant `" + name + "`");
	else
		currentEnum->pairs[name] = shared_ptr<Expr>();
}
| T_ID "=" Expr {
	auto name = $1;
	if(currentEnum->pairs.count(name))
		this->error(yylloc, "Redefinition of enumeration constant `" + name + "`");
	else
		currentEnum->pairs[name] = $3;
}
;

//
// Class
//
Class:
AccessModifier "class" T_ID InheritSlot ImplList {
	auto name = $3;
	auto curClass = make_shared<Class>(
		$1,
		make_shared<Scope>(currentScope),
		$4,
		$5
	);

	if(currentScope->getClass(name))
		this->error(yylloc, "Redefinition of type `" + name + "`");

	currentScope->classes[name] = curClass;
	currentScope = curClass->scope;
} "{" Stmts "}" {
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
	auto curClass = make_shared<Class>(
		$1,
		make_shared<Scope>(currentScope),
		$4
	);
	auto name = $3;

	if(currentScope->getClass(name))
		this->error(yylloc, "Redefinition of type `" + name + "`");

	currentScope->classes[name] = curClass;
	currentScope = curClass->scope;
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
"fn" T_ID "(" ParamDecls ")" ReturnType ";" {
	auto name = $2;

	if(currentScope->getFn(name))
		this->error(yylloc, "Redefinition of function `" + name + "`");
	else
		currentScope->fnDefs[name] = make_shared<FnDef>(
			ACCESS_PUB,
			$4,
			$6
		);
}
| "fn" T_ID "(" ParamDecls ")" ReturnType CodeBlock {
	auto name = $2;

	if(currentScope->getFn(name))
		this->error(yylloc, "Redefinition of function `" + name + "`");
	else
		currentScope->fnDefs[name] = make_shared<FnDef>(
			ACCESS_PUB,
			$4,
			$6,
			$7
		);
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
	auto name = $1;
	auto path = $3;

	// Redefinition check
	if(currentScope->getImport(path))
		this->error(yylloc, "Redefinition of import item `" + name + "`");
	else
		currentScope->imports[name] = make_shared<ImportItem>(path);
}
| T_ID "=" "@" L_STRING {
	auto name = $1;
	auto path = $4;

	// Redefinition check
	if(currentScope->getImport(path))
		this->error(yylloc, "Redefinition of import item `" + name + "`");
	else
		currentScope->imports[path] = make_shared<ImportItem>(name, true);
}
;

//
// Function
//
FnDef:
AccessModifier "fn" T_ID "(" ParamDecls ")" ReturnType CodeBlock {
	auto name = $3;

	if(currentScope->getFn(name))
		this->error(yylloc, "Redefinition of function `" + name + "`");
	else
		currentScope->fnDefs[name] = make_shared<FnDef>(
			$1,
			$5,
			$7,
			$8
		);
}
| AccessModifier "fn" "@" "(" ParamDecls ")" CodeBlock {
	if(currentScope->getFn("@"))
		this->error(yylloc, "Redefinition of constructor");
	else
		currentScope->fnDefs["@"] = make_shared<FnDef>(
			$1,
			$5,
			shared_ptr<TypeName>(),
			$7
		);
}
| AccessModifier "fn" "~" "(" ParamDecls ")" CodeBlock {
	if(currentScope->getFn("~"))
		this->error(yylloc, "Redefinition of constructor");
	else
		currentScope->fnDefs["~"] = make_shared<FnDef>(
			$1,
			$5,
			shared_ptr<TypeName>(),
			$7
		);
}
;

ReturnType:
%empty { $$ = shared_ptr<TypeName>(); }
| ":" TypeName { $$ = $2; }
;

//
// Parameters
//
ParamDecls:
%empty { $$ = shared_ptr<ParamDeclList>(); }
| ParamDeclList { $$ = $1; }
;

ParamDeclList:
ParamDecl {
	$$ = make_shared<ParamDeclList>();
	auto decl = $1;

	$$->decls.push_back(decl);
}
| ParamDeclList "," ParamDecl {
	$$ = $1;
	auto decl = $3;

	if ((*$$)[decl->name])
		this->error(yylloc, "Redefinition of parameter `" + decl->name + "`");
	else
		$$->decls.push_back(decl);
}
;

ParamDecl:
TypeName T_ID {
	auto type = $1;
	auto name = $2;

	$$ = make_shared<ParamDecl>(name, type);
}
| TypeName T_ID "=" Expr {
	auto type = $1;
	auto name = $2;
	auto initValue = $4;

	$$ = make_shared<ParamDecl>(name, type, initValue);
}
;

//
// Variable
//
VarDef:
AccessModifier TypeName VarDecls {
	auto type = $2;
	auto varDecls = $3;
	$$ = make_shared<VarDefInstruction>($1, type, varDecls);
}
;
VarDecls:
VarDecl {
	auto decl = $1;

	$$ = make_shared<VarDeclList>();
	$$->decls.push_back(decl);
}
| VarDecls "," VarDecl {
	$$ = $1;
	auto decl = $3;

	if ((*$$)[decl->name])
		this->error(yylloc, "Redefinition of variable `" + decl->name + "`");
	else
		$$->decls.push_back(decl);
}
;
VarDecl:
T_ID {
	auto name = $1;

	$$ = make_shared<VarDecl>(name);
}
| T_ID "=" Expr {
	auto name = $1;
	auto initValue = $3;

	$$ = make_shared<VarDecl>(name, initValue);
}
;

//
// Instruction
//
Instructions:
Instruction {
	$$ = make_shared<CodeBlock>();
	$$->ins.push_back($1);
}
| Instructions Instruction {
	$$ = $1;
	$$->ins.push_back($2);
}
;

Instruction:
Expr ";" { $$ = make_shared<ExprInstruction>($1); }
| Expr "," Instruction {
	$$ = make_shared<ExprInstruction>(
		$1,
		$3
	);
}
| "return" Expr ";" { $$ = make_shared<ReturnInstruction>($2); }
| "continue" ";" { $$ = make_shared<ContinueInstruction>(); }
| "break" ";" { $$ = make_shared<BreakInstruction>(); }
| IfBlock { $$ = $1; }
| VarDef ";" { $$ = $1; }
;

IfBlock:
"if" "(" Expr ")" CodeBlock {
	$$ = make_shared<IfInstruction>(
		$3,
		$5,
		shared_ptr<Instruction>()
	);
}
| "if" "(" Expr ")" CodeBlock ElseBlock {
	$$ = make_shared<IfInstruction>(
		$3,
		$5,
		$6
	);
}
;

ElseBlock:
"else" IfBlock { $$ = $2; }
| "else" CodeBlock { $$ = $2; }
;

CodeBlock:
"{" Instructions "}" { $$ = $2; }
;

WhileBlock:
"while" "(" Expr ")" CodeBlock {
	$$ = new WhileInstruction($3, $5);
}
;

ForBlock:
"for" "(" VarDef ";" Expr ";" Expr ")" CodeBlock {

}
;

TimesBlock:
"times" "(" Expr ")" CodeBlock {

}

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
	$$ = make_shared<MapExpr>($2);
}
;

PairList:
%empty { $$ = shared_ptr<PairList>(); }
| Pairs { $$ = $1; }
;

Pairs:
Pair {
	$$ = make_shared<PairList>();
	$$->pairs.push_back($1);
}
| Pairs "," Pair {
	$$ = $1;
	$$->pairs.push_back($3);
}
;

Pair:
Expr ":" Expr {
	$$ = make_shared<ExprPair>($1, $3);
}
;

TypeName:
"i8" { $$ = make_shared<TypeName>(EvalType::I8); }
|"i16" { $$ = make_shared<TypeName>(EvalType::I16); }
|"i32" { $$ = make_shared<TypeName>(EvalType::I32); }
|"i64" { $$ = make_shared<TypeName>(EvalType::I64); }
|"u8" { $$ = make_shared<TypeName>(EvalType::U8); }
|"u16" { $$ = make_shared<TypeName>(EvalType::U16); }
|"u32" { $$ = make_shared<TypeName>(EvalType::U32); }
|"u64" { $$ = make_shared<TypeName>(EvalType::U64); }
|"float" { $$ = make_shared<TypeName>(EvalType::FLOAT); }
|"double" { $$ = make_shared<TypeName>(EvalType::DOUBLE); }
|"string" { $$ = make_shared<TypeName>(EvalType::STRING); }
|"auto" { $$ = make_shared<TypeName>(EvalType::AUTO); }
|"@" Ref { $$ = make_shared<CustomTypeName>($2); }
| TypeName "[" TypeName "]" {}
| TypeName "[" "]" {}
;

UnaryOpExpr:
"!" Expr %prec "!" { $$ = make_shared<UnaryOpExpr>(UnaryOp::NOT, $2);}
| "-" Expr %prec OP_NEG { $$ = make_shared<UnaryOpExpr>(UnaryOp::NEG, $2);}
| "++" Expr { $$ = make_shared<UnaryOpExpr>(UnaryOp::INC_F, $2);}
| "--" Expr { $$ = make_shared<UnaryOpExpr>(UnaryOp::DEC_F, $2);}
;

BinaryOpExpr:
Expr "+" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::ADD, $1, $3); }
| Expr "-" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::SUB, $1, $3); }
| Expr "*" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::MUL, $1, $3); }
| Expr "/" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::DIV, $1, $3); }
| Expr "%" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::MOD, $1, $3); }
| Expr "&" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::AND, $1, $3); }
| Expr "|" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::OR, $1, $3); }
| Expr "^" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::XOR, $1, $3); }
| Expr "&&" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::LAND, $1, $3); }
| Expr "||" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::LOR, $1, $3); }
| Expr "==" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::EQ, $1, $3); }
| Expr "!=" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::NEQ, $1, $3); }
| Expr "<=" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::LTEQ, $1, $3); }
| Expr ">=" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::GTEQ, $1, $3); }
| Expr "=" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::ASSIGN, $1, $3); }
| Expr "+=" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::ADD_ASSIGN, $1, $3); }
| Expr "-=" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::SUB_ASSIGN, $1, $3); }
| Expr "*=" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::MUL_ASSIGN, $1, $3); }
| Expr "/=" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::DIV_ASSIGN, $1, $3); }
| Expr "%=" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::MOD_ASSIGN, $1, $3); }
| Expr "&=" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::AND_ASSIGN, $1, $3); }
| Expr "|=" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::OR_ASSIGN, $1, $3); }
| Expr "^=" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::XOR_ASSIGN, $1, $3); }
;

TernaryOpExpr:
Expr "?" Expr ":" Expr %prec "?" {
	$$ = make_shared<TernaryOpExpr>(
		$1,
		$3,
		$5
	);
}
;

InlineSwitchExpr:
Expr "=>" "{" InlineSwitchCases "}" {
	$$ = make_shared<InlineSwitchExpr>(
		$1,
		$4
	);
}
;
InlineSwitchCases:
InlineSwitchCase {
	$$ = make_shared<InlineSwitchCaseList>();
	$$->cases.push_back(
		$1
	);
}
| InlineSwitchCases "," InlineSwitchCase {
	$$ = $1;
	$$->cases.push_back(
		$3
	);
}
;
InlineSwitchCase:
Expr ":" Expr {
	$$ = make_shared<InlineSwitchCase>(
		$1,
		$3
	);
}
| "default" ":" Expr {
	$$ = make_shared<InlineSwitchCase>(
		shared_ptr<Expr>(),
		$3
	);
}
;

CallExpr:
Expr "(" Args ")" %prec Call {
	$$ = make_shared<CallExpr>(
		$1,
		$3
	);
}
| Expr "(" Args ")" "async" %prec Call {
	$$ = make_shared<CallExpr>(
		$1,
		$3,
		true
	);
}
;

AwaitExpr:
"await" Expr %prec Await { $$ = make_shared<AwaitExpr>($2); }
;

Args:
%empty { $$ = shared_ptr<ArgList>(); }
| ArgList { $$ = $1; }
;

ArgList:
Expr {
	$$ = make_shared<ArgList>();
	$$->args.push_back($1);
}
| Args "," Expr {
	$$ = $1;
	$$->args.push_back($3);
}
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

Ref:
"self" "." Ref {
	$$ = make_shared<RefExpr>("", $3);
}
| T_ID "." Ref {
	auto name = $1;
	$$ = make_shared<RefExpr>(name, $3);
}
| "self" {
	$$ = make_shared<RefExpr>("");
}
| T_ID {
	auto name = $1;
	$$ = make_shared<RefExpr>(name);
};
