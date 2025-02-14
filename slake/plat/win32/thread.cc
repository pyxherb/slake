#include <slake/runtime.h>

using namespace slake;

SLAKE_API Mutex::Mutex() {}
SLAKE_API Mutex::~Mutex() {
	if (isValid) {
		DeleteCriticalSection(&nativeHandle);
	}
}

bool Mutex::init() {
	InitializeCriticalSection(&nativeHandle);
	isValid = true;
	return true;
}

SLAKE_API void Mutex::lock() {
	EnterCriticalSection(&nativeHandle);
}

SLAKE_API bool Mutex::tryLock() {
	return TryEnterCriticalSection(&nativeHandle);
}

SLAKE_API void Mutex::unlock() {
	LeaveCriticalSection(&nativeHandle);
}

SLAKE_API Cond::Cond() {}
SLAKE_API Cond::~Cond() {
	DeleteCriticalSection(&criticalSection);
}

SLAKE_API bool Cond::init() {
	InitializeCriticalSection(&criticalSection);
	InitializeConditionVariable(&nativeHandle);
	isValid = true;
	return true;
}

SLAKE_API void Cond::wait() {
	assert(isValid);
	SleepConditionVariableCS(&nativeHandle, &criticalSection, INFINITE);
}

SLAKE_API void Cond::notify() {
	assert(isValid);
	WakeConditionVariable(&nativeHandle);
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

	getCurrentThreadStackBounds(executionThread->nativeExecStackBase, executionThread->nativeExecStackSize);

	executionThread->context = context;

	return executionThread.release();
}

DWORD WINAPI ExecutionThread::_threadWrapperProc(LPVOID lpThreadParameter) {
	ExecutionThread *self = ((ExecutionThread *)lpThreadParameter);

	getCurrentThreadStackBounds(self->nativeExecStackBase, self->nativeExecStackSize);

	self->_initialRunMutex.lock();
	self->_initialRunMutex.unlock();
	self->_initCond.notify();
	{
		if (self->status == ThreadStatus::Dead) {
			return 1;
		}

		self->status = ThreadStatus::Running;

		self->exceptionPtr =
			self->associatedRuntime->execContext(self->context);

		self->status = ThreadStatus::Done;
	}

	return 0;
}

SLAKE_API ExecutionThread::ExecutionThread(Runtime *associatedRuntime) : ManagedThread(associatedRuntime, ThreadKind::ExecutionThread) {
	nativeThreadHandle = NULL;
}

SLAKE_API ExecutionThread::~ExecutionThread() {
	if (nativeThreadHandle != NULL) {
		CloseHandle(nativeThreadHandle);
	}
}

void ExecutionThread::start() {
	_initialRunMutex.unlock();
	_initCond.wait();
}

void ExecutionThread::join() {
	start();
	WaitForSingleObject(nativeThreadHandle, INFINITE);
}

void ExecutionThread::kill() {
	switch (status) {
	case ThreadStatus::Ready:
		status = ThreadStatus::Dead;
		start();

		WaitForSingleObject(nativeThreadHandle, INFINITE);
		break;
	case ThreadStatus::Running:
		status = ThreadStatus::Dead;
		WaitForSingleObject(nativeThreadHandle, INFINITE);
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

	executionThread->_initialRunMutex.lock();

	if ((executionThread->nativeThreadHandle = CreateThread(
			 nullptr,
			 nativeStackSize,
			 ExecutionThread::_threadWrapperProc,
			 (LPVOID)executionThread.get(),
			 0,
			 0)) == NULL) {
		return nullptr;
	}

	executionThread->nativeExecStackSize = nativeStackSize;
	executionThread->context = context;

	return executionThread.release();
}

NativeThreadHandle slake::currentThreadHandle() {
	return GetCurrentThread();
}

void slake::yieldCurrentThread() {
	Sleep(0);
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
