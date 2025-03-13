#include "../runtime.h"

using namespace slake;

SLAKE_API void Runtime::_destructDestructibleObjects() {
	InternalExceptionPointer exception;

	for (InstanceObject *i = destructibleList, *next; i; i = next) {
		next = i->gcInfo.heapless.nextDestructible;

		destructibleList = next;

		i->_flags |= VF_DESTRUCTED;

		HostObjectRef<ContextObject> contextOut;
		for (auto j : i->_class->cachedInstantiatedMethodTable->destructors) {
			if ((exception = execFn(j, nullptr, i, nullptr, 0, contextOut))) {
				if (!_uncaughtExceptionHandler) {
					std::terminate();
				}
				_uncaughtExceptionHandler(std::move(exception));
			}
		}
	}
}

SLAKE_API void Runtime::gc() {
	_flags |= _RT_INGC;

	// TODO: This is a stupid way to make sure that all the destructible objects are destructed.
	// Can we create a separate GC thread in advance and let it to execute them?
	_gcHeapless();

	_szMemUsedAfterLastGc = globalHeapPoolAlloc.szAllocated;
	_szComputedGcLimit = _szMemUsedAfterLastGc + (_szMemUsedAfterLastGc >> 1);

	_flags &= ~_RT_INGC;
}
