#ifndef _SLAKE_PLAT_H_
#define _SLAKE_PLAT_H_

#include "object.h"

#if _WIN32
#include <Windows.h>
#elif __unix__
#include <pthread.h>
#else
#include <thread>
#endif

namespace slake {
	enum class ThreadKind : uint8_t {
		ExecutionThread,
		RuntimeInternal
	};

#if _WIN32
	using NativeThreadHandle = HANDLE;
#elif __unix__
	using NativeThreadHandle = pthread_t;
#else
	using NativeThreadHandle = std::thread::id;
#endif

	enum class ThreadStatus : uint8_t {
		Ready = 0,
		Running,
		Done
	};

	class ManagedThread {
	public:
		Runtime *associatedRuntime;
		ThreadKind threadKind;
		NativeThreadHandle nativeThreadHandle;
		ThreadStatus status = ThreadStatus::Ready;

		SLAKE_API ManagedThread(Runtime *associatedRuntime, ThreadKind threadKind);
		SLAKE_API ~ManagedThread();

		virtual void dealloc() = 0;
	};

	class ExecutionThread : public ManagedThread {
	public:
		ContextObject *context = nullptr;
		void *nativeExecStackBase = nullptr;
		size_t nativeExecStackSize = 0;
		InternalExceptionPointer exceptionPtr;

		#if _WIN32
		#elif __unix__
		bool isInitialRunPthreadCondAllocated = false;
		pthread_cond_t initialRunPthreadCond;
		bool isInitialRunPthreadMutexAllocated = false;
		pthread_mutex_t initialRunPthreadMutex;
		bool isDonePthreadMutexAllocated = false;
		pthread_mutex_t donePthreadMutex;
		#else
		#endif

		SLAKE_API ExecutionThread(Runtime *associatedRuntime);
		SLAKE_API ~ExecutionThread();

		virtual void dealloc() override;

		static ExecutionThread *alloc(Runtime *associatedRuntime);

		void start();
		void join();
	};

	ExecutionThread *createExecutionThread(Runtime *runtime, ContextObject *context, size_t nativeStackSize);
	NativeThreadHandle currentThreadHandle();

	void *estimateCurrentStackPointer();
}

#endif
