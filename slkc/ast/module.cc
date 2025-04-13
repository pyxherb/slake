#include "module.h"
#include "import.h"

using namespace slkc;

SLKC_API MemberNode::MemberNode(
	AstNodeType astNodeType,
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: AstNode(astNodeType, selfAllocator, document),
	  name(selfAllocator),
	  genericArgs(selfAllocator) {
}

SLKC_API MemberNode::MemberNode(const MemberNode &rhs, peff::Alloc *allocator, bool &succeededOut) : AstNode(rhs, allocator), name(allocator), genericArgs(allocator) {
	if (!name.build(rhs.name)) {
		succeededOut = false;
		return;
	}

	accessModifier = rhs.accessModifier;

	if (!genericArgs.resize(rhs.genericArgs.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < genericArgs.size(); ++i) {
		if (!(genericArgs.at(i) = rhs.genericArgs.at(i)->duplicate<TypeNameNode>(allocator))) {
			succeededOut = false;
			return;
		}
	}

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
	  memberIndices(selfAllocator),
	  anonymousImports(selfAllocator),
	  varDefStmts(selfAllocator) {
}

SLKC_API ModuleNode::ModuleNode(const ModuleNode &rhs, peff::Alloc *allocator, bool &succeededOut) : MemberNode(rhs, allocator, succeededOut), members(allocator), memberIndices(allocator), anonymousImports(allocator), varDefStmts(allocator) {
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

	if (!anonymousImports.resize(rhs.anonymousImports.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < anonymousImports.size(); ++i) {
		if (!(anonymousImports.at(i) = rhs.anonymousImports.at(i)->duplicate<ImportNode>(allocator))) {
			succeededOut = false;
			return;
		}
	}

	if (!members.resize(rhs.members.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < members.size(); ++i) {
		if (!(members.at(i) = rhs.members.at(i)->duplicate<MemberNode>(allocator))) {
			succeededOut = false;
			return;
		}

		if (!indexMember(i)) {
			succeededOut = false;
			return;
		}
	}

	succeededOut = true;
}

SLKC_API ModuleNode::~ModuleNode() {
}

SLKC_API size_t ModuleNode::pushMember(peff::SharedPtr<MemberNode> memberNode) noexcept {
	size_t n = members.size();

	if (!members.pushBack(std::move(memberNode))) {
		return SIZE_MAX;
	}

	return n;
}

SLKC_API bool ModuleNode::addMember(peff::SharedPtr<MemberNode> memberNode) noexcept {
	size_t index;

	if ((index = pushMember(memberNode)) == SIZE_MAX) {
		return false;
	}

	return indexMember(index);
}

SLKC_API bool ModuleNode::indexMember(size_t indexInMemberArray) noexcept {
	peff::SharedPtr<MemberNode> m = members.at(indexInMemberArray);

	if (!memberIndices.insert(m->name, +indexInMemberArray)) {
		return false;
	}

	return true;
}

SLKC_API bool ModuleNode::removeMember(const std::string_view &name) noexcept {
	size_t index = memberIndices.at(name);
	if (!members.eraseRange(index, index + 1)) {
		return false;
	}
	memberIndices.remove(name);
	return true;
}
