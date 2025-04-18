#ifndef _SLKC_AST_ATTRIBUTE_H_
#define _SLKC_AST_ATTRIBUTE_H_

#include "var.h"

namespace slkc {
	class AttributeDef : public MemberNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::DynArray<peff::SharedPtr<VarNode>> fields;
		peff::HashMap<std::string_view, size_t> fieldIndices;

		SLKC_API AttributeDef(peff::Alloc *allocator, const peff::SharedPtr<Document> &document);
		SLKC_API AttributeDef(const AttributeDef &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~AttributeDef();
	};

	class AttributeNode : public AstNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		IdRefPtr attributeName;
		peff::DynArray<peff::SharedPtr<ExprNode>> fieldData;
		peff::DynArray<size_t> idxCommaTokens;

		peff::SharedPtr<TypeNameNode> appliedFor;

		SLKC_API AttributeNode(peff::Alloc *allocator, const peff::SharedPtr<Document> &document);
		SLKC_API AttributeNode(const AttributeNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~AttributeNode();
	};
}

#endif
