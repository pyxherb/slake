#ifndef _SLAKE_OBJ_CONTEXT_H_
#define _SLAKE_OBJ_CONTEXT_H_

#include "object.h"
#include "var.h"
#include <slake/except.h>
#include <memory>
#include <peff/base/deallocable.h>

namespace slake {
	struct ExceptionHandler final {
		TypeRef type;
		uint32_t off;
	};

	class CoroutineObject;

	struct ExceptHandler {
		size_t off_next;
		TypeRef type;
		uint32_t off_handler;
	};

	constexpr uint32_t MINOR_FRAME_MAGIC = 0xeffec7ed;

	struct MinorFrame {
		uint32_t magic = MINOR_FRAME_MAGIC;
		size_t off_last_minor_frame = SIZE_MAX;
		size_t off_except_handler = SIZE_MAX;
		size_t off_alloca_records = SIZE_MAX;
		size_t stack_base = 0;
	};

	/// @brief Alloca record, used for invalidating references in the registers, etc.
	/// @note Do not forget to add GC scan codes when you adding anything new into this structure!
	constexpr uint32_t ALLOCA_RECORD_MAGIC = 0xacce55ed;
	struct AllocaRecord {
		size_t off_next;
		uint32_t def_reg;
	};

	struct ResumableContextData {
		uint32_t cur_ins = 0;
		uint32_t last_jump_src = UINT32_MAX;
		size_t off_args = SIZE_MAX, off_next_args = SIZE_MAX, off_next_args_begin = SIZE_MAX;
		size_t num_args = 0, num_next_args = 0;
		/// @brief Offset of current minor frame, note that it is context-dependent offset.
		size_t off_cur_minor_frame = SIZE_MAX;
		size_t num_regs = 0;
		Object *this_object = nullptr;
	};

	class ContextObject;

	constexpr uint32_t MAJOR_FRAME_MAGIC = 0xacce55ed;

	/// @brief A major frame represents a single calling frame.
	struct MajorFrame final {
		uint32_t magic = MAJOR_FRAME_MAGIC;
		size_t off_prev_frame = SIZE_MAX, off_next_frame = SIZE_MAX;

		Runtime *associated_runtime;

		const FnOverloadingObject *cur_fn = nullptr;	 // Current function overloading.
		ContextObject *cur_context = nullptr;
		CoroutineObject *cur_coroutine = nullptr;

		ResumableContextData resumable_context_data;

		uint32_t return_value_out_reg = UINT32_MAX;
		Reference return_struct_ref;

		size_t prev_stack_top = 0;
		size_t off_regs = UINT32_MAX;

		Value cur_except = Value();	// Current exception.

		SLAKE_API MajorFrame(Runtime *rt) noexcept;
		MajorFrame(MajorFrame &&) noexcept = default;
		SLAKE_API ~MajorFrame();

		SLAKE_API void dealloc() noexcept;

		SLAKE_API void replace_allocator(peff::Alloc *allocator) noexcept;
	};

	static_assert(!std::is_polymorphic_v<MajorFrame>);

	using ContextFlags = uint8_t;
	constexpr static ContextFlags
		// Done
		CTX_DONE = 0x01,
		// Yielded
		CTX_YIELDED = 0x02;

	struct Context {
		Runtime *runtime;
		peff::RcObjectPtr<peff::Alloc> self_allocator;
		size_t num_major_frames = 0;
		size_t off_cur_major_frame = SIZE_MAX;	 // Offset of current major frame
		ContextFlags flags = 0;				 // Flags
		char *data_stack = nullptr;			 // Data stack
		size_t stack_top = 0;				 // Stack top
		size_t stack_size;

		SLAKE_API char *stack_alloc(size_t size) noexcept;
		SLAKE_API char *align_stack(size_t alignment) noexcept;
		SLAKE_API char *aligned_stack_alloc(size_t size, size_t alignment) noexcept;

		SLAKE_API Context(Runtime *runtime, peff::Alloc *allocator);

		SLAKE_API ~Context();

		SLAKE_API void replace_allocator(peff::Alloc *allocator) noexcept;

		typedef bool (*MajorFrameWalker)(MajorFrame *major_frame, void *user_data);
		SLAKE_API void for_each_major_frame(MajorFrameWalker walker, void *user_data);
	};

	class ContextObject final : public Object {
	public:
		Context _context;

		SLAKE_API ContextObject(Runtime *rt, peff::Alloc *self_allocator);
		SLAKE_API virtual ~ContextObject();

		SLAKE_API static HostObjectRef<ContextObject> alloc(Runtime *rt, size_t stack_size);
		SLAKE_API virtual void dealloc() override;

		SLAKE_FORCEINLINE Context &get_context() { return _context; }

		SLAKE_API InternalExceptionPointer resume(HostRefHolder *host_ref_holder);
		SLAKE_API bool is_done();

		SLAKE_API virtual void replace_allocator(peff::Alloc *allocator) noexcept override;
	};

	SLAKE_FORCEINLINE constexpr char *calc_stack_addr(char *data, size_t sz_stack, size_t offset) {
		return data + (sz_stack - offset);
	}

	SLAKE_FORCEINLINE constexpr const char *calc_stack_addr(const char *data, size_t sz_stack, size_t offset) {
		return data + (sz_stack - offset);
	}

	class ExecutionRunnable : public Runnable {
	public:
		Thread *thread = nullptr;
		HostObjectRef<ContextObject> context;
		void *native_stack_base = nullptr;
		size_t native_stack_size = 0;
		InternalExceptionPointer except_ptr;
		ThreadStatus status = ThreadStatus::Ready;

		SLAKE_API ExecutionRunnable();

		SLAKE_API virtual void run() override;
	};
}

#endif
