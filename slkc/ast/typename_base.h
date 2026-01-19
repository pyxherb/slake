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

		BCCustom,

		Bad
	};

	class TypeNameNode : public AstNode {
	public:
		TypeNameKind typeNameKind;
		bool isFinal = false;
		bool isLocal = false;

		size_t idxFinalToken = SIZE_MAX, idxLocalToken = SIZE_MAX;

		SLKC_API TypeNameNode(TypeNameKind typeNameKind, peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API TypeNameNode(const TypeNameNode &rhs, peff::Alloc *selfAllocator, DuplicationContext &context);
		SLKC_API virtual ~TypeNameNode();

		SLAKE_FORCEINLINE void setFinal() noexcept {
			isFinal = true;
		}

		SLAKE_FORCEINLINE void setLocal() noexcept {
			isLocal = true;
		}

		SLAKE_FORCEINLINE void clearFinal() noexcept {
			isFinal = false;
		}

		SLAKE_FORCEINLINE void clearLocal() noexcept {
			isLocal = false;
		}

		SLAKE_FORCEINLINE bool isExplicitFinal() const noexcept {
			return idxFinalToken != SIZE_MAX;
		}

		SLAKE_FORCEINLINE bool isExplicitLocal() const noexcept {
			return idxLocalToken != SIZE_MAX;
		}

		SLAKE_FORCEINLINE bool isImplicitFinal() const noexcept {
			return idxFinalToken == SIZE_MAX;
		}

		SLAKE_FORCEINLINE bool isImplicitLocal() const noexcept {
			return idxLocalToken == SIZE_MAX;
		}
	};
}

#endif
