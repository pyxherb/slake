#ifndef _SLKC_AST_BC_FN_H_
#define _SLKC_AST_BC_FN_H_

#include "../var.h"
#include "../generic.h"
#include "stmt.h"

namespace slkc {
	namespace bc {
		using FnFlags = uint32_t;

		constexpr static FnFlags FN_VARG = 0x00000001, FN_VIRTUAL = 0x00000002, FN_LVALUE = 0x00000004;
		constexpr static const char *LVALUE_OPERATOR_NAME_SUFFIX = "_L";

		class BCFnOverloadingNode;

		class BCFnNode : public MemberNode {
		public:
			peff::DynArray<AstNodePtr<BCFnOverloadingNode>> overloadings;

			SLKC_API BCFnNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
			SLKC_API virtual ~BCFnNode();
		};

		enum class BCFnOverloadingKind : uint8_t {
			Invalid = 0,
			Regular,
			Pure,
			Coroutine
		};

		class BCFnOverloadingNode : public MemberNode {
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
			size_t lvalueMarkerIndex = SIZE_MAX;

			AstNodePtr<TypeNameNode> returnType;

			AstNodePtr<TypeNameNode> overridenType;

			peff::DynArray<AstNodePtr<BCStmtNode>> body;
			BCFnOverloadingKind overloadingKind = BCFnOverloadingKind::Invalid;
			FnFlags fnFlags = 0;

			SLKC_API BCFnOverloadingNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
			SLKC_API virtual ~BCFnOverloadingNode();
		};
	}
}

#endif
