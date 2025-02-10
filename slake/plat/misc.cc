#include <slake/runtime.h>

#if defined(__GNUC__)
void *slake::estimateCurrentStackPointer() {
	return __builtin_frame_address(0);
}
#else
void *slake::estimateCurrentStackPointer() {
	void *volatile v = nullptr;
	return (void *)&v;
}
#endif
