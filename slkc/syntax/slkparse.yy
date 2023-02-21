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

%}

%code requires {
#include <compiler/compiler.hh>
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
%token <unsigned long long> L_ULONG "unsigned long integer literal"
%token <float> L_FLOAT "single precision FP literal"
%token <double> L_DOUBLE "double precision FP literal"
%token <std::string> L_STRING "string literal"
%token <Slake::Base::UUID> L_UUID "UUID"

%token KW_ASYNC "async"
%token KW_AWAIT "await"
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
%token KW_WHILE "while"

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
%token OP_ASSIGN_LSHIFT ">>="
%token OP_ASSIGN_RSHIFT "<<="
%token OP_ASSIGN_ACCESS ".="
%token OP_EQ "=="
%token OP_NEQ "!="
%token OP_STRICTEQ "==="
%token OP_STRICTNEQ "!=="
%token OP_LT "<"
%token OP_GT ">"
%token OP_LTEQ "<="
%token OP_GTEQ ">="
%token OP_RSHIFT ">>"
%token OP_LSHIFT "<<"
%token OP_LAND "&&"
%token OP_LOR "||"
%token OP_INC "++"
%token OP_DEC "--"
%token OP_INLINE_SW "=>"
%token OP_WRAP "->"
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
%precedence "!" OP_NEG
%precedence "++" "--"
%nonassoc "(" ")" "[" "]"

%precedence Call
%precedence Await

%precedence ForwardUnaryOp
%precedence BackwardUnaryOp

%expect 0

%type <AccessModifier> AccessModifier
%type <std::shared_ptr<TypeName>> TypeName InheritSlot
%type <std::shared_ptr<ImplList>> ImplementTypeNameList ImplList
%type <std::shared_ptr<ParamDecl>> ParamDecl
%type <std::shared_ptr<ParamDeclList>> ParamDecls ParamDeclList
%type <std::shared_ptr<VarDefStmt>> VarDef
%type <VarDeclList> VarDecls NativeVarDecls
%type <std::shared_ptr<VarDecl>> VarDecl NativeVarDecl
%type <std::shared_ptr<CodeBlock>> Stmts CodeBlock
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
%type <std::shared_ptr<CastExpr>> CastExpr RawCastExpr
%type <std::shared_ptr<NewExpr>> NewExpr
%type <std::shared_ptr<RefExpr>> Ref TypeNameRef
%type <std::shared_ptr<MapExpr>> MapExpr
%type <std::shared_ptr<PairList>> PairList Pairs
%type <std::shared_ptr<ExprPair>> Pair
%type <std::shared_ptr<SubscriptOpExpr>> SubscriptOpExpr
%type <std::shared_ptr<UnaryOpExpr>> UnaryOpExpr
%type <std::shared_ptr<BinaryOpExpr>> BinaryOpExpr
%type <std::shared_ptr<TernaryOpExpr>> TernaryOpExpr
%type <std::shared_ptr<InlineSwitchExpr>> InlineSwitchExpr
%type <std::shared_ptr<ArrayExpr>> ArrayExpr
%type <std::vector<std::shared_ptr<Expr>>> Exprs ExprList
%type <std::shared_ptr<InlineSwitchCase>> InlineSwitchCase
%type <std::shared_ptr<InlineSwitchCaseList>> InlineSwitchCases
%type <std::shared_ptr<CallExpr>> CallExpr
%type <std::shared_ptr<AwaitExpr>> AwaitExpr
%type <std::shared_ptr<ArgList>> Args ArgList
%type <std::shared_ptr<LiteralExpr>> Literal
%type <std::string> OperatorName GenericParam
%type <std::vector<std::string>> GenericParamList GenericParams

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
| Trait
| Enum
| AliasDef ";"
;

//
// Modifiers
//
AccessModifier:
"pub" AccessModifier { $$ |= ACCESS_PUB; }
| "final" AccessModifier { $$ |= ACCESS_FINAL; }
| "const" AccessModifier { $$ |= ACCESS_CONST; }
| "volatile" AccessModifier { $$ |= ACCESS_VOLATILE; }
| "override" AccessModifier { $$ |= ACCESS_OVERRIDE; }
| "static" AccessModifier { $$ |= ACCESS_STATIC; }
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
		this->error(yylloc, "Redefinition of enumeration constant `" + $1 + "`");
	else {
		if(currentEnum->pairs.size()) {
			currentEnum->pairs[$1] = evalConstExpr(
				std::make_shared<UnaryOpExpr>(
					(currentEnum->pairs.rbegin())->second->getLocation(),
					UnaryOp::INC_F,
					(currentEnum->pairs.rbegin())->second
				)
			);
		}
		else
			currentEnum->pairs[$1] = std::make_shared<IntLiteralExpr>(@1, 0);
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
AccessModifier "class" T_ID GenericParamList InheritSlot ImplList {
	auto curClass = std::make_shared<ClassType>(@1, $1, std::make_shared<Scope>(currentScope), $5, $3, $4, $6);

	if(currentScope->types.count($3))
		this->error(yylloc, "Redefinition of type `" + $3 + "`");

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
		this->error(yylloc, "Redefinition of generic parameter `" + $2 + "`");
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
		this->error(yylloc, "Redefinition of type `" + $3 + "`");
	else {
		currentStruct = std::make_shared<StructType>(@1, $1, $3);
		currentScope->types[$3] = currentStruct;
	}
} StructStmts "}" {
	currentStruct.reset();
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
// Trait
//
Trait:
AccessModifier "trait" T_ID GenericParamList InheritSlot {
	auto curType = std::make_shared<TraitType>(
		@1,
		$1,
		std::make_shared<Scope>(currentScope),
		$5,
		$3,
		$4
	);

	if(currentScope->types.count($3))
		this->error(yylloc, "Redefinition of type `" + $3 + "`");

	currentScope->types[$3] = curType;
	currentScope = curType->scope;
} "{" TraitProgram "}" {
	currentScope = currentScope->parent.lock();
}
;

TraitProgram:
TraitStmts
| %empty
;

TraitStmts:
TraitStmt TraitStmts
| TraitStmt
;

TraitStmt:
FnDecl
;

FnDecl:
TypeName T_ID "(" ParamDecls ")" ";" {
	if(currentScope->fnDefs.count($2))
		this->error(yylloc, "Redefinition of function `" + $2 + "`");
	else
		currentScope->fnDefs[$2] = std::make_shared<FnDef>(@1, ACCESS_PUB, $4, $1, std::shared_ptr<CodeBlock>(), $2);
}
| TypeName "operator" OperatorName "(" ParamDecls ")" ";" {
	if(currentScope->fnDefs.count($3)) {
		this->error(yylloc, "Redefinition of operator " + $3);
	}
	else
		currentScope->fnDefs[$3] = std::make_shared<FnDef>(@1, ACCESS_PUB, $5, $1, std::shared_ptr<CodeBlock>(), "operator" + $3);
}
| "operator" TypeName ";" {
}
| NativeFnDecl {}
;

NativeFnDecl:
AccessModifier "native" TypeName T_ID "(" ParamDecls ")" "=" L_UUID ";" {
	if(currentScope->fnDefs.count($4)) {
		this->error(yylloc, "Redefinition of function " + $4);
	}
	else
		currentScope->fnDefs[$4] = std::make_shared<FnDef>(@1, $1, $6, $3, $9, $4);
}
;

//
// Import
//
ImportBlock:
"use" "{" Imports "}"
;

Imports:
Import "," Imports
| Import
;

Import:
T_ID "=" Ref {
	// Redefinition check
	if(currentScope->imports.count($1))
		this->error(yylloc, "Redefinition of import item `" + $1 + "`");
	else
		currentScope->imports[$1] = std::make_shared<ImportItem>(@1, $3);
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
		currentScope->fnDefs[$3] = std::make_shared<FnDef>(@1, $1, $5, $2, $7, $3);
}
| AccessModifier TypeName "operator" OperatorName "(" ParamDecls ")" CodeBlock {
	if(currentScope->fnDefs.count($4)) {
		this->error(yylloc, "Redefinition of operator " + $4);
	}
	else
		currentScope->fnDefs[$4] = std::make_shared<FnDef>(@1, $1, $6, std::shared_ptr<TypeName>(), $8, "operator" + $4);
}
| AccessModifier "operator" "new" "(" ParamDecls ")" CodeBlock {
	if(currentScope->fnDefs.count("new"))
		this->error(yylloc, "Redefinition of constructor");
	else
		currentScope->fnDefs["new"] = std::make_shared<FnDef>(@1, $1, $5, std::shared_ptr<TypeName>(), $7, "operator new");
}
| AccessModifier "operator" "delete" "(" ")" CodeBlock {
	if(currentScope->fnDefs.count("delete"))
		this->error(yylloc, "Redefinition of destructor");
	else
		currentScope->fnDefs["delete"] = std::make_shared<FnDef>(@1, $1, std::make_shared<ParamDeclList>(), std::shared_ptr<TypeName>(), $6, "operator delete");
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
	$$->push_back(std::make_shared<ParamDecl>(@3, "...", std::make_shared<ArrayTypeName>(@3, std::make_shared<TypeName>(@3, EvalType::ANY))));
}
| "..." {
	$$ = std::make_shared<ParamDeclList>();
	$$->push_back(std::make_shared<ParamDecl>(@1, "...", std::make_shared<ArrayTypeName>(@1, std::make_shared<TypeName>(@1, EvalType::ANY))));
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
		this->error(yylloc, "Redefinition of parameter `" + $3->name + "`");
	else
		$$->push_back($3);
}
;

ParamDecl:
TypeName T_ID {
	$$ = std::make_shared<ParamDecl>(@1, $2, $1);
}
| TypeName T_ID "=" Expr {
	$$ = std::make_shared<ParamDecl>(@1, $2, $1, $4);
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
	$$ = std::make_shared<VarDefStmt>($1, $3, $4, true);
}
;
VarDecls:
VarDecl { $$.push_back($1); }
| VarDecls "," VarDecl {
	$$ = $1;
	if ($$[$3->name])
		this->error(yylloc, "Redefinition of variable `" + $3->name + "`");
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
T_ID "=" L_UUID { $$ = std::make_shared<VarDecl>(@1, $1, std::make_shared<UUIDLiteralExpr>(@3, $3)); }
| T_ID { $$ = std::make_shared<VarDecl>(@1, $1, std::shared_ptr<Expr>()); }
;

AliasDef:
"pub" "use" VarDecls {
}
| "use" VarDecls {
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
| TryBlock {}
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
"finally" CodeBlock {
}
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
"(" Expr ")" { $$ = $2; }
| Literal { $$ = $1; }
| SubscriptOpExpr { $$ = $1; }
| UnaryOpExpr { $$ = $1; }
| BinaryOpExpr { $$ = $1; }
| TernaryOpExpr { $$ = $1; }
| InlineSwitchExpr { $$ = $1; }
| Ref { $$ = $1; }
| CallExpr { $$ = $1; }
| AwaitExpr { $$ = $1; }
| MapExpr { $$ = $1; }
| NewExpr { $$ = $1; }
| CastExpr { $$ = $1; }
| RawCastExpr { $$ = $1; }
| "..." { $$ = std::make_shared<RefExpr>(@1, "..."); }
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
| ExprList Expr {
	$$ = $1;
	$$.push_back($2);
}
;

CastExpr:
"(" TypeName ")" Expr { $$ = std::make_shared<CastExpr>(@1, $2, $4); }
;

RawCastExpr:
"<" TypeName ">" "(" Expr ")" { $$ = std::make_shared<CastExpr>(@1, $2, $5); }
;

NewExpr:
"new" TypeName "(" Args ")" { $$ = std::make_shared<NewExpr>(@1, $2, $4); }
;

MapExpr:
"{" PairList "}" {
	$$ = std::make_shared<MapExpr>(@1, $2);
}
;

Closure:
TypeName "(" ParamDecls ")" CodeBlock {
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
| %empty {}
;

TypeNameList:
TypeName {}
| TypeNameList TypeName {}
;

TypeName:
"i8" { $$ = std::make_shared<TypeName>(@1, EvalType::I8); }
|"i16" { $$ = std::make_shared<TypeName>(@1, EvalType::I16); }
|"i32" { $$ = std::make_shared<TypeName>(@1, EvalType::I32); }
|"i64" { $$ = std::make_shared<TypeName>(@1, EvalType::I64); }
|"isize" { $$ = std::make_shared<TypeName>(@1, EvalType::ISIZE); }
|"u8" { $$ = std::make_shared<TypeName>(@1, EvalType::U8); }
|"u16" { $$ = std::make_shared<TypeName>(@1, EvalType::U16); }
|"u32" { $$ = std::make_shared<TypeName>(@1, EvalType::U32); }
|"u64" { $$ = std::make_shared<TypeName>(@1, EvalType::U64); }
|"usize" { $$ = std::make_shared<TypeName>(@1, EvalType::USIZE); }
|"float" { $$ = std::make_shared<TypeName>(@1, EvalType::FLOAT); }
|"double" { $$ = std::make_shared<TypeName>(@1, EvalType::DOUBLE); }
|"string" { $$ = std::make_shared<TypeName>(@1, EvalType::STRING); }
|"auto" { $$ = std::make_shared<TypeName>(@1, EvalType::AUTO); }
|"bool" { $$ = std::make_shared<TypeName>(@1, EvalType::U8); }
|"void" { $$ = std::make_shared<TypeName>(@1, EvalType::NONE); }
|"@" TypeNameRef { $$ = std::make_shared<CustomTypeName>(@1, $2, currentScope); }
| TypeName "[" "]" { $$ = std::make_shared<ArrayTypeName>(@1, $1); }
| "@" "[" TypeName ":" TypeName "]" {}
| TypeName "->" "(" ParamDecls ")" {
	auto typeName = std::make_shared<FnTypeName>(@1, $1);
	$$ = typeName;
	if ($4)
		for (auto &i : *$4)
			typeName->argTypes.push_back(i->typeName);
}
;

SubscriptOpExpr:
Expr "[" Expr "]" { $$ = std::make_shared<SubscriptOpExpr>(@1, $1, $3); }
;

UnaryOpExpr:
"!" Expr %prec "!" { $$ = std::make_shared<UnaryOpExpr>(@1, UnaryOp::NOT, $2);}
| "-" Expr %prec OP_NEG { $$ = std::make_shared<UnaryOpExpr>(@1, UnaryOp::NEG, $2);}
| "++" Expr %prec ForwardUnaryOp { $$ = std::make_shared<UnaryOpExpr>(@1, UnaryOp::INC_F, $2);}
| "--" Expr %prec ForwardUnaryOp { $$ = std::make_shared<UnaryOpExpr>(@1, UnaryOp::DEC_F, $2);}
| Expr "++" %prec BackwardUnaryOp { $$ = std::make_shared<UnaryOpExpr>(@1, UnaryOp::INC_B, $1);}
| Expr "--" %prec BackwardUnaryOp { $$ = std::make_shared<UnaryOpExpr>(@1, UnaryOp::DEC_B, $1);}
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
| Expr "<<" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::LSHIFT, $1, $3); }
| Expr ">>" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::RSHIFT, $1, $3); }
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
| Expr "<<=" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::LSHIFT_ASSIGN, $1, $3); }
| Expr ">>=" Expr { $$ = std::make_shared<BinaryOpExpr>(@1, BinaryOp::RSHIFT_ASSIGN, $1, $3); }
;

TernaryOpExpr:
Expr "?" Expr ":" Expr %prec "?" { $$ = std::make_shared<TernaryOpExpr>(@1, $1, $3, $5); }
;

InlineSwitchExpr:
Expr "=>" "{" InlineSwitchCases "}" {
	$$ = std::make_shared<InlineSwitchExpr>(@1, $1, $4);
}
;
InlineSwitchCases:
InlineSwitchCase {
	$$ = std::make_shared<InlineSwitchCaseList>();
	$$->push_back($1);
}
| InlineSwitchCases "," InlineSwitchCase {
	$$ = $1;
	$$->push_back($3);
}
;
InlineSwitchCase:
Expr ":" Expr { $$ = std::make_shared<InlineSwitchCase>(@1, $1, $3); }
| "default" ":" Expr { $$ = std::make_shared<InlineSwitchCase>(@1, std::shared_ptr<Expr>(), $3); }
;

CallExpr:
Expr "(" Args ")" %prec Call { $$ = std::make_shared<CallExpr>(@1, $1, $3); }
| Expr "(" Args ")" "async" %prec Call { $$ = std::make_shared<CallExpr>(@1, $1, $3, true); }
;

AwaitExpr:
"await" Expr %prec Await { $$ = std::make_shared<AwaitExpr>(@1, $2); }
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

Ref:
"this" "." Ref { $$ = std::make_shared<RefExpr>(@1, "this", $3); }
| "this" { $$ = std::make_shared<RefExpr>(@1, "this"); }
| T_ID "." Ref { $$ = std::make_shared<RefExpr>(@1, $1, $3); }
| T_ID { $$ = std::make_shared<RefExpr>(@1, $1);}
;

TypeNameRef:
"this" "." TypeNameRef { $$ = std::make_shared<RefExpr>(@1, "", $3); }
| "this" { $$ = std::make_shared<RefExpr>(@1, ""); }
| "@" "." TypeNameRef { $$ = std::make_shared<RefExpr>(@1, "@", $3); }
| "@" { $$ = std::make_shared<RefExpr>(@1, "@"); }
| T_ID GenericArgs "." Ref { $$ = std::make_shared<RefExpr>(@1, $1, $4); }
| T_ID GenericArgs { $$ = std::make_shared<RefExpr>(@1, $1); }
;

%%

Slake::Compiler::parser::location_type yylloc;
std::shared_ptr<Slake::Compiler::parser> yyparser;
