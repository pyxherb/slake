#include "module.h"

using namespace slkc;

SLKC_API MemberNode::MemberNode(
	AstNodeType astNodeType,
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: AstNode(astNodeType, selfAllocator, document),
	  name(selfAllocator) {
}

SLKC_API MemberNode::MemberNode(const MemberNode &rhs, peff::Alloc *allocator, bool &succeededOut) : AstNode(rhs, allocator), name(allocator) {
	if (!name.build(rhs.name)) {
		succeededOut = false;
		return;
	}

	accessModifier = rhs.accessModifier;

	succeededOut = true;
}

SLKC_API MemberNode::~MemberNode() {
}

SLKC_API peff::SharedPtr<AstNode> ModuleNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<ModuleNode> duplicatedNode(peff::makeShared<ModuleNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API ModuleNode::ModuleNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document,
	AstNodeType astNodeType)
	: MemberNode(astNodeType, selfAllocator, document),
	  members(selfAllocator),
	  varDefStmts(selfAllocator) {
}

SLKC_API ModuleNode::ModuleNode(const ModuleNode &rhs, peff::Alloc *allocator, bool &succeededOut) : MemberNode(rhs, allocator, succeededOut), members(allocator), varDefStmts(allocator) {
	if (!succeededOut) {
		return;
	}

	if (!varDefStmts.resize(rhs.varDefStmts.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < varDefStmts.size(); ++i) {
		if (!(varDefStmts.at(i) = rhs.varDefStmts.at(i)->duplicate<VarDefStmtNode>(allocator))) {
			succeededOut = false;
			return;
		}
	}

	for (const auto &[name, member] : rhs.members) {
		peff::SharedPtr<MemberNode> duplicatedMember;
		if (!(duplicatedMember = member->duplicate<MemberNode>(allocator))) {
			succeededOut = false;
			return;
		}

		duplicatedMember->setParent(peff::WeakPtr<AstNode>(sharedFromThis()));

		if (!(members.insert(duplicatedMember->name, std::move(duplicatedMember)))) {
			succeededOut = false;
			return;
		}
	}

	succeededOut = true;
}

SLKC_API ModuleNode::~ModuleNode() {
}

SLKC_API bool ModuleNode::addMember(MemberNode *memberNode) noexcept {
	if (!members.insert(memberNode->name, memberNode->sharedFromThis().castTo<MemberNode>())) {
		return false;
	}
	return true;
}

SLKC_API void ModuleNode::removeMember(const std::string_view &name) noexcept {
	members.remove(name);
}
