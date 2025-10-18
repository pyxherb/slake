#include "module.h"
#include "attribute.h"
#include "import.h"
#include "parser.h"
#include "document.h"

using namespace slkc;

SLKC_API MemberNode::MemberNode(
	AstNodeType astNodeType,
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: AstNode(astNodeType, selfAllocator, document),
	  name(selfAllocator),
	  genericArgs(selfAllocator),
	  attributes(selfAllocator) {
}

SLKC_API MemberNode::MemberNode(const MemberNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut) : AstNode(rhs, allocator, context), name(allocator), genericArgs(allocator), attributes(allocator) {
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

	if (!attributes.resize(rhs.attributes.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < attributes.size(); ++i) {
		if (!(attributes.at(i) = rhs.attributes.at(i)->duplicate<AttributeNode>(allocator))) {
			succeededOut = false;
			return;
		}
	}

	succeededOut = true;
}

SLKC_API MemberNode::~MemberNode() {
}

SLKC_API AstNodePtr<AstNode> ModuleNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<ModuleNode> duplicatedNode(makeAstNode<ModuleNode>(newAllocator, *this, newAllocator, context, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
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

SLKC_API ModuleNode::ModuleNode(const ModuleNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut) : MemberNode(rhs, allocator, context, succeededOut), members(allocator), memberIndices(allocator), anonymousImports(allocator), varDefStmts(allocator) {
	if (!succeededOut) {
		return;
	}

	parser = rhs.parser;

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
		AstNodePtr<MemberNode> &m = members.at(i);
		const AstNodePtr<MemberNode> &rm = rhs.members.at(i);
		if (!(m = rm->duplicate<MemberNode>(allocator))) {
			succeededOut = false;
			return;
		}

		if (!indexMember(i)) {
			succeededOut = false;
			return;
		}
	}

	isVarDefStmtsNormalized = rhs.isVarDefStmtsNormalized;

	succeededOut = true;
}

SLKC_API ModuleNode::~ModuleNode() {
}

SLKC_API size_t ModuleNode::pushMember(AstNodePtr<MemberNode> memberNode) noexcept {
	size_t n = members.size();

	if (!members.pushBack(std::move(memberNode))) {
		return SIZE_MAX;
	}

	return n;
}

SLKC_API bool ModuleNode::addMember(AstNodePtr<MemberNode> memberNode) noexcept {
	size_t index;

	if ((index = pushMember(memberNode)) == SIZE_MAX) {
		return false;
	}

	return indexMember(index);
}

SLKC_API bool ModuleNode::indexMember(size_t indexInMemberArray) noexcept {
	AstNodePtr<MemberNode> m = members.at(indexInMemberArray);

	if (!memberIndices.insert(m->name, +indexInMemberArray)) {
		return false;
	}

	m->setParent(this);

	return true;
}

SLKC_API bool ModuleNode::removeMember(const std::string_view &name) noexcept {
	size_t index = memberIndices.at(name);
	if (!members.eraseRange(index, index + 1)) {
		return false;
	}
	memberIndices.remove(name);
	for (auto i : memberIndices) {
		if (i.second > index) {
			--i.second;
		}
	}
	return true;
}

SLKC_API void ModuleNode::setParser(peff::SharedPtr<Parser> parser) {
	parser->document = {};
	parser->curParent = {};
	this->parser = parser;
}
