#include <slake/runtime.h>

using namespace slake;

DWORD WINAPI Thread::_thread_wrapper_proc(LPVOID lp_thread_parameter) {
	Thread *self = ((Thread *)lp_thread_parameter);

	self->_initial_run_mutex.lock();
	self->_initial_run_mutex.unlock();

	self->runnable->run();

	return 0;
}

SLAKE_API Thread::Thread(peff::Alloc *self_allocator, Runnable *runnable) : self_allocator(self_allocator), native_thread_handle(native_thread_handle), runnable(runnable) {
}

SLAKE_API Thread::~Thread() {}

SLAKE_API void Thread::start() {
	_initial_run_mutex.unlock();
}

SLAKE_API void Thread::join() {
	start();
	WaitForSingleObject(native_thread_handle, INFINITE);
}

SLAKE_API void Thread::dealloc() {
	peff::destroy_and_release<Thread>(self_allocator.get(), this, alignof(Thread));
}

SLAKE_API Thread *Thread::alloc(peff::Alloc *self_allocator, Runnable *runnable, size_t stack_size) {
	std::unique_ptr<Thread, peff::DeallocableDeleter<Thread>>
		execution_thread(peff::alloc_and_construct<Thread>(self_allocator, alignof(Thread), self_allocator, runnable));

	if (!execution_thread) {
		return nullptr;
	}

	execution_thread->_initial_run_mutex.lock();

	if ((execution_thread->native_thread_handle = CreateThread(
			 nullptr,
			 stack_size,
			 _thread_wrapper_proc,
			 (LPVOID)execution_thread.get(),
			 0,
			 0)) == NULL) {
		return nullptr;
	}

	return execution_thread.release();
}

NativeThreadHandle slake::current_thread_handle() {
	return GetCurrentThread();
}

void slake::yield_current_thread() {
	SwitchToThread();
}

void slake::get_current_thread_stack_bounds(void *&base_out, size_t &size_out) {
#if _WIN32_WINNT >= 0x0602
	ULONG_PTR low;
	ULONG_PTR high;
	GetCurrentThreadStackLimits(&low, &high);
	base_out = (void *)low;
	size_out = (size_t)high - low;
#else
	MEMORY_BASIC_INFORMATION mem_info;
	VirtualQuery(&mem_info, &mem_info, sizeof(mem_info));

	base_out = mem_info.AllocationBase;
	size_out = (size_t)((NT_TIB *)NtCurrentTeb())->StackBase - (size_t)mem_info.AllocationBase;
#endif
}
