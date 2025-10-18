#ifndef _SLKC_AST_ATTRIBUTE_H_
#define _SLKC_AST_ATTRIBUTE_H_

#include "var.h"
#include "fn.h"

namespace slkc {
	class AttributeDefNode : public ModuleNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		peff::DynArray<AstNodePtr<GenericParamNode>> genericParams;
		peff::HashMap<std::string_view, size_t> genericParamIndices;
		peff::DynArray<size_t> idxGenericParamCommaTokens;
		size_t idxLAngleBracketToken = SIZE_MAX, idxRAngleBracketToken = SIZE_MAX;

		bool isGenericParamsIndexed = false;

		SLKC_API AttributeDefNode(peff::Alloc *allocator, const peff::SharedPtr<Document> &document);
		SLKC_API AttributeDefNode(const AttributeDefNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~AttributeDefNode();
	};

	class AttributeNode : public AstNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		IdRefPtr attributeName;
		peff::DynArray<AstNodePtr<ExprNode>> fieldData;
		peff::DynArray<size_t> idxCommaTokens;

		AstNodePtr<TypeNameNode> appliedFor;

		SLKC_API AttributeNode(peff::Alloc *allocator, const peff::SharedPtr<Document> &document);
		SLKC_API AttributeNode(const AttributeNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~AttributeNode();
	};
}

#endif
