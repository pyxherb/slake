#include <slake/runtime.h>
#include <algorithm>

using namespace slake;

SLAKE_API ArrayObject::ArrayObject(Runtime *rt, peff::Alloc *selfAllocator, const Type &elementType, size_t elementSize)
	: Object(rt, selfAllocator),
	  elementType(elementType),
	  elementSize(elementSize) {
}

SLAKE_API ArrayObject::~ArrayObject() {
	if (data) {
		selfAllocator->release(data, elementSize * length, elementSize);
	}
}

SLAKE_API ObjectKind ArrayObject::getKind() const { return ObjectKind::Array; }

SLAKE_API ArrayObject *ArrayObject::alloc(Runtime *rt, const Type &elementType, size_t elementSize) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();
	std::unique_ptr<ArrayObject, util::DeallocableDeleter<ArrayObject>> ptr(
		peff::allocAndConstruct<ArrayObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			rt, curGenerationAllocator.get(), elementType, elementSize));
	if (!ptr)
		return nullptr;

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void ArrayObject::dealloc() {
	peff::destroyAndRelease<ArrayObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

InternalExceptionPointer slake::raiseInvalidArrayIndexError(Runtime *rt, size_t index) {
	return allocOutOfMemoryErrorIfAllocFailed(InvalidArrayIndexError::alloc(rt->getFixedAlloc(), index));
}
