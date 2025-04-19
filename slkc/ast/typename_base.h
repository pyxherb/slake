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
		U8,
		U16,
		U32,
		U64,
		F32,
		F64,
		ISize,
		USize,
		String,
		Bool,
		Object,
		Any,
		Custom,

		Fn,
		Array,
		Ref,
		TempRef,

		Bad
	};

	class TypeNameNode : public AstNode {
	public:
		TypeNameKind typeNameKind;

		SLKC_API TypeNameNode(TypeNameKind typeNameKind, peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API TypeNameNode(const TypeNameNode &rhs, peff::Alloc *selfAllocator);
		SLKC_API virtual ~TypeNameNode();
	};
}

#endif
