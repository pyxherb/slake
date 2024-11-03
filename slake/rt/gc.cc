#include "../runtime.h"

using namespace slake;

SLAKE_API void Runtime::gc() {
	_flags |= _RT_INGC;

	//if (!_gcDfs()) {
		_gcHeapless();
	//}

	_szMemUsedAfterLastGc = globalHeapPoolResource.szAllocated;
	_flags &= ~_RT_INGC;
}
