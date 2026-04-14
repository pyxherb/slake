#ifndef _SLAKE_RUNTIME_H_
#define _SLAKE_RUNTIME_H_

#include <thread>
#include <memory>

#include "slxfmt.h"
#include "except.h"
#include "generated/config.h"
#include "util/debug.h"
#include "object.h"
#include "plat.h"
#include <peff/containers/map.h>
#include <peff/base/deallocable.h>

namespace slake {
	class CountablePoolAlloc : public peff::Alloc {
	protected:
		std::atomic_size_t ref_count = 0;

	public:
		Runtime *runtime;

		peff::RcObjectPtr<peff::Alloc> upstream;
		std::atomic_size_t sz_allocated = 0;

		SLAKE_API CountablePoolAlloc(Runtime *runtime, peff::Alloc *upstream);

		SLAKE_API virtual size_t inc_ref(size_t global_rc) noexcept override;
		SLAKE_API virtual size_t dec_ref(size_t global_rc) noexcept override;

		SLAKE_API virtual void *alloc(size_t size, size_t alignment) noexcept override;
		SLAKE_API virtual void *realloc(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept override;
		SLAKE_API virtual void *realloc_in_place(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept override;
		SLAKE_API virtual void release(void *p, size_t size, size_t alignment) noexcept override;

		SLAKE_API virtual bool is_replaceable(const peff::Alloc *rhs) const noexcept override;

		SLAKE_API virtual peff::UUID type_identity() const noexcept override;
		SLAKE_API virtual void on_ref_zero() noexcept;
	};

	class GenerationalPoolAlloc : public peff::Alloc {
	protected:
		std::atomic_size_t ref_count = 0;
		friend class Runtime;

	public:
		Runtime *runtime;

#ifndef _NDEBUG
		peff::Set<size_t> recorded_ref_points;
#endif

		peff::RcObjectPtr<peff::Alloc> upstream;
		std::atomic_size_t sz_allocated = 0;

		SLAKE_API GenerationalPoolAlloc(Runtime *runtime, peff::Alloc *upstream);

		SLAKE_API virtual size_t inc_ref(size_t global_rc) noexcept override;
		SLAKE_API virtual size_t dec_ref(size_t global_rc) noexcept override;

		SLAKE_API virtual void *alloc(size_t size, size_t alignment) noexcept override;
		SLAKE_API virtual void *realloc(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept override;
		SLAKE_API virtual void *realloc_in_place(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept override;
		SLAKE_API virtual void release(void *p, size_t size, size_t alignment) noexcept override;

		SLAKE_API virtual bool is_replaceable(const peff::Alloc *rhs) const noexcept override;

		SLAKE_API virtual peff::UUID type_identity() const noexcept override;
		SLAKE_API virtual void on_ref_zero() noexcept;
	};

	using RuntimeFlags = uint32_t;
	constexpr static RuntimeFlags
		// No JIT, do not set unless you want to debug the JIT engine.
		RT_NOJIT = 0x0000001,
		// Enable Debugging, set for module debugging.
		RT_DEBUG = 0x0000002,
		// Enable GC Debugging, do not set unless you want to debug the garbage collector.
		RT_GCDBG = 0x0000004,
		// Enable strict mode
		RT_STRICT = 0x00000008,
		// The runtime is destructing.
		_RT_DEINITING = 0x20000000,
		// The runtime is in a GC cycle.
		_RT_INGC = 0x40000000,
		// The runtime is initializing.
		_RT_INITING = 0x80000000;

	struct SecurityPolicy {
		bool allow_unsafe;
	};

	using LoadModuleFlags = uint8_t;
	constexpr LoadModuleFlags
		// Do not put the module onto the path where corresponds to the module name.
		LMOD_NOMODNAME = 0x01,
		// Return directly if module with such name exists.
		LMOD_NORELOAD = 0x02,
		// Throw an exception if module that corresponds to the module name exists.
		LMOD_NOCONFLICT = 0x04,
		// Do not import modules.
		LMOD_NOIMPORT = 0x08,
		// Load native members
		LMOD_LOADNATIVE = 0x10,
		// Load as implicitly loaded.
		LMOD_IMPLICIT = 0x80;

	using NewClassInstanceFlags = uint8_t;
	constexpr static NewClassInstanceFlags
		_NEWCLSINST_PARENT = 0x80;

	typedef void (*UncaughtExceptionHandler)(InternalExceptionPointer &&exception);

	struct GCWalkContext {
	private:
		Object *walkable_list = nullptr;
		Mutex access_mutex;
		InstanceObject *unwalked_instance_list = nullptr;
		InstanceObject *destructible_list = nullptr;
		InstanceObject *type_def_list = nullptr;
		Object *unwalked_list = nullptr;
		Object *walked_list = nullptr;

	public:
		SLAKE_API void remove_from_cur_gcset(Object *object);
		SLAKE_API void push_object(Object *object);
		SLAKE_API void remove_from_unwalked_list(Object *v);
		SLAKE_API void remove_from_walkable_list(Object *v);
		SLAKE_API void remove_from_destructible_list(Object *v);

		SLAKE_API bool is_walkable_list_empty();
		SLAKE_API Object *get_walkable_list();
		SLAKE_API void push_walkable(Object *walkable_object);

		SLAKE_API Object *get_unwalked_list(bool clear_list);
		SLAKE_API void push_unwalked(Object *walkable_object);
		SLAKE_API void update_unwalked_list(Object *deleted_object);

		SLAKE_API Object *get_walked_list();
		SLAKE_API void push_walked(Object *walked_object);

		SLAKE_API InstanceObject *get_destructible_list();
		SLAKE_API void push_destructible(InstanceObject *v);

		SLAKE_API void reset();
	};

	class Runtime final {
	public:
		class GenericInstantiationContext;

		struct GenericInstantiationDispatcher;

		mutable CountablePoolAlloc fixed_alloc;
		mutable GenerationalPoolAlloc young_alloc;
		mutable GenerationalPoolAlloc persistent_alloc;

	private:
		peff::RcObjectPtr<peff::Alloc> self_allocator;
		/// @brief Root value of the runtime.
		ModuleObject *_root_object;

		struct GenericLookupEntry {
			MemberObject *original_object;
			peff::DynArray<Value> generic_args;
		};
		mutable peff::Map<const MemberObject *, GenericLookupEntry> _generic_cache_lookup_table;

		using GenericCacheTable =
			peff::Map<
				peff::DynArray<Value>,	// Generic arguments.
				MemberObject *,			// Cached instantiated value.
				GenericArgListComparator,
				true>;

		using GenericCacheDirectory = peff::Map<
			MemberObject *,	 // Original uninstantiated generic value.
			GenericCacheTable>;

		/// @brief Cached instances of generic values.
		mutable GenericCacheDirectory _generic_cache_dir;

		/// @brief Size of memory allocated for values after last GC cycle.
		size_t _sz_mem_used_after_last_gc = 0,
			   _sz_computed_gc_limit = 0,
			   _sz_persistent_mem_used_after_last_gc = 0,
			   _sz_computed_persistent_gc_limit = 0;

		UncaughtExceptionHandler _uncaught_except_handler = nullptr;

		struct LoaderContext {
			std::istream &fs;
			Object *owner_object;
			bool is_in_generic_scope;
		};

		SLAKE_API MinorFrame *_fetch_minor_frame_unchecked(
			Context *context,
			const MajorFrame *major_frame,
			size_t stack_offset);
		SLAKE_API MinorFrame *_fetch_minor_frame(
			Context *context,
			const MajorFrame *major_frame,
			size_t stack_offset);
		SLAKE_API AllocaRecord *_alloc_alloca_record(Context *context, const MajorFrame *frame, uint32_t output_reg);
		SLAKE_API static Value *_fetch_arg_stack(
			char *data_stack,
			size_t stack_size,
			const MajorFrame *major_frame,
			size_t stack_offset);
		SLAKE_API AllocaRecord *_fetch_alloca_record(
			Context *context,
			const MajorFrame *major_frame,
			size_t stack_offset);
		SLAKE_API static MajorFrame *_fetch_major_frame_unchecked(
			Context *context,
			size_t stack_offset);
		SLAKE_API static MajorFrame *_fetch_major_frame(
			Context *context,
			size_t stack_offset);
		SLAKE_API ExceptHandler *_fetch_except_handler(
			Context *context,
			MajorFrame *major_frame,
			size_t stack_offset);
		/// @brief Execute a single instruction.
		/// @param context Context for execution.
		/// @param ins Instruction to be executed.
		[[nodiscard]] InternalExceptionPointer _exec_ins(ContextObject *const context, MajorFrame *const cur_major_frame, const Opcode opcode, const size_t output, const size_t num_operands, const Value *const operands, bool &is_context_changed_out) noexcept;

		friend struct Context;

	public:
		Object *young_object_list = nullptr, *persistent_object_list = nullptr;
		size_t num_young_objects = 0, num_persistent_objects = 0;

		Object *instance_object_list = nullptr;
		Object *context_object_list = nullptr;
		Object *class_object_list = nullptr;

		peff::Set<TypeDefObject *, TypeDefComparator, true> type_defs;

		SLAKE_API TypeDefObject *get_equal_type_def(TypeDefObject *type_def) const noexcept;
		SLAKE_API void unregister_type_def(TypeDefObject *type_def) noexcept;
		SLAKE_API InternalExceptionPointer register_type_def(TypeDefObject *type_def) noexcept;

	private:
		SLAKE_API void _gc_walk(GCWalkContext *context, MethodTable *method_table);
		SLAKE_API void _gc_walk(GCWalkContext *context, GenericParamList &generic_param_list);
		SLAKE_API void _gc_walk(GCWalkContext *context, const TypeRef &type);
		SLAKE_API void _gc_walk(GCWalkContext *context, const Value &i);
		SLAKE_API void _gc_walk(GCWalkContext *context, char *stack_top, size_t sz_stack, ResumableContextData &i);
		SLAKE_API void _gc_walk(GCWalkContext *context, Object *i);
		SLAKE_API void _gc_walk(GCWalkContext *context, char *data_stack, size_t stack_size, MajorFrame *major_frame);
		SLAKE_API void _gc_walk(GCWalkContext *context, Context &value);
		SLAKE_API void _gc_serial(Object *&object_list, Object *&end_object_out, size_t &num_objects, ObjectGeneration new_generation, peff::Alloc *new_generation_allocator);

		std::unique_ptr<Thread, peff::DeallocableDeleter<Thread>> parallel_gc_thread;

		enum class ParallelGcThreadState : uint8_t {
			Uninit = 0,
			Alive,
			NotifyTermination,
			Terminated
		};

		class ParallelGcThreadRunnable : public Runnable {
		public:
			Runtime *runtime;
			GCWalkContext context;
			bool is_active = false, is_done = false;
			Cond active_cond, done_cond;
			ParallelGcThreadState thread_state = ParallelGcThreadState::Uninit;

			SLAKE_API ParallelGcThreadRunnable(Runtime *runtime);
			SLAKE_API virtual void run() override;

			SLAKE_API void dealloc();
		};

		std::unique_ptr<ParallelGcThreadRunnable, peff::DeallocableDeleter<ParallelGcThreadRunnable>> parallel_gc_thread_runnable;

		SLAKE_API bool _alloc_parallel_gc_resources();
		SLAKE_API void _release_parallel_gc_resources();

		SLAKE_API void _gc_parallel_heapless(Object *&object_list, Object *&end_object_out, size_t &num_objects, ObjectGeneration new_generation);

		SLAKE_API void _destruct_destructible_objects(InstanceObject *destructible_list);

		[[nodiscard]] SLAKE_API InternalExceptionPointer _instantiate_module_fields(GenericInstantiationDispatcher &dispatcher, BasicModuleObject *mod, GenericInstantiationContext *instantiation_context);

		[[nodiscard]] SLAKE_API InternalExceptionPointer _map_generic_params(const Object *v, GenericInstantiationContext *instantiation_context) const;
		[[nodiscard]] SLAKE_API InternalExceptionPointer _map_generic_params(const FnOverloadingObject *ol, GenericInstantiationContext *instantiation_context) const;

		[[nodiscard]] SLAKE_API InternalExceptionPointer _instantiate_generic_object(GenericInstantiationDispatcher &dispatcher, Reference dest, Value value, GenericInstantiationContext *instantiation_context);
		[[nodiscard]] SLAKE_API InternalExceptionPointer _instantiate_generic_object(GenericInstantiationDispatcher &dispatcher, TypeRef &type, GenericInstantiationContext *instantiation_context);
		[[nodiscard]] SLAKE_API InternalExceptionPointer _instantiate_generic_object(GenericInstantiationDispatcher &dispatcher, Value &value, GenericInstantiationContext *instantiation_context);
		[[nodiscard]] SLAKE_API InternalExceptionPointer _instantiate_generic_object(GenericInstantiationDispatcher &dispatcher, Object *v, GenericInstantiationContext *instantiation_context);
		[[nodiscard]] SLAKE_API InternalExceptionPointer _instantiate_generic_object(GenericInstantiationDispatcher &dispatcher, FnOverloadingObject *ol, GenericInstantiationContext *instantiation_context);

		friend class Object;
		friend class RegularFnOverloadingObject;
		friend class FnObject;
		friend class InstanceObject;
		friend class ModuleObject;

	public:
		[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer _add_local_var(Context *context, const MajorFrame *frame, TypeRef type, uint32_t output_reg, Reference &object_ref_out) noexcept;
		[[nodiscard]] SLAKE_API InternalExceptionPointer _fill_args(
			Context *context,
			MajorFrame *new_major_frame,
			const FnOverloadingObject *fn,
			const Value *args,
			uint32_t num_args);
		[[nodiscard]] SLAKE_API InternalExceptionPointer _create_new_coroutine_major_frame(
			Context *context,
			CoroutineObject *coroutine,
			uint32_t return_value_out,
			const Reference *return_struct_ref) noexcept;
		[[nodiscard]] SLAKE_API InternalExceptionPointer _create_new_major_frame(
			ContextObject *context_object,
			Object *this_object,
			const FnOverloadingObject *fn,
			const Value *args,
			size_t off_args,
			uint32_t num_args,
			uint32_t return_value_out,
			const Reference *return_struct_ref) noexcept;
		SLAKE_API void _leave_major_frame(Context *context) noexcept;

		/// @brief Runtime flags.
		RuntimeFlags runtime_flags = 0;

		/// @brief Active contexts of threads.
		std::map<std::thread::id, ContextObject *> active_contexts;

		/// @brief Thread IDs of threads which are executing destructors.
		peff::Map<NativeThreadHandle, ExecutionRunnable *> managed_thread_runnables;

		SLAKE_API Runtime(Runtime &) = delete;
		SLAKE_API Runtime(Runtime &&) = delete;
		SLAKE_API Runtime &operator=(Runtime &) = delete;
		SLAKE_API Runtime &operator=(Runtime &&) = delete;

		SLAKE_API Runtime(peff::Alloc *self_allocator, peff::Alloc *upstream, RuntimeFlags flags = 0);
		SLAKE_API virtual ~Runtime();

		class GenericInstantiationContext final {
		public:
			std::atomic_size_t ref_count = 0;
			peff::RcObjectPtr<peff::Alloc> self_allocator;
			const Object *mapped_object;
			const peff::DynArray<Value> *generic_args;
			peff::HashMap<std::string_view, Value> mapped_generic_args;

			SLAKE_FORCEINLINE GenericInstantiationContext(peff::Alloc *self_allocator, peff::Alloc *resource_allocator) : self_allocator(self_allocator), mapped_generic_args(resource_allocator) {}
			SLAKE_FORCEINLINE GenericInstantiationContext(
				peff::Alloc *self_allocator,
				const Object *mapped_object,
				const peff::DynArray<Value> *generic_args,
				peff::HashMap<std::string_view, Value> &&mapped_generic_args)
				: self_allocator(self_allocator),
				  mapped_object(mapped_object),
				  generic_args(generic_args),
				  mapped_generic_args(std::move(mapped_generic_args)) {
			}

			SLAKE_FORCEINLINE size_t inc_ref(size_t global_rc) noexcept {
				SLAKE_REFERENCED_PARAM(global_rc);
				return ++ref_count;
			}

			SLAKE_FORCEINLINE size_t dec_ref(size_t global_rc) noexcept {
				SLAKE_REFERENCED_PARAM(global_rc);
				if (!--ref_count) {
					on_ref_zero();
					return 0;
				}

				return ref_count;
			}

			SLAKE_FORCEINLINE void on_ref_zero() noexcept {
				peff::destroy_and_release<GenericInstantiationContext>(self_allocator.get(), this, alignof(GenericInstantiationContext));
			}
		};

		SLAKE_API void invalidate_generic_cache(MemberObject *i);

		/// @brief Instantiate an generic value (e.g. generic class, etc).
		/// @param v Object to be instantiated.
		/// @param generic_args Generic arguments for instantiation.
		/// @return Instantiated value.
		/// @note You must make sure that there's no local values in the instantiation context, or the operation will trigger termination.
		[[nodiscard]] SLAKE_API InternalExceptionPointer instantiate_generic_object(MemberObject *object, MemberObject *&object_out, GenericInstantiationContext *instantiation_context);

		SLAKE_API InternalExceptionPointer set_generic_cache(MemberObject *object, const peff::DynArray<Value> &generic_args, MemberObject *instantiated_object);

		/// @brief Resolve a reference and get the referenced value.
		/// @param ref Reference to be resolved.
		/// @param scope_object Scope value for resolving.
		/// @return Resolved value which is referred by the reference.
		SLAKE_API InternalExceptionPointer resolve_id_ref(IdRefObject *ref, Reference &object_ref_out, Object *scope_object = nullptr);

		[[nodiscard]] SLAKE_API bool add_object(Object *object) noexcept;
		SLAKE_API void remove_object(Object *object) noexcept;
		SLAKE_FORCEINLINE peff::Alloc *get_fixed_alloc() const {
			return &fixed_alloc;
		}
		SLAKE_API peff::Alloc *get_cur_gen_alloc();

		SLAKE_API HostObjectRef<ModuleObject> load_module(const void *buf, size_t size, LoadModuleFlags flags);

		SLAKE_FORCEINLINE ModuleObject *get_root_object() { return _root_object; }

		[[nodiscard]] SLAKE_API bool get_full_ref(peff::Alloc *allocator, const MemberObject *v, peff::DynArray<IdRefEntry> &id_ref_out) const;

		/// @brief Run a cycle of GC.
		SLAKE_API void gc();

		[[nodiscard]] SLAKE_API InternalExceptionPointer init_method_table_for_class(ClassObject *cls, ClassObject *parent_class);

		[[nodiscard]] SLAKE_API InternalExceptionPointer init_object_layout_for_module(BasicModuleObject *mod, ObjectLayout *object_layout);
		[[nodiscard]] SLAKE_API InternalExceptionPointer init_object_layout_for_class(ClassObject *cls, ClassObject *parent_class);
		[[nodiscard]] SLAKE_API InternalExceptionPointer init_object_layout_for_struct(StructObject *s);
		[[nodiscard]] SLAKE_API InternalExceptionPointer init_object_layout_for_union_enum_item(UnionEnumItemObject *s);

		[[nodiscard]] SLAKE_API InternalExceptionPointer prepare_class_for_instantiation(ClassObject *cls);
		[[nodiscard]] SLAKE_API InternalExceptionPointer prepare_struct_for_instantiation(StructObject *cls);
		[[nodiscard]] SLAKE_API InternalExceptionPointer prepare_union_enum_for_instantiation(UnionEnumObject *cls);
		[[nodiscard]] SLAKE_API InternalExceptionPointer prepare_union_enum_item_for_instantiation(UnionEnumItemObject *cls);

		SLAKE_API HostObjectRef<InstanceObject> new_class_instance(ClassObject *cls, NewClassInstanceFlags flags);
		SLAKE_API HostObjectRef<ArrayObject> new_array_instance(Runtime *rt, const TypeRef &type, size_t length);

		[[nodiscard]] SLAKE_API InternalExceptionPointer exec_context(ContextObject *context) noexcept;
		/// @brief Execute a function on current thread.
		/// @param overloading Function overloading to be executed.
		/// @param prev_context Previous context for execution.
		/// @param this_object This object for execution.
		/// @param args Argument that will be passed to the function.
		/// @param num_args Number of arguments.
		/// @param context_out Where to receive the execution context.
		/// @param native_stack_base_current_ptr Approximate value of the initial stack pointer, used for platforms that do not support stack information features.
		/// @param native_stack_size Approximate value of the native stack size, used for platforms that do not support stack information features.
		/// @note `native_base_current_ptr` and `native_stack_size` is used for native
		/// stack size estimation on platforms that do not support stack information features,
		/// for platform with stack information features, the arguments are ignored.
		///
		/// @return
		[[nodiscard]] SLAKE_API InternalExceptionPointer exec_fn(
			const FnOverloadingObject *overloading,
			ContextObject *context,
			Object *this_object,
			const Value *args,
			uint32_t num_args,
			Value &value_out);
		[[nodiscard]] SLAKE_API InternalExceptionPointer exec_fn_with_separated_execution_thread(
			const FnOverloadingObject *overloading,
			ContextObject *context,
			Object *this_object,
			const Value *args,
			uint32_t num_args,
			HostObjectRef<ContextObject> &context_out);
		[[nodiscard]] SLAKE_API InternalExceptionPointer create_coroutine_instance(
			const FnOverloadingObject *fn,
			Object *this_object,
			const Value *args,
			uint32_t num_args,
			HostObjectRef<CoroutineObject> &coroutine_out);
		[[nodiscard]] SLAKE_API InternalExceptionPointer resume_coroutine(
			ContextObject *context,
			CoroutineObject *coroutine,
			Value &result_out,
			void *native_stack_base_current_ptr = nullptr,
			size_t native_stack_size = 0);

		SLAKE_API static void *locate_value_base_ptr(const Reference &entity_ref) noexcept;
		SLAKE_API static TypeRef typeof_var(const Reference &entity_ref) noexcept;
		SLAKE_API static void read_var_with_type(const Reference &entity_ref, const TypeRef &t, Value &value_out) noexcept;
		SLAKE_FORCEINLINE static void read_var(const Reference& entity_ref, Value& value_out) noexcept {
			read_var_with_type(entity_ref, typeof_var(entity_ref), value_out);
		}
		SLAKE_API static void write_var_with_type(const Reference &entity_ref, const TypeRef &t, const Value &value) noexcept;
		SLAKE_FORCEINLINE static void write_var(const Reference& entity_ref, const Value& value) noexcept {
			write_var_with_type(entity_ref, typeof_var(entity_ref), value);
		}
		SLAKE_FORCEINLINE InternalExceptionPointer write_var_checked(const Reference &entity_ref, const Value &value) const noexcept {
			TypeRef t = typeof_var(entity_ref);
			if (!is_compatible(t, value))
				return MismatchedVarTypeError::alloc(get_fixed_alloc(), t);
			write_var(entity_ref, value);
			return {};
		}

		SLAKE_API static size_t sizeof_type(const TypeRef &type);
		SLAKE_API static size_t alignof_type(const TypeRef &type);
		SLAKE_API Value default_value_of(const TypeRef &type) const;
		SLAKE_API InternalExceptionPointer load_deferred_custom_type_def(CustomTypeDefObject *custom_type_def);

		[[nodiscard]] SLAKE_API static bool construct_at(Runtime *dest, peff::Alloc *upstream, RuntimeFlags flags = 0);
		[[nodiscard]] SLAKE_API static Runtime *alloc(peff::Alloc *self_allocator, peff::Alloc *upstream, RuntimeFlags flags = 0);

		SLAKE_API void dealloc() noexcept;
	};
}

#endif
