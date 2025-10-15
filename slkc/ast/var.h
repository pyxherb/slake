#ifndef _SLKC_AST_VAR_H_
#define _SLKC_AST_VAR_H_

#include "module.h"

namespace slkc {
	class NamespaceNode;

	class VarNode : public MemberNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		bool isTypeDeducedFromInitialValue = false;
		AstNodePtr<TypeNameNode> type;
		AstNodePtr<ExprNode> initialValue;
		uint32_t idxReg = UINT32_MAX;

		SLKC_API VarNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API VarNode(const VarNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~VarNode();
	};
}

#endif
