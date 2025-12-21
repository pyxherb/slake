#ifndef _SLKC_AST_TYPENAME_BASE_H_
#define _SLKC_AST_TYPENAME_BASE_H_

#include "astnode.h"

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
		Coroutine,
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

		MetaType,

		Bad
	};

	class TypeNameNode : public AstNode {
	public:
		TypeNameKind typeNameKind;
		bool isFinal = false;

		SLKC_API TypeNameNode(TypeNameKind typeNameKind, peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API TypeNameNode(const TypeNameNode &rhs, peff::Alloc *selfAllocator, DuplicationContext &context);
		SLKC_API virtual ~TypeNameNode();
	};
}

#endif
