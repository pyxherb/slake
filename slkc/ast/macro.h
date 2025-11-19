#ifndef _SLKC_AST_MACRO_H_
#define _SLKC_AST_MACRO_H_

#include "fn.h"

namespace slkc {
	class MacroNode : public MemberNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		peff::DynArray<AstNodePtr<VarNode>> params;
		peff::HashMap<std::string_view, size_t> paramIndices;
		bool isParamsIndexed = false;

		peff::DynArray<size_t> idxParamCommaTokens;
		size_t lParentheseIndex = SIZE_MAX;
		size_t rParentheseIndex = SIZE_MAX;

		AstNodePtr<TypeNameNode> returnType;

		AstNodePtr<CodeBlockStmtNode> body;

		SLKC_API MacroNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API MacroNode(const MacroNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~MacroNode();
	};
}

#endif
