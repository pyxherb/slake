#ifndef _SLKC_AST_FN_H_
#define _SLKC_AST_FN_H_

#include "var.h"
#include "stmt.h"

namespace slkc {
	using FnFlags = uint32_t;

	class MacroNode : public MemberNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

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
		SLKC_API MacroNode(const MacroNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~MacroNode();
	};
}

#endif
