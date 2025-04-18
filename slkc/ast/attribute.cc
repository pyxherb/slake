#include "attribute.h"

using namespace slkc;

SLKC_API peff::SharedPtr<AstNode> AttributeDef::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<AttributeDef> duplicatedNode(peff::makeShared<AttributeDef>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API AttributeDef::AttributeDef(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: MemberNode(AstNodeType::Attribute, selfAllocator, document),
	  fields(selfAllocator),
	  fieldIndices(selfAllocator) {
}

SLKC_API AttributeDef::AttributeDef(const AttributeDef &rhs, peff::Alloc *allocator, bool &succeededOut) : MemberNode(rhs, allocator, succeededOut), fields(allocator), fieldIndices(allocator) {
	if (!succeededOut) {
		return;
	}

	if (!fields.resize(rhs.fields.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < fields.size(); ++i) {
		peff::SharedPtr<VarNode> df;
		if (!(df = rhs.fields.at(i)->duplicate<VarNode>(allocator))) {
			succeededOut = false;
			return;
		}

		df->setParent(this);

		fields.at(i) = df;

		if (!(fieldIndices.insert(df->name, std::move(df)))) {
			succeededOut = false;
			return;
		}
	}

	succeededOut = true;
}

SLKC_API AttributeDef::~AttributeDef() {
}

SLKC_API peff::SharedPtr<AstNode> AttributeNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<AttributeNode> duplicatedNode(peff::makeShared<AttributeNode>(newAllocator, *this, newAllocator, succeeded));
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
