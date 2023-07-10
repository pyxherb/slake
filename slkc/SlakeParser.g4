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

imports: 'using' '{' ID '=' ref (',' ID '=' ref)* '}';

params: ((typeName ID (',' typeName ID)*)? (',' '...')? | '...'?);

moduleDecl: 'module' ref ';';

fnDecl: genericArgs? typeName ID '(' params ')';
fnDef: fnDecl codeBlock;
stmts:
	expr ';'			# ExprStmt
	| varDef ';'		# VarDefStmt
	| 'break' ';'		# BreakStmt
	| 'continue' ';'	# ContinueStmt
	| forIns			# ForStmt
	| whileIns			# WhileStmt
	| timesIns			# TimesStmt
	| returnIns ';'		# ReturnStmt
	| ifIns				# IfStmt
	| tryIns			# TryStmt
	| switchIns			# SwitchStmt;

forIns: 'for' '(' varDef ';' expr ';' expr ')' codeBlock;
whileIns: 'while' '(' expr ')' codeBlock;
timesIns: 'times' '(' expr ')' codeBlock;
returnIns: 'return' expr?;
ifIns:
	'if' '(' expr ')' codeBlock ('elif' '(' expr ')' codeBlock)? (
		'else' codeBlock
	)?;

tryIns: 'try' codeBlock catchBlock+ ('final' codeBlock)?;
catchBlock: 'catch' ('(' typeName ID ')')? codeBlock;

switchIns:
	'switch' '(' expr ')' '{' switchCase+ (
		'default' ':' codeBlock
	)? '}';
switchCase: 'case' expr ':' codeBlock?;

varDecl: typeName ID (',' ID)*;
varDef: typeName varDefEntry (',' varDefEntry)*;
varDefEntry: ID ('=' expr)?;

accessModifiers:
	'pub'
	| 'final'
	| 'const'
	| 'override'
	| 'static'
	| 'native';
access: accessModifiers+;

classDef:
	'class' ID genericParams? inheritSlot? implementList? '{' classStmts* '}';
classStmts:
	access? fnDecl ';'
	| access? fnDef
	| access? operatorDecl ';'
	| access? operatorDef
	| access? constructorDecl ';'
	| access? destructorDecl ';'
	| access? constructorDef
	| access? destructorDef
	| access? varDecl ';'
	| access? varDef ';';

genericParams: '<' genericParam+ '>';
genericParam: ID baseSpec? traitSpec? interfaceSpec?;
baseSpec: '(' typeName ')'; // Base class specifier
traitSpec: '[' typeName (',' typeName)* ']'; // Trait specifier
interfaceSpec:
	':' typeName (',' typeName)*; // Interface specifier

inheritSlot: '(' customTypeName ')';
implementList: ':' (customTypeName (',' customTypeName)*)?;

operatorDecl: typeName 'operator' operatorName '(' params ')';
operatorDef: operatorDecl codeBlock;

constructorDecl: 'operator' 'new' '(' params ')';
constructorDef: constructorDecl codeBlock;

destructorDecl: 'operator' 'delete' '(' ')';
destructorDef: destructorDecl codeBlock;

interfaceDef:
	'interface' ID inheritSlot '{' interfaceStmts* '}';
interfaceStmts: fnDecl ';' | operatorDecl ';';

traitDef: 'trait' ID inheritSlot '{' traitStmts '}';
traitStmts: fnDecl ';' | varDecl ';' | operatorDecl ';';

operatorName:
	'+'
	| '-'
	| '*'
	| '/'
	| '%'
	| '&'
	| '|'
	| '^'
	| '&&'
	| '||'
	| '~'
	| '!'
	| '='
	| '+='
	| '-='
	| '*='
	| '/='
	| '%='
	| '&='
	| '|='
	| '^='
	| '~='
	| '=='
	| '!='
	| '>'
	| '<'
	| '>='
	| '<='
	| '[' ']'
	| '(' ')';

codeBlock: '{' stmts* '}';

args: expr (',' expr)*;

expr:
	'(' expr ')'													# WrappedExpr
	| ref															# RefExpr
	| literal														# LiteralExpr
	| array															# ArrayExpr
	| map															# MapExpr
	| typeName '(' params ')' codeBlock								# ClosureExpr
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
	L_I8		# I8
	| L_I16		# I16
	| L_I32		# I32
	| L_I64		# I64
	| L_U8		# U8
	| L_U16		# U16
	| L_U32		# U32
	| L_U64		# U64
	| L_F32		# F32
	| L_F64		# F64
	| L_STRING	# String;

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
customTypeName: '@' ref;

genericArgs: '<' typeName (',' typeName)* '>';
