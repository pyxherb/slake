#include "fn.h"

using namespace slkc;

SLKC_API peff::SharedPtr<AstNode> FnSlotNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<FnSlotNode> duplicatedNode(peff::makeShared<FnSlotNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API FnSlotNode::FnSlotNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: MemberNode(AstNodeType::FnSlot, selfAllocator, document),
	  overloadings(selfAllocator) {
}

SLKC_API FnSlotNode::FnSlotNode(const FnSlotNode &rhs, peff::Alloc *allocator, bool &succeededOut) : MemberNode(rhs, allocator, succeededOut), overloadings(allocator) {
	if (!succeededOut) {
		return;
	}

	if (!overloadings.resize(rhs.overloadings.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < overloadings.size(); ++i) {
		if (!(overloadings.at(i) = rhs.overloadings.at(i)->duplicate<FnNode>(allocator))) {
			succeededOut = false;
			return;
		}

		overloadings.at(i)->setParent(sharedFromThis().castTo<MemberNode>());
	}

	succeededOut = true;
}

SLKC_API FnSlotNode::~FnSlotNode() {
}

SLKC_API peff::SharedPtr<AstNode> FnNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<FnNode> duplicatedNode(peff::makeShared<FnNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API FnNode::FnNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: MemberNode(AstNodeType::Fn, selfAllocator, document),
	  params(selfAllocator),
	  paramIndices(selfAllocator),
	  genericParams(selfAllocator),
	  genericParamIndices(selfAllocator),
	  idxParamCommaTokens(selfAllocator),
	  idxGenericParamCommaTokens(selfAllocator) {
}

SLKC_API FnNode::FnNode(const FnNode &rhs, peff::Alloc *allocator, bool &succeededOut)
	: MemberNode(rhs, allocator, succeededOut),
	  params(allocator),
	  paramIndices(allocator),
	  genericParams(allocator),
	  genericParamIndices(allocator),
	  idxParamCommaTokens(allocator),
	  idxGenericParamCommaTokens(allocator),
	  lAngleBracketIndex(rhs.lAngleBracketIndex),
	  rAngleBracketIndex(rhs.rAngleBracketIndex) {
	if (!(body = rhs.body->duplicate<CodeBlockStmtNode>(allocator))) {
		succeededOut = false;
		return;
	}

	if (!(returnType = rhs.returnType->duplicate<TypeNameNode>(allocator))) {
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

		if (!paramIndices.insert(params.at(i)->name, +i)) {
			succeededOut = false;
			return;
		}

		params.at(i)->setParent(sharedFromThis().castTo<MemberNode>());
	}

	if (!genericParams.resize(rhs.genericParams.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < genericParams.size(); ++i) {
		if (!(genericParams.at(i) = rhs.genericParams.at(i)->duplicate<GenericParamNode>(allocator))) {
			succeededOut = false;
			return;
		}

		if (!genericParamIndices.insert(genericParams.at(i)->name, +i)) {
			succeededOut = false;
			return;
		}

		genericParams.at(i)->setParent(sharedFromThis().castTo<MemberNode>());
	}

	if (!idxGenericParamCommaTokens.resize(rhs.idxGenericParamCommaTokens.size())) {
		succeededOut = false;
		return;
	}
	memcpy(idxGenericParamCommaTokens.data(), rhs.idxGenericParamCommaTokens.data(), idxGenericParamCommaTokens.size() * sizeof(size_t));

	succeededOut = true;
}

SLKC_API FnNode::~FnNode() {
}
