#ifndef _SLKC_AST_FN_H_
#define _SLKC_AST_FN_H_

#include "var.h"
#include "stmt.h"
#include "generic.h"

namespace slkc {
	using FnFlags = uint32_t;

	constexpr static FnFlags FN_PURE = 0x00000001, FN_VARG = 0x00000002, FN_VIRTUAL = 0x00000004;

	class FnNode;

	class FnSlotNode : public MemberNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::DynArray<peff::SharedPtr<FnNode>> overloadings;

		SLKC_API FnSlotNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API FnSlotNode(const FnSlotNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~FnSlotNode();
	};

	class FnNode : public MemberNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::DynArray<peff::SharedPtr<VarNode>> params;
		peff::HashMap<std::string_view, size_t> paramIndices;
		bool isParamsIndexed = false;

		peff::DynArray<size_t> idxParamCommaTokens;
		size_t lParentheseIndex = SIZE_MAX;
		size_t rParentheseIndex = SIZE_MAX;

		peff::DynArray<peff::SharedPtr<GenericParamNode>> genericParams;
		peff::HashMap<std::string_view, size_t> genericParamIndices;
		peff::DynArray<size_t> idxGenericParamCommaTokens;
		size_t lAngleBracketIndex = SIZE_MAX;
		size_t rAngleBracketIndex = SIZE_MAX;

		peff::SharedPtr<TypeNameNode> returnType;

		peff::SharedPtr<CodeBlockStmtNode> body;
		FnFlags fnFlags = 0;

		SLKC_API FnNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API FnNode(const FnNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~FnNode();
	};
}

#endif
