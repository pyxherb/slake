#ifndef _SLKC_AST_IDREF_H_
#define _SLKC_AST_IDREF_H_

#include "typename_base.h"
#include <peff/containers/dynarray.h>

namespace slkc {
	struct IdRefEntry {
		peff::String name;
		peff::DynArray<AstNodePtr<AstNode>> generic_args;
		size_t access_op_token_index = SIZE_MAX, name_token_index = SIZE_MAX, generic_scope_token_index = SIZE_MAX, left_angle_bracket_token_index = SIZE_MAX, right_angle_bracket_token_index = SIZE_MAX;
		peff::DynArray<size_t> comma_token_indices;

		SLAKE_FORCEINLINE IdRefEntry(peff::Alloc *self_allocator): name(self_allocator), generic_args(self_allocator), comma_token_indices(self_allocator) {}
		SLAKE_FORCEINLINE IdRefEntry(IdRefEntry&& rhs): name(std::move(rhs.name)), generic_args(std::move(rhs.generic_args)), access_op_token_index(rhs.access_op_token_index), name_token_index(rhs.name_token_index), left_angle_bracket_token_index(rhs.left_angle_bracket_token_index), right_angle_bracket_token_index(rhs.right_angle_bracket_token_index), comma_token_indices(std::move(rhs.comma_token_indices)) {
		}
	};

	SLKC_API peff::Option<IdRefEntry> duplicate_id_ref_entry(peff::Alloc *self_allocator, const IdRefEntry &rhs);

	class IdRef final {
	public:
		peff::RcObjectPtr<peff::Alloc> self_allocator;
		peff::DynArray<IdRefEntry> entries;
		TokenRange token_range;

		SLKC_API IdRef(peff::Alloc *self_allocator);
		SLKC_API virtual ~IdRef();

		SLKC_API void dealloc() noexcept;
	};

	using IdRefPtr = std::unique_ptr<IdRef, peff::DeallocableDeleter<IdRef>>;

	SLKC_API IdRefPtr duplicate_id_ref(peff::Alloc *self_allocator, IdRef *rhs);
}

#endif
