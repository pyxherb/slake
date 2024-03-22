///
/// @file visitor.cc
/// @brief Implementations of the visitor module.
///
/// @copyright Copyright (c) 2023 Slake contributors, all rights reserved.
///
///
#include "../compiler.h"

using namespace antlr4;
using namespace antlrcpp;
using namespace slake::slkc;

struct FnDecl {
	string name;
	FnOverloadingRegistry overloadingRegistry;

	FnDecl(const string &name, const FnOverloadingRegistry &overloadingRegistry)
		: name(name), overloadingRegistry(overloadingRegistry) {
	}
};

void AstVisitor::_putDefinition(
	Location locName,
	string name,
	shared_ptr<MemberNode> member) {
	if (curScope->members.count(name))
		throw FatalCompilationError(
			Message(
				locName,
				MessageType::Error,
				"Redefinition of `" + name + "'"));
	curScope->members[name] = member;
	member->parent = (MemberNode *)curScope->owner;
}

void AstVisitor::_putFnDefinition(
	Location locName,
	string name,
	FnOverloadingRegistry overloadingRegistry) {
	if (!curScope->members.count(name)) {
		curScope->members[name] = make_shared<FnNode>(compiler, name);
		curScope->members[name]->parent = (MemberNode *)curScope->owner;
	}

	if (curScope->members.at(name)->getNodeType() != NodeType::Fn) {
		throw FatalCompilationError(
			Message(
				locName,
				MessageType::Error,
				"Redefinition of `" + name + "'"));
	} else {
		static_pointer_cast<FnNode>(curScope->members.at(name))->overloadingRegistries.push_back(overloadingRegistry);
	}
}

#define VISIT_METHOD_DECL(name) \
	antlrcpp::Any slake::slkc::AstVisitor::visit##name(SlakeParser::name##Context *context)

VISIT_METHOD_DECL(Prog) {
	compiler->_targetModule = (curModule = make_shared<ModuleNode>(compiler, Location()));
	curScope = curModule->scope;
	return visitChildren(context);
}
VISIT_METHOD_DECL(ProgFnDecl) {
	auto decl = any_cast<FnDecl>(visit(context->fnDecl()));
	if (context->access())
		decl.overloadingRegistry.access = any_cast<AccessModifier>(visit(context->access()));

	_putFnDefinition(
		Location(context->fnDecl()->ID()),
		decl.name,
		decl.overloadingRegistry);

	return visitChildren(context);
}
VISIT_METHOD_DECL(ProgFnDef) {
	auto decl = any_cast<FnDecl>(visit(context->fnDef()));
	if (context->access())
		decl.overloadingRegistry.access = any_cast<AccessModifier>(visit(context->access()));

	_putFnDefinition(
		Location(context->fnDef()->fnDecl()->ID()),
		decl.name,
		decl.overloadingRegistry);

	return visitChildren(context);
}
VISIT_METHOD_DECL(ProgClassDef) {
	auto cls = any_cast<shared_ptr<ClassNode>>(visit(context->classDef()));

	if (context->access())
		cls->access = any_cast<AccessModifier>(visit(context->access()));

	return Any();
}
VISIT_METHOD_DECL(ProgVarDef) {
	return visitChildren(context);
}

VISIT_METHOD_DECL(Imports) {
	for (auto i : context->importItem()) {
		auto item = any_cast<pair<string, Ref>>(visit(i));
		curModule->imports[item.first] = item.second;
	}
	return visitChildren(context);
}

VISIT_METHOD_DECL(ImportItem) {
	return pair<string, Ref>(
		context->ID()->getText(),
		any_cast<Ref>(visit(context->moduleRef())));
}

VISIT_METHOD_DECL(ModuleDecl) {
	curModule->moduleName = any_cast<Ref>(visit(context->children[1]));
	return visitChildren(context);
}

VISIT_METHOD_DECL(FnDecl) {
	GenericParamNodeList genericParams;
	auto returnType = any_cast<shared_ptr<TypeNameNode>>(visit(context->typeName()));
	auto name = context->ID()->getText();
	auto params = any_cast<deque<Param>>(visit(context->params()));

	Location loc;
	if (context->genericParams()) {
		loc = Location(context->getToken(SlakeParser::RuleGenericParams, 0));
		genericParams = any_cast<GenericParamNodeList>(visit(context->genericParams()));
	} else {
		loc = returnType->getLocation();
	}

	return FnDecl(
		name,
		FnOverloadingRegistry(
			loc, returnType,
			genericParams,
			params));
}
VISIT_METHOD_DECL(FnDef) {
	auto decl = any_cast<FnDecl>(visit(context->children[0]));
	decl.overloadingRegistry.body = static_pointer_cast<BlockStmtNode>(any_cast<shared_ptr<StmtNode>>(visit(context->children[1])));
	return decl;
}

VISIT_METHOD_DECL(ExprStmt) {
	return static_pointer_cast<StmtNode>(make_shared<ExprStmtNode>(any_cast<shared_ptr<ExprNode>>(visit(context->expr()))));
}
VISIT_METHOD_DECL(VarDefStmt) {
	return visit(context->varDef());
}
VISIT_METHOD_DECL(BreakStmt) {
	return static_pointer_cast<StmtNode>(make_shared<BreakStmtNode>(Location(context->KW_BREAK())));
}
VISIT_METHOD_DECL(ContinueStmt) {
	return static_pointer_cast<StmtNode>(make_shared<BreakStmtNode>(Location(context->KW_CONTINUE())));
}
VISIT_METHOD_DECL(ForStmt) {
	return static_pointer_cast<StmtNode>(make_shared<ForStmtNode>(
		Location(context->KW_FOR()),
		context->varDef()
			? static_pointer_cast<VarDefStmtNode>(any_cast<shared_ptr<StmtNode>>(visit(context->varDef())))
			: shared_ptr<VarDefStmtNode>(),
		any_cast<shared_ptr<ExprNode>>(visit(context->condition)),
		context->endExpr
			? any_cast<shared_ptr<ExprNode>>(visit(context->endExpr))
			: shared_ptr<ExprNode>(),
		any_cast<shared_ptr<StmtNode>>(visit(context->stmt()))));
}
VISIT_METHOD_DECL(WhileStmt) {
	return static_pointer_cast<StmtNode>(make_shared<WhileStmtNode>(
		Location(context->KW_WHILE()),
		any_cast<shared_ptr<ExprNode>>(visit(context->expr())),
		any_cast<shared_ptr<StmtNode>>(visit(context->stmt()))));
}
VISIT_METHOD_DECL(ReturnStmt) {
	return static_pointer_cast<StmtNode>(make_shared<ReturnStmtNode>(
		Location(context->KW_RETURN()),
		context->expr()
			? any_cast<shared_ptr<ExprNode>>(visit(context->expr()))
			: shared_ptr<ExprNode>()));
}
VISIT_METHOD_DECL(YieldStmt) {
	return static_pointer_cast<StmtNode>(make_shared<YieldStmtNode>(
		Location(context->KW_YIELD()),
		context->expr()
			? any_cast<shared_ptr<ExprNode>>(visit(context->expr()))
			: shared_ptr<ExprNode>()));
}
VISIT_METHOD_DECL(IfStmt) {
	return static_pointer_cast<StmtNode>(make_shared<IfStmtNode>(
		Location(context->KW_IF()),
		any_cast<shared_ptr<ExprNode>>(visit(context->expr())),
		context->varDef()
			? any_cast<shared_ptr<VarDefStmtNode>>(visit(context->varDef()))
			: shared_ptr<VarDefStmtNode>(),
		any_cast<shared_ptr<StmtNode>>(visit(context->stmt())),
		context->elseBranch()
			? any_cast<shared_ptr<StmtNode>>(visit(context->elseBranch()))
			: shared_ptr<StmtNode>()));
}
VISIT_METHOD_DECL(TryStmt) {
	deque<CatchBlock> catchBlocks;
	for (auto i : context->catchBlock())
		catchBlocks.push_back(any_cast<CatchBlock>(visit(i)));
	return static_pointer_cast<StmtNode>(make_shared<TryStmtNode>(
		Location(context->KW_TRY()),
		any_cast<shared_ptr<StmtNode>>(visit(context->stmt())),
		catchBlocks,
		context->finalBlock()
			? any_cast<FinalBlock>(visit(context->finalBlock()))
			: FinalBlock()));
}
VISIT_METHOD_DECL(SwitchStmt) {
	deque<SwitchCase> cases;
	for (auto i : context->switchCase())
		cases.push_back(any_cast<SwitchCase>(visit(i)));
	if (context->defaultBranch())
		cases.push_back(any_cast<SwitchCase>(visit(context->defaultBranch())));
	return static_pointer_cast<StmtNode>(make_shared<SwitchStmtNode>(
		Location(context->KW_SWITCH()),
		any_cast<shared_ptr<ExprNode>>(context->expr()),
		cases));
}
VISIT_METHOD_DECL(CodeBlockStmt) {
	return visit(context->codeBlock());
}

VISIT_METHOD_DECL(ElseBranch) {
	return visit(context->stmt());
}

VISIT_METHOD_DECL(CatchBlock) {
	return CatchBlock(
		Location{ context->KW_CATCH() },
		context->type
			? any_cast<shared_ptr<TypeNameNode>>(visit(context->type))
			: shared_ptr<TypeNameNode>(),
		context->varName
			? context->varName->getText()
			: "",
		any_cast<shared_ptr<StmtNode>>(visit(context->stmt())));
}
VISIT_METHOD_DECL(FinalBlock) {
	return FinalBlock(
		Location{ context->KW_FINAL() },
		any_cast<shared_ptr<StmtNode>>(visit(context->stmt())));
}

VISIT_METHOD_DECL(SwitchCase) {
	deque<shared_ptr<StmtNode>> body;
	for (auto i : context->stmt())
		body.push_back(any_cast<shared_ptr<StmtNode>>(visit(i)));
	return SwitchCase(
		Location(context->KW_CASE()),
		body,
		any_cast<shared_ptr<ExprNode>>(visit(context->expr())));
}
VISIT_METHOD_DECL(DefaultBranch) {
	deque<shared_ptr<StmtNode>> body;
	for (auto i : context->stmt())
		body.push_back(any_cast<shared_ptr<StmtNode>>(visit(i)));
	return SwitchCase(
		Location(context->KW_DEFAULT()),
		body);
}

VISIT_METHOD_DECL(VarDef) {
	auto stmt = make_shared<VarDefStmtNode>(
		Location{ context->varDefEntry(0)->getStart() },
		any_cast<shared_ptr<TypeNameNode>>(visit(context->children[0])));

	for (auto i : context->varDefEntry()) {
		auto entry = any_cast<VarDefEntry>(visit(i));
		stmt->varDefs[entry.name] = entry;
	}

	return static_pointer_cast<StmtNode>(stmt);
}
VISIT_METHOD_DECL(VarDefEntry) {
	return VarDefEntry(
		Location(context->ID()),
		context->ID()->getText(),
		context->expr()
			? any_cast<shared_ptr<ExprNode>>(visit(context->expr()))
			: shared_ptr<ExprNode>());
}

VISIT_METHOD_DECL(PubAccess) {
	return ACCESS_PUB;
}
VISIT_METHOD_DECL(FinalAccess) {
	return ACCESS_FINAL;
}
VISIT_METHOD_DECL(ConstAccess) {
	return ACCESS_CONST;
}
VISIT_METHOD_DECL(OverrideAccess) {
	return ACCESS_OVERRIDE;
}
VISIT_METHOD_DECL(StaticAccess) {
	return ACCESS_STATIC;
}
VISIT_METHOD_DECL(NativeAccess) {
	return ACCESS_NATIVE;
}

static map<slake::AccessModifier, string> _accessModifierNames = {
	{ slake::ACCESS_PUB, "pub" },
	{ slake::ACCESS_STATIC, "static" },
	{ slake::ACCESS_NATIVE, "native" },
	{ slake::ACCESS_OVERRIDE, "override" },
	{ slake::ACCESS_FINAL, "final" },
	{ slake::ACCESS_CONST, "const" }
};

VISIT_METHOD_DECL(Access) {
	AccessModifier access = 0;
	for (auto i : context->getTokens(SlakeParser::RuleAccessModifiers)) {
		AccessModifier curAccess = any_cast<AccessModifier>(visit(i));

		if (access & curAccess) {
			compiler->messages.push_back(
				Message(
					Location{ i },
					MessageType::Warn,
					"Duplicated modifier `" + _accessModifierNames.at(curAccess) + "'"));
		} else
			access |= curAccess;
	}

	return access;
}

VISIT_METHOD_DECL(ClassDef) {
	string name = context->ID()->getText();
	auto cls = make_shared<ClassNode>(
		Location(context->KW_CLASS()),
		compiler,
		name,
		context->inheritSlot()
			? static_pointer_cast<CustomTypeNameNode>(any_cast<shared_ptr<TypeNameNode>>(visit(context->inheritSlot())))
			: shared_ptr<CustomTypeNameNode>(),
		context->implementList()
			? any_cast<deque<shared_ptr<CustomTypeNameNode>>>(visit(context->implementList()))
			: deque<shared_ptr<CustomTypeNameNode>>(),
		context->genericParams()
			? any_cast<GenericParamNodeList>(visit(context->genericParams()))
			: GenericParamNodeList());

	_putDefinition(Location(context->ID()), name, cls);

	auto savedScope = curScope;
	curScope = cls->scope;
	curScope->parent = savedScope.get();

	for (auto i : context->classStmts())
		visit(i);

	curScope = savedScope;

	return cls;
}
VISIT_METHOD_DECL(ClassFnDecl) {
	auto decl = any_cast<FnDecl>(visit(context->fnDecl()));
	if (context->access())
		decl.overloadingRegistry.access = any_cast<AccessModifier>(visit(context->access()));

	_putFnDefinition(
		Location(context->fnDecl()->ID()),
		decl.name,
		decl.overloadingRegistry);

	return visitChildren(context);
}
VISIT_METHOD_DECL(ClassFnDef) {
	auto decl = any_cast<FnDecl>(visit(context->fnDef()));
	if (context->access())
		decl.overloadingRegistry.access = any_cast<AccessModifier>(visit(context->access()));

	_putFnDefinition(
		Location(context->fnDef()->fnDecl()->ID()),
		decl.name,
		decl.overloadingRegistry);

	return visitChildren(context);
}
VISIT_METHOD_DECL(ClassOperatorDecl) {
	auto decl = any_cast<FnDecl>(visit(context->operatorDecl()));
	if (context->access())
		decl.overloadingRegistry.access = any_cast<AccessModifier>(visit(context->access()));

	_putFnDefinition(
		Location(context->operatorDecl()->KW_OPERATOR()),
		decl.name,
		decl.overloadingRegistry);

	return visitChildren(context);
}
VISIT_METHOD_DECL(ClassOperatorDef) {
	auto decl = any_cast<FnDecl>(visit(context->operatorDef()));
	if (context->access())
		decl.overloadingRegistry.access = any_cast<AccessModifier>(visit(context->access()));

	_putFnDefinition(
		Location(context->operatorDef()->operatorDecl()->KW_OPERATOR()),
		decl.name,
		decl.overloadingRegistry);

	return visitChildren(context);
}
VISIT_METHOD_DECL(ClassConstructorDecl) {
	auto decl = any_cast<FnDecl>(visit(context->constructorDecl()));
	if (context->access())
		decl.overloadingRegistry.access = any_cast<AccessModifier>(visit(context->access()));

	_putFnDefinition(
		Location(context->constructorDecl()->KW_OPERATOR()),
		decl.name,
		decl.overloadingRegistry);

	return visitChildren(context);
}
VISIT_METHOD_DECL(ClassDestructorDecl) {
	auto decl = any_cast<FnDecl>(visit(context->destructorDecl()));
	if (context->access())
		decl.overloadingRegistry.access = any_cast<AccessModifier>(visit(context->access()));

	_putFnDefinition(
		Location(context->destructorDecl()->KW_OPERATOR()),
		decl.name,
		decl.overloadingRegistry);

	return visitChildren(context);
}
VISIT_METHOD_DECL(ClassConstructorDef) {
	auto decl = any_cast<FnDecl>(visit(context->constructorDef()));
	if (context->access())
		decl.overloadingRegistry.access = any_cast<AccessModifier>(visit(context->access()));

	_putFnDefinition(
		Location(context->constructorDef()->constructorDecl()->KW_OPERATOR()),
		decl.name,
		decl.overloadingRegistry);

	return visitChildren(context);
}
VISIT_METHOD_DECL(ClassDestructorDef) {
	auto decl = any_cast<FnDecl>(visit(context->destructorDef()));
	if (context->access())
		decl.overloadingRegistry.access = any_cast<AccessModifier>(visit(context->access()));

	_putFnDefinition(
		Location(context->destructorDef()->destructorDecl()->KW_OPERATOR()),
		decl.name,
		decl.overloadingRegistry);

	return visitChildren(context);
}
VISIT_METHOD_DECL(ClassVarDef) {
	auto varDef = static_pointer_cast<VarDefStmtNode>(any_cast<shared_ptr<StmtNode>>(visit(context->varDef())));

	for (auto i : varDef->varDefs) {
		auto v = make_shared<VarNode>(
			i.second.loc,
			compiler,
			any_cast<AccessModifier>(visit(context->access())),
			varDef->type,
			i.first,
			i.second.initValue);

		_putDefinition(
			v->getLocation(),
			v->name,
			v);
	}

	return Any();
}
VISIT_METHOD_DECL(ClassClassDef) { return Any(); }

VISIT_METHOD_DECL(GenericParams) {
	GenericParamNodeList genericParams;

	for (auto i : context->genericParam()) {
		genericParams.push_back(any_cast<shared_ptr<GenericParamNode>>(visit(i)));
	}

	return genericParams;
}
VISIT_METHOD_DECL(GenericParam) {
	shared_ptr<GenericParamNode> param = make_shared<GenericParamNode>(Location(context->ID()), context->ID()->getText());

	if (auto spec = context->baseSpec(); spec) {
		param->baseType = any_cast<shared_ptr<TypeNameNode>>(visit(spec));
	}

	if (auto spec = context->interfaceSpec(); spec) {
		param->interfaceTypes = std::move(any_cast<deque<shared_ptr<TypeNameNode>>>(visit(spec)));
	}
	if (auto spec = context->traitSpec(); spec) {
		param->traitTypes = std::move(any_cast<deque<shared_ptr<TypeNameNode>>>(visit(spec)));
	}

	return param;
}
VISIT_METHOD_DECL(BaseSpec) {
	return any_cast<shared_ptr<TypeNameNode>>(visit(context->children[1]));
}
VISIT_METHOD_DECL(TraitSpec) {
	deque<shared_ptr<TypeNameNode>> traits;

	for (auto i : context->typeName()) {
		traits.push_back(any_cast<shared_ptr<TypeNameNode>>(visit(i)));
	}

	return traits;
}
VISIT_METHOD_DECL(InterfaceSpec) {
	deque<shared_ptr<TypeNameNode>> interfaces;

	for (auto i : context->typeName()) {
		interfaces.push_back(any_cast<shared_ptr<TypeNameNode>>(visit(i)));
	}

	return interfaces;
}

VISIT_METHOD_DECL(InheritSlot) {
	return visit(context->customTypeName());
}
VISIT_METHOD_DECL(ImplementList) {
	deque<shared_ptr<CustomTypeNameNode>> typeNames;
	for (auto i : context->getTokens(SlakeParser::RuleImplementList))
		typeNames.push_back(any_cast<shared_ptr<CustomTypeNameNode>>(visit(i)));
	return typeNames;
}

VISIT_METHOD_DECL(OperatorDecl) {
	GenericParamNodeList genericParams;
	auto returnType = any_cast<shared_ptr<TypeNameNode>>(visit(context->typeName()));
	auto name = "operator" + context->operatorName()->getText();
	auto params = any_cast<deque<Param>>(visit(context->params()));

	Location loc;
	if (context->genericParams()) {
		loc = Location(context->getToken(SlakeParser::RuleGenericParams, 0));
		genericParams = any_cast<GenericParamNodeList>(visit(context->genericParams()));
	} else {
		loc = returnType->getLocation();
	}

	return FnDecl(
		name,
		FnOverloadingRegistry(
			loc, returnType,
			genericParams,
			params));
}
VISIT_METHOD_DECL(OperatorDef) {
	auto decl = any_cast<FnDecl>(visit(context->children[0]));
	decl.overloadingRegistry.body = static_pointer_cast<BlockStmtNode>(any_cast<shared_ptr<StmtNode>>(visit(context->children[1])));
	return decl;
}

VISIT_METHOD_DECL(ConstructorDecl) {
	auto name = "new";
	auto params = any_cast<deque<Param>>(visit(context->params()));

	return FnDecl(
		name,
		FnOverloadingRegistry(
			Location(context->KW_OPERATOR()),
			make_shared<VoidTypeNameNode>(Location(context->KW_OPERATOR()), false),
			{},
			params));
}
VISIT_METHOD_DECL(ConstructorDef) {
	auto decl = any_cast<FnDecl>(visit(context->constructorDecl()));
	decl.overloadingRegistry.body = static_pointer_cast<BlockStmtNode>(any_cast<shared_ptr<StmtNode>>(visit(context->codeBlock())));
	return decl;
}

VISIT_METHOD_DECL(DestructorDecl) {
	auto name = "delete";

	return FnDecl(
		name,
		FnOverloadingRegistry(
			Location(context->KW_OPERATOR()),
			make_shared<VoidTypeNameNode>(Location(context->KW_OPERATOR()), false),
			{},
			{}));
}
VISIT_METHOD_DECL(DestructorDef) {
	auto decl = any_cast<FnDecl>(visit(context->destructorDecl()));
	decl.overloadingRegistry.body = static_pointer_cast<BlockStmtNode>(any_cast<shared_ptr<StmtNode>>(visit(context->codeBlock())));
	return decl;
}

VISIT_METHOD_DECL(InterfaceDef) {
	string name = context->ID()->getText();
	auto interface = make_shared<InterfaceNode>(
		Location(context->KW_INTERFACE()),
		name,
		context->implementList()
			? any_cast<deque<shared_ptr<CustomTypeNameNode>>>(visit(context->implementList()))
			: deque<shared_ptr<CustomTypeNameNode>>(),
		context->genericParams()
			? any_cast<GenericParamNodeList>(visit(context->genericParams()))
			: GenericParamNodeList());

	_putDefinition(Location(context->ID()), name, interface);

	auto savedScope = curScope;
	curScope = interface->scope;
	curScope->parent = savedScope.get();

	for (auto i : context->interfaceStmts())
		visit(i);

	curScope = savedScope;

	return interface;
}
VISIT_METHOD_DECL(InterfaceFnDecl) { return Any(); }
VISIT_METHOD_DECL(InterfaceOperatorDecl) { return Any(); }

VISIT_METHOD_DECL(TraitDef) { return Any(); }
VISIT_METHOD_DECL(TraitFnDecl) { return Any(); }
VISIT_METHOD_DECL(TraitOperatorDecl) { return Any(); }

VISIT_METHOD_DECL(OperatorAdd) { return "+"; }
VISIT_METHOD_DECL(OperatorSub) { return "-"; }
VISIT_METHOD_DECL(OperatorMul) { return "*"; }
VISIT_METHOD_DECL(OperatorDiv) { return "/"; }
VISIT_METHOD_DECL(OperatorMod) { return "%"; }
VISIT_METHOD_DECL(OperatorAnd) { return "&"; }
VISIT_METHOD_DECL(OperatorOr) { return "|"; }
VISIT_METHOD_DECL(OperatorXor) { return "^"; }
VISIT_METHOD_DECL(OperatorLAnd) { return "&&"; }
VISIT_METHOD_DECL(OperatorLOr) { return "||"; }
VISIT_METHOD_DECL(OperatorRev) { return "~"; }
VISIT_METHOD_DECL(OperatorNot) { return "!"; }
VISIT_METHOD_DECL(OperatorAssign) { return "="; }
VISIT_METHOD_DECL(OperatorAddAssign) { return "+="; }
VISIT_METHOD_DECL(OperatorSubAssign) { return "-="; }
VISIT_METHOD_DECL(OperatorMulAssign) { return "*="; }
VISIT_METHOD_DECL(OperatorDivAssign) { return "/="; }
VISIT_METHOD_DECL(OperatorModAssign) { return "%="; }
VISIT_METHOD_DECL(OperatorAndAssign) { return "&="; }
VISIT_METHOD_DECL(OperatorOrAssign) { return "|="; }
VISIT_METHOD_DECL(OperatorXorAssign) { return "^="; }
VISIT_METHOD_DECL(OperatorEq) { return "=="; }
VISIT_METHOD_DECL(OperatorNeq) { return "!="; }
VISIT_METHOD_DECL(OperatorGt) { return ">"; }
VISIT_METHOD_DECL(OperatorLt) { return "<"; }
VISIT_METHOD_DECL(OperatorGtEq) { return ">="; }
VISIT_METHOD_DECL(OperatorLtEq) { return "<="; }
VISIT_METHOD_DECL(OperatorSubscript) { return "[]"; }
VISIT_METHOD_DECL(OperatorCall) { return "()"; }

VISIT_METHOD_DECL(CodeBlock) {
	deque<shared_ptr<StmtNode>> stmts;
	for (auto &i : context->stmt()) {
		stmts.push_back(any_cast<shared_ptr<StmtNode>>(visit(i)));
	}
	return static_pointer_cast<StmtNode>(make_shared<BlockStmtNode>(
		Location(context->LBRACE()),
		stmts));
}

VISIT_METHOD_DECL(Args) {
	deque<shared_ptr<ExprNode>> args;

	for (auto i : context->expr())
		args.push_back(any_cast<shared_ptr<ExprNode>>(visit(i)));

	return args;
}

VISIT_METHOD_DECL(WrappedExpr) { return visit(context->expr()); }
VISIT_METHOD_DECL(HeadedRefExpr) {
	Ref ref;

	for (size_t i = 2;
		 i < context->children.size();
		 i += 2) {
		ref.push_back(any_cast<RefEntry>(visit(context->children[i])));
	}

	return static_pointer_cast<ExprNode>(
		make_shared<HeadedRefExprNode>(
			any_cast<shared_ptr<ExprNode>>(visit(context->expr())), ref));
}
VISIT_METHOD_DECL(RefExpr) {
	return static_pointer_cast<ExprNode>(
		make_shared<RefExprNode>(
			any_cast<Ref>(visit(context->ref()))));
}
VISIT_METHOD_DECL(LiteralExpr) { return visit(context->literal()); }
VISIT_METHOD_DECL(ArrayExpr) { return visit(context->array()); }
VISIT_METHOD_DECL(MapExpr) { return visit(context->map()); }
VISIT_METHOD_DECL(ClosureExpr) {
	return Any();
}
VISIT_METHOD_DECL(CallExpr) {
	return static_pointer_cast<ExprNode>(
		make_shared<CallExprNode>(
			any_cast<shared_ptr<ExprNode>>(visit(context->expr())),
			context->args()
				? any_cast<deque<shared_ptr<ExprNode>>>(visit(context->args()))
				: deque<shared_ptr<ExprNode>>{},
			context->KW_ASYNC() != nullptr));
}
VISIT_METHOD_DECL(AwaitExpr) {
	return static_pointer_cast<ExprNode>(
		make_shared<AwaitExprNode>(
			Location{ context->KW_AWAIT() },
			any_cast<shared_ptr<ExprNode>>(context->expr())));
}
VISIT_METHOD_DECL(NewExpr) {
	return static_pointer_cast<ExprNode>(
		make_shared<NewExprNode>(
			Location{ context->KW_NEW() },
			any_cast<shared_ptr<TypeNameNode>>(visit(context->typeName())),
			context->args()
				? any_cast<deque<shared_ptr<ExprNode>>>(visit(context->args()))
				: deque<shared_ptr<ExprNode>>{}));
}
VISIT_METHOD_DECL(NewArrayExpr) {
	return static_pointer_cast<ExprNode>(
		make_shared<NewExprNode>(
			Location{ context->KW_NEW() },
			any_cast<shared_ptr<TypeNameNode>>(visit(context->typeName())),
			context->array()
				? any_cast<deque<shared_ptr<ExprNode>>>(visit(context->array()))
				: deque<shared_ptr<ExprNode>>{}));
}
VISIT_METHOD_DECL(NewMapExpr) {
	return static_pointer_cast<ExprNode>(
		make_shared<NewExprNode>(
			Location{ context->KW_NEW() },
			any_cast<shared_ptr<TypeNameNode>>(visit(context->typeName())),
			context->map()
				? any_cast<deque<shared_ptr<ExprNode>>>(visit(context->map()))
				: deque<shared_ptr<ExprNode>>{}));
}
VISIT_METHOD_DECL(MatchExpr) { return Any(); }
VISIT_METHOD_DECL(CastExpr) {
	return static_pointer_cast<ExprNode>(make_shared<CastExprNode>(
		Location{ context->LPARENTHESE() },
		any_cast<shared_ptr<TypeNameNode>>(visit(context->typeName())),
		any_cast<shared_ptr<ExprNode>>(visit(context->expr()))));
}
VISIT_METHOD_DECL(TypeofExpr) {
	return static_pointer_cast<ExprNode>(make_shared<TypeofExprNode>(
		Location{ context->KW_TYPEOF() },
		any_cast<shared_ptr<ExprNode>>(visit(context->expr()))));
}
VISIT_METHOD_DECL(TypeTypeofExpr) { return Any(); }
VISIT_METHOD_DECL(SubscriptExpr) {
	auto lhs = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[0])),
		 rhs = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[1]));

	return static_pointer_cast<ExprNode>(make_shared<BinaryOpExprNode>(lhs->getLocation(), BinaryOp::Subscript, lhs, rhs));
}
VISIT_METHOD_DECL(ForwardIncDecExpr) {
	return static_pointer_cast<ExprNode>(make_shared<UnaryOpExprNode>(
		Location{ context->op },
		context->op->getType() == SlakeParser::OP_INC
			? UnaryOp::IncF
			: UnaryOp::DecF,
		any_cast<shared_ptr<ExprNode>>(visit(context->expr()))));
}
VISIT_METHOD_DECL(BackwardIncDecExpr) {
	auto x = any_cast<shared_ptr<ExprNode>>(visit(context->expr()));
	return static_pointer_cast<ExprNode>(make_shared<UnaryOpExprNode>(
		x->getLocation(),
		context->op->getType() == SlakeParser::OP_INC
			? UnaryOp::IncB
			: UnaryOp::DecB,
		x));
}
VISIT_METHOD_DECL(NotExpr) {
	return static_pointer_cast<ExprNode>(make_shared<UnaryOpExprNode>(
		Location{ context->OP_LNOT() },
		UnaryOp::LNot,
		any_cast<shared_ptr<ExprNode>>(visit(context->expr()))));
}
VISIT_METHOD_DECL(MulDivExpr) {
	auto lhs = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[0])),
		 rhs = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[1]));

	BinaryOp op;

	switch (context->op->getType()) {
		case SlakeParser::OP_MUL:
			op = BinaryOp::Mul;
			break;
		case SlakeParser::OP_DIV:
			op = BinaryOp::Div;
			break;
		case SlakeParser::OP_MOD:
			op = BinaryOp::Mod;
			break;
		default:
			throw std::logic_error("Unrecognized opeartion type");
	}

	return static_pointer_cast<ExprNode>(
		make_shared<BinaryOpExprNode>(
			lhs->getLocation(), op, lhs, rhs));
}
VISIT_METHOD_DECL(AddSubExpr) {
	auto lhs = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[0])),
		 rhs = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[1]));

	BinaryOp op;

	switch (context->op->getType()) {
		case SlakeParser::OP_ADD:
			op = BinaryOp::Add;
			break;
		case SlakeParser::OP_SUB:
			op = BinaryOp::Sub;
			break;
		default:
			throw std::logic_error("Unrecognized opeartion type");
	}

	return static_pointer_cast<ExprNode>(
		make_shared<BinaryOpExprNode>(
			lhs->getLocation(), op, lhs, rhs));
}
VISIT_METHOD_DECL(LtGtExpr) {
	auto lhs = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[0])),
		 rhs = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[1]));

	BinaryOp op;

	switch (context->op->getType()) {
		case SlakeParser::OP_LT:
			op = BinaryOp::Lt;
			break;
		case SlakeParser::OP_GT:
			op = BinaryOp::Gt;
			break;
		case SlakeParser::OP_LTEQ:
			op = BinaryOp::LtEq;
			break;
		case SlakeParser::OP_GTEQ:
			op = BinaryOp::GtEq;
			break;
		default:
			throw std::logic_error("Unrecognized opeartion type");
	}

	return static_pointer_cast<ExprNode>(
		make_shared<BinaryOpExprNode>(
			lhs->getLocation(), op, lhs, rhs));
}
VISIT_METHOD_DECL(ShiftExpr) {
	auto lhs = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[0])),
		 rhs = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[1]));

	BinaryOp op;

	switch (context->op->getType()) {
		case SlakeParser::OP_LSH:
			op = BinaryOp::Lsh;
			break;
		case SlakeParser::OP_RSH:
			op = BinaryOp::Rsh;
			break;
		default:
			throw std::logic_error("Unrecognized opeartion type");
	}

	return static_pointer_cast<ExprNode>(
		make_shared<BinaryOpExprNode>(
			lhs->getLocation(), op, lhs, rhs));
}
VISIT_METHOD_DECL(EqExpr) {
	auto lhs = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[0])),
		 rhs = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[1]));

	BinaryOp op;

	switch (context->op->getType()) {
		case SlakeParser::OP_EQ:
			op = BinaryOp::Eq;
			break;
		case SlakeParser::OP_NEQ:
			op = BinaryOp::Neq;
			break;
		default:
			throw std::logic_error("Unrecognized opeartion type");
	}

	return static_pointer_cast<ExprNode>(
		make_shared<BinaryOpExprNode>(
			lhs->getLocation(), op, lhs, rhs));
}
VISIT_METHOD_DECL(AndExpr) {
	auto lhs = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[0])),
		 rhs = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[1]));

	return static_pointer_cast<ExprNode>(
		make_shared<BinaryOpExprNode>(
			lhs->getLocation(), BinaryOp::And, lhs, rhs));
}
VISIT_METHOD_DECL(XorExpr) {
	auto lhs = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[0])),
		 rhs = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[1]));

	return static_pointer_cast<ExprNode>(
		make_shared<BinaryOpExprNode>(
			lhs->getLocation(), BinaryOp::Xor, lhs, rhs));
}
VISIT_METHOD_DECL(OrExpr) {
	auto lhs = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[0])),
		 rhs = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[1]));

	return static_pointer_cast<ExprNode>(
		make_shared<BinaryOpExprNode>(
			lhs->getLocation(), BinaryOp::Or, lhs, rhs));
}
VISIT_METHOD_DECL(LogicalAndExpr) {
	auto lhs = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[0])),
		 rhs = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[1]));

	return static_pointer_cast<ExprNode>(
		make_shared<BinaryOpExprNode>(
			lhs->getLocation(), BinaryOp::LAnd, lhs, rhs));
}
VISIT_METHOD_DECL(LogicalOrExpr) {
	auto lhs = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[0])),
		 rhs = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[1]));

	return static_pointer_cast<ExprNode>(
		make_shared<BinaryOpExprNode>(
			lhs->getLocation(), BinaryOp::LOr, lhs, rhs));
}
VISIT_METHOD_DECL(TernaryExpr) {
	auto cond = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[0])),
		 x = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[1])),
		 y = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[2]));

	return static_pointer_cast<ExprNode>(
		make_shared<TernaryOpExprNode>(cond, x, y));
}
VISIT_METHOD_DECL(AssignExpr) {
	auto lhs = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[0])),
		 rhs = any_cast<shared_ptr<ExprNode>>(visit(context->expr()[1]));

	BinaryOp op;

	switch (context->op->getType()) {
		case SlakeParser::OP_ASSIGN:
			op = BinaryOp::Assign;
			break;
		case SlakeParser::OP_ASSIGN_ADD:
			op = BinaryOp::AssignAdd;
			break;
		case SlakeParser::OP_ASSIGN_SUB:
			op = BinaryOp::AssignSub;
			break;
		case SlakeParser::OP_ASSIGN_MUL:
			op = BinaryOp::AssignMul;
			break;
		case SlakeParser::OP_ASSIGN_DIV:
			op = BinaryOp::AssignDiv;
			break;
		case SlakeParser::OP_ASSIGN_MOD:
			op = BinaryOp::AssignMod;
			break;
		case SlakeParser::OP_ASSIGN_AND:
			op = BinaryOp::AssignAnd;
			break;
		case SlakeParser::OP_ASSIGN_OR:
			op = BinaryOp::AssignOr;
			break;
		case SlakeParser::OP_ASSIGN_XOR:
			op = BinaryOp::AssignXor;
			break;
		case SlakeParser::OP_ASSIGN_LSH:
			op = BinaryOp::AssignLsh;
			break;
		case SlakeParser::OP_ASSIGN_RSH:
			op = BinaryOp::AssignRsh;
			break;
		default:
			throw std::logic_error("Unrecognized opeartion type");
	}

	return static_pointer_cast<ExprNode>(
		make_shared<BinaryOpExprNode>(
			lhs->getLocation(), op, lhs, rhs));
}

VISIT_METHOD_DECL(Array) { return Any(); }
VISIT_METHOD_DECL(Map) { return Any(); }
VISIT_METHOD_DECL(Pair) { return Any(); }

VISIT_METHOD_DECL(Int) { return visit(context->intLiteral()); }
VISIT_METHOD_DECL(Long) { return visit(context->longLiteral()); }
VISIT_METHOD_DECL(UInt) { return visit(context->uintLiteral()); }
VISIT_METHOD_DECL(ULong) { return visit(context->ulongLiteral()); }

VISIT_METHOD_DECL(BinInt) {
	// -0b
	std::string s = context->getText().substr(3);

	return static_pointer_cast<ExprNode>(
		make_shared<I32LiteralExprNode>(
			Location(context->L_INT_BIN()),
			-(int32_t)strtol(s.c_str(), nullptr, 2)));
}
VISIT_METHOD_DECL(OctInt) {
	// -0
	std::string s = context->getText().substr(2);

	return static_pointer_cast<ExprNode>(
		make_shared<I32LiteralExprNode>(
			Location(context->L_INT_OCT()),
			-(int32_t)strtol(s.c_str(), nullptr, 8)));
}
VISIT_METHOD_DECL(DecInt) {
	return static_pointer_cast<ExprNode>(
		make_shared<I32LiteralExprNode>(
			Location(context->L_INT_DEC()),
			(int32_t)strtol(context->getText().c_str(), nullptr, 10)));
}
VISIT_METHOD_DECL(HexInt) {
	// -0x
	std::string s = context->getText().substr(3);

	return static_pointer_cast<ExprNode>(
		make_shared<I32LiteralExprNode>(
			Location(context->L_INT_HEX()),
			-(int32_t)strtol(s.c_str(), nullptr, 2)));
}
VISIT_METHOD_DECL(BinLong) {
	// -0b
	std::string s = context->getText().substr(3);

	s.pop_back();  // Long suffix

	return static_pointer_cast<ExprNode>(
		make_shared<I64LiteralExprNode>(
			Location(context->L_LONG_BIN()),
			-(int64_t)strtoll(s.c_str(), nullptr, 2)));
}
VISIT_METHOD_DECL(OctLong) {
	// -0
	std::string s = context->getText().substr(2);

	s.pop_back();  // Long suffix

	return static_pointer_cast<ExprNode>(
		make_shared<I64LiteralExprNode>(
			Location(context->L_LONG_OCT()),
			-(int64_t)strtoll(s.c_str(), nullptr, 8)));
}
VISIT_METHOD_DECL(DecLong) {
	std::string s = context->getText();

	s.pop_back();  // Long suffix

	return static_pointer_cast<ExprNode>(
		make_shared<I64LiteralExprNode>(
			Location(context->L_LONG_DEC()),
			(int64_t)strtoll(s.c_str(), nullptr, 10)));
}
VISIT_METHOD_DECL(HexLong) {
	// -0x
	std::string s = context->getText().substr(3);

	s.pop_back();  // Long suffix

	return static_pointer_cast<ExprNode>(
		make_shared<I64LiteralExprNode>(
			Location(context->L_LONG_HEX()),
			-(int64_t)strtoll(s.c_str(), nullptr, 16)));
}
VISIT_METHOD_DECL(BinUInt) {
	// 0b
	std::string s = context->getText().substr(2);

	s.pop_back();  // Unsigend suffix

	return static_pointer_cast<ExprNode>(
		make_shared<U32LiteralExprNode>(
			Location(context->L_UINT_BIN()),
			(uint32_t)strtoul(s.c_str(), nullptr, 2)));
}
VISIT_METHOD_DECL(OctUInt) {
	// 0
	std::string s = context->getText().substr(1);

	s.pop_back();  // Unsigend suffix

	return static_pointer_cast<ExprNode>(
		make_shared<U32LiteralExprNode>(
			Location(context->L_UINT_OCT()),
			(uint32_t)strtoul(s.c_str(), nullptr, 8)));
}
VISIT_METHOD_DECL(DecUInt) {
	std::string s = context->getText();

	s.pop_back();  // Unsigned suffix

	return static_pointer_cast<ExprNode>(
		make_shared<U32LiteralExprNode>(
			Location(context->L_UINT_DEC()),
			(uint32_t)strtoul(s.c_str(), nullptr, 10)));
}
VISIT_METHOD_DECL(HexUInt) {
	// 0x
	std::string s = context->getText().substr(2);

	s.pop_back();  // Unsigend suffix

	return static_pointer_cast<ExprNode>(
		make_shared<U32LiteralExprNode>(
			Location(context->L_UINT_HEX()),
			(uint32_t)strtoul(s.c_str(), nullptr, 16)));
}
VISIT_METHOD_DECL(BinULong) {
	// 0b
	std::string s = context->getText().substr(2);

	s.resize(s.size() - 2);	 // Unsigned and long suffix

	return static_pointer_cast<ExprNode>(
		make_shared<U64LiteralExprNode>(Location(context->L_ULONG_BIN()), (uint64_t)strtoll(s.c_str(), nullptr, 2)));
}
VISIT_METHOD_DECL(OctULong) {
	// 0
	std::string s = context->getText().substr(1);

	s.resize(s.size() - 2);	 // Unsigned and long suffix

	return static_pointer_cast<ExprNode>(
		make_shared<U64LiteralExprNode>(Location(context->L_ULONG_OCT()), (uint64_t)strtoll(s.c_str(), nullptr, 8)));
}
VISIT_METHOD_DECL(DecULong) {
	std::string s = context->getText();

	s.resize(s.size() - 2);	 // Unsigned and long suffix

	return static_pointer_cast<ExprNode>(
		make_shared<U64LiteralExprNode>(Location(context->L_ULONG_DEC()), (uint64_t)strtoul(s.c_str(), nullptr, 10)));
}
VISIT_METHOD_DECL(HexULong) {
	// 0x
	std::string s = context->getText().substr(2);

	s.resize(s.size() - 2);	 // Unsigned and long suffix

	return static_pointer_cast<ExprNode>(
		make_shared<U64LiteralExprNode>(Location(context->L_ULONG_HEX()), (uint64_t)strtoul(s.c_str(), nullptr, 16)));
}

VISIT_METHOD_DECL(F32) {
	std::string s = context->getText();

	return static_pointer_cast<ExprNode>(
		make_shared<F32LiteralExprNode>(Location(context->L_F32()), strtof(s.c_str(), nullptr)));
}

VISIT_METHOD_DECL(F64) {
	std::string s = context->getText();

	return static_pointer_cast<ExprNode>(
		make_shared<F64LiteralExprNode>(Location(context->L_F64()), strtod(s.c_str(), nullptr)));
}

enum class StringParseState : uint8_t {
	Initial = 0,
	Escape,
	OctEscape,
	HexEscape,
	UnicodeEscape
};

VISIT_METHOD_DECL(String) {
	auto src = context->L_STRING()->getText();

	StringParseState state = StringParseState::Initial;

	std::string s;
	union {
		struct {
			char c;
			uint8_t nCharsEscaped;
		} esc;
		struct {
			uint16_t c;
			uint8_t nCharsEscaped;
		} unicodeEsc;
	} stateData;

	for (size_t i = 1; i < src.size() - 1; ++i) {
		auto c = src[i];
		switch (state) {
			case StringParseState::Initial:
				switch (c) {
					case '\\':
						state = StringParseState::Escape;
						break;
					default:
						s += c;
				}
				break;
			case StringParseState::Escape:
				switch (c) {
					case '\'':
						state = StringParseState::Initial;
						s += '\'';
						break;
					case '"':
						state = StringParseState::Initial;
						s += '"';
						break;
					case '\\':
						state = StringParseState::Initial;
						s += '\\';
						break;
					case 'a':
						state = StringParseState::Initial;
						s += '\a';
						break;
					case 'b':
						state = StringParseState::Initial;
						s += '\b';
						break;
					case 'f':
						state = StringParseState::Initial;
						s += '\f';
						break;
					case 'n':
						state = StringParseState::Initial;
						s += '\n';
						break;
					case 'r':
						state = StringParseState::Initial;
						s += '\r';
						break;
					case 't':
						state = StringParseState::Initial;
						s += '\t';
						break;
					case 'v':
						state = StringParseState::Initial;
						s += '\v';
						break;
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
						state = StringParseState::OctEscape;
						stateData.esc.nCharsEscaped = 1;
						stateData.esc.c = c - '0';
						break;
					case 'x':
					case 'X':
						state = StringParseState::HexEscape;
						stateData.esc = { 0 };
						break;
					case 'u':
					case 'U':
						state = StringParseState::UnicodeEscape;
						stateData.unicodeEsc = { 0 };
						break;
				}
				break;
			case StringParseState::OctEscape: {
				if ((stateData.esc.nCharsEscaped > 3) ||
					(c < '0' || c > '7')) {
					state = StringParseState::Initial;
					s += stateData.esc.c;
					break;
				}
				stateData.esc.c *= 10;
				stateData.esc.c += c - '0';
				++stateData.esc.nCharsEscaped;
				break;
			}
			case StringParseState::HexEscape: {
				if ((stateData.esc.nCharsEscaped > 2)) {
					state = StringParseState::Initial;
					s += stateData.esc.c;
					break;
				}
				stateData.esc.c <<= 4;
				stateData.esc.c += c - '0';
				++stateData.esc.nCharsEscaped;
				break;
			}
			case StringParseState::UnicodeEscape: {
				if ((stateData.unicodeEsc.nCharsEscaped > 4)) {
					state = StringParseState::Initial;
					s += stateData.esc.c;
					break;
				}
				stateData.unicodeEsc.c <<= 4;
				stateData.unicodeEsc.c += c - '0';
				++stateData.unicodeEsc.nCharsEscaped;
				break;
			}
		}
	}

	return static_pointer_cast<ExprNode>(make_shared<StringLiteralExprNode>(Location(context->L_STRING()->getSymbol()), s));
}
VISIT_METHOD_DECL(True) {
	return static_pointer_cast<ExprNode>(make_shared<BoolLiteralExprNode>(Location(context->KW_TRUE()), true));
}
VISIT_METHOD_DECL(False) {
	return static_pointer_cast<ExprNode>(make_shared<BoolLiteralExprNode>(Location(context->KW_FALSE()), false));
}

VISIT_METHOD_DECL(RefScope) {
	return RefEntry{
		Location(context->name),
		context->name->getText(),
		context->genericArgs()
			? any_cast<deque<shared_ptr<TypeNameNode>>>(visit(context->genericArgs()))
			: deque<shared_ptr<TypeNameNode>>{}
	};
}
VISIT_METHOD_DECL(NormalRef) {
	Ref ref;

	size_t i = 0;

	if (context->head) {
		switch (context->head->getType()) {
			case SlakeLexer::KW_THIS:
				ref.push_back({ Location(context->head), { "this" } });
				i += 2;
				break;
			case SlakeLexer::KW_BASE:
				ref.push_back({ Location(context->head), { "base" } });
				i += 2;
				break;
			case SlakeLexer::OP_SCOPE:
				ref.push_back({ Location(context->head), { "" } });
				++i;
				break;
		}
	}

	for (;
		 i < context->children.size();
		 i += 2) {
		ref.push_back(any_cast<RefEntry>(visit(context->children[i])));
	}
	return ref;
}
VISIT_METHOD_DECL(ThisRef) {
	Ref ref{
		{ Location(context->KW_THIS()), "this" }
	};
	return ref;
}
VISIT_METHOD_DECL(NewRef) {
	Ref ref;

	if (context->head)
		ref.push_back({ Location(context->head), context->head->getText() });
	ref.push_back({ Location(context->KW_NEW()), "new" });
	return ref;
}
VISIT_METHOD_DECL(ModuleRef) {
	Ref ref;

	for (auto i : context->ID()) {
		ref.push_back(RefEntry(Location(i), i->getText()));
	}

	return ref;
}
VISIT_METHOD_DECL(FnTypeName) {
	return Any();
}
VISIT_METHOD_DECL(ArrayTypeName) {
	return static_pointer_cast<TypeNameNode>(make_shared<ArrayTypeNameNode>(
		any_cast<shared_ptr<TypeNameNode>>(visit(context->children[0]))));
}
VISIT_METHOD_DECL(MapTypeName) {
	return static_pointer_cast<TypeNameNode>(make_shared<MapTypeNameNode>(
		any_cast<shared_ptr<TypeNameNode>>(visit(context->children[0])),
		any_cast<shared_ptr<TypeNameNode>>(visit(context->children[2]))));
}
VISIT_METHOD_DECL(AdoptPrimitiveTypeName) {
	return visit(context->children[0]);
}
VISIT_METHOD_DECL(AdoptCustomTypeName) {
	return visit(context->children[0]);
}
VISIT_METHOD_DECL(PrimitiveTypeName) {
	auto loc = Location(context->getToken(context->name->getType(), 0));
	switch (context->name->getType()) {
		case SlakeParser::TN_I8:
			return static_pointer_cast<TypeNameNode>(make_shared<I8TypeNameNode>(loc));
		case SlakeParser::TN_I16:
			return static_pointer_cast<TypeNameNode>(make_shared<I16TypeNameNode>(loc));
		case SlakeParser::TN_I32:
			return static_pointer_cast<TypeNameNode>(make_shared<I32TypeNameNode>(loc));
		case SlakeParser::TN_I64:
			return static_pointer_cast<TypeNameNode>(make_shared<I64TypeNameNode>(loc));
		case SlakeParser::TN_U8:
			return static_pointer_cast<TypeNameNode>(make_shared<U8TypeNameNode>(loc));
		case SlakeParser::TN_U16:
			return static_pointer_cast<TypeNameNode>(make_shared<U16TypeNameNode>(loc));
		case SlakeParser::TN_U32:
			return static_pointer_cast<TypeNameNode>(make_shared<U32TypeNameNode>(loc));
		case SlakeParser::TN_U64:
			return static_pointer_cast<TypeNameNode>(make_shared<U64TypeNameNode>(loc));
		case SlakeParser::TN_F32:
			return static_pointer_cast<TypeNameNode>(make_shared<F32TypeNameNode>(loc));
		case SlakeParser::TN_F64:
			return static_pointer_cast<TypeNameNode>(make_shared<F64TypeNameNode>(loc));
		case SlakeParser::TN_STRING:
			return static_pointer_cast<TypeNameNode>(make_shared<StringTypeNameNode>(loc));
		case SlakeParser::TN_AUTO:
			return static_pointer_cast<TypeNameNode>(make_shared<AutoTypeNameNode>(loc));
		case SlakeParser::TN_BOOL:
			return static_pointer_cast<TypeNameNode>(make_shared<BoolTypeNameNode>(loc));
		case SlakeParser::TN_VOID:
			return static_pointer_cast<TypeNameNode>(make_shared<VoidTypeNameNode>(loc));
		case SlakeParser::TN_ANY:
			return static_pointer_cast<TypeNameNode>(make_shared<AnyTypeNameNode>(loc));
	}

	throw logic_error("Unrecognized primitive type");
}
VISIT_METHOD_DECL(CustomTypeName) {
	return static_pointer_cast<TypeNameNode>(
		make_shared<CustomTypeNameNode>(
			Location(context->ref()->getStart()),
			any_cast<Ref>(visit(context->ref())),
			compiler,
			curScope.get()));
}
VISIT_METHOD_DECL(GenericArgs) {
	deque<shared_ptr<TypeNameNode>> args;
	for (auto i : context->typeName())
		args.push_back(any_cast<shared_ptr<TypeNameNode>>(visit(i)));
	return args;
}
VISIT_METHOD_DECL(Params) {
	deque<Param> params;
	for (auto i : context->paramDecl())
		params.push_back(any_cast<Param>(visit(i)));
	if (context->VARARG()) {
		auto varArg = context->VARARG();
		params.push_back(
			Param(
				Location(varArg),
				make_shared<ArrayTypeNameNode>(
					make_shared<AnyTypeNameNode>(
						Location(varArg))),
				"..."));
	}
	return params;
}
VISIT_METHOD_DECL(ParamDecl) {
	auto type = any_cast<shared_ptr<TypeNameNode>>(visit(context->typeName()));
	auto name = context->ID()->getText();
	Location loc =
		context->KW_CONST()
			? Location(context->KW_CONST())
			: loc = type->getLocation();

	return Param(loc, type, name);
}
