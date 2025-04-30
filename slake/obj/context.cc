#include <slake/runtime.h>

using namespace slake;

SLAKE_API MinorFrame::MinorFrame(
	Runtime *rt,
	size_t stackBase)
	: exceptHandlers(&rt->globalHeapPoolAlloc),
	  stackBase(stackBase) {
}

SLAKE_API MajorFrame::MajorFrame(Runtime *rt, Context *context)
	: context(context),
	  argStack(&rt->globalHeapPoolAlloc),
	  nextArgStack(&rt->globalHeapPoolAlloc),
	  minorFrames(&rt->globalHeapPoolAlloc),
	  lvarRecordOffsets(&rt->globalHeapPoolAlloc),
	  offRegs(SIZE_MAX) {
}

SLAKE_API bool MajorFrame::leave() {
	context->stackTop = minorFrames.back().stackBase;
	if (!minorFrames.popBackAndResizeCapacity())
		return false;
	return true;
}

SLAKE_API void Context::leaveMajor() {
	MajorFrame *frameToBeDestroyed = (MajorFrame *)calcStackAddr(dataStack, SLAKE_STACK_MAX, offMajorFrame);
	stackTop = frameToBeDestroyed->stackBase;
	offMajorFrame = frameToBeDestroyed->offNext;
	std::destroy_at<MajorFrame>(frameToBeDestroyed);
	--majorFrameDepth;
}

SLAKE_API char *Context::stackAlloc(size_t size) {
	if (size_t newStackTop = stackTop + size;
		newStackTop > SLAKE_STACK_MAX) {
		return nullptr;
	} else
		stackTop = newStackTop;

	return dataStack + SLAKE_STACK_MAX - stackTop;
}

SLAKE_API Context::Context(Runtime *runtime) : runtime(runtime), offMajorFrame(SIZE_MAX), offStackTopMajorFrame(SIZE_MAX) {
	// TODO: Move the allocation out of the constructor.
	dataStack = (char *)runtime->globalHeapPoolAlloc.alloc(SLAKE_STACK_MAX, sizeof(std::max_align_t));
}

SLAKE_API Context::~Context() {
	while (offMajorFrame != SIZE_MAX)
		leaveMajor();
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

	if (!rt->createdObjects.insert(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API MajorFrame::~MajorFrame() {
}

SLAKE_API void slake::ContextObject::dealloc() {
	peff::destroyAndRelease<ContextObject>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API InternalExceptionPointer ContextObject::resume(HostRefHolder *hostRefHolder) {
	_context.flags &= ~CTX_YIELDED;
	return associatedRuntime->execContext(this);
}

SLAKE_API bool ContextObject::isDone() {
	return _context.flags & CTX_DONE;
}
