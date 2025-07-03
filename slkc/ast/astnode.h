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
		This,

		Bad
	};

	struct TokenRange {
		size_t beginIndex = SIZE_MAX, endIndex = SIZE_MAX;

		inline TokenRange() = default;
		inline TokenRange(size_t index) : beginIndex(index), endIndex(index) {}
		inline TokenRange(size_t beginIndex, size_t endIndex) : beginIndex(beginIndex), endIndex(endIndex) {}

		SLAKE_FORCEINLINE operator bool() const {
			return beginIndex != SIZE_MAX;
		}

		SLAKE_FORCEINLINE bool operator<(const TokenRange &rhs) const {
			return beginIndex < rhs.beginIndex;
		}

		SLAKE_FORCEINLINE bool operator>(const TokenRange &rhs) const {
			return beginIndex < rhs.beginIndex;
		}
	};

	constexpr static size_t ASTNODE_ALIGNMENT = sizeof(std::max_align_t);

	class AstNode;

	typedef void (*AstNodeDestructor)(AstNode *astNode);

	class AstNode : public peff::SharedFromThis<AstNode> {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const;

	public:
		AstNodeType astNodeType;
		peff::RcObjectPtr<peff::Alloc> selfAllocator;
		Document *document;
		TokenRange tokenRange;

		AstNode *nextDestructible = nullptr;
		AstNodeDestructor destructor = nullptr;

		SLKC_API AstNode(AstNodeType astNodeType, peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API AstNode(const AstNode &other, peff::Alloc *newAllocator);
		SLKC_API virtual ~AstNode();

		template <typename T>
		SLAKE_FORCEINLINE peff::SharedPtr<T> duplicate(peff::Alloc *newAllocator) const {
			return doDuplicate(newAllocator).castTo<T>();
		}
	};

	SLKC_API void addAstNodeToDestructibleList(AstNode *astNode, AstNodeDestructor destructor);

	template <typename T>
	struct AstNodeControlBlock : public peff::SharedPtr<T>::DefaultSharedPtrControlBlock {
		PEFF_FORCEINLINE AstNodeControlBlock(peff::Alloc *allocator, T *ptr) noexcept : peff::SharedPtr<T>::DefaultSharedPtrControlBlock(allocator, ptr) {}
		inline virtual ~AstNodeControlBlock() {}

		inline virtual void onStrongRefZero() noexcept override {
			addAstNodeToDestructibleList(this->ptr, [](AstNode *astNode) {
				peff::destroyAndRelease<T>(astNode->selfAllocator.get(), static_cast<T *>(astNode), alignof(T));
			});
		}

		inline virtual void onRefZero() noexcept override {
			peff::destroyAndRelease<AstNodeControlBlock<T>>(this->allocator.get(), this, alignof(AstNodeControlBlock<T>));
		}
	};
}

#endif
