#include <slake/runtime.h>

using namespace slake;

SLAKE_API MinorFrame::MinorFrame(
	Runtime *rt,
	uint32_t nLocalVars,
	uint32_t nRegs,
	size_t stackBase)
	: exceptHandlers(&rt->globalHeapPoolResource),
	  nLocalVars(nLocalVars),
	  nRegs(nRegs),
	  stackBase(stackBase) {
}

SLAKE_API MajorFrame::MajorFrame(Runtime *rt, Context *context)
	: context(context),
	  argStack(&rt->globalHeapPoolResource),
	  nextArgStack(&rt->globalHeapPoolResource),
	  localVarRecords(&rt->globalHeapPoolResource),
	  regs(&rt->globalHeapPoolResource),
	  minorFrames(&rt->globalHeapPoolResource) {
	localVarAccessor = LocalVarAccessorVarObject::alloc(rt, context, this).get();
	minorFrames.push_back(MinorFrame(rt, 0, 0, context->stackTop));
}

SLAKE_API bool MajorFrame::lload(Runtime *rt, uint32_t off, VarRef &varRefOut) {
	if (off >= localVarRecords.size()) {
		rt->setThreadLocalInternalException(
			std::this_thread::get_id(),
			InvalidLocalVarIndexError::alloc(rt, off));
		return false;
	}

	varRefOut = VarRef(localVarAccessor, VarRefContext::makeLocalVarContext(off));
	return true;
}

SLAKE_API bool MajorFrame::larg(Runtime *rt, uint32_t off, VarRef &varRefOut) {
	if (off >= argStack.size()) {
		rt->setThreadLocalInternalException(
			std::this_thread::get_id(),
			InvalidArgumentIndexError::alloc(rt, off));
		return false;
	}

	varRefOut = argStack.at(off);
	return true;
}

SLAKE_API void MajorFrame::leave() {
	context->stackTop = minorFrames.back().stackBase;
	regs.resize(minorFrames.back().nRegs);
	minorFrames.pop_back();
}

SLAKE_API char *Context::stackAlloc(size_t size) {
	char *stackBase = dataStack + size;

	if (size_t newStackTop = stackTop + size;
		newStackTop > SLAKE_STACK_MAX) {
		return nullptr;
	} else
		stackTop = newStackTop;

	return stackBase;
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
	using Alloc = std::pmr::polymorphic_allocator<ContextObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<ContextObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::ContextObject::dealloc() {
	std::pmr::polymorphic_allocator<ContextObject> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API bool ContextObject::resume(HostRefHolder *hostRefHolder) {
	_context.flags &= ~CTX_YIELDED;
	return associatedRuntime->execContext(this);
}

SLAKE_API Value ContextObject::getResult() {
	return _context.majorFrames.back()->returnValue;
}

SLAKE_API bool ContextObject::isDone() {
	return _context.flags & CTX_DONE;
}
