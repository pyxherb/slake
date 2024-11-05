#include "../runtime.h"

using namespace slake;

SLAKE_API void Runtime::gc() {
	_flags |= _RT_INGC;

	_gcHeapless();

	_szMemUsedAfterLastGc = globalHeapPoolResource.szAllocated;
	_flags &= ~_RT_INGC;
}
