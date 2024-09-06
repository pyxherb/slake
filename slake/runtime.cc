#include "runtime.h"

using namespace slake;

MinorFrame::MinorFrame(uint32_t nLocalVars, uint32_t nRegs, size_t stackBase) : nLocalVars(nLocalVars), nRegs(nRegs), stackBase(stackBase) {
}

MajorFrame::MajorFrame(Runtime *rt, Context *context) : context(context) {
	localVarAccessor = LocalVarAccessorVarObject::alloc(rt, context, this).get();
	minorFrames.push_back(MinorFrame(0, 0, context->stackTop));
}

void MajorFrame::leave() {
	context->stackTop = minorFrames.back().stackBase;
	regs.resize(minorFrames.back().nRegs);
	minorFrames.pop_back();
}

Runtime::Runtime(RuntimeFlags flags) : _flags(flags) {
	_flags |= _RT_INITING;
	_rootObject = RootObject::alloc(this).release();
	_flags &= ~_RT_INITING;
}

Runtime::~Runtime() {
	_genericCacheDir.clear();
	_genericCacheLookupTable.clear();

	_rootObject = nullptr;
	activeContexts.clear();

	gc();

	assert(!createdObjects.size());
	assert(!globalHeapPoolResource.szAllocated);
}
