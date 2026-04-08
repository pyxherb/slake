#ifndef _SLAKE_PLAT_THREAD_H_
#define _SLAKE_PLAT_THREAD_H_

#include <slake/basedefs.h>
#include <atomic>
#include <peff/base/alloc.h>

#if _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
	#define SLAKE_IS_GET_THREAD_STACK_INFO_SUPPORTED 1
#elif __APPLE__ || __unix__
	#include <pthread.h>
	#include <unistd.h>
	#define SLAKE_IS_GET_THREAD_STACK_INFO_SUPPORTED 1
#else
	#define SLAKE_IS_GET_THREAD_STACK_INFO_SUPPORTED 0
#endif

namespace slake {
#if _WIN32
	using NativeThreadHandle = HANDLE;
#elif __APPLE__ || __unix__
	using NativeThreadHandle = pthread_t;
#else
	#error NativeThreadHandle is not defined for this platform.
#endif

	NativeThreadHandle current_thread_handle();
	void yield_current_thread();

	void get_current_thread_stack_bounds(void *&base_out, size_t &size_out);

	class Mutex final {
	public:
#if _WIN32
		CRITICAL_SECTION native_handle;
#elif __APPLE__ || __unix__
		pthread_mutex_t native_handle;
#else
		#error Mutex is not implemented for this platform.
#endif

		SLAKE_API Mutex();
		SLAKE_API ~Mutex();

		SLAKE_API void lock();
		SLAKE_API bool try_lock();
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

	class Spinlock final {
	public:
		std::atomic_bool locked = false;

		SLAKE_FORCEINLINE Spinlock() {
		}
		SLAKE_FORCEINLINE ~Spinlock() {
		}

		SLAKE_FORCEINLINE void lock() noexcept {
			bool expected = false;
			while (!locked.compare_exchange_strong(expected, true, std::memory_order_acquire)) {
				expected = false;
				yield_current_thread();
			}
		}

		SLAKE_FORCEINLINE bool try_lock() noexcept {
			bool expected = false;
			if(!locked.compare_exchange_strong(expected, true, std::memory_order_acquire)) {
				return false;
			}
			return true;
		}

		SLAKE_FORCEINLINE void unlock() noexcept {
			locked.store(false, std::memory_order_release);
		}
	};

	struct SpinlockGuard final {
	public:
		Spinlock &_mutex;
		bool released = false;

		SpinlockGuard() = delete;
		SpinlockGuard(const SpinlockGuard &) = delete;
		SpinlockGuard(SpinlockGuard &&) = delete;
		SpinlockGuard &operator=(const Spinlock &) = delete;
		SpinlockGuard &operator=(Spinlock &&) = delete;

		SLAKE_FORCEINLINE SpinlockGuard(Spinlock &mutex) : _mutex(mutex) {
			_mutex.lock();
		}

		SLAKE_FORCEINLINE ~SpinlockGuard() {
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
		CRITICAL_SECTION critical_section;
		CONDITION_VARIABLE native_handle;
#elif __APPLE__ || __unix__
		Mutex internal_mutex;
		pthread_cond_t native_handle;
#else
		#error Cond is not implemented for this platform.
#endif

		SLAKE_API Cond();
		SLAKE_API ~Cond();

		SLAKE_API void wait();
		SLAKE_API void notify();
		SLAKE_API void notify_all();
	};

	enum class ThreadKind : uint8_t {
		AttachedExecutionThread = 0,
		ExecutionThread,
		RuntimeThread,
	};

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
		Mutex _initial_run_mutex;
		Mutex _done_mutex;

#if _WIN32
		static DWORD WINAPI _thread_wrapper_proc(LPVOID lp_thread_parameter);
#elif __APPLE__ || __unix__
		static void *_thread_wrapper_proc(void *arg);
#endif

	public:
		peff::RcObjectPtr<peff::Alloc> self_allocator;
		NativeThreadHandle native_thread_handle;
		Runnable *runnable;

		SLAKE_API Thread(peff::Alloc *self_allocator, Runnable *runnable);
		SLAKE_API ~Thread();

		SLAKE_API void start();
		SLAKE_API void join();

		SLAKE_API void dealloc();

		SLAKE_API static Thread *alloc(peff::Alloc *self_allocator, Runnable *runnable, size_t stack_size);
	};
}

#endif
