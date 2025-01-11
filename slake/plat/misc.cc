#include <slake/runtime.h>

void *slake::estimateCurrentStackPointer() {
	void *volatile v = nullptr;
	return (void *)&v;
}
