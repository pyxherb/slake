#include "runtime.h"

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

SLAKE_API VarRef MajorFrame::lload(uint32_t off) {
	if (off >= localVarRecords.size())
		throw InvalidLocalVarIndexError("Invalid local variable index", off);

	return VarRef(localVarAccessor, VarRefContext::makeLocalVarContext(off));
}

SLAKE_API VarRef MajorFrame::larg(uint32_t off) {
	if (off >= argStack.size())
		throw InvalidArgumentIndexError("Invalid argument index", off);

	return VarRef(argStack.at(off));
}

SLAKE_API char *Context::stackAlloc(size_t size) {
	char *stackBase = dataStack + size;

	if (size_t newStackTop = stackTop + size;
		newStackTop > SLAKE_STACK_MAX)
		throw StackOverflowError("Stack overflowed");
	else
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

SLAKE_API CountablePoolResource::CountablePoolResource(std::pmr::memory_resource *upstream) : upstream(upstream) {}

SLAKE_API void *CountablePoolResource::do_allocate(size_t bytes, size_t alignment) {
	void *p = upstream->allocate(bytes, alignment);

	szAllocated += bytes;

	return p;
}

SLAKE_API void CountablePoolResource::do_deallocate(void *p, size_t bytes, size_t alignment) {
	upstream->deallocate(p, bytes, alignment);

	szAllocated -= bytes;
}

SLAKE_API bool CountablePoolResource::do_is_equal(const std::pmr::memory_resource &other) const noexcept {
	return this == &other;
}

SLAKE_API void MajorFrame::leave() {
	context->stackTop = minorFrames.back().stackBase;
	regs.resize(minorFrames.back().nRegs);
	minorFrames.pop_back();
}

SLAKE_API Runtime::Runtime(std::pmr::memory_resource *upstreamMemoryResource, RuntimeFlags flags)
	: globalHeapPoolResource(upstreamMemoryResource),
	  _flags(flags) {
	_flags |= _RT_INITING;
	_rootObject = RootObject::alloc(this).release();
	_flags &= ~_RT_INITING;
}

SLAKE_API Runtime::~Runtime() {
	_genericCacheDir.clear();
	_genericCacheLookupTable.clear();

	_rootObject = nullptr;
	activeContexts.clear();

	gc();

	assert(!createdObjects.size());
	assert(!globalHeapPoolResource.szAllocated);
}
