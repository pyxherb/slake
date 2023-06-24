#include "visitor.h"
#include "typename.h"

using namespace antlr4;
using namespace antlrcpp;

VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, Prog) {
	return visitChildren(context);
}
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, ProgFnDecl) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, ProgFnDef) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, ProgClassDef) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, ProgVarDef) { return Any(); }

VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, Imports) {
	return Any();
}
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, ModuleDecl) {
	visitChildren(context);

	return Any();
}

VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, FnDecl) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, FnDef) { return Any(); }

VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, ExprStmt) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, VarDefStmt) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, BreakStmt) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, ContinueStmt) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, ForStmt) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, WhileStmt) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, TimesStmt) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, ReturnStmt) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, IfStmt) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, TryStmt) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, SwitchStmt) { return Any(); }

VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, ForIns) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, WhileIns) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, TimesIns) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, ReturnIns) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, IfIns) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, TryIns) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, CatchBlock) { return Any(); }

VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, SwitchIns) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, SwitchCase) { return Any(); }

VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, VarDecl) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, VarDef) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, VarDefEntry) { return Any(); }

VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, AccessModifiers) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, Access) { return Any(); }

VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, ClassDef) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, ClassStmts) { return Any(); }

VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, GenericParams) {
	for (size_t i = 1; i < context->children.size(); i += 2) {

	}

	return Any();
}
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, GenericParam) {
	return Any();
}
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, BaseSpec) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, TraitSpec) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, InterfaceSpec) { return Any(); }

VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, InheritSlot) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, ImplementList) { return Any(); }

VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, OperatorDecl) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, OperatorDef) { return Any(); }

VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, ConstructorDecl) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, ConstructorDef) { return Any(); }

VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, DestructorDecl) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, DestructorDef) { return Any(); }

VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, InterfaceDef) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, InterfaceStmts) { return Any(); }

VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, TraitDef) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, TraitStmts) { return Any(); }

VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, OperatorName) { return Any(); }

VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, CodeBlock) { return Any(); }

VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, Args) { return Any(); }

VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, WrappedExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, RefExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, LiteralExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, ArrayExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, MapExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, ClosureExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, CallExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, AwaitExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, NewExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, NewArrayExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, NewMapExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, MatchExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, CastExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, TypeofExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, TypeTypeofExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, SubscriptExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, ForwardIncDecExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, BackwardIncDecExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, NotExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, MulDivExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, AddSubExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, LtGtExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, ShiftExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, EqExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, AndExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, XorExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, OrExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, LogicalAndExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, LogicalOrExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, TernaryExpr) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, AssignExpr) { return Any(); }

VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, Array) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, Map) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, Pair) { return Any(); }

VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, I8) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, I16) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, I32) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, I64) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, U8) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, U16) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, U32) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, U64) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, F32) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, F64) { return Any(); }
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, String) { return Any(); }

VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, Scope) {
	RefScope scope{ context->name->toString() };
	auto a = context->gArgs;
	return Any();
}
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, NormalRef) {
	Ref ref;
	for (size_t i = 0; i < context->children.size(); i += 2) {
		visit(context->children[i]);
	}
	return Any();
}
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, ThisRef) {
	Ref ref{ { "this" } };
	return ref;
}
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, NewRef) {
	Ref ref{
		{ context->children[0]->getText() },
		{ "new" }
	};
	return ref;
}
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, FnTypeName) {
	return Any();
}
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, ArrayTypeName) {
	return Any();
}
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, MapTypeName) {
	return Any();
}
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, AdoptPrimitiveTypeName) {
	return visit(context->children[0]);
}
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, AdoptCustomTypeName) {
	return visit(context->children[0]);
}
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, PrimitiveTypeName) {
	switch (context->name->getType()) {
		case SlakeParser::TN_I8:
			return make_shared<TypeName>(TYPE_I8);
		case SlakeParser::TN_I16:
			return make_shared<TypeName>(TYPE_I16);
		case SlakeParser::TN_I32:
			return make_shared<TypeName>(TYPE_I32);
		case SlakeParser::TN_I64:
			return make_shared<TypeName>(TYPE_I64);
		case SlakeParser::TN_U8:
			return make_shared<TypeName>(TYPE_U8);
		case SlakeParser::TN_U16:
			return make_shared<TypeName>(TYPE_U16);
		case SlakeParser::TN_U32:
			return make_shared<TypeName>(TYPE_U32);
		case SlakeParser::TN_U64:
			return make_shared<TypeName>(TYPE_U64);
		case SlakeParser::TN_F32:
			return make_shared<TypeName>(TYPE_F32);
		case SlakeParser::TN_F64:
			return make_shared<TypeName>(TYPE_F64);
		case SlakeParser::TN_STRING:
			return make_shared<TypeName>(TYPE_STRING);
		case SlakeParser::TN_AUTO:
			return make_shared<TypeName>(TYPE_AUTO);
		case SlakeParser::TN_BOOL:
			return make_shared<TypeName>(TYPE_BOOL);
		case SlakeParser::TN_VOID:
			return make_shared<TypeName>(TYPE_VOID);
		case SlakeParser::TN_ANY:
			return make_shared<TypeName>(TYPE_ANY);
	}

	throw logic_error("Unrecognized primitive type");
}
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, CustomTypeName) {
	return make_shared<CustomTypeName>(any_cast<shared_ptr<Ref>>(visit(context->ref())));
}
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, GenericArgs) {
	return Any();
}
VISIT_METHOD_IMPL(Slake::Compiler::AstVisitor, Params) {
	return Any();
}
