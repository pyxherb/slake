#include <slake/runtime.h>
#include <slake/util/scope_guard.h>

using namespace slake;

SLAKE_API CoroutineObject::CoroutineObject(Runtime *rt, peff::Alloc *selfAllocator) : Object(rt, selfAllocator), curContext(nullptr), curMajorFrame(nullptr), resumable(selfAllocator), overloading(nullptr), stackData(nullptr), lenStackData(0), offStackTop(0) {
}

SLAKE_API CoroutineObject::~CoroutineObject() {
	releaseStackData();
}

SLAKE_API ObjectKind CoroutineObject::getKind() const { return ObjectKind::Coroutine; }

SLAKE_API HostObjectRef<CoroutineObject> slake::CoroutineObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<CoroutineObject, util::DeallocableDeleter<CoroutineObject>> ptr(
		peff::allocAndConstruct<CoroutineObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			rt, curGenerationAllocator.get()));

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::CoroutineObject::dealloc() {
	peff::destroyAndRelease<CoroutineObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API char* slake::CoroutineObject::allocStackData(size_t size) {
	assert(!stackData);
	if (size) {
		if (!(stackData = (char *)selfAllocator->alloc(size, 1))) {
			return nullptr;
		}
		lenStackData = size;
		return stackData;
	}
	return nullptr;
}

SLAKE_API void slake::CoroutineObject::releaseStackData() {
	if (stackData) {
		selfAllocator->release(stackData, lenStackData, 1);
		stackData = nullptr;
		lenStackData = 0;
	}
}
