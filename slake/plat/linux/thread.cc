#include <slake/runtime.h>
#include <sys/mman.h>

using namespace slake;

void *_threadWrapperProc(void *arg) {
	// Initial run to collect neccessary information.
	((ExecutionThread *)arg)->nativeExecStackBase = estimateCurrentStackPointer();

	ExecutionThread *self = ((ExecutionThread *)arg);

	pthread_cond_wait(&self->initialRunPthreadCond, &self->initialRunPthreadMutex);

	self->status = ThreadStatus::Running;

	self->exceptionPtr =
		self->associatedRuntime->execContext(self->context);

	self->status = ThreadStatus::Done;

	return nullptr;
}

SLAKE_API ExecutionThread::ExecutionThread(Runtime *associatedRuntime) : ManagedThread(associatedRuntime, ThreadKind::ExecutionThread) {
}

SLAKE_API ExecutionThread::~ExecutionThread() {
	if (isInitialRunPthreadMutexAllocated) {
		pthread_mutex_destroy(&initialRunPthreadMutex);
	}
	if (isInitialRunPthreadCondAllocated) {
		pthread_cond_destroy(&initialRunPthreadCond);
	}
	if (isDonePthreadMutexAllocated) {
		pthread_mutex_destroy(&donePthreadMutex);
	}
}

void ExecutionThread::start() {
	pthread_cond_signal(&initialRunPthreadCond);
}

void ExecutionThread::join() {
	start();
	pthread_mutex_lock(&donePthreadMutex);
}

ExecutionThread *slake::createExecutionThread(Runtime *runtime, ContextObject *context, size_t nativeStackSize) {
	std::unique_ptr<ExecutionThread, util::DeallocableDeleter<ExecutionThread>>
		executionThread(ExecutionThread::alloc(runtime));

	{
		pthread_attr_t attr = {};

		pthread_attr_setstacksize(&attr, nativeStackSize + 4096);

		if (pthread_create(&executionThread->nativeThreadHandle, &attr, _threadWrapperProc, (void *)executionThread.get())) {
			pthread_attr_destroy(&attr);
			return nullptr;
		}

		pthread_attr_destroy(&attr);
	}

	if (pthread_mutex_init(&executionThread->initialRunPthreadMutex, nullptr)) {
		return nullptr;
	}
	executionThread->isInitialRunPthreadMutexAllocated = true;

	if (pthread_mutex_init(&executionThread->donePthreadMutex, nullptr)) {
		return nullptr;
	}
	executionThread->isDonePthreadMutexAllocated = true;

	if (pthread_cond_init(&executionThread->initialRunPthreadCond, nullptr)) {
		return nullptr;
	}
	executionThread->isInitialRunPthreadCondAllocated = true;

	executionThread->nativeExecStackSize = nativeStackSize;
	executionThread->context = context;

	return executionThread.release();
}

NativeThreadHandle slake::currentThreadHandle() {
	return pthread_self();
}
