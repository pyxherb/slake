#include <slake/runtime.h>

using namespace slake;

SLAKE_API void MinorFrame::replaceAllocator(peff::Alloc *allocator) noexcept {
	exceptHandlers.replaceAllocator(allocator);
}

SLAKE_API ResumableObject::ResumableObject(Runtime *rt, peff::Alloc *allocator) : Object(rt, allocator), argStack(allocator), lvarRecordOffsets(allocator), nextArgStack(allocator), minorFrames(allocator) {
}

SLAKE_API ResumableObject::~ResumableObject() {}

SLAKE_API ObjectKind ResumableObject::getKind() const {
	return ObjectKind::Resumable;
}

SLAKE_API ResumableObject *ResumableObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<ResumableObject, util::DeallocableDeleter<ResumableObject>> ptr(
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
	: exceptHandlers(allocator),
	  stackBase(stackBase) {
}

SLAKE_API MajorFrame::MajorFrame(Runtime *rt, peff::Alloc *allocator)
	: associatedRuntime(rt),
	  selfAllocator(allocator) {
}

SLAKE_API void MajorFrame::dealloc() noexcept {
	peff::destroyAndRelease<MajorFrame>(selfAllocator.get(), this, alignof(MajorFrame));
}

SLAKE_API void MajorFrame::replaceAllocator(peff::Alloc *allocator) noexcept {
	peff::verifyReplaceable(selfAllocator.get(), allocator);

	selfAllocator = allocator;
}

SLAKE_API void Context::leaveMajor() {
	stackTop = majorFrames.back()->stackBase;

	majorFrames.popBack();
}

SLAKE_API char *Context::stackAlloc(size_t size) {
	if (size_t newStackTop = stackTop + size;
		newStackTop > SLAKE_STACK_MAX) {
		return nullptr;
	} else
		stackTop = newStackTop;

	return dataStack + SLAKE_STACK_MAX - stackTop;
}

SLAKE_API Context::Context(Runtime *runtime, peff::Alloc *selfAllocator) : runtime(runtime), selfAllocator(selfAllocator), majorFrames(selfAllocator) {
}

SLAKE_API Context::~Context() {
	if (dataStack) {
		selfAllocator->release(dataStack, SLAKE_STACK_MAX, sizeof(std::max_align_t));
	}
}

SLAKE_API void Context::replaceAllocator(peff::Alloc *allocator) noexcept {
	peff::verifyReplaceable(selfAllocator.get(), allocator);

	selfAllocator = allocator;

	majorFrames.replaceAllocator(allocator);

	for (auto &i : majorFrames) {
		i->replaceAllocator(allocator);
	}
}

SLAKE_API ContextObject::ContextObject(
	Runtime *rt,
	peff::Alloc *selfAllocator)
	: Object(rt, selfAllocator), _context(rt, selfAllocator) {
}

SLAKE_API ContextObject::~ContextObject() {
}

SLAKE_API ObjectKind ContextObject::getKind() const { return ObjectKind::Context; }

SLAKE_API HostObjectRef<ContextObject> slake::ContextObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<ContextObject, util::DeallocableDeleter<ContextObject>> ptr(
		peff::allocAndConstruct<ContextObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			rt, curGenerationAllocator.get()));
	if (!ptr)
		return nullptr;

	if (!(ptr->_context.dataStack = (char *)curGenerationAllocator->alloc(SLAKE_STACK_MAX, sizeof(std::max_align_t))))
		return nullptr;

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
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	return peff::allocAndConstruct<MajorFrame>(curGenerationAllocator.get(), alignof(MajorFrame), rt, curGenerationAllocator.get());
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
