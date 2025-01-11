#include <slake/runtime.h>

using namespace slake;

SLAKE_API ExecutionThread::ExecutionThread(Runtime *associatedRuntime) : ManagedThread(associatedRuntime, ThreadKind::ExecutionThread) {
}

SLAKE_API ExecutionThread::~ExecutionThread() {
}

ExecutionThread *slake::createExecutionThread(Runtime *runtime, ContextObject *context, size_t nativeStackSize) {
}
