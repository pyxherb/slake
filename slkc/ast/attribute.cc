#include "attribute.h"

using namespace slkc;

SLKC_API AstNodePtr<AstNode> AttributeDefNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<AttributeDefNode> duplicatedNode(makeAstNode<AttributeDefNode>(newAllocator, *this, newAllocator, context, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API AttributeDefNode::AttributeDefNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: ModuleNode(selfAllocator, document, AstNodeType::Attribute), genericParams(selfAllocator), genericParamIndices(selfAllocator), idxGenericParamCommaTokens(selfAllocator) {
}

SLKC_API AttributeDefNode::AttributeDefNode(const AttributeDefNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut) : ModuleNode(rhs, allocator, context, succeededOut), genericParams(allocator), genericParamIndices(allocator), idxGenericParamCommaTokens(allocator) {
	if (!succeededOut) {
		return;
	}

	succeededOut = true;
}

SLKC_API AttributeDefNode::~AttributeDefNode() {
}

SLKC_API AstNodePtr<AstNode> AttributeNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<AttributeNode> duplicatedNode(makeAstNode<AttributeNode>(newAllocator, *this, newAllocator, context, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API AttributeNode::AttributeNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: AstNode(AstNodeType::Attribute, selfAllocator, document),
	  fieldData(selfAllocator),
	  idxCommaTokens(selfAllocator) {
}

SLKC_API AttributeNode::AttributeNode(const AttributeNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut) : AstNode(rhs, allocator, context), fieldData(allocator), idxCommaTokens(allocator) {
	if (!(attributeName = duplicateIdRef(allocator, rhs.attributeName.get()))) {
		succeededOut = false;
		return;
	}

	if (!fieldData.resize(rhs.fieldData.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < rhs.fieldData.size(); ++i) {
		if (!context.pushTask([this, i, &rhs, allocator, &context]() -> bool {
				AstNodePtr<ExprNode> dd;
				if (!(dd = rhs.fieldData.at(i)->duplicate<ExprNode>(allocator))) {
					return false;
				}

				fieldData.at(i) = dd;
				return true;
			})) {
			succeededOut = false;
			return;
		}
	}

	if (!context.pushTask([this, &rhs, allocator, &context]() -> bool {
			if (!(appliedFor = appliedFor->duplicate<TypeNameNode>(allocator))) {
				return false;
			}
			return true;
		})) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API AttributeNode::~AttributeNode() {
}
