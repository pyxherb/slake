#include "cxxast.h"

using namespace slake;
using namespace slake::slkaot;
using namespace slake::slkaot::cxxast;

bool FnOverloadingSignatureComparator::operator()(const FnOverloadingSignature &lhs, const FnOverloadingSignature &rhs) {
	if(lhs.isConst < rhs.isConst)
		return true;
	if(lhs.isConst > rhs.isConst)
		return false;
	if (lhs.paramTypes.size() < rhs.paramTypes.size())
		return true;
	if (lhs.paramTypes.size() > rhs.paramTypes.size())
		return false;
	for (size_t i = 0; i < lhs.paramTypes.size(); ++i) {
		const Type &lhsType = lhs.paramTypes.at(i), &rhsType = rhs.paramTypes.at(i);

		if (lhsType < rhsType)
			return true;
		if (lhsType > rhsType)
			return false;
	}
	return false;
}

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

void AbstractModule::addPublicMember(std::string &&name, std::shared_ptr<AbstractMember> memberNode) {
	removeMember(name);
	publicMembers.insert({ std::move(name), memberNode });
}

void AbstractModule::addProtectedMember(std::string &&name, std::shared_ptr<AbstractMember> memberNode) {
	removeMember(name);
	protectedMembers.insert({ std::move(name), memberNode });
}

std::shared_ptr<AbstractMember> AbstractModule::getMember() {
	if (auto it = publicMembers.find(name); it != publicMembers.end()) {
		return it->second;
	}
	if (auto it = protectedMembers.find(name); it != protectedMembers.end()) {
		return it->second;
	}
}

void AbstractModule::removeMember(const std::string &name) {
	if (auto it = publicMembers.find(name); it != publicMembers.end()) {
		publicMembers.erase(it);
	}
	if (auto it = protectedMembers.find(name); it != protectedMembers.end()) {
		protectedMembers.erase(it);
	}
}

FnOverloading::FnOverloading() : ASTNode(NodeKind::FnOverloading) {
}

FnOverloading::~FnOverloading() {
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

ArrayTypeName::ArrayTypeName(std::shared_ptr<TypeName> elementType, size_t rank) : TypeName(TypeNameKind::Array), elementType(elementType), rank(rank) {
}

ArrayTypeName::~ArrayTypeName() {
}

PointerTypeName::PointerTypeName(std::shared_ptr<TypeName> pointedType)
	: TypeName(TypeNameKind::Pointer), pointedType(pointedType) {
}

PointerTypeName::~PointerTypeName() {
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

Stmt::Stmt(StmtKind stmtKind) : ASTNode(NodeKind::Stmt) {
}

Stmt::~Stmt() {
}

Expr::Expr(ExprKind stmtKind) : ASTNode(NodeKind::Expr) {
}

Expr::~Expr() {
}

NullptrLiteralExpr::NullptrLiteralExpr() : Expr(ExprKind::NullptrLiteral) {
}

NullptrLiteralExpr::~NullptrLiteralExpr() {
}

IdExpr::IdExpr(std::string &&name) : Expr(ExprKind::Id), name(std::move(name)) {
}

IdExpr::~IdExpr() {
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
