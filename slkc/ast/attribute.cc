#include "attribute.h"

using namespace slkc;

SLKC_API peff::SharedPtr<AstNode> AttributeDefNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<AttributeDefNode> duplicatedNode(peff::makeSharedWithControlBlock<AttributeDefNode, AstNodeControlBlock<AttributeDefNode>>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API AttributeDefNode::AttributeDefNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: ModuleNode(selfAllocator, document, AstNodeType::Attribute) {
}

SLKC_API AttributeDefNode::AttributeDefNode(const AttributeDefNode &rhs, peff::Alloc *allocator, bool &succeededOut) : ModuleNode(rhs, allocator, succeededOut) {
	if (!succeededOut) {
		return;
	}

	succeededOut = true;
}

SLKC_API AttributeDefNode::~AttributeDefNode() {
}

SLKC_API peff::SharedPtr<AstNode> AttributeNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<AttributeNode> duplicatedNode(peff::makeSharedWithControlBlock<AttributeNode, AstNodeControlBlock<AttributeNode>>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API AttributeNode::AttributeNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: AstNode(AstNodeType::Attribute, selfAllocator, document),
	  fieldData(selfAllocator),
	  idxCommaTokens(selfAllocator) {
}

SLKC_API AttributeNode::AttributeNode(const AttributeNode &rhs, peff::Alloc *allocator, bool &succeededOut) : AstNode(rhs, allocator), fieldData(allocator), idxCommaTokens(allocator) {
	if (!(attributeName = duplicateIdRef(allocator, rhs.attributeName.get()))) {
		succeededOut = false;
		return;
	}

	if (!fieldData.resize(rhs.fieldData.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < rhs.fieldData.size(); ++i) {
		peff::SharedPtr<ExprNode> dd;
		if (!(dd = rhs.fieldData.at(i)->duplicate<ExprNode>(allocator))) {
			succeededOut = false;
			return;
		}

		fieldData.at(i) = dd;
	}

	if (!(appliedFor = appliedFor->duplicate<TypeNameNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API AttributeNode::~AttributeNode() {
}
