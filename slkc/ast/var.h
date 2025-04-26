#ifndef _SLKC_AST_VAR_H_
#define _SLKC_AST_VAR_H_

#include "module.h"

namespace slkc {
	class NamespaceNode;

	class VarNode : public MemberNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::SharedPtr<TypeNameNode> type;
		peff::SharedPtr<ExprNode> initialValue;
		bool isTypeSealed = false;
		uint32_t idxReg = UINT32_MAX;

		SLKC_API VarNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API VarNode(const VarNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~VarNode();
	};
}

#endif
