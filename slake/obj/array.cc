#include <slake/runtime.h>
#include <algorithm>

using namespace slake;

SLAKE_API ArrayObject::ArrayObject(Runtime *rt, peff::Alloc *selfAllocator, const TypeRef &elementType, size_t elementSize)
	: Object(rt, selfAllocator, ObjectKind::Array),
	  elementType(elementType),
	  elementSize(elementSize) {
}

SLAKE_API ArrayObject::~ArrayObject() {
	if (data) {
		selfAllocator->release(data, elementSize * length, elementAlignment);
	}
}

SLAKE_API ArrayObject *ArrayObject::alloc(Runtime *rt, const TypeRef &elementType, size_t elementSize) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();
	std::unique_ptr<ArrayObject, util::DeallocableDeleter<ArrayObject>> ptr(
		peff::allocAndConstruct<ArrayObject>(
			curGenerationAllocator.get(),
			alignof(ArrayObject),
			rt, curGenerationAllocator.get(), elementType, elementSize));
	if (!ptr)
		return nullptr;

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void ArrayObject::dealloc() {
	peff::destroyAndRelease<ArrayObject>(selfAllocator.get(), this, alignof(ArrayObject));
}

InternalExceptionPointer slake::raiseInvalidArrayIndexError(Runtime *rt, size_t index) {
	return allocOutOfMemoryErrorIfAllocFailed(InvalidArrayIndexError::alloc(rt->getFixedAlloc(), index));
}
