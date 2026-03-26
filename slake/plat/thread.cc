#include "../runtime.h"

using namespace slake;

SLAKE_API ExecutionRunnable::ExecutionRunnable() {
}

SLAKE_API void ExecutionRunnable::run() {
	switch (status) {
		case ThreadStatus::Ready:
			get_current_thread_stack_bounds(native_stack_base, native_stack_size);
			break;
		case ThreadStatus::Running:
			std::terminate();
		case ThreadStatus::Done:
		case ThreadStatus::Dead:
			return;
	}
	except_ptr = context->associated_runtime->exec_context(context.get());
}
