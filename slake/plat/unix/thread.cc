#include <slake/runtime.h>

using namespace slake;

void *Thread::_thread_wrapper_proc(void *arg) {
	// Initial run to collect neccessary information.
#if !SLAKE_IS_GET_THREAD_STACK_INFO_SUPPORTED
	((ExecutionThread *)arg)->native_exec_stack_base = estimate_current_stack_pointer();
#endif

	Thread *self = ((Thread *)arg);
	MutexGuard done_mutex_guard(self->_done_mutex);
	self->_initial_run_mutex.lock();
	self->_initial_run_mutex.unlock();

	self->runnable->run();

	return nullptr;
}

SLAKE_API Thread::Thread(peff::Alloc *self_allocator, Runnable *runnable) : self_allocator(self_allocator), native_thread_handle((pthread_t)-1), runnable(runnable) {
}

SLAKE_API Thread::~Thread() {}

SLAKE_API void Thread::start() {
	_initial_run_mutex.unlock();
}

SLAKE_API void Thread::join() {
	start();
	_done_mutex.lock();
	_done_mutex.unlock();
}

SLAKE_API void Thread::dealloc() {
	peff::destroy_and_release<Thread>(self_allocator.get(), this, alignof(Thread));
}

SLAKE_API Thread *Thread::alloc(peff::Alloc *self_allocator, Runnable *runnable, size_t stack_size) {
	std::unique_ptr<Thread, peff::DeallocableDeleter<Thread>>
		execution_thread(peff::alloc_and_construct<Thread>(self_allocator, alignof(Thread), self_allocator, runnable));

	execution_thread->_initial_run_mutex.lock();

	{
		pthread_attr_t attr;

		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, (std::max)((size_t)PTHREAD_STACK_MIN, stack_size));

		if (pthread_create(&execution_thread->native_thread_handle, &attr, _thread_wrapper_proc, (void *)execution_thread.get())) {
			pthread_attr_destroy(&attr);
			return nullptr;
		}

		pthread_attr_destroy(&attr);
	}

	return execution_thread.release();
}

NativeThreadHandle slake::current_thread_handle() {
	return pthread_self();
}

void slake::yield_current_thread() {
#if _POSIX_PRIORITY_SCHEDULING
	sched_yield();
#else
	pthread_yield();
#endif
}

void slake::get_current_thread_stack_bounds(void *&base_out, size_t &size_out) {
	pthread_attr_t attr = {};

	if(pthread_attr_init(&attr)) {
		base_out = nullptr;
		size_out = 0;
		return;
	}

	void *stack_addr;
	size_t stack_size;

	if (!pthread_attr_getstack(&attr, &stack_addr, &stack_size)) {
		base_out = nullptr;
		size_out = 0;
		return;
	}

	base_out = stack_addr;
	size_out = stack_size;
}
