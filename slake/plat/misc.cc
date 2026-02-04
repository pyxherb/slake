#include <slake/runtime.h>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

#if defined(_MSC_VER)
void *slake::estimateCurrentStackPointer() {
	return _AddressOfReturnAddress();
}
#elif defined(__GNUC__)
void *slake::estimateCurrentStackPointer() {
	return __builtin_frame_address(0);
}
#else
void *slake::estimateCurrentStackPointer() {
	void *volatile v = nullptr;
	return (void *)&v;
}
#endif
