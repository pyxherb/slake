#include "stmt.h"

using namespace slkc;

SLKC_API StmtNode::StmtNode(StmtKind stmtKind, peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : AstNode(AstNodeType::Stmt, selfAllocator, document), stmtKind(stmtKind) {
}

SLKC_API StmtNode::StmtNode(const StmtNode &rhs, peff::Alloc *allocator) : AstNode(rhs, allocator), stmtKind(rhs.stmtKind) {
}

SLKC_API StmtNode::~StmtNode() {
}

SLKC_API peff::SharedPtr<AstNode> ExprStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<ExprStmtNode> duplicatedNode(peff::makeShared<ExprStmtNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API ExprStmtNode::ExprStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : StmtNode(StmtKind::Expr, selfAllocator, document) {
}

SLKC_API ExprStmtNode::ExprStmtNode(const ExprStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut) : StmtNode(rhs, allocator) {
	if(!(expr = rhs.expr->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API ExprStmtNode::~ExprStmtNode() {
}

SLKC_API peff::SharedPtr<AstNode> DeferStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<DeferStmtNode> duplicatedNode(peff::makeShared<DeferStmtNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API DeferStmtNode::DeferStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : StmtNode(StmtKind::Defer, selfAllocator, document) {
}

SLKC_API DeferStmtNode::DeferStmtNode(const DeferStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut) : StmtNode(rhs, allocator) {
	if (!(expr = rhs.expr->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API DeferStmtNode::~DeferStmtNode() {
}

SLKC_API VarDefEntry::VarDefEntry(peff::Alloc *selfAllocator, peff::String &&name, const peff::SharedPtr<TypeNameNode> &type, const peff::SharedPtr<ExprNode> &initialValue) : selfAllocator(selfAllocator), name(std::move(name)), type(type), initialValue(initialValue) {
}
SLKC_API VarDefEntry::~VarDefEntry() {
}
SLKC_API void VarDefEntry::dealloc() noexcept {
	peff::destroyAndRelease<VarDefEntry>(selfAllocator.get(), this, ASTNODE_ALIGNMENT);
}

SLKC_API VarDefEntryPtr slkc::duplicateVarDefEntry(VarDefEntry *varDefEntry, peff::Alloc *allocator) {
	peff::String copiedName(allocator);
	if (!copiedName.build(varDefEntry->name)) {
		return {};
	}

	peff::SharedPtr<TypeNameNode> type = varDefEntry->type->duplicate<TypeNameNode>(allocator);
	if (!type) {
		return {};
	}

	peff::SharedPtr<ExprNode> initialValue = varDefEntry->initialValue->duplicate<ExprNode>(allocator);
	if (!initialValue) {
		return {};
	}

	return VarDefEntryPtr(peff::allocAndConstruct<VarDefEntry>(allocator, ASTNODE_ALIGNMENT, allocator, std::move(copiedName), type, initialValue));
}

SLKC_API peff::SharedPtr<AstNode> VarDefStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<VarDefStmtNode> duplicatedNode(peff::makeShared<VarDefStmtNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API VarDefStmtNode::VarDefStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, peff::DynArray<VarDefEntryPtr> &&varDefEntries) : StmtNode(StmtKind::VarDef, selfAllocator, document), varDefEntries(std::move(varDefEntries)) {
}

SLKC_API VarDefStmtNode::VarDefStmtNode(const VarDefStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut) : StmtNode(rhs, allocator), varDefEntries(allocator) {
	if (!(varDefEntries.resize(rhs.varDefEntries.size()))) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < varDefEntries.size(); ++i) {
		if (!(varDefEntries.at(i) = duplicateVarDefEntry(rhs.varDefEntries.at(i).get(), allocator))) {
			succeededOut = false;
			return;
		}
	}

	succeededOut = true;
}

SLKC_API VarDefStmtNode::~VarDefStmtNode() {
}

SLKC_API peff::SharedPtr<AstNode> BreakStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<BreakStmtNode> duplicatedNode(peff::makeShared<BreakStmtNode>(newAllocator, *this, newAllocator));
	if (!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API BreakStmtNode::BreakStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : StmtNode(StmtKind::VarDef, selfAllocator, document) {
}

SLKC_API BreakStmtNode::BreakStmtNode(const BreakStmtNode &rhs, peff::Alloc *allocator) : StmtNode(rhs, allocator) {
}

SLKC_API BreakStmtNode::~BreakStmtNode() {
}

SLKC_API peff::SharedPtr<AstNode> ContinueStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<ContinueStmtNode> duplicatedNode(peff::makeShared<ContinueStmtNode>(newAllocator, *this, newAllocator));
	if (!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API ContinueStmtNode::ContinueStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : StmtNode(StmtKind::VarDef, selfAllocator, document) {
}

SLKC_API ContinueStmtNode::ContinueStmtNode(const ContinueStmtNode &rhs, peff::Alloc *allocator) : StmtNode(rhs, allocator) {
}

SLKC_API ContinueStmtNode::~ContinueStmtNode() {
}

SLKC_API peff::SharedPtr<AstNode> ForStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<ForStmtNode> duplicatedNode(peff::makeShared<ForStmtNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API ForStmtNode::ForStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : StmtNode(StmtKind::For, selfAllocator, document), varDefEntries(selfAllocator), cond(cond), step(step), body(body) {
}

SLKC_API ForStmtNode::ForStmtNode(const ForStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut) : StmtNode(rhs, allocator), varDefEntries(allocator) {
	if (!(varDefEntries.resize(rhs.varDefEntries.size()))) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < varDefEntries.size(); ++i) {
		if (!(varDefEntries.at(i) = duplicateVarDefEntry(rhs.varDefEntries.at(i).get(), allocator))) {
			succeededOut = false;
			return;
		}
	}

	if (!(cond = rhs.cond->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}

	if (!(step = rhs.step->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}

	if (!(body = rhs.body->duplicate<StmtNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API ForStmtNode::~ForStmtNode() {
}

SLKC_API peff::SharedPtr<AstNode> ForEachStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<ForEachStmtNode> duplicatedNode(peff::makeShared<ForEachStmtNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API ForEachStmtNode::ForEachStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, peff::String &&varName, const peff::SharedPtr<ExprNode> &cond, const peff::SharedPtr<StmtNode> &body) : StmtNode(StmtKind::ForEach, selfAllocator, document), varName(std::move(varName)), cond(cond), body(body) {
}

SLKC_API ForEachStmtNode::ForEachStmtNode(const ForEachStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut) : StmtNode(rhs, allocator), varName(allocator) {
	if (!varName.build(rhs.varName)) {
		succeededOut = false;
		return;
	}

	if (!(cond = rhs.cond->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}

	if (!(body = rhs.body->duplicate<StmtNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API ForEachStmtNode::~ForEachStmtNode() {
}

SLKC_API peff::SharedPtr<AstNode> WhileStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<WhileStmtNode> duplicatedNode(peff::makeShared<WhileStmtNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API WhileStmtNode::WhileStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : StmtNode(StmtKind::While, selfAllocator, document) {
}

SLKC_API WhileStmtNode::WhileStmtNode(const WhileStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut) : StmtNode(rhs, allocator) {
	if (!(cond = rhs.cond->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}

	if (!(body = rhs.body->duplicate<StmtNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API WhileStmtNode::~WhileStmtNode() {
}

SLKC_API peff::SharedPtr<AstNode> ReturnStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<ReturnStmtNode> duplicatedNode(peff::makeShared<ReturnStmtNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API ReturnStmtNode::ReturnStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const peff::SharedPtr<ExprNode> &value) : StmtNode(StmtKind::Return, selfAllocator, document), value(value) {
}

SLKC_API ReturnStmtNode::ReturnStmtNode(const ReturnStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut) : StmtNode(rhs, allocator) {
	if (!(value = rhs.value->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API ReturnStmtNode::~ReturnStmtNode() {
}

SLKC_API peff::SharedPtr<AstNode> YieldStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<YieldStmtNode> duplicatedNode(peff::makeShared<YieldStmtNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API YieldStmtNode::YieldStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const peff::SharedPtr<ExprNode> &value) : StmtNode(StmtKind::Yield, selfAllocator, document), value(value) {
}

SLKC_API YieldStmtNode::YieldStmtNode(const YieldStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut) : StmtNode(rhs, allocator) {
	if (!(value = rhs.value->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API YieldStmtNode::~YieldStmtNode() {
}

SLKC_API peff::SharedPtr<AstNode> IfStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<IfStmtNode> duplicatedNode(peff::makeShared<IfStmtNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API IfStmtNode::IfStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const peff::SharedPtr<ExprNode> &cond, const peff::SharedPtr<StmtNode> &trueBody, const peff::SharedPtr<StmtNode> &falseBody) : StmtNode(StmtKind::If, selfAllocator, document), cond(cond), trueBody(trueBody), falseBody(falseBody) {
}

SLKC_API IfStmtNode::IfStmtNode(const IfStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut) : StmtNode(rhs, allocator) {
	if (!(cond = rhs.cond->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}

	if (!(trueBody = rhs.trueBody->duplicate<StmtNode>(allocator))) {
		succeededOut = false;
		return;
	}

	if (!(falseBody = rhs.falseBody->duplicate<StmtNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API IfStmtNode::~IfStmtNode() {
}

SLKC_API peff::SharedPtr<AstNode> CodeBlockStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<CodeBlockStmtNode> duplicatedNode(peff::makeShared<CodeBlockStmtNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API CodeBlockStmtNode::CodeBlockStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document): StmtNode(StmtKind::CodeBlock, selfAllocator, document), body(selfAllocator) {
}

SLKC_API CodeBlockStmtNode::CodeBlockStmtNode(const CodeBlockStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut) : StmtNode(rhs, allocator), body(allocator) {
	if(!(body.resize(rhs.body.size()))) {
		succeededOut = false;
		return;
	}

	for(size_t i = 0 ; i < body.size(); ++i) {
		if(!(body.at(i) = rhs.body.at(i)->duplicate<StmtNode>(allocator))) {
			succeededOut = false;
			return;
		}
	}

	succeededOut = true;
}

SLKC_API CodeBlockStmtNode::~CodeBlockStmtNode() {
}

SLKC_API peff::SharedPtr<AstNode> BadStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded;
	peff::SharedPtr<BadStmtNode> duplicatedNode(peff::makeShared<BadStmtNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API BadStmtNode::BadStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const peff::SharedPtr<StmtNode> &body): StmtNode(StmtKind::Bad, selfAllocator, document), body(body) {
}

SLKC_API BadStmtNode::BadStmtNode(const BadStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut) : StmtNode(rhs, allocator) {
	if(!(body = rhs.body->duplicate<StmtNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API BadStmtNode::~BadStmtNode() {
}
