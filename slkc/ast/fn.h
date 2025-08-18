#ifndef _SLKC_AST_FN_H_
#define _SLKC_AST_FN_H_

#include "var.h"
#include "stmt.h"
#include "generic.h"

namespace slkc {
	using FnFlags = uint32_t;

	constexpr static FnFlags FN_PURE = 0x00000001, FN_VARG = 0x00000002, FN_VIRTUAL = 0x00000004;

	class FnOverloadingNode;

	class FnNode : public MemberNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::DynArray<AstNodePtr<FnOverloadingNode>> overloadings;

		SLKC_API FnNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API FnNode(const FnNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~FnNode();
	};

	class FnOverloadingNode : public MemberNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::DynArray<AstNodePtr<VarNode>> params;
		peff::HashMap<std::string_view, size_t> paramIndices;
		bool isParamsIndexed = false;

		peff::DynArray<size_t> idxParamCommaTokens;
		size_t lParentheseIndex = SIZE_MAX;
		size_t rParentheseIndex = SIZE_MAX;

		peff::DynArray<AstNodePtr<GenericParamNode>> genericParams;
		peff::HashMap<std::string_view, size_t> genericParamIndices;
		peff::DynArray<size_t> idxGenericParamCommaTokens;
		size_t lAngleBracketIndex = SIZE_MAX;
		size_t rAngleBracketIndex = SIZE_MAX;

		AstNodePtr<TypeNameNode> returnType;

		AstNodePtr<CodeBlockStmtNode> body;
		FnFlags fnFlags = 0;

		SLKC_API FnOverloadingNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API FnOverloadingNode(const FnOverloadingNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~FnOverloadingNode();
	};
}

#endif
