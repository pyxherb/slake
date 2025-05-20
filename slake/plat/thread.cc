#include "../runtime.h"

using namespace slake;

SLAKE_API ExecutionRunnable::ExecutionRunnable() {
}

SLAKE_API void ExecutionRunnable::run() {
	switch (status) {
		case ThreadStatus::Ready:
			getCurrentThreadStackBounds(nativeStackBase, nativeStackSize);
			break;
		case ThreadStatus::Running:
			std::terminate();
		case ThreadStatus::Done:
		case ThreadStatus::Dead:
			return;
	}
	exceptPtr = context->associatedRuntime->execContext(context.get());
}
