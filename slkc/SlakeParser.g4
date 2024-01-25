parser grammar SlakeParser;
options {
	tokenVocab = SlakeLexer;
}

prog: moduleDecl? imports? progStmt* EOF;
progStmt:
	access? fnDecl ';'		# ProgFnDecl
	| access? fnDef			# ProgFnDef
	| access? classDef		# ProgClassDef
	| access? varDef ';'	# ProgVarDef;

imports: 'use' '{' importItem (',' importItem)* '}';
importItem: ID '=' ref;

params: ((paramDecl (',' paramDecl)*)? (',' '...')? | '...'?);
paramDecl: 'const'? typeName ID;

moduleDecl: 'module' ref ';';

fnDecl: genericParams? typeName ID '(' params ')';
fnDef: fnDecl stmt;
stmt:
	varDef ';'																# VarDefStmt
	| 'break' ';'															# BreakStmt
	| 'continue' ';'														# ContinueStmt
	| 'for' '(' varDef? ';' condition = expr ';' endExpr = expr? ')' stmt	# ForStmt
	| 'while' '(' expr ')' stmt												# WhileStmt
	| 'return' expr? ';'													# ReturnStmt
	| 'yield' expr? ';'														# YieldStmt
	| 'if' varDef? '(' expr ')' stmt elseBranch?							# IfStmt
	| 'try' stmt catchBlock+ finalBlock?									# TryStmt
	| 'switch' '(' expr ')' '{' switchCase+ defaultBranch? '}'				# SwitchStmt
	| codeBlock																# CodeBlockStmt
	| expr ';'																# ExprStmt;

elseBranch: 'else' stmt;

catchBlock:
	'catch' ('(' type = typeName varName = ID ')')? stmt;
finalBlock: 'final' stmt;

switchCase: 'case' expr ':' stmt*;
defaultBranch: 'default' ':' stmt*;

varDef: typeName varDefEntry (',' varDefEntry)*;
varDefEntry: ID ('=' expr)?;

accessModifiers:
	'pub'			# PubAccess
	| 'final'		# FinalAccess
	| 'const'		# ConstAccess
	| 'override'	# OverrideAccess
	| 'static'		# StaticAccess
	| 'native'		# NativeAccess;
access: accessModifiers+;

classDef:
	'class' ID genericParams? inheritSlot? implementList? '{' classStmts* '}';
classStmts:
	access? fnDecl ';'				# ClassFnDecl
	| access? fnDef					# ClassFnDef
	| access? operatorDecl ';'		# ClassOperatorDecl
	| access? operatorDef			# ClassOperatorDef
	| access? constructorDecl ';'	# ClassConstructorDecl
	| access? destructorDecl ';'	# ClassDestructorDecl
	| access? constructorDef		# ClassConstructorDef
	| access? destructorDef			# ClassDestructorDef
	| access? varDef ';'			# ClassVarDef
	| access? classDef				# ClassClassDef;

genericParams: '<' genericParam (',' genericParam)* '>';
genericParam: ID baseSpec? traitSpec? interfaceSpec?;
baseSpec: '(' typeName ')'; // Base class specifier
traitSpec: '[' typeName (',' typeName)* ']'; // Trait specifier
interfaceSpec:
	':' typeName (',' typeName)*; // Interface specifier

inheritSlot: '(' customTypeName ')';
implementList: ':' (customTypeName (',' customTypeName)*)?;

operatorDecl: genericParams? typeName 'operator' operatorName '(' params ')';
operatorDef: operatorDecl codeBlock;

constructorDecl: 'operator' 'new' '(' params ')';
constructorDef: constructorDecl codeBlock;

destructorDecl: 'operator' 'delete' '(' ')';
destructorDef: destructorDecl codeBlock;

interfaceDef:
	'interface' ID inheritSlot '{' interfaceStmts* '}';
interfaceStmts:
	fnDecl ';'			# InterfaceFnDecl
	| operatorDecl ';'	# InterfaceOperatorDecl;

traitDef: 'trait' ID inheritSlot '{' traitStmts '}';
traitStmts:
	fnDecl ';'			# TraitFnDecl
	| operatorDecl ';'	# TraitOperatorDecl;

operatorName:
	'+'			# OperatorAdd
	| '-'		# OperatorSub
	| '*'		# OperatorMul
	| '/'		# OperatorDiv
	| '%'		# OperatorMod
	| '&'		# OperatorAnd
	| '|'		# OperatorOr
	| '^'		# OperatorXor
	| '&&'		# OperatorLAnd
	| '||'		# OperatorLOr
	| '~'		# OperatorRev
	| '!'		# OperatorNot
	| '='		# OperatorAssign
	| '+='		# OperatorAddAssign
	| '-='		# OperatorSubAssign
	| '*='		# OperatorMulAssign
	| '/='		# OperatorDivAssign
	| '%='		# OperatorModAssign
	| '&='		# OperatorAndAssign
	| '|='		# OperatorOrAssign
	| '^='		# OperatorXorAssign
	| '=='		# OperatorEq
	| '!='		# OperatorNeq
	| '>'		# OperatorGt
	| '<'		# OperatorLt
	| '>='		# OperatorGtEq
	| '<='		# OperatorLtEq
	| '[' ']'	# OperatorSubscript
	| '(' ')'	# OperatorCall;

codeBlock: '{' stmt* '}';

args: expr (',' expr)*;

expr:
	'(' expr ')'													# WrappedExpr
	| ref															# RefExpr
	| literal														# LiteralExpr
	| array															# ArrayExpr
	| map															# MapExpr
	| typeName '(' params ')' ('[' expr+ ']')? '->' stmt			# ClosureExpr
	| expr '(' args? ')' 'async'?									# CallExpr
	| 'await' expr													# AwaitExpr
	| 'new' typeName '(' args? ')'									# NewExpr
	| 'new' typeName array											# NewArrayExpr
	| 'new' typeName map											# NewMapExpr
	| expr '=>' '{' pair (',' pair)* (',' 'default' ':' expr)? '}'	# MatchExpr
	| '(' typeName ')' expr											# CastExpr
	| 'typeof' '(' expr ')'											# TypeofExpr
	| 'typeof' '(' typeName ')'										# TypeTypeofExpr
	| expr '[' expr ']'												# SubscriptExpr
	| op = ('++' | '--') expr										# ForwardIncDecExpr
	| expr op = ('++' | '--')										# BackwardIncDecExpr
	| '!' expr														# NotExpr
	| <assoc = left> expr op = ('*' | '/' | '%') expr				# MulDivExpr
	| <assoc = left> expr op = ('+' | '-') expr						# AddSubExpr
	| <assoc = left> expr op = ('<' | '>' | '<=' | '>=') expr		# LtGtExpr
	| <assoc = left> expr op = ('<<' | '>>') expr					# ShiftExpr
	| <assoc = left> expr op = ('==' | '!=') expr					# EqExpr
	| <assoc = left> expr '&' expr									# AndExpr
	| <assoc = left> expr '^' expr									# XorExpr
	| <assoc = left> expr '|' expr									# OrExpr
	| <assoc = left> expr '&&' expr									# LogicalAndExpr
	| <assoc = left> expr '||' expr									# LogicalOrExpr
	| <assoc = right> expr '?' expr ':' expr						# TernaryExpr
	| <assoc = right> expr op = (
		'='
		| '+='
		| '-='
		| '*='
		| '/='
		| '%='
		| '&='
		| '|='
		| '^='
		| '<<='
		| '>>='
	) expr # AssignExpr;

array: '{' (expr (',' expr)*)? '}';
map: '[' (pair (',' pair)*)? ']';
pair: expr ':' expr;

literal:
	intLiteral		# Int
	| longLiteral	# Long
	| uintLiteral	# UInt
	| ulongLiteral	# ULong
	| L_F32			# F32
	| L_F64			# F64
	| L_STRING		# String
	| 'true'		# True
	| 'false'		# False;

intLiteral:
	L_INT_BIN	# BinInt
	| L_INT_OCT	# OctInt
	| L_INT_DEC	# DecInt
	| L_INT_HEX	# HexInt;
longLiteral:
	L_LONG_BIN		# BinLong
	| L_LONG_OCT	# OctLong
	| L_LONG_DEC	# DecLong
	| L_LONG_HEX	# HexLong;
uintLiteral:
	L_UINT_BIN		# BinUInt
	| L_UINT_OCT	# OctUInt
	| L_UINT_DEC	# DecUInt
	| L_UINT_HEX	# HexUInt;
ulongLiteral:
	L_ULONG_BIN		# BinULong
	| L_ULONG_OCT	# OctULong
	| L_ULONG_DEC	# DecULong
	| L_ULONG_HEX	# HexULong;

scope: name = ID gArgs = genericArgs?;

ref:
	(head = 'this' '.' | head = 'base' '.' | head = '::')? (
		scope ('.' scope)*
	)											# NormalRef
	| head = 'this'								# ThisRef
	| (head = 'this' | head = 'base') '.' 'new'	# NewRef;

typeName:
	primitiveTypeName				# AdoptPrimitiveTypeName
	| customTypeName				# AdoptCustomTypeName
	| typeName '->' '(' params ')'	# FnTypeName
	| typeName '[' ']'				# ArrayTypeName
	| typeName '[' typeName ']'		# MapTypeName;
primitiveTypeName:
	name = (
		'i8'
		| 'i16'
		| 'i32'
		| 'i64'
		| 'u8'
		| 'u16'
		| 'u32'
		| 'u64'
		| 'f32'
		| 'f64'
		| 'string'
		| 'auto'
		| 'bool'
		| 'void'
		| 'any'
	);
customTypeName: ref;

genericArgs: '<' typeName (',' typeName)* '>';
