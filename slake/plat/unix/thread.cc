#include <slake/runtime.h>

using namespace slake;

SLAKE_API Mutex::Mutex() {}
SLAKE_API Mutex::~Mutex() {
	if (isValid) {
		pthread_mutex_destroy(&nativeHandle);
	}
}

bool Mutex::init() {
	if (pthread_mutex_init(&nativeHandle, nullptr))
		return false;
	isValid = true;
	return true;
}

SLAKE_API void Mutex::lock() {
	assert(isValid);
	pthread_mutex_lock(&nativeHandle);
}

SLAKE_API bool Mutex::tryLock() {
	assert(isValid);
	pthread_mutex_trylock(&nativeHandle);
}

SLAKE_API void Mutex::unlock() {
	assert(isValid);
	pthread_mutex_unlock(&nativeHandle);
}

SLAKE_API Cond::Cond() {}
SLAKE_API Cond::~Cond() {
	if (isValid) {
		pthread_cond_destroy(&nativeHandle);
	}
}

SLAKE_API bool Cond::init() {
	if (!internalMutex.init())
		return false;
	if (pthread_cond_init(&nativeHandle, nullptr))
		return false;
	isValid = true;
	return true;
}

SLAKE_API void Cond::wait() {
	assert(isValid);
	pthread_cond_wait(&nativeHandle, &internalMutex.nativeHandle);
}

SLAKE_API void Cond::notify() {
	assert(isValid);
	pthread_cond_signal(&nativeHandle);
}

SLAKE_API AttachedExecutionThread::AttachedExecutionThread(Runtime *associatedRuntime) : ManagedThread(associatedRuntime, ThreadKind::AttachedExecutionThread) {
}

SLAKE_API AttachedExecutionThread::~AttachedExecutionThread() {
}

AttachedExecutionThread *slake::createAttachedExecutionThreadForCurrentThread(Runtime *runtime, ContextObject *context, void *nativeStackBaseCurrentPtr, size_t nativeStackSize) {
	std::unique_ptr<AttachedExecutionThread, util::DeallocableDeleter<AttachedExecutionThread>>
		executionThread(AttachedExecutionThread::alloc(runtime));

	if (!executionThread->_initialRunMutex.init())
		return nullptr;
	if (!executionThread->_initCond.init())
		return nullptr;
	if (!executionThread->_doneMutex.init())
		return nullptr;

	executionThread->nativeThreadHandle = currentThreadHandle();

	executionThread->_initialRunMutex.lock();

#if SLAKE_IS_GET_THREAD_STACK_INFO_SUPPORTED
	getCurrentThreadStackBounds(executionThread->nativeExecStackBase, executionThread->nativeExecStackSize);
#else
	executionThread->nativeExecStackBase = (void *)(((char *)nativeStackBaseCurrentPtr) - nativeStackSize);
	executionThread->nativeExecStackSize = nativeStackSize;
#endif

	executionThread->context = context;

	return executionThread.release();
}

void *ExecutionThread::_threadWrapperProc(void *arg) {
	// Initial run to collect neccessary information.
#if !SLAKE_IS_GET_THREAD_STACK_INFO_SUPPORTED
	((ExecutionThread *)arg)->nativeExecStackBase = estimateCurrentStackPointer();
#endif

	ExecutionThread *self = ((ExecutionThread *)arg);
	MutexGuard doneMutexGuard(self->_doneMutex);
	self->_initialRunMutex.lock();
	self->_initialRunMutex.unlock();
	self->_initCond.notify();
	{
		if (self->status == ThreadStatus::Dead) {
			return nullptr;
		}

		self->status = ThreadStatus::Running;

		self->exceptionPtr =
			self->associatedRuntime->execContext(self->context);

		self->status = ThreadStatus::Done;
	}

	return nullptr;
}

SLAKE_API ExecutionThread::ExecutionThread(Runtime *associatedRuntime) : ManagedThread(associatedRuntime, ThreadKind::ExecutionThread) {
}

SLAKE_API ExecutionThread::~ExecutionThread() {
}

void ExecutionThread::start() {
	_initialRunMutex.unlock();
	_initCond.wait();
}

void ExecutionThread::join() {
	start();
	_doneMutex.lock();
	_doneMutex.unlock();
}

void ExecutionThread::kill() {
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

ExecutionThread *slake::createExecutionThread(Runtime *runtime, ContextObject *context, size_t nativeStackSize) {
	std::unique_ptr<ExecutionThread, util::DeallocableDeleter<ExecutionThread>>
		executionThread(ExecutionThread::alloc(runtime));

	if (!executionThread->_initialRunMutex.init())
		return nullptr;
	if (!executionThread->_initCond.init())
		return nullptr;
	if (!executionThread->_doneMutex.init())
		return nullptr;

	executionThread->_initialRunMutex.lock();

	{
		pthread_attr_t attr = {};

		pthread_attr_setstacksize(&attr, nativeStackSize + 4096);

		if (pthread_create(&executionThread->nativeThreadHandle, &attr, ExecutionThread::_threadWrapperProc, (void *)executionThread.get())) {
			pthread_attr_destroy(&attr);
			return nullptr;
		}

		pthread_attr_destroy(&attr);
	}

	executionThread->nativeExecStackSize = nativeStackSize;
	executionThread->context = context;

	return executionThread.release();
}

NativeThreadHandle slake::currentThreadHandle() {
	return pthread_self();
}

void slake::yieldCurrentThread() {
#if _POSIX_PRIORITY_SCHEDULING
	sched_yield();
#else
	pthread_yield();
#endif
}

void slake::getCurrentThreadStackBounds(void *&baseOut, size_t &sizeOut) {
	pthread_attr_t attr;
	void *stackAddr;
	size_t stackSize;

	if (!pthread_attr_getstack(&attr, &stackAddr, &stackSize)) {
		baseOut = nullptr;
		sizeOut = SIZE_MAX;
	}

	baseOut = stackAddr;
	sizeOut = stackSize;
}
