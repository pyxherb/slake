#include <slake/runtime.h>

using namespace slake;

DWORD WINAPI Thread::_threadWrapperProc(LPVOID lpThreadParameter) {
	Thread *self = ((Thread *)lpThreadParameter);

	self->_initialRunMutex.lock();
	self->_initialRunMutex.unlock();

	self->runnable->run();

	return 0;
}

SLAKE_API Thread::Thread(peff::Alloc *selfAllocator, Runnable *runnable) : selfAllocator(selfAllocator), nativeThreadHandle(nativeThreadHandle), runnable(runnable) {
}

SLAKE_API Thread::~Thread() {}

SLAKE_API void Thread::start() {
	_initialRunMutex.unlock();
}

SLAKE_API void Thread::join() {
	start();
	WaitForSingleObject(nativeThreadHandle, INFINITE);
}

SLAKE_API void Thread::dealloc() {
	peff::destroyAndRelease<Thread>(selfAllocator.get(), this, alignof(Thread));
}

SLAKE_API Thread *Thread::alloc(peff::Alloc *selfAllocator, Runnable *runnable, size_t stackSize) {
	std::unique_ptr<Thread, peff::DeallocableDeleter<Thread>>
		executionThread(peff::allocAndConstruct<Thread>(selfAllocator, alignof(Thread), selfAllocator, runnable));

	if (!executionThread) {
		return nullptr;
	}

	executionThread->_initialRunMutex.lock();

	if ((executionThread->nativeThreadHandle = CreateThread(
			 nullptr,
			 stackSize,
			 _threadWrapperProc,
			 (LPVOID)executionThread.get(),
			 0,
			 0)) == NULL) {
		return nullptr;
	}

	return executionThread.release();
}

NativeThreadHandle slake::currentThreadHandle() {
	return GetCurrentThread();
}

void slake::yieldCurrentThread() {
	SwitchToThread();
}

void slake::getCurrentThreadStackBounds(void *&baseOut, size_t &sizeOut) {
#if _WIN32_WINNT >= 0x0602
	ULONG_PTR low;
	ULONG_PTR high;
	GetCurrentThreadStackLimits(&low, &high);
	baseOut = (void *)low;
	sizeOut = (size_t)high - low;
#else
	MEMORY_BASIC_INFORMATION memInfo;
	VirtualQuery(&memInfo, &memInfo, sizeof(memInfo));

	baseOut = memInfo.AllocationBase;
	sizeOut = (size_t)((NT_TIB *)NtCurrentTeb())->StackBase - (size_t)memInfo.AllocationBase;
#endif
}
