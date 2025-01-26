#include <slake/runtime.h>

using namespace slake;

SLAKE_API MinorFrame::MinorFrame(
	Runtime *rt,
	uint32_t nLocalVars,
	size_t stackBase)
	: exceptHandlers(&rt->globalHeapPoolAlloc),
	  nLocalVars(nLocalVars),
	  stackBase(stackBase) {
}

SLAKE_API MajorFrame::MajorFrame(Runtime *rt, Context *context)
	: context(context),
	  argStack(&rt->globalHeapPoolAlloc),
	  nextArgStack(&rt->globalHeapPoolAlloc),
	  localVarRecords(&rt->globalHeapPoolAlloc),
	  regs(&rt->globalHeapPoolAlloc),
	  minorFrames(&rt->globalHeapPoolAlloc) {
}

SLAKE_API bool MajorFrame::leave() {
	context->stackTop = minorFrames.back().stackBase;
	if (!localVarRecords.resize(minorFrames.back().nLocalVars))
		return false;
	if (!minorFrames.popBackAndResizeCapacity())
		return false;
	return true;
}

SLAKE_API char *Context::stackAlloc(size_t size) {
	if (size_t newStackTop = stackTop + size;
		newStackTop > SLAKE_STACK_MAX) {
		return nullptr;
	} else
		stackTop = newStackTop;

	return dataStack + SLAKE_STACK_MAX - stackTop;
}

SLAKE_API Context::Context(Runtime *runtime) {
	dataStack = new char[SLAKE_STACK_MAX];
}

SLAKE_API Context::~Context() {
	if (dataStack)
		delete[] dataStack;
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

	if (!rt->createdObjects.pushBack(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::ContextObject::dealloc() {
	peff::destroyAndRelease<ContextObject>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API InternalExceptionPointer ContextObject::resume(HostRefHolder *hostRefHolder) {
	_context.flags &= ~CTX_YIELDED;
	return associatedRuntime->execContext(this);
}

SLAKE_API Value ContextObject::getResult() {
	return _context.majorFrames.back()->regs.at(0);
}

SLAKE_API bool ContextObject::isDone() {
	return _context.flags & CTX_DONE;
}
