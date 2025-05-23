#ifndef _SLAKE_PLAT_THREAD_H_
#define _SLAKE_PLAT_THREAD_H_

#if _WIN32
	#define WIN32_LEAN_AND_MEAN
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
		CRITICAL_SECTION nativeHandle;
#elif __unix__
		pthread_mutex_t nativeHandle;
#endif

		SLAKE_API Mutex();
		SLAKE_API ~Mutex();

		SLAKE_API void lock();
		SLAKE_API bool tryLock();
		SLAKE_API void unlock();
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
		CRITICAL_SECTION criticalSection;
		CONDITION_VARIABLE nativeHandle;
#elif __unix__
		Mutex internalMutex;
		pthread_cond_t nativeHandle;
#endif

		SLAKE_API Cond();
		SLAKE_API ~Cond();

		SLAKE_API void wait();
		SLAKE_API void notify();
		SLAKE_API void notifyAll();
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

	class Runnable {
	public:
		virtual void run() = 0;
	};

	class Thread final {
	private:
		Mutex _initialRunMutex;
		Mutex _doneMutex;

#if _WIN32
		static DWORD WINAPI _threadWrapperProc(LPVOID lpThreadParameter);
#elif __unix__
		static void *_threadWrapperProc(void *arg);
#endif

	public:
		peff::RcObjectPtr<peff::Alloc> selfAllocator;
		NativeThreadHandle nativeThreadHandle;
		Runnable *runnable;

		SLAKE_API Thread(peff::Alloc *selfAllocator, Runnable *runnable);
		SLAKE_API ~Thread();

		SLAKE_API void start();
		SLAKE_API void join();

		SLAKE_API void dealloc();

		SLAKE_API static Thread *alloc(peff::Alloc *selfAllocator, Runnable *runnable, size_t stackSize);
	};

	NativeThreadHandle currentThreadHandle();
	void yieldCurrentThread();

	void getCurrentThreadStackBounds(void *&baseOut, size_t &sizeOut);
}

#endif
