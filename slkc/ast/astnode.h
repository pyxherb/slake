#ifndef _SLKC_AST_ASTNODE_H_
#define _SLKC_AST_ASTNODE_H_

#include "lexer.h"
#include <peff/advutils/shared_ptr.h>
#include <peff/base/deallocable.h>

namespace slkc {
	enum class AstNodeType : uint8_t {
		Struct = 0,
		ConstEnum,
		ScopedEnum,
		UnionEnum,
		EnumItem,
		UnionEnumItem,
		AttributeDef,
		Attribute,
		MacroAttribute,
		Fn,
		FnOverloading,
		AttributeMacroDef,
		MemberLevelMacro,
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

		BCFn,
		BCFnOverloading,
		BCStmt,
		BCTypeName,

		Bad
	};

	class ModuleNode;

	struct TokenRange {
		ModuleNode *moduleNode = nullptr;
		size_t beginIndex = SIZE_MAX, endIndex = SIZE_MAX;

		inline TokenRange() = default;
		inline TokenRange(ModuleNode *moduleNode, size_t index) : moduleNode(moduleNode), beginIndex(index), endIndex(index) {}
		inline TokenRange(ModuleNode *moduleNode, size_t beginIndex, size_t endIndex) : moduleNode(moduleNode), beginIndex(beginIndex), endIndex(endIndex) {}

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

	template <typename T>
	class AstNodePtr {
	public:
		using ThisType = AstNodePtr<T>;

		peff::SharedPtr<T> inMemory;
		uint32_t cachedIndex = UINT32_MAX;

		SLAKE_FORCEINLINE AstNodePtr() {}
		SLAKE_FORCEINLINE AstNodePtr(const peff::SharedPtr<T> &inMemory) : inMemory(inMemory) {}
		SLAKE_FORCEINLINE explicit AstNodePtr(uint32_t cachedIndex) : cachedIndex(cachedIndex) {}

		SLAKE_FORCEINLINE AstNodePtr(const ThisType &) = default;
		SLAKE_FORCEINLINE AstNodePtr(ThisType &&inMemory) = default;

		SLAKE_FORCEINLINE ThisType &operator=(const ThisType &) = default;
		SLAKE_FORCEINLINE ThisType &operator=(ThisType &&rhs) noexcept {
			inMemory = std::move(rhs.inMemory);
			cachedIndex = rhs.cachedIndex;

			rhs.cachedIndex = UINT32_MAX;

			return *this;
		}

		SLAKE_FORCEINLINE T *get() const noexcept {
			assert(inMemory);
			return inMemory.get();
		}

		SLAKE_FORCEINLINE T *operator*() const noexcept {
			assert(inMemory);
			return inMemory.get();
		}

		SLAKE_FORCEINLINE T *operator->() const noexcept {
			assert(inMemory);
			return inMemory.get();
		}

		PEFF_FORCEINLINE int comparesTo(const ThisType &rhs) const noexcept {
			if (!inMemory) {
				if (rhs.inMemory) {
					return -1;
				}
				if (cachedIndex < rhs.cachedIndex)
					return -1;
				if (cachedIndex > rhs.cachedIndex)
					return 1;
				return 0;
			} else {
				if (!rhs.inMemory) {
					return 1;
				}
				if (inMemory < rhs.inMemory)
					return -1;
				if (inMemory > rhs.inMemory)
					return 1;
				return 0;
			}
		}

		PEFF_FORCEINLINE bool operator<(const ThisType &rhs) const noexcept {
			return comparesTo(rhs) < 0;
		}

		PEFF_FORCEINLINE bool operator>(const ThisType &rhs) const noexcept {
			return comparesTo(rhs) > 0;
		}

		PEFF_FORCEINLINE bool operator==(const ThisType &rhs) const noexcept {
			return comparesTo(rhs) == 0;
		}

		PEFF_FORCEINLINE bool operator!=(const ThisType &rhs) const noexcept {
			return comparesTo(rhs) != 0;
		}

		PEFF_FORCEINLINE operator bool() const noexcept {
			return (bool)inMemory;
		}

		template <typename T1>
		PEFF_FORCEINLINE AstNodePtr<T1> castTo() const noexcept {
			if (!inMemory) {
				if (cachedIndex != UINT32_MAX) {
					return AstNodePtr<T1>(cachedIndex);
				}
				return {};
			} else
				return AstNodePtr<T1>(inMemory.castTo<T1>());
		}
	};

	template<typename T>
	PEFF_FORCEINLINE peff::WeakPtr<T> toWeakPtr(const AstNodePtr<T>& ptr) noexcept {
		return peff::WeakPtr<T>(ptr.inMemory);
	}

	struct BaseAstNodeDuplicationTask {
		peff::RcObjectPtr<peff::Alloc> allocator;

		SLAKE_FORCEINLINE BaseAstNodeDuplicationTask(peff::Alloc *allocator) : allocator(allocator) {}
		SLAKE_API ~BaseAstNodeDuplicationTask();

		virtual void dealloc() = 0;
		[[nodiscard]] virtual bool perform() = 0;
	};

	template <typename T>
	struct AstNodeDuplicationTask final : public BaseAstNodeDuplicationTask {
		using ThisType = AstNodeDuplicationTask<T>;

		T callable;

		SLAKE_FORCEINLINE AstNodeDuplicationTask(peff::Alloc *allocator, T &&callable) : BaseAstNodeDuplicationTask(allocator), callable(std::move(callable)) {}
		SLAKE_FORCEINLINE virtual ~AstNodeDuplicationTask() {}

		SLAKE_FORCEINLINE static ThisType *alloc(peff::Alloc *allocator, T &&callable) noexcept {
			return peff::allocAndConstruct<ThisType>(allocator, alignof(ThisType), allocator, std::move(callable));
		}
		SLAKE_FORCEINLINE virtual void dealloc() override {
			peff::destroyAndRelease<ThisType>(allocator.get(), this, alignof(ThisType));
		}
		[[nodiscard]] virtual bool perform() override {
			return callable();
		}
	};

	struct DuplicationContext {
		peff::RcObjectPtr<peff::Alloc> allocator;
		peff::List<std::unique_ptr<BaseAstNodeDuplicationTask, peff::DeallocableDeleter<BaseAstNodeDuplicationTask>>> tasks;

		PEFF_FORCEINLINE DuplicationContext(peff::Alloc *allocator) : allocator(allocator), tasks(allocator) {}

		[[nodiscard]] PEFF_FORCEINLINE bool exec() noexcept {
			while (tasks.size()) {
				if (!tasks.front()->perform())
					return false;
				tasks.popFront();
			}
			return true;
		}

		template <typename T>
		[[nodiscard]] bool pushTask(T &&callable) noexcept {
			auto task =
				std::unique_ptr<
					BaseAstNodeDuplicationTask,
					peff::DeallocableDeleter<BaseAstNodeDuplicationTask>>(
					AstNodeDuplicationTask<T>::alloc(allocator.get(), std::move(callable)));
			if (!task)
				return false;
			return tasks.pushBack(std::move(task));
		}
	};

	class AstNode : public peff::SharedFromThis<AstNode> {
	private:
		AstNodeType _astNodeType;

	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const;

	public:
		peff::RcObjectPtr<peff::Alloc> selfAllocator;
		Document *document;
		TokenRange tokenRange;

		AstNode *_nextDestructible = nullptr;
		AstNodeDestructor _destructor = nullptr;

		SLKC_API AstNode(AstNodeType astNodeType, peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API AstNode(const AstNode &other, peff::Alloc *newAllocator, DuplicationContext &context);
		SLKC_API virtual ~AstNode();

		template <typename T>
		SLAKE_FORCEINLINE AstNodePtr<T> duplicate(peff::Alloc *newAllocator) const noexcept {
			DuplicationContext context(newAllocator);

			auto newNode = doDuplicate(newAllocator, context);

			if (!newNode)
				return {};

			if (!context.exec())
				return {};

			return newNode.castTo<T>();
		}

		SLAKE_FORCEINLINE AstNodeType getAstNodeType() const noexcept {
			return _astNodeType;
		}
	};

	SLKC_API void addAstNodeToDestructibleList(AstNode *astNode, AstNodeDestructor _destructor);

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

	template <typename T, typename... Args>
	SLAKE_FORCEINLINE AstNodePtr<T> makeAstNode(peff::Alloc *allocator, Args &&...args) {
		return peff::makeSharedWithControlBlock<T, AstNodeControlBlock<T>>(allocator, std::forward<Args>(args)...);
	}
}

#endif
