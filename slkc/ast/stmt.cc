#include "stmt.h"

using namespace slkc;

SLKC_API StmtNode::StmtNode(StmtKind stmtKind, peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : AstNode(AstNodeType::Stmt, selfAllocator, document), stmtKind(stmtKind) {
}

SLKC_API StmtNode::StmtNode(const StmtNode &rhs, peff::Alloc *allocator) : AstNode(rhs, allocator), stmtKind(rhs.stmtKind) {
}

SLKC_API StmtNode::~StmtNode() {
}

SLKC_API AstNodePtr<AstNode> ExprStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<ExprStmtNode> duplicatedNode(makeAstNode<ExprStmtNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API ExprStmtNode::ExprStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : StmtNode(StmtKind::Expr, selfAllocator, document) {
}

SLKC_API ExprStmtNode::ExprStmtNode(const ExprStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut) : StmtNode(rhs, allocator) {
	if (!(expr = rhs.expr->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API ExprStmtNode::~ExprStmtNode() {
}

SLKC_API VarDefEntry::VarDefEntry(peff::Alloc *selfAllocator, peff::String &&name, const AstNodePtr<TypeNameNode> &type, const AstNodePtr<ExprNode> &initialValue, bool isTypeSealed) : selfAllocator(selfAllocator), name(std::move(name)), type(type), initialValue(initialValue), isTypeSealed(isTypeSealed) {
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

	AstNodePtr<TypeNameNode> type = varDefEntry->type->duplicate<TypeNameNode>(allocator);
	if (!type) {
		return {};
	}

	AstNodePtr<ExprNode> initialValue;
	if (varDefEntry->initialValue) {
		if (!(initialValue = varDefEntry->initialValue->duplicate<ExprNode>(allocator))) {
			return {};
		}
	}

	return VarDefEntryPtr(peff::allocAndConstruct<VarDefEntry>(allocator, ASTNODE_ALIGNMENT, allocator, std::move(copiedName), type, initialValue));
}

SLKC_API AstNodePtr<AstNode> VarDefStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<VarDefStmtNode> duplicatedNode(makeAstNode<VarDefStmtNode>(newAllocator, *this, newAllocator, succeeded));
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

	accessModifier = rhs.accessModifier;

	succeededOut = true;
}

SLKC_API VarDefStmtNode::~VarDefStmtNode() {
}

SLKC_API AstNodePtr<AstNode> BreakStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	AstNodePtr<BreakStmtNode> duplicatedNode(makeAstNode<BreakStmtNode>(newAllocator, *this, newAllocator));
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

SLKC_API AstNodePtr<AstNode> ContinueStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	AstNodePtr<ContinueStmtNode> duplicatedNode(makeAstNode<ContinueStmtNode>(newAllocator, *this, newAllocator));
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

SLKC_API AstNodePtr<AstNode> ForStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<ForStmtNode> duplicatedNode(makeAstNode<ForStmtNode>(newAllocator, *this, newAllocator, succeeded));
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

SLKC_API AstNodePtr<AstNode> ForEachStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<ForEachStmtNode> duplicatedNode(makeAstNode<ForEachStmtNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API ForEachStmtNode::ForEachStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, peff::String &&varName, const AstNodePtr<ExprNode> &cond, const AstNodePtr<StmtNode> &body) : StmtNode(StmtKind::ForEach, selfAllocator, document), varName(std::move(varName)), cond(cond), body(body) {
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

SLKC_API AstNodePtr<AstNode> WhileStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<WhileStmtNode> duplicatedNode(makeAstNode<WhileStmtNode>(newAllocator, *this, newAllocator, succeeded));
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

SLKC_API AstNodePtr<AstNode> ReturnStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<ReturnStmtNode> duplicatedNode(makeAstNode<ReturnStmtNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API ReturnStmtNode::ReturnStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const AstNodePtr<ExprNode> &value) : StmtNode(StmtKind::Return, selfAllocator, document), value(value) {
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

SLKC_API AstNodePtr<AstNode> YieldStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<YieldStmtNode> duplicatedNode(makeAstNode<YieldStmtNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API YieldStmtNode::YieldStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const AstNodePtr<ExprNode> &value) : StmtNode(StmtKind::Yield, selfAllocator, document), value(value) {
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

SLKC_API AstNodePtr<AstNode> IfStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<IfStmtNode> duplicatedNode(makeAstNode<IfStmtNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API IfStmtNode::IfStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : StmtNode(StmtKind::If, selfAllocator, document) {
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

SLKC_API WithConstraintEntry::WithConstraintEntry(peff::Alloc *selfAllocator) : selfAllocator(selfAllocator), genericParamName(selfAllocator) {}
SLKC_API WithConstraintEntry::~WithConstraintEntry() {}
SLKC_API void WithConstraintEntry::dealloc() noexcept {
	peff::destroyAndRelease<WithConstraintEntry>(selfAllocator.get(), this, alignof(WithConstraintEntry));
}

WithConstraintEntryPtr slkc::duplicateWithConstraintEntry(peff::Alloc *allocator, const WithConstraintEntry *constraint) {
	WithConstraintEntryPtr ptr(peff::allocAndConstruct<WithConstraintEntry>(allocator, alignof(WithConstraintEntry), allocator));

	if (!ptr) {
		return nullptr;
	}

	if (!ptr->genericParamName.build(constraint->genericParamName)) {
		return nullptr;
	}

	if (!(ptr->constraint = duplicateGenericConstraint(allocator, constraint->constraint.get()))) {
		return nullptr;
	}

	return ptr;
}

SLKC_API AstNodePtr<AstNode> WithStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<WithStmtNode> duplicatedNode(makeAstNode<WithStmtNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API WithStmtNode::WithStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : StmtNode(StmtKind::With, selfAllocator, document), constraints(selfAllocator) {
}

SLKC_API WithStmtNode::WithStmtNode(const WithStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut) : StmtNode(rhs, allocator), constraints(allocator) {
	if (!constraints.resize(rhs.constraints.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < rhs.constraints.size(); ++i) {
		if (!(constraints.at(i) = duplicateWithConstraintEntry(allocator, rhs.constraints.at(i).get()))) {
			succeededOut = false;
			return;
		}
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

SLKC_API WithStmtNode::~WithStmtNode() {
}

SLKC_API AstNodePtr<AstNode> CaseLabelStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<CaseLabelStmtNode> duplicatedNode(makeAstNode<CaseLabelStmtNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API CaseLabelStmtNode::CaseLabelStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : StmtNode(StmtKind::CaseLabel, selfAllocator, document) {
}

SLKC_API CaseLabelStmtNode::CaseLabelStmtNode(const CaseLabelStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut) : StmtNode(rhs, allocator) {
	if (!(condition = rhs.condition->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API CaseLabelStmtNode::~CaseLabelStmtNode() {
}

SLKC_API AstNodePtr<AstNode> SwitchStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<SwitchStmtNode> duplicatedNode(makeAstNode<SwitchStmtNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API SwitchStmtNode::SwitchStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : StmtNode(StmtKind::Switch, selfAllocator, document), caseOffsets(selfAllocator), body(selfAllocator) {
}

SLKC_API SwitchStmtNode::SwitchStmtNode(const SwitchStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut) : StmtNode(rhs, allocator), caseOffsets(allocator), body(allocator) {
	if (!(condition = rhs.condition->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}

	if (!(body.resize(rhs.body.size()))) {
		succeededOut = false;
		return;
	}

	if (!caseOffsets.resize(rhs.caseOffsets.size())) {
		succeededOut = false;
		return;
	}

	memcpy(caseOffsets.data(), rhs.caseOffsets.data(), sizeof(size_t) * caseOffsets.size());

	for (size_t i = 0; i < body.size(); ++i) {
		if (!(body.at(i) = rhs.body.at(i)->duplicate<StmtNode>(allocator))) {
			succeededOut = false;
			return;
		}
	}

	succeededOut = true;
}

SLKC_API SwitchStmtNode::~SwitchStmtNode() {
}

SLKC_API AstNodePtr<AstNode> LabelStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<LabelStmtNode> duplicatedNode(makeAstNode<LabelStmtNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API LabelStmtNode::LabelStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : StmtNode(StmtKind::Label, selfAllocator, document), name(selfAllocator) {
}

SLKC_API LabelStmtNode::LabelStmtNode(const LabelStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut) : StmtNode(rhs, allocator), name(allocator) {
	if (!name.build(rhs.name)) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API LabelStmtNode::~LabelStmtNode() {
}

SLKC_API AstNodePtr<AstNode> CodeBlockStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<CodeBlockStmtNode> duplicatedNode(makeAstNode<CodeBlockStmtNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API CodeBlockStmtNode::CodeBlockStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : StmtNode(StmtKind::CodeBlock, selfAllocator, document), body(selfAllocator) {
}

SLKC_API CodeBlockStmtNode::CodeBlockStmtNode(const CodeBlockStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut) : StmtNode(rhs, allocator), body(allocator) {
	if (!(body.resize(rhs.body.size()))) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < body.size(); ++i) {
		if (!(body.at(i) = rhs.body.at(i)->duplicate<StmtNode>(allocator))) {
			succeededOut = false;
			return;
		}
	}

	succeededOut = true;
}

SLKC_API CodeBlockStmtNode::~CodeBlockStmtNode() {
}

SLKC_API AstNodePtr<AstNode> BadStmtNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded;
	AstNodePtr<BadStmtNode> duplicatedNode(makeAstNode<BadStmtNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API BadStmtNode::BadStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const AstNodePtr<StmtNode> &body) : StmtNode(StmtKind::Bad, selfAllocator, document), body(body) {
}

SLKC_API BadStmtNode::BadStmtNode(const BadStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut) : StmtNode(rhs, allocator) {
	if (!(body = rhs.body->duplicate<StmtNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API BadStmtNode::~BadStmtNode() {
}
