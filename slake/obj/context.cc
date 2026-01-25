#include <slake/runtime.h>

using namespace slake;

SLAKE_API void MinorFrame::replaceAllocator(peff::Alloc *allocator) noexcept {
	exceptHandlers.replaceAllocator(allocator);
}

SLAKE_API ResumableObject::ResumableObject(Runtime *rt, peff::Alloc *allocator) : Object(rt, allocator, ObjectKind::Resumable), argStack(allocator), lvarRecordOffsets(allocator), nextArgStack(allocator), minorFrames(allocator) {
}

SLAKE_API ResumableObject::~ResumableObject() {}

SLAKE_API ResumableObject *ResumableObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<ResumableObject, peff::DeallocableDeleter<ResumableObject>> ptr(
		peff::allocAndConstruct<ResumableObject>(
			curGenerationAllocator.get(),
			alignof(ResumableObject),
			rt, curGenerationAllocator.get()));

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void ResumableObject::dealloc() noexcept {
	return peff::destroyAndRelease<ResumableObject>(selfAllocator.get(), this, alignof(ResumableObject));
}

SLAKE_API void ResumableObject::replaceAllocator(peff::Alloc *allocator) noexcept {
	this->Object::replaceAllocator(allocator);

	argStack.replaceAllocator(allocator);

	lvarRecordOffsets.replaceAllocator(allocator);

	nextArgStack.replaceAllocator(allocator);

	minorFrames.replaceAllocator(allocator);

	for (auto &i : minorFrames) {
		i.replaceAllocator(allocator);
	}
}

SLAKE_API MinorFrame::MinorFrame(
	Runtime *rt,
	peff::Alloc *allocator,
	size_t stackBase)
	: stackBase(stackBase),
	  exceptHandlers(allocator) {
}

SLAKE_API MajorFrame::MajorFrame(Runtime *rt)
	: associatedRuntime(rt) {
}

SLAKE_API void MajorFrame::dealloc() noexcept {
	peff::destroyAndRelease<MajorFrame>(associatedRuntime->getFixedAlloc(), this, alignof(MajorFrame));
}

SLAKE_API void Context::leaveMajor() {
	stackTop = majorFrames.back()->stackBase;

	majorFrames.popBack();
}

SLAKE_API char *Context::stackAlloc(size_t size) {
	if (size_t newStackTop = stackTop + size;
		newStackTop > stackSize) {
		return nullptr;
	} else
		stackTop = newStackTop;

	return dataStack + stackSize - stackTop;
}

SLAKE_API Context::Context(Runtime *runtime, peff::Alloc *selfAllocator) : runtime(runtime), selfAllocator(selfAllocator), majorFrames(selfAllocator) {
}

SLAKE_API Context::~Context() {
	if (dataStack) {
		selfAllocator->release(dataStack, stackSize, sizeof(std::max_align_t));
	}
}

SLAKE_API void Context::replaceAllocator(peff::Alloc *allocator) noexcept {
	peff::verifyReplaceable(selfAllocator.get(), allocator);

	selfAllocator = allocator;

	majorFrames.replaceAllocator(allocator);
}

SLAKE_API ContextObject::ContextObject(
	Runtime *rt,
	peff::Alloc *selfAllocator)
	: Object(rt, selfAllocator, ObjectKind::Context), _context(rt, selfAllocator) {
}

SLAKE_API ContextObject::~ContextObject() {
}

SLAKE_API HostObjectRef<ContextObject> slake::ContextObject::alloc(Runtime *rt, size_t stackSize) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<ContextObject, peff::DeallocableDeleter<ContextObject>> ptr(
		peff::allocAndConstruct<ContextObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			rt, curGenerationAllocator.get()));
	if (!ptr)
		return nullptr;

	if (!(ptr->_context.dataStack = (char *)curGenerationAllocator->alloc(stackSize, sizeof(std::max_align_t))))
		return nullptr;

	ptr->_context.dataStackTopPtr = ptr->_context.dataStack + stackSize;

	ptr->_context.stackSize = stackSize;

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API MajorFrame::~MajorFrame() {
}

SLAKE_API void slake::ContextObject::dealloc() {
	peff::destroyAndRelease<ContextObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API MajorFrame *MajorFrame::alloc(Runtime *rt, Context *context) {
	return peff::allocAndConstruct<MajorFrame>(rt->getFixedAlloc(), alignof(MajorFrame), rt);
}

SLAKE_API InternalExceptionPointer ContextObject::resume(HostRefHolder *hostRefHolder) {
	_context.flags &= ~CTX_YIELDED;
	return associatedRuntime->execContext(this);
}

SLAKE_API bool ContextObject::isDone() {
	return _context.flags & CTX_DONE;
}

SLAKE_API void ContextObject::replaceAllocator(peff::Alloc *allocator) noexcept {
	this->Object::replaceAllocator(allocator);

	_context.replaceAllocator(allocator);
}
