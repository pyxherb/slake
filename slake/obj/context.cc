#include <slake/runtime.h>

using namespace slake;

SLAKE_API ResumableContext::ResumableContext(peff::Alloc *allocator) : argStack(allocator), lvarRecordOffsets(allocator), nextArgStack(allocator), minorFrames(allocator) {
}

SLAKE_API ResumableContext::~ResumableContext() {}

SLAKE_API ResumableContext &ResumableContext::operator=(ResumableContext &&rhs) {
	curIns = rhs.curIns;
	lastJumpSrc = rhs.lastJumpSrc;
	argStack = std::move(rhs.argStack);
	lvarRecordOffsets = std::move(rhs.lvarRecordOffsets);
	nextArgStack = std::move(rhs.nextArgStack);
	minorFrames = std::move(rhs.minorFrames);
	nRegs = rhs.nRegs;
	thisObject = rhs.thisObject;

	rhs.curIns = 0;
	rhs.lastJumpSrc = UINT32_MAX;
	rhs.nRegs = 0;
	rhs.thisObject = nullptr;

	return *this;
}

SLAKE_API MinorFrame::MinorFrame(
	Runtime *rt,
	size_t stackBase)
	: exceptHandlers(&rt->globalHeapPoolAlloc),
	  stackBase(stackBase) {
}

SLAKE_API MajorFrame::MajorFrame(Runtime *rt)
	: associatedRuntime(rt),
	  resumable(&rt->globalHeapPoolAlloc) {
}

SLAKE_API void MajorFrame::dealloc() noexcept {
	peff::destroyAndRelease<MajorFrame>(&associatedRuntime->globalHeapPoolAlloc, this, alignof(MajorFrame));
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

SLAKE_API Context::Context(Runtime *runtime) : runtime(runtime), majorFrames(&runtime->globalHeapPoolAlloc) {
}

SLAKE_API Context::~Context() {
	if (dataStack) {
		runtime->globalHeapPoolAlloc.release(dataStack, SLAKE_STACK_MAX, sizeof(std::max_align_t));
	}
}

SLAKE_API ContextObject::ContextObject(
	Runtime *rt)
	: Object(rt), _context(rt) {
}

SLAKE_API ContextObject::~ContextObject() {
}

SLAKE_API ObjectKind ContextObject::getKind() const { return ObjectKind::Context; }

SLAKE_API HostObjectRef<ContextObject> slake::ContextObject::alloc(Runtime *rt) {
	std::unique_ptr<ContextObject, util::DeallocableDeleter<ContextObject>> ptr(
		peff::allocAndConstruct<ContextObject>(
			&rt->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			rt));
	if (!ptr)
		return nullptr;

	if (!(ptr->_context.dataStack = (char *)rt->globalHeapPoolAlloc.alloc(SLAKE_STACK_MAX, sizeof(std::max_align_t))))
		return nullptr;

	if (!rt->createdObjects.insert(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API MajorFrame::~MajorFrame() {
}

SLAKE_API void slake::ContextObject::dealloc() {
	peff::destroyAndRelease<ContextObject>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API MajorFrame *MajorFrame::alloc(Runtime *rt, Context *context) {
	return peff::allocAndConstruct<MajorFrame>(&rt->globalHeapPoolAlloc, alignof(MajorFrame), rt);
}

SLAKE_API InternalExceptionPointer ContextObject::resume(HostRefHolder *hostRefHolder) {
	_context.flags &= ~CTX_YIELDED;
	return associatedRuntime->execContext(this);
}

SLAKE_API bool ContextObject::isDone() {
	return _context.flags & CTX_DONE;
}
