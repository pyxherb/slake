#include "var.h"

using namespace slkc;

SLKC_API peff::SharedPtr<AstNode> VarNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<VarNode> duplicatedNode(peff::makeSharedWithControlBlock<VarNode, AstNodeControlBlock<VarNode>>(newAllocator, *this, newAllocator, succeeded));
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

	if (rhs.initialValue && !(initialValue = rhs.initialValue->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}

	isTypeDeducedFromInitialValue = rhs.isTypeDeducedFromInitialValue;
	isTypeSealed = rhs.isTypeSealed;
	idxReg = rhs.idxReg;

	succeededOut = true;
}

SLKC_API VarNode::~VarNode() {
}
