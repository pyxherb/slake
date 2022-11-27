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
#include <syntax/token.hh>
#include <compiler/common.hh>
#include <cstdio>
#include <cstdarg>

int yylex();
void yyerror(const char* fmt, ...);

using namespace SpkC::Syntax;
using std::static_pointer_cast;
using std::make_shared;
using std::shared_ptr;

int yydebug = 1;
%}

%code requires {
#include <syntax/token.hh>
#include <compiler/common.hh>
#include <cstdio>
#include <cstdarg>

void yyerror(const char* fmt, ...);
}

%define parse.error verbose
%locations

%token T_ID
%token T_VARARG "..."

%token L_INT
%token L_UINT
%token L_LONG
%token L_ULONG
%token L_FLOAT
%token L_DOUBLE
%token L_STRING

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

%right '=' "+=" "-=" "*=" "/=" "%=" "&=" "|=" "^=" "~="
%right '?' "=>"
%left "||"
%left "&&"
%left '|'
%left '^'
%left '&'
%left "==" "!="
%left '<' "<=" '>' ">="
%left '+' '-'
%left '*' '/' '%'
%precedence '!' OP_NEG "++" "--"
%nonassoc '(' ')'

%precedence Call
%precedence Await

%expect 0

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
| VarDef ';'
| Class
| Interface
| Enum
;

//
// Modifiers
//
AccessModifier:
"pub" AccessModifier {
	if(!$$)
		$$ = make_shared<Token<AccessModifier>>(0);
	static_pointer_cast<Token<AccessModifier>>($$)->data |= ACCESS_PUB;
}
| "final" AccessModifier {
	if(!$$)
		$$ = make_shared<Token<AccessModifier>>(0);
	static_pointer_cast<Token<AccessModifier>>($$)->data |= ACCESS_FINAL;
}
| %empty {
	if(!$$)
		$$ = make_shared<Token<AccessModifier>>(0);
}
;

StorageModifier:
"const" StorageModifier { static_pointer_cast<Token<StorageModifier>>($$) |= STORAGE_CONST; }
| %empty {}
;

//
// Enumeration
//
Enum:
AccessModifier "enum" T_ID ':' TypeName '{' {
	auto name = static_pointer_cast<Token<std::string>>($3);

	if(currentScope->getEnum(name->data))
		yyerror("Redefinition of enumeration `%s`", name->data.c_str());
	else {
		currentEnum = make_shared<Enum>(
			static_pointer_cast<Token<AccessModifier>>($1)->data,
			static_pointer_cast<TypeName>($5)
		);
		currentScope->enums[name->data] = currentEnum;
	}
} EnumPairs '}' {
	currentEnum = shared_ptr<Enum>();
}
;

EnumPairs:
EnumPair ',' EnumPairs
| EnumPair
;
EnumPair:
T_ID {
	auto name = static_pointer_cast<Token<std::string>>($1)->data;
	if(currentEnum->pairs.count(name))
		yyerror("Redefinition of enumeration constant `%s`", name.c_str());
	else
		currentEnum->pairs[name] = shared_ptr<Expr>();
}
| T_ID '=' Expr {
	auto name = static_pointer_cast<Token<std::string>>($1)->data;
	if(currentEnum->pairs.count(name))
		yyerror("Redefinition of enumeration constant `%s`", name.c_str());
	else
		currentEnum->pairs[name] = static_pointer_cast<Expr>($3);
}
;

//
// Class
//
Class:
AccessModifier "class" T_ID InheritSlot ImplList {
	auto name = static_pointer_cast<Token<std::string>>($3)->data;
	auto curClass = make_shared<Class>(
		static_pointer_cast<Token<AccessModifier>>($1)->data,
		make_shared<Scope>(currentScope),
		static_pointer_cast<TypeName>($4),
		static_pointer_cast<ImplList>($5)
	);

	if(currentScope->getClass(name))
		yyerror("Redefinition of type `%s`", name.c_str());

	currentScope->classes[name] = curClass;
	currentScope = curClass->scope;
} '{' Stmts '}' {
	currentScope = currentScope->parent;
}
;

InheritSlot:
%empty { $$ = shared_ptr<IToken>(); }
| '(' TypeName ')' { $$ = $2; }
;

ImplList:
%empty { $$ = shared_ptr<IToken>(); }
| ':' ImplementTypeNameList { $$ = $2; }
;

ImplementTypeNameList:
TypeName {
	$$ = make_shared<ImplList>();
	static_pointer_cast<ImplList>($$)->impls.push_back(static_pointer_cast<TypeName>($1));
}
| ImplementTypeNameList ',' TypeName {
	$$ = $1;
	static_pointer_cast<ImplList>($$)->impls.push_back(static_pointer_cast<TypeName>($3));
}
;

//
// Interface
//
Interface:
AccessModifier "interface" T_ID InheritSlot {
	auto curClass = make_shared<Class>(
		static_pointer_cast<Token<AccessModifier>>($1)->data,
		make_shared<Scope>(currentScope),
		static_pointer_cast<TypeName>($4)
	);
	auto name = static_pointer_cast<Token<std::string>>($3)->data;

	if(currentScope->getClass(name))
		yyerror("Redefinition of type `%s`", name.c_str());

	currentScope->classes[name] = curClass;
	currentScope = curClass->scope;
} '{' InterfaceStmts '}' {
	currentScope = currentScope->parent;
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
"fn" T_ID '(' ParamDecls ')' ReturnType ';' {
	auto name = static_pointer_cast<Token<std::string>>($2)->data;

	if(currentScope->getFn(name))
		yyerror("Redefinition of function `%s`", name.c_str());
	else
		currentScope->fnDefs[name] = make_shared<FnDef>(
			ACCESS_PUB,
			static_pointer_cast<ParamDeclList>($4),
			static_pointer_cast<TypeName>($6)
		);
}
| "fn" T_ID '(' ParamDecls ')' ReturnType CodeBlock {
	auto name = static_pointer_cast<Token<std::string>>($2)->data;

	if(currentScope->getFn(name))
		yyerror("Redefinition of function `%s`", name.c_str());
	else
		currentScope->fnDefs[name] = make_shared<FnDef>(
			ACCESS_PUB,
			static_pointer_cast<ParamDeclList>($4),
			static_pointer_cast<TypeName>($6),
			static_pointer_cast<CodeBlock>($7)
		);
}
;

//
// Import
//
ImportBlock:
"import" '{' Imports '}'
;

Imports:
Import ',' Imports
| Import
;

Import:
T_ID '=' L_STRING {
	auto name = static_pointer_cast<Token<std::string>>($1)->data;
	auto path = static_pointer_cast<Token<std::string>>($3)->data;

	// Redefinition check
	if(currentScope->getImport(path))
		yyerror("Redefinition of import `%s`", path.c_str());
	else
		currentScope->imports[name] = make_shared<ImportItem>(path);
}
| T_ID '=' '@' L_STRING {
	auto name = static_pointer_cast<Token<std::string>>($1)->data;
	auto path = static_pointer_cast<Token<std::string>>($4)->data;

	// Redefinition check
	if(currentScope->getImport(path))
		yyerror("Redefinition of import `%s`", path.c_str());
	else
		currentScope->imports[path] = make_shared<ImportItem>(name, true);
}
;

//
// Function
//
FnDef:
AccessModifier "fn" T_ID '(' ParamDecls ')' ReturnType CodeBlock {
	auto name = static_pointer_cast<Token<std::string>>($3)->data;

	if(currentScope->getFn(name))
		yyerror("Redefinition of function `%s`", name.c_str());
	else
		currentScope->fnDefs[name] = make_shared<FnDef>(
			static_pointer_cast<Token<AccessModifier>>($1)->data,
			static_pointer_cast<ParamDeclList>($5),
			static_pointer_cast<TypeName>($7),
			static_pointer_cast<CodeBlock>($8)
		);
}
| AccessModifier "fn" '@' '(' ParamDecls ')' CodeBlock {
	if(currentScope->getFn("@"))
		yyerror("Redefinition of constructor");
	else
		currentScope->fnDefs["@"] = make_shared<FnDef>(
			static_pointer_cast<Token<AccessModifier>>($1)->data,
			static_pointer_cast<ParamDeclList>($5),
			shared_ptr<TypeName>(),
			static_pointer_cast<CodeBlock>($7)
		);
}
| AccessModifier "fn" '~' '(' ParamDecls ')' CodeBlock {
	if(currentScope->getFn("~"))
		yyerror("Redefinition of constructor");
	else
		currentScope->fnDefs["~"] = make_shared<FnDef>(
			static_pointer_cast<Token<AccessModifier>>($1)->data,
			static_pointer_cast<ParamDeclList>($5),
			shared_ptr<TypeName>(),
			static_pointer_cast<CodeBlock>($7)
		);
}
;

ReturnType:
%empty { $$ = shared_ptr<IToken>(); }
| ':' TypeName { $$ = $2; }
;

//
// Parameters
//
ParamDecls:
%empty { $$ = shared_ptr<IToken>(); }
| ParamDeclList { $$ = $1; }
;

ParamDeclList:
ParamDecl {
	$$ = make_shared<ParamDeclList>();
	auto decl = static_pointer_cast<ParamDecl>($1);

	static_pointer_cast<ParamDeclList>($$)->decls.push_back(decl);
}
| ParamDeclList ',' ParamDecl {
	$$ = $1;
	auto decl = static_pointer_cast<ParamDecl>($3);

	if ((*static_pointer_cast<ParamDeclList>($$))[decl->name])
		yyerror("Redefinition of parameter `%s`", decl->name.c_str());
	else
		static_pointer_cast<ParamDeclList>($$)->decls.push_back(decl);
}
;

ParamDecl:
TypeName T_ID {
	auto type = static_pointer_cast<TypeName>($1);
	auto name = static_pointer_cast<Token<std::string>>($2)->data;

	$$ = make_shared<ParamDecl>(name, type);
}
| TypeName T_ID '=' Expr {
	auto type = static_pointer_cast<TypeName>($1);
	auto name = static_pointer_cast<Token<std::string>>($2)->data;
	auto initValue = static_pointer_cast<Expr>($4);

	$$ = make_shared<ParamDecl>(name, type, initValue);
}
;

//
// Variable
//
VarDef:
AccessModifier TypeName VarDecls {
	auto type = static_pointer_cast<TypeName>($2);
	auto varDecls = static_pointer_cast<VarDeclList>($3);
	$$ = make_shared<VarDefInstruction>(static_pointer_cast<Token<AccessModifier>>($1)->data, type, varDecls);
}
;
VarDecls:
VarDecl {
	auto decl = static_pointer_cast<VarDecl>($1);

	$$ = make_shared<VarDeclList>();
	static_pointer_cast<VarDeclList>($$)->decls.push_back(decl);
}
| VarDecls ',' VarDecl {
	$$ = $1;
	auto decl = static_pointer_cast<VarDecl>($3);

	if ((*static_pointer_cast<VarDeclList>($$))[decl->name])
		yyerror("Redefinition of variable `%s`", decl->name.c_str());
	else
		static_pointer_cast<VarDeclList>($$)->decls.push_back(decl);
}
;
VarDecl:
T_ID {
	auto name = static_pointer_cast<Token<std::string>>($1)->data;

	$$ = make_shared<VarDecl>(name);
}
| T_ID '=' Expr {
	auto name = static_pointer_cast<Token<std::string>>($1)->data;
	auto initValue = static_pointer_cast<Expr>($3);

	$$ = make_shared<VarDecl>(name, initValue);
}
;

//
// Instruction
//
Instructions:
Instruction {
	$$ = make_shared<CodeBlock>();
	static_pointer_cast<CodeBlock>($$)->ins.push_back(static_pointer_cast<Instruction>($1));
}
| Instructions Instruction {
	$$ = $1;
	static_pointer_cast<CodeBlock>($$)->ins.push_back(static_pointer_cast<Instruction>($2));
}
;

Instruction:
Expr ';' { $$ = make_shared<ExprInstruction>(static_pointer_cast<Expr>($1)); }
| Expr ',' Instruction {
	$$ = make_shared<ExprInstruction>(
		static_pointer_cast<Expr>($1),
		static_pointer_cast<Instruction>($3)
	);
}
| "return" Expr ';' { $$ = make_shared<ReturnInstruction>(static_pointer_cast<Expr>($2)); }
| "continue" ';' { $$ = make_shared<ContinueInstruction>(); }
| "break" ';' { $$ = make_shared<BreakInstruction>(); }
| IfBlock { $$ = $1; }
| VarDef ';' { $$ = $1; }
;

IfBlock:
"if" '(' Expr ')' CodeBlock {
	$$ = make_shared<IfInstruction>(
		static_pointer_cast<Expr>($3),
		static_pointer_cast<CodeBlock>($5),
		shared_ptr<Instruction>()
	);
}
| "if" '(' Expr ')' CodeBlock ElseBlock {
	$$ = make_shared<IfInstruction>(
		static_pointer_cast<Expr>($3),
		static_pointer_cast<CodeBlock>($5),
		static_pointer_cast<Instruction>($6)
	);
}
;

ElseBlock:
"else" IfBlock { $$ = $2; }
| "else" CodeBlock { $$ = $2; }
;

CodeBlock:
'{' Instructions '}' { $$ = $2; }
;

WhileBlock:
"while" '(' Expr ')' CodeBlock {
	$$ = new WhileInstruction($3, $5);
}
;

ForBlock:
"for" '(' VarDef ';' Expr ';' Expr ')' CodeBlock {

}
;

TimesBlock:
"times" '(' Expr ')' CodeBlock {

}

//
// Expression
//
Expr:
'(' Expr ')' { $$ = $2; }
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
'[' PairList ']' {
	$$ = make_shared<MapExpr>(static_pointer_cast<PairList>($2));
}
;

PairList:
%empty { $$ = shared_ptr<IToken>(); }
| Pairs { $$ = $1; }
;

Pairs:
Pair {
	$$ = make_shared<PairList>();
	static_pointer_cast<PairList>($$)->pairs.push_back(static_pointer_cast<ExprPair>($1));
}
| Pairs ',' Pair {
	$$ = $1;
	static_pointer_cast<PairList>($$)->pairs.push_back(static_pointer_cast<ExprPair>($3));
}
;

Pair:
Expr ':' Expr {
	$$ = make_shared<ExprPair>(static_pointer_cast<Expr>($1), static_pointer_cast<Expr>($3));
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
|'@' Ref { $$ = make_shared<CustomTypeName>(static_pointer_cast<RefExpr>($2)); }
| TypeName '[' TypeName ']' {}
| TypeName '[' ']' {}
;

UnaryOpExpr:
'!' Expr %prec '!' { $$ = make_shared<UnaryOpExpr>(UnaryOp::NOT, static_pointer_cast<Expr>($2));}
| '-' Expr %prec OP_NEG { $$ = make_shared<UnaryOpExpr>(UnaryOp::NEG, static_pointer_cast<Expr>($2));}
| "++" Expr { $$ = make_shared<UnaryOpExpr>(UnaryOp::INC_F, static_pointer_cast<Expr>($2));}
| "--" Expr { $$ = make_shared<UnaryOpExpr>(UnaryOp::DEC_F, static_pointer_cast<Expr>($2));}
;

BinaryOpExpr:
Expr '+' Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::ADD, static_pointer_cast<Expr>($1), static_pointer_cast<Expr>($3)); }
| Expr '-' Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::SUB, static_pointer_cast<Expr>($1), static_pointer_cast<Expr>($3)); }
| Expr '*' Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::MUL, static_pointer_cast<Expr>($1), static_pointer_cast<Expr>($3)); }
| Expr '/' Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::DIV, static_pointer_cast<Expr>($1), static_pointer_cast<Expr>($3)); }
| Expr '%' Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::MOD, static_pointer_cast<Expr>($1), static_pointer_cast<Expr>($3)); }
| Expr '&' Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::AND, static_pointer_cast<Expr>($1), static_pointer_cast<Expr>($3)); }
| Expr '|' Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::OR, static_pointer_cast<Expr>($1), static_pointer_cast<Expr>($3)); }
| Expr '^' Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::XOR, static_pointer_cast<Expr>($1), static_pointer_cast<Expr>($3)); }
| Expr "&&" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::LAND, static_pointer_cast<Expr>($1), static_pointer_cast<Expr>($3)); }
| Expr "||" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::LOR, static_pointer_cast<Expr>($1), static_pointer_cast<Expr>($3)); }
| Expr "==" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::EQ, static_pointer_cast<Expr>($1), static_pointer_cast<Expr>($3)); }
| Expr "!=" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::NEQ, static_pointer_cast<Expr>($1), static_pointer_cast<Expr>($3)); }
| Expr "<=" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::LTEQ, static_pointer_cast<Expr>($1), static_pointer_cast<Expr>($3)); }
| Expr ">=" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::GTEQ, static_pointer_cast<Expr>($1), static_pointer_cast<Expr>($3)); }
| Expr '=' Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::ASSIGN, static_pointer_cast<Expr>($1), static_pointer_cast<Expr>($3)); }
| Expr "+=" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::ADD_ASSIGN, static_pointer_cast<Expr>($1), static_pointer_cast<Expr>($3)); }
| Expr "-=" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::SUB_ASSIGN, static_pointer_cast<Expr>($1), static_pointer_cast<Expr>($3)); }
| Expr "*=" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::MUL_ASSIGN, static_pointer_cast<Expr>($1), static_pointer_cast<Expr>($3)); }
| Expr "/=" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::DIV_ASSIGN, static_pointer_cast<Expr>($1), static_pointer_cast<Expr>($3)); }
| Expr "%=" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::MOD_ASSIGN, static_pointer_cast<Expr>($1), static_pointer_cast<Expr>($3)); }
| Expr "&=" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::AND_ASSIGN, static_pointer_cast<Expr>($1), static_pointer_cast<Expr>($3)); }
| Expr "|=" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::OR_ASSIGN, static_pointer_cast<Expr>($1), static_pointer_cast<Expr>($3)); }
| Expr "^=" Expr { $$ = make_shared<BinaryOpExpr>(BinaryOp::XOR_ASSIGN, static_pointer_cast<Expr>($1), static_pointer_cast<Expr>($3)); }
;

TernaryOpExpr:
Expr '?' Expr ':' Expr %prec '?' {
	$$ = make_shared<TernaryOpExpr>(
		static_pointer_cast<Expr>($1),
		static_pointer_cast<Expr>($3),
		static_pointer_cast<Expr>($5)
	);
}
;

InlineSwitchExpr:
Expr "=>" '{' InlineSwitchCases '}' {
	$$ = make_shared<InlineSwitchExpr>(
		static_pointer_cast<Expr>($1),
		static_pointer_cast<InlineSwitchCaseList>($4)
	);
}
;
InlineSwitchCases:
InlineSwitchCase {
	$$ = make_shared<InlineSwitchCaseList>();
	static_pointer_cast<InlineSwitchCaseList>($$)->cases.push_back(
		static_pointer_cast<InlineSwitchCase>($1)
	);
}
| InlineSwitchCases ',' InlineSwitchCase {
	$$ = $1;
	static_pointer_cast<InlineSwitchCaseList>($$)->cases.push_back(
		static_pointer_cast<InlineSwitchCase>($3)
	);
}
;
InlineSwitchCase:
Expr ':' Expr {
	$$ = make_shared<InlineSwitchCase>(
		static_pointer_cast<Expr>($1),
		static_pointer_cast<Expr>($3)
	);
}
| "default" ':' Expr {
	$$ = make_shared<InlineSwitchCase>(
		shared_ptr<Expr>(),
		static_pointer_cast<Expr>($3)
	);
}
;

CallExpr:
Expr '(' Args ')' %prec Call {
	$$ = make_shared<CallExpr>(
		static_pointer_cast<Expr>($1),
		static_pointer_cast<ArgList>($3)
	);
}
| Expr '(' Args ')' "async" %prec Call {
	$$ = make_shared<CallExpr>(
		static_pointer_cast<Expr>($1),
		static_pointer_cast<ArgList>($3),
		true
	);
}
;

AwaitExpr:
"await" Expr %prec Await { $$ = make_shared<AwaitExpr>(static_pointer_cast<Expr>($2)); }
;

Args:
%empty { $$ = shared_ptr<IToken>(); }
| ArgList { $$ = $1; }
;

ArgList:
Expr {
	$$ = make_shared<ArgList>();
	static_pointer_cast<ArgList>($$)->args.push_back(static_pointer_cast<Expr>($1));
}
| Args ',' Expr {
	$$ = $1;
	static_pointer_cast<ArgList>($$)->args.push_back(static_pointer_cast<Expr>($3));
}
;

Literal:
L_INT { $$ = make_shared<IntLiteralExpr>(static_pointer_cast<Token<int>>($1)->data); }
| L_UINT { $$ = make_shared<UIntLiteralExpr>(static_pointer_cast<Token<unsigned int>>($1)->data); }
| L_LONG { $$ = make_shared<LongLiteralExpr>(static_pointer_cast<Token<long long>>($1)->data); }
| L_ULONG { $$ = make_shared<ULongLiteralExpr>(static_pointer_cast<Token<unsigned long long>>($1)->data); }
| L_FLOAT { $$ = make_shared<FloatLiteralExpr>(static_pointer_cast<Token<float>>($1)->data); }
| L_DOUBLE { $$ = make_shared<DoubleLiteralExpr>(static_pointer_cast<Token<double>>($1)->data); }
| "null" { $$ = make_shared<NullLiteralExpr>(); }
| L_STRING { $$ = make_shared<StringLiteralExpr>(static_pointer_cast<Token<std::string>>($1)->data); }
;

Ref:
"self" '.' Ref {
	$$ = make_shared<RefExpr>("", static_pointer_cast<RefExpr>($3));
}
| T_ID '.' Ref {
	auto name = static_pointer_cast<Token<std::string>>($1)->data;
	$$ = make_shared<RefExpr>(name, static_pointer_cast<RefExpr>($3));
}
| "self" {
	$$ = make_shared<RefExpr>("");
}
| T_ID {
	auto name = static_pointer_cast<Token<std::string>>($1)->data;
	$$ = make_shared<RefExpr>(name);
};
