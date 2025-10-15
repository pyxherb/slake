#include "macro.h"

using namespace slkc;

SLKC_API AstNodePtr<AstNode> MacroNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<MacroNode> duplicatedNode(makeAstNode<MacroNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API MacroNode::MacroNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: MemberNode(AstNodeType::Macro, selfAllocator, document),
	  params(selfAllocator),
	  paramIndices(selfAllocator),
	  idxParamCommaTokens(selfAllocator) {
}

SLKC_API MacroNode::MacroNode(const MacroNode &rhs, peff::Alloc *allocator, bool &succeededOut)
	: MemberNode(rhs, allocator, succeededOut),
	  params(allocator),
	  paramIndices(allocator),
	  idxParamCommaTokens(allocator) {
	/* if (rhs.body && !(body = rhs.body->duplicate<CodeBlockStmtNode>(allocator))) {
		succeededOut = false;
		return;
	}*/

	if (rhs.returnType && !(returnType = rhs.returnType->duplicate<TypeNameNode>(allocator))) {
		succeededOut = false;
		return;
	}

	if (!params.resize(rhs.params.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < params.size(); ++i) {
		if (!(params.at(i) = rhs.params.at(i)->duplicate<VarNode>(allocator))) {
			succeededOut = false;
			return;
		}

		params.at(i)->setParent(this);
	}

	for (auto i : rhs.paramIndices) {
		auto &curParam = params.at(i.second);
		if (!(paramIndices.insert(curParam->name, +i.second))) {
			succeededOut = false;
			return;
		}
	}

	isParamsIndexed = rhs.isParamsIndexed;

	succeededOut = true;
}

SLKC_API MacroNode::~MacroNode() {
}
