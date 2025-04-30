#include <slake/runtime.h>
#include <slake/util/scope_guard.h>

using namespace slake;

SLAKE_API CoroutineObject::CoroutineObject(Runtime *rt) : Object(rt), curContext(nullptr), curMajorFrame(nullptr), args(&rt->globalHeapPoolAlloc), overloading(nullptr), offIns(0), stackData(nullptr), lenStackData(0), nRegs(0), offRegs(SIZE_MAX) {
}

SLAKE_API CoroutineObject::~CoroutineObject() {
	releaseStackData();
}

SLAKE_API ObjectKind CoroutineObject::getKind() const { return ObjectKind::Coroutine; }

SLAKE_API HostObjectRef<CoroutineObject> slake::CoroutineObject::alloc(Runtime *rt) {
	std::unique_ptr<CoroutineObject, util::DeallocableDeleter<CoroutineObject>> ptr(
		peff::allocAndConstruct<CoroutineObject>(
			&rt->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			rt));

	if (!rt->createdObjects.insert(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::CoroutineObject::dealloc() {
	peff::destroyAndRelease<CoroutineObject>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API char* slake::CoroutineObject::allocStackData(size_t size) {
	if (!(stackData = (char*)associatedRuntime->globalHeapPoolAlloc.alloc(lenStackData, 1))) {
		return nullptr;
	}
	return stackData;
}

SLAKE_API void slake::CoroutineObject::releaseStackData() {
	if (stackData) {
		assert(lenStackData);
		associatedRuntime->globalHeapPoolAlloc.release(stackData, lenStackData, 1);
	}
}
