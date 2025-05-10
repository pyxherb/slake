#include "var.h"
#include <slake/runtime.h>

using namespace slake;

InternalExceptionPointer slake::raiseMismatchedVarTypeError(Runtime *rt) {
	return allocOutOfMemoryErrorIfAllocFailed(MismatchedVarTypeError::alloc(&rt->globalHeapPoolAlloc));
}
