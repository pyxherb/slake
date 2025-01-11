#ifndef _SLAKE_PLAT_THREAD_H_
#define _SLAKE_PLAT_THREAD_H_

#include <slake/object.h>

#if _WIN32
	#include <Windows.h>
	#define SLAKE_IS_GET_THREAD_STACK_INFO_SUPPORTED 1
#elif __unix__
	#include <pthread.h>
	#include <unistd.h>
	#define SLAKE_IS_GET_THREAD_STACK_INFO_SUPPORTED 1
#else
	#define SLAKE_IS_GET_THREAD_STACK_INFO_SUPPORTED 0
#endif

namespace slake {
	class Mutex final {
	public:
#if _WIN32
#elif __unix__
		pthread_mutex_t nativeHandle;
		bool isValid = false;
#endif

		SLAKE_API Mutex();
		SLAKE_API ~Mutex();

		SLAKE_API bool init();

		SLAKE_API void lock();
		SLAKE_API bool tryLock();
		SLAKE_API void unlock();

		SLAKE_FORCEINLINE operator bool() {
#if _WIN32
#elif __unix__
			return isValid;
#endif
		}
	};

	struct MutexGuard final {
	public:
		Mutex &_mutex;
		bool released = false;

		MutexGuard() = delete;
		MutexGuard(const MutexGuard &) = delete;
		MutexGuard(MutexGuard &&) = delete;
		MutexGuard &operator=(const Mutex &) = delete;
		MutexGuard &operator=(Mutex &&) = delete;

		SLAKE_FORCEINLINE MutexGuard(Mutex &mutex) : _mutex(mutex) {
			_mutex.lock();
		}

		SLAKE_FORCEINLINE ~MutexGuard() {
			if (!released) {
				_mutex.unlock();
			}
		}

		SLAKE_FORCEINLINE void release() {
			assert(!released);
			_mutex.unlock();
			released = true;
		}
	};

	class Cond final {
	public:
#if _WIN32
#elif __unix__
		Mutex internalMutex;
		pthread_cond_t nativeHandle;
		bool isValid = false;
#endif

		SLAKE_API Cond();
		SLAKE_API ~Cond();

		SLAKE_API bool init();

		SLAKE_API void wait();
		SLAKE_API void notify();

		SLAKE_FORCEINLINE operator bool() {
#if _WIN32
#elif __unix__
			return isValid;
#endif
		}
	};

	enum class ThreadKind : uint8_t {
		AttachedExecutionThread = 0,
		ExecutionThread,
		RuntimeThread,
	};

#if _WIN32
	using NativeThreadHandle = HANDLE;
#elif __unix__
	using NativeThreadHandle = pthread_t;
#endif

	enum class ThreadStatus : uint8_t {
		Ready = 0,
		Running,
		Done,
		Dead
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

		virtual void start() = 0;
		virtual void join() = 0;
		virtual void kill() = 0;
	};

	class AttachedExecutionThread : public ManagedThread {
	private:
		Mutex _initialRunMutex;
		Cond _initCond;
		Mutex _doneMutex;

		friend AttachedExecutionThread *createAttachedExecutionThreadForCurrentThread(Runtime *runtime, ContextObject *context);

	public:
		ContextObject *context = nullptr;
		void *nativeExecStackBase = nullptr;
		size_t nativeExecStackSize;

		SLAKE_API AttachedExecutionThread(Runtime *associatedRuntime);
		SLAKE_API ~AttachedExecutionThread();

		virtual void dealloc() override;

		static AttachedExecutionThread *alloc(Runtime *associatedRuntime);

		virtual void start() override;
		virtual void join() override;
		virtual void kill() override;
	};

	AttachedExecutionThread *createAttachedExecutionThreadForCurrentThread(Runtime *runtime, ContextObject *context);

	class ExecutionThread : public ManagedThread {
	private:
		Mutex _initialRunMutex;
		Cond _initCond;
		Mutex _doneMutex;

#if _WIN32
#elif __unix__

		static void *_threadWrapperProc(void *arg);
#endif

		friend ExecutionThread *createExecutionThread(Runtime *runtime, ContextObject *context, size_t nativeStackSize);

	public:
		ContextObject *context = nullptr;
		void *nativeExecStackBase = nullptr;
		size_t nativeExecStackSize = 0;
		InternalExceptionPointer exceptionPtr;

		SLAKE_API ExecutionThread(Runtime *associatedRuntime);
		SLAKE_API ~ExecutionThread();

		virtual void dealloc() override;

		static ExecutionThread *alloc(Runtime *associatedRuntime);

		virtual void start() override;
		virtual void join() override;
		virtual void kill() override;
	};

	ExecutionThread *createExecutionThread(Runtime *runtime, ContextObject *context, size_t nativeStackSize);
	NativeThreadHandle currentThreadHandle();
	void yieldCurrentThread();

	void *getCurrentThreadStackBase();
	size_t getCurrentThreadStackSize();
}

#endif
