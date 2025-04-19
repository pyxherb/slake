#ifndef _SLKC_AST_ATTRIBUTE_H_
#define _SLKC_AST_ATTRIBUTE_H_

#include "module.h"

namespace slkc {
	class AttributeDefNode : public ModuleNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		SLKC_API AttributeDefNode(peff::Alloc *allocator, const peff::SharedPtr<Document> &document);
		SLKC_API AttributeDefNode(const AttributeDefNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~AttributeDefNode();
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
