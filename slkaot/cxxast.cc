#include "cxxast.h"

using namespace slake;
using namespace slake::slkaot;
using namespace slake::slkaot::cxxast;

ASTNode::ASTNode(NodeKind nodeKind) : nodeKind(nodeKind) {
}

ASTNode::~ASTNode() {
}

AbstractMember::AbstractMember(NodeKind nodeKind, std::string &&name) : ASTNode(nodeKind), name(std::move(name)) {
}

AbstractMember::~AbstractMember() {
}

AbstractModule::AbstractModule(NodeKind nodeKind, std::string &&name) : AbstractMember(nodeKind, std::move(name)) {
}

AbstractModule::~AbstractModule() {
}

void AbstractModule::addPublicMember(std::shared_ptr<AbstractMember> memberNode) {
	removeMember(memberNode->name);
	publicMembers.insert({ memberNode->name, memberNode });
	memberNode->parent = weak_from_this();
}

void AbstractModule::addProtectedMember(std::shared_ptr<AbstractMember> memberNode) {
	removeMember(memberNode->name);
	protectedMembers.insert({ memberNode->name, memberNode });
	memberNode->parent = weak_from_this();
}

std::shared_ptr<AbstractMember> AbstractModule::getMember(const std::string_view &name) {
	if (auto it = publicMembers.find(name); it != publicMembers.end()) {
		return it->second;
	}
	if (auto it = protectedMembers.find(name); it != protectedMembers.end()) {
		return it->second;
	}
	return {};
}

void AbstractModule::removeMember(const std::string_view &name) {
	if (auto it = publicMembers.find(name); it != publicMembers.end()) {
		publicMembers.erase(it);
	}
	if (auto it = protectedMembers.find(name); it != protectedMembers.end()) {
		protectedMembers.erase(it);
	}
}

Fn::Fn(std::string &&name) : AbstractMember(NodeKind::Fn, std::move(name)) {
}

Fn::~Fn() {
}

Struct::Struct(std::string &&name) : AbstractModule(NodeKind::Struct, std::move(name)) {
}

Struct::~Struct() {
}

Class::Class(std::string &&name) : AbstractModule(NodeKind::Class, std::move(name)) {
}

Class::~Class() {
}

Namespace::Namespace(std::string &&name) : AbstractModule(NodeKind::Namespace, std::move(name)) {
}

Namespace::~Namespace() {
}

TypeName::TypeName(TypeNameKind typeNameKind) : ASTNode(NodeKind::TypeName), typeNameKind(typeNameKind) {
}

TypeName::~TypeName() {
}

VoidTypeName::VoidTypeName() : TypeName(TypeNameKind::Void) {
}

VoidTypeName::~VoidTypeName() {
}

IntTypeName::IntTypeName(
	SignKind signKind,
	IntModifierKind modifierKind)
	: TypeName(TypeNameKind::Int),
	  signKind(signKind),
	  modifierKind(modifierKind) {
}

IntTypeName::~IntTypeName() {
}

BoolTypeName::BoolTypeName() : TypeName(TypeNameKind::Bool) {
}

BoolTypeName::~BoolTypeName() {
}

CharTypeName::CharTypeName(
	SignKind signKind)
	: TypeName(TypeNameKind::Int),
	  signKind(signKind) {
}

CharTypeName::~CharTypeName() {
}

FloatTypeName::FloatTypeName() : TypeName(TypeNameKind::Float) {
}

FloatTypeName::~FloatTypeName() {
}

DoubleTypeName::DoubleTypeName() : TypeName(TypeNameKind::Double) {
}

DoubleTypeName::~DoubleTypeName() {
}

ArrayTypeName::ArrayTypeName(std::shared_ptr<TypeName> elementType) : TypeName(TypeNameKind::Array), elementType(elementType) {
}

ArrayTypeName::~ArrayTypeName() {
}

PointerTypeName::PointerTypeName(std::shared_ptr<TypeName> pointedType)
	: TypeName(TypeNameKind::Pointer), pointedType(pointedType) {
}

PointerTypeName::~PointerTypeName() {
}

RefTypeName::RefTypeName(std::shared_ptr<TypeName> refType)
	: TypeName(TypeNameKind::Ref), refType(refType) {
}

RefTypeName::~RefTypeName() {
}

RvalueTypeName::RvalueTypeName(std::shared_ptr<TypeName> refType)
	: TypeName(TypeNameKind::Rvalue), refType(refType) {
}

RvalueTypeName::~RvalueTypeName() {
}

FnPointerTypeName::FnPointerTypeName(std::shared_ptr<TypeName> returnType,
	std::vector<std::shared_ptr<TypeName>> &&paramTypes,
	bool hasVarArgs)
	: TypeName(TypeNameKind::FnPointer), returnType(returnType), paramTypes(std::move(paramTypes)), hasVarArgs(hasVarArgs) {
}

FnPointerTypeName::~FnPointerTypeName() {
}

CustomTypeName::CustomTypeName(
	bool hasTypeNamePrefix,
	std::shared_ptr<Expr> refTarget)
	: TypeName(TypeNameKind::Custom),
	  hasTypeNamePrefix(hasTypeNamePrefix),
	  refTarget(refTarget) {
}

CustomTypeName::~CustomTypeName() {
}

Stmt::Stmt(StmtKind stmtKind) : ASTNode(NodeKind::Stmt), stmtKind(stmtKind) {
}

Stmt::~Stmt() {
}

ExprStmt::ExprStmt(std::shared_ptr<Expr> expr) : Stmt(StmtKind::Expr), expr(expr) {
}

ExprStmt::~ExprStmt() {
}

LocalVarDefStmt::LocalVarDefStmt(std::shared_ptr<TypeName> type,
	std::vector<VarDefPair> &&varDefPairs)
	: Stmt(StmtKind::LocalVarDef),
	  type(type),
	  varDefPairs(std::move(varDefPairs)) {
}

LocalVarDefStmt::~LocalVarDefStmt() {
}

IfStmt::IfStmt(
	std::shared_ptr<Expr> condition,
	std::shared_ptr<Stmt> trueBranch,
	std::shared_ptr<Stmt> elseBranch)
	: Stmt(StmtKind::If),
	  condition(condition),
	  trueBranch(trueBranch),
	  elseBranch(elseBranch) {
}

IfStmt::~IfStmt() {
}

ForStmt::ForStmt(
	std::shared_ptr<LocalVarDefStmt> varDefs,
	std::shared_ptr<Expr> condition,
	std::shared_ptr<Expr> endExpr,
	std::shared_ptr<Stmt> body)
	: Stmt(StmtKind::For),
	  varDefs(varDefs),
	  condition(condition),
	  endExpr(endExpr),
	  body(body) {
}

ForStmt::~ForStmt() {
}

WhileStmt::WhileStmt(
	std::shared_ptr<Expr> condition,
	std::shared_ptr<Stmt> body)
	: Stmt(StmtKind::While),
	  condition(condition),
	  body(body) {
}

WhileStmt::~WhileStmt() {
}

BreakStmt::BreakStmt()
	: Stmt(StmtKind::Break) {
}

BreakStmt::~BreakStmt() {
}

ContinueStmt::ContinueStmt()
	: Stmt(StmtKind::Continue) {
}

ContinueStmt::~ContinueStmt() {
}

ReturnStmt::ReturnStmt(std::shared_ptr<Expr> value) : Stmt(StmtKind::Return), value(value) {
}

ReturnStmt::~ReturnStmt() {
}

BlockStmt::BlockStmt(std::vector<std::shared_ptr<Stmt>> &&body) : Stmt(StmtKind::Block), body(std::move(body)) {
}

BlockStmt::~BlockStmt() {
}

LabelStmt::LabelStmt(std::string &&name) : Stmt(StmtKind::Label), name(std::move(name)) {
}

LabelStmt::~LabelStmt() {
}

GotoStmt::GotoStmt(std::string &&name) : Stmt(StmtKind::Goto), name(std::move(name)) {
}

GotoStmt::~GotoStmt() {
}

Expr::Expr(ExprKind stmtKind) : ASTNode(NodeKind::Expr), exprKind(stmtKind) {
}

Expr::~Expr() {
}

NullptrLiteralExpr::NullptrLiteralExpr() : Expr(ExprKind::NullptrLiteral) {
}

NullptrLiteralExpr::~NullptrLiteralExpr() {
}

IdExpr::IdExpr(std::string &&name, GenericArgList &&genericArgs) : Expr(ExprKind::Id), name(std::move(name)), genericArgs(genericArgs) {
}

IdExpr::~IdExpr() {
}

InitializerListExpr::InitializerListExpr(
	std::shared_ptr<TypeName> type,
	std::vector<std::shared_ptr<Expr>> &&args)
	: Expr(ExprKind::InitializerList),
	  type(type),
	  args(std::move(args)) {
}

InitializerListExpr::~InitializerListExpr() {
}

UnaryExpr::UnaryExpr(UnaryOp op, std::shared_ptr<Expr> operand) : Expr(ExprKind::Unary), op(op), operand(operand) {
}

UnaryExpr::~UnaryExpr() {
}

BinaryExpr::BinaryExpr(BinaryOp op, std::shared_ptr<Expr> lhs, std::shared_ptr<Expr> rhs) : Expr(ExprKind::Binary), op(op), lhs(lhs), rhs(rhs) {
}

BinaryExpr::~BinaryExpr() {
}

CallExpr::CallExpr(
	std::shared_ptr<Expr> callee,
	std::vector<std::shared_ptr<Expr>> &&args)
	: Expr(ExprKind::Call),
	  callee(callee),
	  args(std::move(args)) {
}

CallExpr::~CallExpr() {
}

CastExpr::CastExpr(std::shared_ptr<TypeName> destType, std::shared_ptr<Expr> source)
	: Expr(ExprKind::Cast),
	  destType(destType),
	  source(source) {
}

CastExpr::~CastExpr() {
}

NewExpr::NewExpr(
	std::shared_ptr<TypeName> type,
	std::vector<std::shared_ptr<Expr>> &&args)
	: Expr(ExprKind::New),
	  type(type),
	  args(std::move(args)) {
}

NewExpr::~NewExpr() {
}

ConditionalExpr::ConditionalExpr(
	std::shared_ptr<Expr> condition,
	std::shared_ptr<Expr> trueExpr,
	std::shared_ptr<Expr> falseExpr)
	: Expr(ExprKind::Conditional),
	  condition(condition),
	  trueExpr(trueExpr),
	  falseExpr(falseExpr) {
}

ConditionalExpr::~ConditionalExpr() {
}

TypeSizeofExpr::TypeSizeofExpr(std::shared_ptr<TypeName> target)
	: Expr(ExprKind::TypeSizeof),
	  target(target) {
}

TypeSizeofExpr::~TypeSizeofExpr() {
}

ExprSizeofExpr::ExprSizeofExpr(std::shared_ptr<Expr> target)
	: Expr(ExprKind::ExprSizeof),
	  target(target) {
}

ExprSizeofExpr::~ExprSizeofExpr() {
}

ThisExpr::ThisExpr() : Expr(ExprKind::This) {
}

ThisExpr::~ThisExpr() {
}

Directive::Directive(std::string &&name, std::vector<std::shared_ptr<Expr>> &&args)
	: ASTNode(NodeKind::Directive),
	  name(std::move(name)),
	  args(std::move(args)) {
}

Directive::~Directive() {
}

IncludeDirective::IncludeDirective(
	std::string &&name,
	bool isSystem)
	: ASTNode(NodeKind::IncludeDirective),
	  name(std::move(name)),
	  isSystem(isSystem) {
}

IncludeDirective::~IncludeDirective() {
}

IfDirective::IfDirective(std::shared_ptr<Expr> condition,
	std::shared_ptr<ASTNode> trueBranch,
	std::shared_ptr<ASTNode> elseBranch)
	: ASTNode(NodeKind::IfDirective),
	  condition(condition),
	  trueBranch(trueBranch),
	  elseBranch(elseBranch) {
}

IfDirective::~IfDirective() {
}

IfdefDirective::IfdefDirective(
	std::string &&name,
	std::shared_ptr<ASTNode> trueBranch,
	std::shared_ptr<ASTNode> elseBranch)
	: ASTNode(NodeKind::IfdefDirective),
	  name(std::move(name)),
	  trueBranch(trueBranch),
	  elseBranch(elseBranch) {
}

IfdefDirective::~IfdefDirective() {
}

IfndefDirective::IfndefDirective(
	std::string &&name,
	std::vector<std::shared_ptr<ASTNode>> &&trueBranch,
	std::vector<std::shared_ptr<ASTNode>> &&elseBranch)
	: ASTNode(NodeKind::IfndefDirective),
	  name(std::move(name)),
	  trueBranch(std::move(trueBranch)),
	  elseBranch(std::move(elseBranch)) {
}

IfndefDirective::~IfndefDirective() {
}

Var::Var(
	std::string &&name,
	StorageClass storageClass,
	std::shared_ptr<TypeName> type,
	std::shared_ptr<Expr> initialValue)
	: AbstractMember(NodeKind::Var, std::move(name)),
	  storageClass(storageClass),
	  type(type),
	  initialValue(initialValue) {
}

Var::~Var() {
}
