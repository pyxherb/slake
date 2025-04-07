#include "var.h"

using namespace slkc;

SLKC_API peff::SharedPtr<AstNode> VarNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<VarNode> duplicatedNode(peff::makeShared<VarNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API VarNode::VarNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: MemberNode(AstNodeType::Var, selfAllocator, document) {
}

SLKC_API VarNode::VarNode(const VarNode &rhs, peff::Alloc *allocator, bool &succeededOut) : MemberNode(rhs, allocator, succeededOut) {
	if (!succeededOut) {
		return;
	}

	if (!(type = rhs.type->duplicate<TypeNameNode>(allocator))) {
		succeededOut = false;
		return;
	}

	if (!(initialValue = rhs.initialValue->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API VarNode::~VarNode() {
}
