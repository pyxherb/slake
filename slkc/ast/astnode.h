#ifndef _SLKC_AST_ASTNODE_H_
#define _SLKC_AST_ASTNODE_H_

#include "lexer.h"
#include <peff/advutils/shared_ptr.h>
#include <peff/base/deallocable.h>
#include <variant>

#if SLKC_WITH_AST_DUMPING
	#include <wandjson/dump.h>
#endif

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
		ModuleNode *module_node = nullptr;
		size_t begin_index = SIZE_MAX, end_index = SIZE_MAX;

		inline TokenRange() = default;
		inline TokenRange(ModuleNode *module_node, size_t index) : module_node(module_node), begin_index(index), end_index(index) {}
		inline TokenRange(ModuleNode *module_node, size_t begin_index, size_t end_index) : module_node(module_node), begin_index(begin_index), end_index(end_index) {}

		SLAKE_FORCEINLINE operator bool() const {
			return begin_index != SIZE_MAX;
		}

		SLAKE_FORCEINLINE bool operator<(const TokenRange &rhs) const {
			return begin_index < rhs.begin_index;
		}

		SLAKE_FORCEINLINE bool operator>(const TokenRange &rhs) const {
			return begin_index < rhs.begin_index;
		}
	};

	constexpr static size_t ASTNODE_ALIGNMENT = sizeof(std::max_align_t);

	class AstNode;

	typedef void (*AstNodeDestructor)(AstNode *ast_node);

	template <typename T>
	class AstNodePtr {
	public:
		using ThisType = AstNodePtr<T>;

		peff::SharedPtr<T> in_memory;
		uint32_t cached_index = UINT32_MAX;

		SLAKE_FORCEINLINE void reset() noexcept {
			if (in_memory)
				in_memory.reset();
			cached_index = UINT32_MAX;
		}

		SLAKE_FORCEINLINE AstNodePtr() {}
		SLAKE_FORCEINLINE AstNodePtr(const peff::SharedPtr<T> &in_memory) : in_memory(in_memory) {}
		SLAKE_FORCEINLINE AstNodePtr(peff::SharedPtr<T> &&in_memory) : in_memory(std::move(in_memory)) {}
		SLAKE_FORCEINLINE explicit AstNodePtr(uint32_t cached_index) : cached_index(cached_index) {}
		SLAKE_FORCEINLINE ~AstNodePtr() {
			in_memory.reset();
		}

		SLAKE_FORCEINLINE AstNodePtr(const ThisType &rhs) noexcept : in_memory(rhs.in_memory), cached_index(rhs.cached_index) {
		}
		SLAKE_FORCEINLINE AstNodePtr(ThisType &&rhs) noexcept {
			if (rhs.in_memory) {
				in_memory = std::move(rhs.in_memory);
			}
			cached_index = rhs.cached_index;
			rhs.cached_index = UINT32_MAX;
		}

		SLAKE_FORCEINLINE ThisType &operator=(const ThisType &rhs) noexcept {
			reset();
			in_memory = rhs.in_memory;
			cached_index = rhs.cached_index;

			return *this;
		}
		SLAKE_FORCEINLINE ThisType &operator=(ThisType &&rhs) noexcept {
			reset();
			in_memory = std::move(rhs.in_memory);
			cached_index = rhs.cached_index;

			rhs.cached_index = UINT32_MAX;

			return *this;
		}

		SLAKE_FORCEINLINE T *get() const noexcept {
			assert(in_memory);
			return in_memory.get();
		}

		SLAKE_FORCEINLINE T *operator*() const noexcept {
			assert(in_memory);
			return in_memory.get();
		}

		SLAKE_FORCEINLINE T *operator->() const noexcept {
			assert(in_memory);
			return in_memory.get();
		}

		PEFF_FORCEINLINE int compares_to(const ThisType &rhs) const noexcept {
			if (!in_memory) {
				if (rhs.in_memory) {
					return -1;
				}
				if (cached_index < rhs.cached_index)
					return -1;
				if (cached_index > rhs.cached_index)
					return 1;
				return 0;
			} else {
				if (!rhs.in_memory) {
					return 1;
				}
				if (in_memory < rhs.in_memory)
					return -1;
				if (in_memory > rhs.in_memory)
					return 1;
				return 0;
			}
		}

		PEFF_FORCEINLINE bool operator<(const ThisType &rhs) const noexcept {
			return compares_to(rhs) < 0;
		}

		PEFF_FORCEINLINE bool operator>(const ThisType &rhs) const noexcept {
			return compares_to(rhs) > 0;
		}

		PEFF_FORCEINLINE bool operator==(const ThisType &rhs) const noexcept {
			return compares_to(rhs) == 0;
		}

		PEFF_FORCEINLINE bool operator!=(const ThisType &rhs) const noexcept {
			return compares_to(rhs) != 0;
		}

		PEFF_FORCEINLINE operator bool() const noexcept {
			return (bool)in_memory;
		}

		template <typename T1>
		PEFF_FORCEINLINE AstNodePtr<T1> cast_to() const noexcept {
			if (!in_memory) {
				if (cached_index != UINT32_MAX) {
					return AstNodePtr<T1>(cached_index);
				}
				return {};
			} else
				return AstNodePtr<T1>(in_memory.template cast_to<T1>());
		}
	};

	template <typename T>
	PEFF_FORCEINLINE peff::WeakPtr<T> to_weak_ptr(const AstNodePtr<T> &ptr) noexcept {
		return peff::WeakPtr<T>(ptr.in_memory);
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
			return peff::alloc_and_construct<ThisType>(allocator, alignof(ThisType), allocator, std::move(callable));
		}
		SLAKE_FORCEINLINE virtual void dealloc() override {
			peff::destroy_and_release<ThisType>(allocator.get(), this, alignof(ThisType));
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
				tasks.pop_front();
			}
			return true;
		}

		template <typename T>
		[[nodiscard]] bool push_task(T &&callable) noexcept {
			auto task =
				std::unique_ptr<
					BaseAstNodeDuplicationTask,
					peff::DeallocableDeleter<BaseAstNodeDuplicationTask>>(
					AstNodeDuplicationTask<T>::alloc(allocator.get(), std::move(callable)));
			if (!task)
				return false;
			return tasks.push_back(std::move(task));
		}
	};

#if SLKC_WITH_AST_DUMPING
	enum class AstDumpingTaskType {
		ObjectMember = 0,
		ArrayInsertion,
	};

	struct ObjectMemberAstDumpingTaskExData {
		wandjson::ObjectValue *object_value;
		std::string_view name;	// The name will always refer to a constant or present string.
		AstNodePtr<AstNode> ast_node;

		SLAKE_FORCEINLINE ObjectMemberAstDumpingTaskExData(wandjson::ObjectValue *object_value, const std::string_view &name, const AstNodePtr<AstNode> &ast_node) : object_value(object_value), name(name), ast_node(ast_node) {}
	};

	struct ArrayInsertionAstDumpingTaskExData {
		wandjson::ArrayValue *array_value;
		AstNodePtr<AstNode> ast_node;

		SLAKE_FORCEINLINE ArrayInsertionAstDumpingTaskExData(wandjson::ArrayValue *array_value, const AstNodePtr<AstNode> &ast_node) : array_value(array_value), ast_node(ast_node) {}
	};

	struct AstDumpingTask {
		std::variant<
			ObjectMemberAstDumpingTaskExData,
			ArrayInsertionAstDumpingTaskExData>
			ex_data;
		AstDumpingTaskType task_type;

		SLAKE_FORCEINLINE AstDumpingTask(ObjectMemberAstDumpingTaskExData &&ex_data) : ex_data(std::move(ex_data)), task_type(AstDumpingTaskType::ObjectMember) {}
		SLAKE_FORCEINLINE AstDumpingTask(ArrayInsertionAstDumpingTaskExData &&ex_data) : ex_data(std::move(ex_data)), task_type(AstDumpingTaskType::ArrayInsertion) {}
	};

	struct AstDumpingContext {
		peff::List<AstDumpingTask> dumping_tasks;

		SLAKE_FORCEINLINE AstDumpingContext(peff::Alloc *allocator) : dumping_tasks(allocator) {}
	};
#endif

	class Scope;

	class AstNode : public peff::SharedFromThis<AstNode> {
	private:
		AstNodeType _ast_node_type;

	public:
		const peff::RcObjectPtr<peff::Alloc> self_allocator;
		Document *document;
		TokenRange token_range;

		AstNode *_next_destructible = nullptr;
		AstNodeDestructor _destructor = nullptr;

		SLKC_API AstNode(AstNodeType ast_node_type, peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API AstNode(const AstNode &other, peff::Alloc *new_allocator, DuplicationContext &context);
		SLKC_API virtual ~AstNode();

		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const;
#if SLKC_WITH_AST_DUMPING
		SLKC_API virtual wandjson::Value *do_dump(peff::Alloc *allocator, AstDumpingContext &context) const;
#endif

		template <typename T>
		SLAKE_FORCEINLINE AstNodePtr<T> duplicate(peff::Alloc *new_allocator) const noexcept {
			DuplicationContext context(new_allocator);

			auto new_node = do_duplicate(new_allocator, context);

			if (!new_node)
				return {};

			if (!context.exec())
				return {};

			return new_node.cast_to<T>();
		}

#if SLKC_WITH_AST_DUMPING
		SLAKE_API wandjson::Value *dump(peff::Alloc *allocator) const noexcept;
#endif

		SLAKE_FORCEINLINE AstNodeType get_ast_node_type() const noexcept {
			return _ast_node_type;
		}
	};

	SLKC_API void add_ast_node_to_destructible_list(AstNode *ast_node, AstNodeDestructor _destructor);

	template <typename T>
	struct AstNodeControlBlock : public peff::SharedPtr<T>::DefaultSharedPtrControlBlock {
		PEFF_FORCEINLINE AstNodeControlBlock(peff::Alloc *allocator, T *ptr) noexcept : peff::SharedPtr<T>::DefaultSharedPtrControlBlock(allocator, ptr) {}
		inline virtual ~AstNodeControlBlock() {}

		inline virtual void on_strong_ref_zero() noexcept override {
			add_ast_node_to_destructible_list(this->ptr, [](AstNode *ast_node) {
				peff::destroy_and_release<T>(ast_node->self_allocator.get(), static_cast<T *>(ast_node), alignof(T));
			});
		}

		inline virtual void on_ref_zero() noexcept override {
			peff::destroy_and_release<AstNodeControlBlock<T>>(this->allocator.get(), this, alignof(AstNodeControlBlock<T>));
		}
	};

	template <typename T, typename... Args>
	SLAKE_FORCEINLINE AstNodePtr<T> make_ast_node(peff::Alloc *allocator, Args &&...args) {
		return peff::make_shared_with_control_block<T, AstNodeControlBlock<T>>(allocator, std::forward<Args>(args)...);
	}
}

#endif
