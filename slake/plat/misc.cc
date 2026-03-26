#include <slake/runtime.h>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

#if defined(_MSC_VER)
void *slake::estimate_current_stack_pointer() {
	return _AddressOfReturnAddress();
}
#elif defined(__GNUC__)
void *slake::estimate_current_stack_pointer() {
	return __builtin_frame_address(0);
}
#else
void *slake::estimate_current_stack_pointer() {
	void *volatile v = nullptr;
	return (void *)&v;
}
#endif
