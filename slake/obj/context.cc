#include <slake/runtime.h>

using namespace slake;

SLAKE_API void ResumableContextData::replaceAllocator(peff::Alloc *allocator) noexcept {
	argStack.replaceAllocator(allocator);

	nextArgStack.replaceAllocator(allocator);
}

SLAKE_API ResumableContextData::ResumableContextData(peff::Alloc *allocator) noexcept : argStack(allocator), nextArgStack(allocator) {
}

SLAKE_API ResumableContextData::~ResumableContextData() {
}

SLAKE_API MajorFrame::MajorFrame(Runtime *rt) noexcept
	: associatedRuntime(rt) {
}

SLAKE_API void MajorFrame::dealloc() noexcept {
	peff::destroyAndRelease<MajorFrame>(associatedRuntime->getFixedAlloc(), this, alignof(MajorFrame));
}

SLAKE_API char *Context::stackAlloc(size_t size) {
	if (size_t newStackTop = stackTop + size;
		newStackTop > stackSize) {
		return nullptr;
	} else
		stackTop = newStackTop;

	return dataStack + stackSize - stackTop;
}

SLAKE_API Context::Context(Runtime *runtime, peff::Alloc *selfAllocator) : runtime(runtime), selfAllocator(selfAllocator) {
}

SLAKE_API Context::~Context() {
	{
		MajorFrame *curMajorFrame;
		size_t offCurMajorFrame = this->offCurMajorFrame;

		if (offCurMajorFrame != SIZE_MAX)
			do {
				curMajorFrame = this->runtime->_fetchMajorFrame(this, offCurMajorFrame);
				offCurMajorFrame = curMajorFrame->offPrevFrame;
				std::destroy_at(curMajorFrame);
			} while (offCurMajorFrame != SIZE_MAX);
	}

	if (dataStack) {
		selfAllocator->release(dataStack, stackSize, sizeof(std::max_align_t));
	}
}

SLAKE_API void Context::replaceAllocator(peff::Alloc *allocator) noexcept {
	peff::verifyReplaceable(selfAllocator.get(), allocator);

	selfAllocator = allocator;

	MajorFrame *curMajorFrame;
	size_t offCurMajorFrame = this->offCurMajorFrame;

	do {
		curMajorFrame = this->runtime->_fetchMajorFrame(this, offCurMajorFrame);
		curMajorFrame->replaceAllocator(allocator);
		offCurMajorFrame = curMajorFrame->offPrevFrame;
	} while (curMajorFrame->offPrevFrame != SIZE_MAX);
}

SLAKE_API void Context::forEachMajorFrame(MajorFrameWalker walker, void *userData) {
	MajorFrame *curMajorFrame;
	size_t offCurMajorFrame = this->offCurMajorFrame;

	do {
		curMajorFrame = this->runtime->_fetchMajorFrame(this, offCurMajorFrame);
		if (!walker(curMajorFrame, userData))
			break;
		offCurMajorFrame = curMajorFrame->offPrevFrame;
	} while (curMajorFrame->offPrevFrame != SIZE_MAX);
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

SLAKE_API void MajorFrame::replaceAllocator(peff::Alloc *allocator) noexcept {
	if (resumableContextData.hasValue())
		resumableContextData->replaceAllocator(allocator);
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
