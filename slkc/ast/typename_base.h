#ifndef _SLKC_AST_TYPENAME_BASE_H_
#define _SLKC_AST_TYPENAME_BASE_H_

#include "document.h"

namespace slkc {
	enum class TypeNameKind : uint8_t {
		Void = 0,
		I8,
		I16,
		I32,
		I64,
		ISize,
		U8,
		U16,
		U32,
		U64,
		USize,
		F32,
		F64,
		String,
		Bool,
		Object,
		Any,
		Custom,
		Unpacking,

		Fn,
		Array,
		Ref,
		TempRef,
		Tuple,
		SIMD,
		ParamTypeList,
		UnpackedParams,
		UnpackedArgs,

		Null,

		BCCustom,

		Bad
	};

	class TypeNameNode : public AstNode {
	public:
		const TypeNameKind tn_kind;
		bool is_final = false;
		bool is_local = false;
		bool is_nullable = false;

		size_t idx_final_token = SIZE_MAX, idx_local_token = SIZE_MAX, idx_nullable_token = SIZE_MAX;

		SLKC_API TypeNameNode(TypeNameKind tn_kind, peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API TypeNameNode(const TypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context);
		SLKC_API virtual ~TypeNameNode();

		SLAKE_FORCEINLINE void set_final() noexcept {
			is_final = true;
		}

		SLAKE_FORCEINLINE void set_local() noexcept {
			is_local = true;
		}

		SLAKE_FORCEINLINE void set_nullable() noexcept {
			is_nullable = true;
		}

		SLAKE_FORCEINLINE void clear_final() noexcept {
			is_final = false;
		}

		SLAKE_FORCEINLINE void clear_local() noexcept {
			is_local = false;
		}

		SLAKE_FORCEINLINE void clear_nullable() noexcept {
			is_nullable = false;
		}

		SLAKE_FORCEINLINE bool is_explicit_final() const noexcept {
			return idx_final_token != SIZE_MAX;
		}

		SLAKE_FORCEINLINE bool is_explicit_local() const noexcept {
			return idx_local_token != SIZE_MAX;
		}

		SLAKE_FORCEINLINE bool is_explicit_nullable() const noexcept {
			return idx_nullable_token != SIZE_MAX;
		}

		SLAKE_FORCEINLINE bool is_implicit_final() const noexcept {
			return idx_final_token == SIZE_MAX;
		}

		SLAKE_FORCEINLINE bool is_implicit_local() const noexcept {
			return idx_local_token == SIZE_MAX;
		}

		SLAKE_FORCEINLINE bool is_implicit_nullable() const noexcept {
			return idx_nullable_token == SIZE_MAX;
		}
	};
}

#endif
