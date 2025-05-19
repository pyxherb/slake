#include "../runtime.h"

using namespace slake;

SLAKE_API ManagedThread::ManagedThread(Runtime *associatedRuntime, ThreadKind threadKind) : associatedRuntime(associatedRuntime), threadKind(threadKind) {
}

SLAKE_API ManagedThread::~ManagedThread() {
}

SLAKE_API void ExecutionThread::dealloc() {
	peff::destroyAndRelease<ExecutionThread>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API ExecutionThread *ExecutionThread::alloc(Runtime *associatedRuntime) {
	return peff::allocAndConstruct<ExecutionThread>(&associatedRuntime->globalHeapPoolAlloc, sizeof(std::max_align_t), associatedRuntime);
}

SLAKE_API void AttachedExecutionThread::start() {
	_initialRunMutex.unlock();
	_initCond.wait();
}

SLAKE_API void AttachedExecutionThread::join() {
	start();
	_doneMutex.lock();
	_doneMutex.unlock();
}

SLAKE_API void AttachedExecutionThread::kill() {
	switch (status) {
		case ThreadStatus::Ready:
			status = ThreadStatus::Dead;
			start();

			_doneMutex.lock();
			_doneMutex.unlock();
			break;
		case ThreadStatus::Running:
			status = ThreadStatus::Dead;
			_doneMutex.lock();
			_doneMutex.unlock();
			break;
		case ThreadStatus::Done:
		case ThreadStatus::Dead:
			break;
	}
}

SLAKE_API void AttachedExecutionThread::dealloc() {
	peff::destroyAndRelease<AttachedExecutionThread>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API AttachedExecutionThread *AttachedExecutionThread::alloc(Runtime *associatedRuntime) {
	return peff::allocAndConstruct<AttachedExecutionThread>(&associatedRuntime->globalHeapPoolAlloc, sizeof(std::max_align_t), associatedRuntime);
}
