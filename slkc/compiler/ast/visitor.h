#ifndef _SLKC_COMPILER_AST_H_
#define _SLKC_COMPILER_AST_H_

#include <SlakeLexer.h>
#include <SlakeParser.h>
#include <SlakeParserVisitor.h>
#include <any>
#include "scope.h"
#include "module.h"
#include "typename.h"

#define VISIT_METHOD_DECL(name) virtual antlrcpp::Any visit##name(SlakeParser::name##Context *context) override
#define VISIT_METHOD_IMPL(cls, name) antlrcpp::Any cls::visit##name(SlakeParser::name##Context *context)

namespace slake {
	namespace slkc {
		class AstVisitor : public SlakeParserVisitor {
		private:
			shared_ptr<Scope> curScope;
			shared_ptr<ModuleNode> curModule = std::make_shared<ModuleNode>();

		public:
			inline AstVisitor() {
			}
			virtual ~AstVisitor() = default;

			VISIT_METHOD_DECL(Prog);
			VISIT_METHOD_DECL(ProgFnDecl);
			VISIT_METHOD_DECL(ProgFnDef);
			VISIT_METHOD_DECL(ProgClassDef);
			VISIT_METHOD_DECL(ProgVarDef);

			VISIT_METHOD_DECL(Imports);
			VISIT_METHOD_DECL(ModuleDecl);

			VISIT_METHOD_DECL(FnDecl);
			VISIT_METHOD_DECL(FnDef);

			VISIT_METHOD_DECL(ExprStmt);
			VISIT_METHOD_DECL(VarDefStmt);
			VISIT_METHOD_DECL(BreakStmt);
			VISIT_METHOD_DECL(ContinueStmt);
			VISIT_METHOD_DECL(ForStmt);
			VISIT_METHOD_DECL(WhileStmt);
			VISIT_METHOD_DECL(TimesStmt);
			VISIT_METHOD_DECL(ReturnStmt);
			VISIT_METHOD_DECL(IfStmt);
			VISIT_METHOD_DECL(TryStmt);
			VISIT_METHOD_DECL(SwitchStmt);

			VISIT_METHOD_DECL(ForIns);
			VISIT_METHOD_DECL(WhileIns);
			VISIT_METHOD_DECL(TimesIns);
			VISIT_METHOD_DECL(ReturnIns);
			VISIT_METHOD_DECL(IfIns);
			VISIT_METHOD_DECL(TryIns);
			VISIT_METHOD_DECL(CatchBlock);

			VISIT_METHOD_DECL(SwitchIns);
			VISIT_METHOD_DECL(SwitchCase);

			VISIT_METHOD_DECL(VarDecl);
			VISIT_METHOD_DECL(VarDef);
			VISIT_METHOD_DECL(VarDefEntry);

			VISIT_METHOD_DECL(AccessModifiers);
			VISIT_METHOD_DECL(Access);

			VISIT_METHOD_DECL(ClassDef);
			VISIT_METHOD_DECL(ClassStmts);

			VISIT_METHOD_DECL(GenericParams);
			VISIT_METHOD_DECL(GenericParam);
			VISIT_METHOD_DECL(BaseSpec);
			VISIT_METHOD_DECL(TraitSpec);
			VISIT_METHOD_DECL(InterfaceSpec);

			VISIT_METHOD_DECL(InheritSlot);
			VISIT_METHOD_DECL(ImplementList);

			VISIT_METHOD_DECL(OperatorDecl);
			VISIT_METHOD_DECL(OperatorDef);

			VISIT_METHOD_DECL(ConstructorDecl);
			VISIT_METHOD_DECL(ConstructorDef);

			VISIT_METHOD_DECL(DestructorDecl);
			VISIT_METHOD_DECL(DestructorDef);

			VISIT_METHOD_DECL(InterfaceDef);
			VISIT_METHOD_DECL(InterfaceStmts);

			VISIT_METHOD_DECL(TraitDef);
			VISIT_METHOD_DECL(TraitStmts);

			VISIT_METHOD_DECL(OperatorName);

			VISIT_METHOD_DECL(CodeBlock);

			VISIT_METHOD_DECL(Args);

			VISIT_METHOD_DECL(WrappedExpr);
			VISIT_METHOD_DECL(RefExpr);
			VISIT_METHOD_DECL(LiteralExpr);
			VISIT_METHOD_DECL(ArrayExpr);
			VISIT_METHOD_DECL(MapExpr);
			VISIT_METHOD_DECL(ClosureExpr);
			VISIT_METHOD_DECL(CallExpr);
			VISIT_METHOD_DECL(AwaitExpr);
			VISIT_METHOD_DECL(NewExpr);
			VISIT_METHOD_DECL(NewArrayExpr);
			VISIT_METHOD_DECL(NewMapExpr);
			VISIT_METHOD_DECL(MatchExpr);
			VISIT_METHOD_DECL(CastExpr);
			VISIT_METHOD_DECL(TypeofExpr);
			VISIT_METHOD_DECL(TypeTypeofExpr);
			VISIT_METHOD_DECL(SubscriptExpr);
			VISIT_METHOD_DECL(ForwardIncDecExpr);
			VISIT_METHOD_DECL(BackwardIncDecExpr);
			VISIT_METHOD_DECL(NotExpr);
			VISIT_METHOD_DECL(MulDivExpr);
			VISIT_METHOD_DECL(AddSubExpr);
			VISIT_METHOD_DECL(LtGtExpr);
			VISIT_METHOD_DECL(ShiftExpr);
			VISIT_METHOD_DECL(EqExpr);
			VISIT_METHOD_DECL(AndExpr);
			VISIT_METHOD_DECL(XorExpr);
			VISIT_METHOD_DECL(OrExpr);
			VISIT_METHOD_DECL(LogicalAndExpr);
			VISIT_METHOD_DECL(LogicalOrExpr);
			VISIT_METHOD_DECL(TernaryExpr);
			VISIT_METHOD_DECL(AssignExpr);

			VISIT_METHOD_DECL(Array);
			VISIT_METHOD_DECL(Map);
			VISIT_METHOD_DECL(Pair);

			VISIT_METHOD_DECL(I8);
			VISIT_METHOD_DECL(I16);
			VISIT_METHOD_DECL(I32);
			VISIT_METHOD_DECL(I64);
			VISIT_METHOD_DECL(U8);
			VISIT_METHOD_DECL(U16);
			VISIT_METHOD_DECL(U32);
			VISIT_METHOD_DECL(U64);
			VISIT_METHOD_DECL(F32);
			VISIT_METHOD_DECL(F64);
			VISIT_METHOD_DECL(String);

			VISIT_METHOD_DECL(Scope);
			VISIT_METHOD_DECL(NormalRef);
			VISIT_METHOD_DECL(ThisRef);
			VISIT_METHOD_DECL(NewRef);
			VISIT_METHOD_DECL(FnTypeName);
			VISIT_METHOD_DECL(ArrayTypeName);
			VISIT_METHOD_DECL(MapTypeName);
			VISIT_METHOD_DECL(AdoptPrimitiveTypeName);
			VISIT_METHOD_DECL(AdoptCustomTypeName);
			VISIT_METHOD_DECL(PrimitiveTypeName);
			VISIT_METHOD_DECL(CustomTypeName);
			VISIT_METHOD_DECL(GenericArgs);
			VISIT_METHOD_DECL(Params);
		};
	}
}

#endif
