#ifndef _SLKC_AST_VAR_H_
#define _SLKC_AST_VAR_H_

#include "module.h"

namespace slkc {
	class NamespaceNode;

	class VarNode : public MemberNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		bool is_type_deduced_from_initial_value = false;
		AstNodePtr<TypeNameNode> type;
		AstNodePtr<ExprNode> initial_value;
		uint32_t idx_reg = UINT32_MAX;

		SLKC_API VarNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API VarNode(const VarNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~VarNode();

		SLAKE_FORCEINLINE bool is_local_var() const {
			return !parent;
		}
	};
}

#endif
