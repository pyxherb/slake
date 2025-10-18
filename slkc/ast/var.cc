#include "var.h"

using namespace slkc;

SLKC_API AstNodePtr<AstNode> VarNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<VarNode> duplicatedNode(makeAstNode<VarNode>(newAllocator, *this, newAllocator, context, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API VarNode::VarNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: MemberNode(AstNodeType::Var, selfAllocator, document) {
}

SLKC_API VarNode::VarNode(const VarNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut) : MemberNode(rhs, allocator, context, succeededOut) {
	if (!succeededOut) {
		return;
	}

	if (!context.pushTask([this, &rhs, allocator, &context]() -> bool {
			if (rhs.type && !(type = rhs.type->duplicate<TypeNameNode>(allocator)))
				return false;
			return true;
		})) {
		succeededOut = false;
		return;
	}

	if (!context.pushTask([this, &rhs, allocator, &context]() -> bool {
			if (rhs.initialValue && !(initialValue = rhs.initialValue->duplicate<ExprNode>(allocator)))
				return false;
			return true;
		})) {
		succeededOut = false;
		return;
	}

	isTypeDeducedFromInitialValue = rhs.isTypeDeducedFromInitialValue;
	idxReg = rhs.idxReg;

	succeededOut = true;
}

SLKC_API VarNode::~VarNode() {
}
