#ifndef _SLKC_AST_ASTNODE_H_
#define _SLKC_AST_ASTNODE_H_

#include "lexer.h"
#include <peff/advutils/shared_ptr.h>

namespace slkc {
	enum class AstNodeType : uint8_t {
		Struct = 0,
		Enum,
		EnumItem,
		AttributeDef,
		Attribute,
		FnSlot,
		Fn,
		Stmt,
		Expr,
		TypeName,
		Using,
		Var,
		GenericParam,
		Module,
		Class,
		Interface,
		Import,

		Root,

		Bad
	};

	struct TokenRange {
		size_t beginIndex = SIZE_MAX, endIndex = SIZE_MAX;

		inline TokenRange() = default;
		inline TokenRange(size_t index) : beginIndex(index), endIndex(index) {}
		inline TokenRange(size_t beginIndex, size_t endIndex) : beginIndex(beginIndex), endIndex(endIndex) {}

		SLAKE_FORCEINLINE operator bool() {
			return beginIndex != SIZE_MAX;
		}
	};

	constexpr static size_t ASTNODE_ALIGNMENT = sizeof(std::max_align_t);

	class AstNode : public peff::SharedFromThis<AstNode> {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const;

	public:
		AstNodeType astNodeType;
		peff::WeakPtr<Document> document;
		TokenRange tokenRange;

		SLKC_API AstNode(AstNodeType astNodeType, peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLAKE_FORCEINLINE AstNode(const AstNode &other, peff::Alloc *newAllocator) {
			document = other.document;
			astNodeType = other.astNodeType;
			tokenRange = other.tokenRange;
		}
		SLKC_API virtual ~AstNode();

		template <typename T>
		SLAKE_FORCEINLINE peff::SharedPtr<T> duplicate(peff::Alloc *newAllocator) const {
			return doDuplicate(newAllocator).castTo<T>();
		}
	};
}

#endif
