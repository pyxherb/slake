#include "visitor.h"
#include "typename.h"

using namespace antlr4;
using namespace antlrcpp;

VISIT_METHOD_IMPL(slake::slkc::AstVisitor, Prog) {
	return visitChildren(context);
}
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, ProgFnDecl) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, ProgFnDef) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, ProgClassDef) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, ProgVarDef) { return Any(); }

VISIT_METHOD_IMPL(slake::slkc::AstVisitor, Imports) {
	return Any();
}
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, ModuleDecl) {
	visitChildren(context);

	return Any();
}

VISIT_METHOD_IMPL(slake::slkc::AstVisitor, FnDecl) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, FnDef) { return Any(); }

VISIT_METHOD_IMPL(slake::slkc::AstVisitor, ExprStmt) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, VarDefStmt) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, BreakStmt) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, ContinueStmt) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, ForStmt) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, WhileStmt) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, TimesStmt) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, ReturnStmt) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, IfStmt) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, TryStmt) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, SwitchStmt) { return Any(); }

VISIT_METHOD_IMPL(slake::slkc::AstVisitor, ForIns) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, WhileIns) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, TimesIns) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, ReturnIns) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, IfIns) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, TryIns) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, CatchBlock) { return Any(); }

VISIT_METHOD_IMPL(slake::slkc::AstVisitor, SwitchIns) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, SwitchCase) { return Any(); }

VISIT_METHOD_IMPL(slake::slkc::AstVisitor, VarDecl) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, VarDef) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, VarDefEntry) { return Any(); }

VISIT_METHOD_IMPL(slake::slkc::AstVisitor, AccessModifiers) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, Access) { return Any(); }

VISIT_METHOD_IMPL(slake::slkc::AstVisitor, ClassDef) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, ClassStmts) { return Any(); }

VISIT_METHOD_IMPL(slake::slkc::AstVisitor, GenericParams) {
	for (size_t i = 1; i < context->children.size(); i += 2) {

	}

	return Any();
}
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, GenericParam) {
	return Any();
}
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, BaseSpec) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, TraitSpec) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, InterfaceSpec) { return Any(); }

VISIT_METHOD_IMPL(slake::slkc::AstVisitor, InheritSlot) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, ImplementList) { return Any(); }

VISIT_METHOD_IMPL(slake::slkc::AstVisitor, OperatorDecl) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, OperatorDef) { return Any(); }

VISIT_METHOD_IMPL(slake::slkc::AstVisitor, ConstructorDecl) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, ConstructorDef) { return Any(); }

VISIT_METHOD_IMPL(slake::slkc::AstVisitor, DestructorDecl) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, DestructorDef) { return Any(); }

VISIT_METHOD_IMPL(slake::slkc::AstVisitor, InterfaceDef) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, InterfaceStmts) { return Any(); }

VISIT_METHOD_IMPL(slake::slkc::AstVisitor, TraitDef) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, TraitStmts) { return Any(); }

VISIT_METHOD_IMPL(slake::slkc::AstVisitor, OperatorName) { return Any(); }

VISIT_METHOD_IMPL(slake::slkc::AstVisitor, CodeBlock) { return Any(); }

VISIT_METHOD_IMPL(slake::slkc::AstVisitor, Args) { return Any(); }

VISIT_METHOD_IMPL(slake::slkc::AstVisitor, WrappedExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, RefExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, LiteralExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, ArrayExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, MapExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, ClosureExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, CallExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, AwaitExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, NewExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, NewArrayExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, NewMapExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, MatchExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, CastExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, TypeofExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, TypeTypeofExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, SubscriptExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, ForwardIncDecExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, BackwardIncDecExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, NotExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, MulDivExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, AddSubExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, LtGtExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, ShiftExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, EqExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, AndExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, XorExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, OrExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, LogicalAndExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, LogicalOrExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, TernaryExpr) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, AssignExpr) { return Any(); }

VISIT_METHOD_IMPL(slake::slkc::AstVisitor, Array) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, Map) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, Pair) { return Any(); }

VISIT_METHOD_IMPL(slake::slkc::AstVisitor, I8) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, I16) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, I32) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, I64) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, U8) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, U16) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, U32) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, U64) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, F32) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, F64) { return Any(); }
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, String) { return Any(); }

VISIT_METHOD_IMPL(slake::slkc::AstVisitor, Scope) {
	RefScope scope{ context->name->toString() };
	auto a = context->gArgs;
	return Any();
}
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, NormalRef) {
	Ref ref;
	for (size_t i = 0; i < context->children.size(); i += 2) {
		visit(context->children[i]);
	}
	return Any();
}
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, ThisRef) {
	Ref ref{ { "this" } };
	return ref;
}
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, NewRef) {
	Ref ref{
		{ context->children[0]->getText() },
		{ "new" }
	};
	return ref;
}
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, FnTypeName) {
	return Any();
}
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, ArrayTypeName) {
	return Any();
}
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, MapTypeName) {
	return Any();
}
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, AdoptPrimitiveTypeName) {
	return visit(context->children[0]);
}
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, AdoptCustomTypeName) {
	return visit(context->children[0]);
}
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, PrimitiveTypeName) {
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
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, CustomTypeName) {
	return make_shared<CustomTypeName>(visit(context->ref()).as<shared_ptr<Ref>>());
}
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, GenericArgs) {
	return Any();
}
VISIT_METHOD_IMPL(slake::slkc::AstVisitor, Params) {
	return Any();
}
