#include "import.h"

using namespace slkc;

SLKC_API peff::SharedPtr<AstNode> ImportNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<ImportNode> duplicatedNode(peff::makeSharedWithControlBlock<ImportNode, AstNodeControlBlock<ImportNode>>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API ImportNode::ImportNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: MemberNode(AstNodeType::Import, selfAllocator, document) {
}

SLKC_API ImportNode::ImportNode(const ImportNode &rhs, peff::Alloc *allocator, bool &succeededOut) : MemberNode(rhs, allocator, succeededOut) {
	if (!succeededOut) {
		return;
	}

	if (!(idRef = duplicateIdRef(allocator, rhs.idRef.get()))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API ImportNode::~ImportNode() {
}
