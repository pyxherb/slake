#ifndef _SLKC_AST_FN_H_
#define _SLKC_AST_FN_H_

#include "var.h"
#include "stmt.h"
#include "generic.h"

namespace slkc {
	using FnFlags = uint32_t;

	constexpr static FnFlags FN_VARG = 0x00000001, FN_VIRTUAL = 0x00000002, FN_LVALUE = 0x00000004;
	constexpr static const char *LVALUE_OPERATOR_NAME_SUFFIX = "_L";

	class FnOverloadingNode;

	class FnNode : public MemberNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		peff::DynArray<AstNodePtr<FnOverloadingNode>> overloadings;

		SLKC_API FnNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API FnNode(const FnNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~FnNode();
	};

	enum class FnOverloadingKind : uint8_t {
		Invalid = 0,
		Regular,
		Pure,
		Coroutine
	};

	class FnOverloadingNode : public MemberNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

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
		size_t return_type_token_index = SIZE_MAX;

		AstNodePtr<TypeNameNode> overriden_type;

		AstNodePtr<CodeBlockStmtNode> body;
		FnOverloadingKind overloading_kind;
		FnFlags fn_flags = 0;

		SLKC_API FnOverloadingNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API FnOverloadingNode(const FnOverloadingNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~FnOverloadingNode();

		SLAKE_FORCEINLINE bool is_return_type_auto_inferred() const noexcept {
			return return_type_token_index != SIZE_MAX;
		}

		SLAKE_FORCEINLINE bool is_varidic() const noexcept {
			return fn_flags & FN_VARG;
		}
	};
}

#endif
