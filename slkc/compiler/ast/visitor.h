///
/// @file visitor.h
/// @brief Header file of the visitor module.
///
/// @copyright Copyright (c) 2023 Slake contributors
///
///
#ifndef _SLKC_COMPILER_AST_VISITOR_H_
#define _SLKC_COMPILER_AST_VISITOR_H_

#include <SlakeLexer.h>
#include <SlakeParser.h>
#include <SlakeParserVisitor.h>
#include <any>
#include "scope.h"
#include "module.h"
#include "fn.h"
#include "var.h"
#include "expr.h"

#define VISIT_METHOD_DECL(name) virtual antlrcpp::Any visit##name(SlakeParser::name##Context *context) override

namespace slake {
	namespace slkc {
		class Compiler;

		class AstVisitor : public SlakeParserVisitor {
		private:
			shared_ptr<Scope> curScope;
			shared_ptr<ModuleNode> curModule;
			Compiler *compiler;

			void _putDefinition(Location locName, string name, shared_ptr<MemberNode> member);
			void _putFnDefinition(Location locName, string name, FnOverloadingRegistry overloadingRegistry);

		public:
			inline AstVisitor(Compiler *compiler) : compiler(compiler) {
			}
			virtual ~AstVisitor() = default;

			VISIT_METHOD_DECL(Prog);
			VISIT_METHOD_DECL(ProgFnDecl);
			VISIT_METHOD_DECL(ProgFnDef);
			VISIT_METHOD_DECL(ProgClassDef);
			VISIT_METHOD_DECL(ProgVarDef);

			VISIT_METHOD_DECL(Imports);
			VISIT_METHOD_DECL(ImportItem);
			VISIT_METHOD_DECL(ModuleDecl);

			VISIT_METHOD_DECL(FnDecl);
			VISIT_METHOD_DECL(FnDef);

			VISIT_METHOD_DECL(ExprStmt);
			VISIT_METHOD_DECL(VarDefStmt);
			VISIT_METHOD_DECL(BreakStmt);
			VISIT_METHOD_DECL(ContinueStmt);
			VISIT_METHOD_DECL(ForStmt);
			VISIT_METHOD_DECL(WhileStmt);
			VISIT_METHOD_DECL(ReturnStmt);
			VISIT_METHOD_DECL(YieldStmt);
			VISIT_METHOD_DECL(IfStmt);
			VISIT_METHOD_DECL(TryStmt);
			VISIT_METHOD_DECL(SwitchStmt);
			VISIT_METHOD_DECL(CodeBlockStmt);

			VISIT_METHOD_DECL(ElseBranch);

			VISIT_METHOD_DECL(CatchBlock);
			VISIT_METHOD_DECL(FinalBlock);

			VISIT_METHOD_DECL(SwitchCase);
			VISIT_METHOD_DECL(DefaultBranch);

			VISIT_METHOD_DECL(VarDef);
			VISIT_METHOD_DECL(VarDefEntry);

			VISIT_METHOD_DECL(Access);
			VISIT_METHOD_DECL(PubAccess);
			VISIT_METHOD_DECL(FinalAccess);
			VISIT_METHOD_DECL(ConstAccess);
			VISIT_METHOD_DECL(OverrideAccess);
			VISIT_METHOD_DECL(StaticAccess);
			VISIT_METHOD_DECL(NativeAccess);

			VISIT_METHOD_DECL(ClassDef);
			VISIT_METHOD_DECL(ClassFnDecl);
			VISIT_METHOD_DECL(ClassFnDef);
			VISIT_METHOD_DECL(ClassOperatorDecl);
			VISIT_METHOD_DECL(ClassOperatorDef);
			VISIT_METHOD_DECL(ClassConstructorDecl);
			VISIT_METHOD_DECL(ClassDestructorDecl);
			VISIT_METHOD_DECL(ClassConstructorDef);
			VISIT_METHOD_DECL(ClassDestructorDef);
			VISIT_METHOD_DECL(ClassVarDef);
			VISIT_METHOD_DECL(ClassClassDef);

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
			VISIT_METHOD_DECL(InterfaceFnDecl);
			VISIT_METHOD_DECL(InterfaceOperatorDecl);

			VISIT_METHOD_DECL(TraitDef);
			VISIT_METHOD_DECL(TraitFnDecl);
			VISIT_METHOD_DECL(TraitOperatorDecl);

			VISIT_METHOD_DECL(OperatorAdd);
			VISIT_METHOD_DECL(OperatorSub);
			VISIT_METHOD_DECL(OperatorMul);
			VISIT_METHOD_DECL(OperatorDiv);
			VISIT_METHOD_DECL(OperatorMod);
			VISIT_METHOD_DECL(OperatorAnd);
			VISIT_METHOD_DECL(OperatorOr);
			VISIT_METHOD_DECL(OperatorXor);
			VISIT_METHOD_DECL(OperatorLAnd);
			VISIT_METHOD_DECL(OperatorLOr);
			VISIT_METHOD_DECL(OperatorRev);
			VISIT_METHOD_DECL(OperatorNot);
			VISIT_METHOD_DECL(OperatorAssign);
			VISIT_METHOD_DECL(OperatorAddAssign);
			VISIT_METHOD_DECL(OperatorSubAssign);
			VISIT_METHOD_DECL(OperatorMulAssign);
			VISIT_METHOD_DECL(OperatorDivAssign);
			VISIT_METHOD_DECL(OperatorModAssign);
			VISIT_METHOD_DECL(OperatorAndAssign);
			VISIT_METHOD_DECL(OperatorOrAssign);
			VISIT_METHOD_DECL(OperatorXorAssign);
			VISIT_METHOD_DECL(OperatorEq);
			VISIT_METHOD_DECL(OperatorNeq);
			VISIT_METHOD_DECL(OperatorGt);
			VISIT_METHOD_DECL(OperatorLt);
			VISIT_METHOD_DECL(OperatorGtEq);
			VISIT_METHOD_DECL(OperatorLtEq);
			VISIT_METHOD_DECL(OperatorSubscript);
			VISIT_METHOD_DECL(OperatorCall);

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

			VISIT_METHOD_DECL(Int);
			VISIT_METHOD_DECL(Long);
			VISIT_METHOD_DECL(UInt);
			VISIT_METHOD_DECL(ULong);

			VISIT_METHOD_DECL(BinInt);
			VISIT_METHOD_DECL(OctInt);
			VISIT_METHOD_DECL(DecInt);
			VISIT_METHOD_DECL(HexInt);
			VISIT_METHOD_DECL(BinLong);
			VISIT_METHOD_DECL(OctLong);
			VISIT_METHOD_DECL(DecLong);
			VISIT_METHOD_DECL(HexLong);
			VISIT_METHOD_DECL(BinUInt);
			VISIT_METHOD_DECL(OctUInt);
			VISIT_METHOD_DECL(DecUInt);
			VISIT_METHOD_DECL(HexUInt);
			VISIT_METHOD_DECL(BinULong);
			VISIT_METHOD_DECL(OctULong);
			VISIT_METHOD_DECL(DecULong);
			VISIT_METHOD_DECL(HexULong);
			VISIT_METHOD_DECL(F32);
			VISIT_METHOD_DECL(F64);
			VISIT_METHOD_DECL(String);
			VISIT_METHOD_DECL(True);
			VISIT_METHOD_DECL(False);

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
			VISIT_METHOD_DECL(ParamDecl);
		};
	}
}

#undef VISIT_METHOD_DECL

#endif
