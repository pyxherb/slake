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

			SLKC_API BCFnNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
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
			peff::HashMap<std::string_view, size_t> param_indices;
			bool is_params_indexed = false;

			peff::DynArray<size_t> idx_param_comma_tokens;
			size_t l_parenthese_index = SIZE_MAX;
			size_t r_parenthese_index = SIZE_MAX;

			peff::DynArray<AstNodePtr<GenericParamNode>> generic_params;
			peff::HashMap<std::string_view, size_t> generic_param_indices;
			peff::DynArray<size_t> idx_generic_param_comma_tokens;
			size_t l_angle_bracket_index = SIZE_MAX;
			size_t r_angle_bracket_index = SIZE_MAX;
			size_t lvalue_marker_index = SIZE_MAX;

			AstNodePtr<TypeNameNode> return_type;

			AstNodePtr<TypeNameNode> overriden_type;

			peff::DynArray<AstNodePtr<BCStmtNode>> body;
			BCFnOverloadingKind overloading_kind = BCFnOverloadingKind::Invalid;
			FnFlags fn_flags = 0;

			SLKC_API BCFnOverloadingNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
			SLKC_API virtual ~BCFnOverloadingNode();
		};
	}
}

#endif
