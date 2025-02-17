#include <slake/runtime.h>
#include <algorithm>

using namespace slake;

SLAKE_API ArrayObject::ArrayObject(Runtime *rt, const Type &elementType, size_t elementSize)
	: Object(rt),
	  elementType(elementType),
	  elementSize(elementSize) {
}

SLAKE_API ArrayObject::~ArrayObject() {
	if (data) {
		associatedRuntime->globalHeapPoolAlloc.release(data, elementSize * length, elementSize);
	}
}

SLAKE_API ObjectKind ArrayObject::getKind() const { return ObjectKind::Array; }

SLAKE_API ArrayObject *ArrayObject::alloc(Runtime *rt, const Type &elementType, size_t elementSize) {
	std::unique_ptr<ArrayObject, util::DeallocableDeleter<ArrayObject>> ptr(
		peff::allocAndConstruct<ArrayObject>(
			&rt->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			rt, elementType, elementSize));
	if (!ptr)
		return nullptr;

	if (!rt->createdObjects.insert(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void ArrayObject::dealloc() {
	peff::destroyAndRelease<ArrayObject>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

InvalidArrayIndexError *slake::raiseInvalidArrayIndexError(Runtime *rt, size_t index) {
	return InvalidArrayIndexError::alloc(rt, index);
}
