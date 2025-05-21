#include <slake/runtime.h>

using namespace slake;

void *Thread::_threadWrapperProc(void *arg) {
	// Initial run to collect neccessary information.
#if !SLAKE_IS_GET_THREAD_STACK_INFO_SUPPORTED
	((ExecutionThread *)arg)->nativeExecStackBase = estimateCurrentStackPointer();
#endif

	Thread *self = ((Thread *)arg);
	MutexGuard doneMutexGuard(self->_doneMutex);
	self->_initialRunMutex.lock();
	self->_initialRunMutex.unlock();

	self->runnable->run();

	return nullptr;
}

SLAKE_API Thread::Thread(peff::Alloc* selfAllocator, Runnable* runnable): selfAllocator(selfAllocator), nativeThreadHandle(nativeThreadHandle), runnable(runnable) {
}

SLAKE_API Thread::~Thread() {}

SLAKE_API void Thread::start() {
	_initialRunMutex.unlock();
}

SLAKE_API void Thread::join() {
	start();
	_doneMutex.lock();
	_doneMutex.unlock();
}

SLAKE_API void Thread::dealloc() {
	peff::destroyAndRelease<Thread>(selfAllocator.get(), this, alignof(Thread));
}

SLAKE_API Thread* Thread::alloc(peff::Alloc* selfAllocator, Runnable* runnable, size_t stackSize) {
	std::unique_ptr<Thread, util::DeallocableDeleter<Thread>>
		executionThread(peff::allocAndConstruct<Thread>(selfAllocator, alignof(Thread), selfAllocator, runnable));

	executionThread->_initialRunMutex.lock();

	{
		pthread_attr_t attr = {};

		pthread_attr_setstacksize(&attr, stackSize);

		if (pthread_create(&executionThread->nativeThreadHandle, &attr, _threadWrapperProc, (void *)executionThread.get())) {
			pthread_attr_destroy(&attr);
			return nullptr;
		}

		pthread_attr_destroy(&attr);
	}

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
