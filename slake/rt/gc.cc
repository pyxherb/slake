#include "../runtime.h"

using namespace slake;

SLAKE_API void Runtime::_destructDestructibleObjects(InstanceObject *destructibleList) {
	InternalExceptionPointer exception;
	/*
	for (InstanceObject *i = destructibleList, *next; i; i = next) {
		next = (InstanceObject *)i->nextSameKindObject;

		destructibleList = next;

		i->objectFlags |= VF_DESTRUCTED;

		Value resultOut;
		for (auto j : i->_class->cachedInstantiatedMethodTable->destructors) {
			if ((exception = execFn(j, nullptr, i, nullptr, 0, resultOut))) {
				if (!_uncaughtExceptionHandler) {
					std::terminate();
				}
				_uncaughtExceptionHandler(std::move(exception));
			}
		}
	}*/
}

SLAKE_API void Runtime::gc() {
	runtimeFlags |= _RT_INGC;

	// TODO: This is a stupid way to make sure that all the destructible objects are destructed.
	// Can we create a separate GC thread in advance and let it to execute them?
	Object *youngObjectsEnd;
	_gcSerial(youngObjectList, youngObjectsEnd, nYoungObjects, ObjectGeneration::Persistent, &persistentAlloc);

	size_t youngSize = youngAlloc.szAllocated.load();
	if (youngSize) {
		persistentAlloc.szAllocated += youngSize;
		youngAlloc.szAllocated = 0;
	}

	if (youngObjectsEnd) {
		youngObjectsEnd->nextSameGenObject = persistentObjectList;
	}
	if (persistentObjectList) {
		persistentObjectList->prevSameGenObject = youngObjectsEnd;
	}
	persistentObjectList = youngObjectList;
	nPersistentObjects += nYoungObjects;

	youngObjectList = nullptr;
	nYoungObjects = 0;

	if (persistentAlloc.szAllocated >= _szComputedPersistentGcLimit) {
		Object *persistentObjectsEnd;
		_gcSerial(persistentObjectList, persistentObjectsEnd, nPersistentObjects, ObjectGeneration::Persistent, nullptr);

		_szPersistentMemUsedAfterLastGc = persistentAlloc.szAllocated;
		_szComputedPersistentGcLimit = _szPersistentMemUsedAfterLastGc + (_szPersistentMemUsedAfterLastGc >> 1);
	}

	_szMemUsedAfterLastGc = youngAlloc.szAllocated + persistentAlloc.szAllocated;
	_szComputedGcLimit = _szMemUsedAfterLastGc + (_szMemUsedAfterLastGc >> 1);

#ifndef _NDEBUG
	if (youngAlloc.refCount) {
		puts("Detected unreplaced allocator references!");

		for (auto i : youngAlloc.recordedRefPoints) {
			printf("Reference point #%zu\n", i);
		}

		puts("Dump completed");
		std::terminate();
	}
#endif

	runtimeFlags &= ~_RT_INGC;
}
