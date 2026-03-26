#ifndef _SLAKE_OBJ_COROUTINE_H_
#define _SLAKE_OBJ_COROUTINE_H_

#include "fn.h"
#include "var.h"
#include "context.h"

namespace slake {
	class CoroutineObject : public Object {
	public:
		Context *cur_context;
		MajorFrame *bound_major_frame;

		char *stack_data;
		size_t len_stack_data;
		size_t off_stack_top;
		size_t off_regs;
		const FnOverloadingObject *overloading;
		peff::Option<ResumableContextData> resumable;

		Value final_result;

		SLAKE_API CoroutineObject(Runtime *rt, peff::Alloc *self_allocator);
		SLAKE_API virtual ~CoroutineObject();

		SLAKE_API static HostObjectRef<CoroutineObject> alloc(Runtime *rt);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API char *alloc_stack_data(size_t size);
		SLAKE_API void release_stack_data();

		SLAKE_FORCEINLINE void bind_to_context(Context *cur_context, MajorFrame *cur_major_frame) noexcept {
			this->cur_context = cur_context;
			this->bound_major_frame = cur_major_frame;
		}
		SLAKE_FORCEINLINE void unbind_context() noexcept {
			this->cur_context = nullptr;
			this->bound_major_frame = nullptr;
		}

		SLAKE_FORCEINLINE bool is_active() const noexcept {
			return this->cur_context;
		}

		SLAKE_FORCEINLINE bool is_done() const {
			if (resumable) {
				return resumable->cur_ins == UINT32_MAX;
			}
			return false;
		}
		SLAKE_FORCEINLINE void set_done() {
			assert(resumable);
			resumable->cur_ins = UINT32_MAX;
		}

		SLAKE_API virtual void replace_allocator(peff::Alloc *allocator) noexcept override;
	};
}

#endif
