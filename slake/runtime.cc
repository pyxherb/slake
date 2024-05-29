#include "runtime.h"

using namespace slake;

MinorFrame::MinorFrame(uint32_t nLocalVars, uint32_t nRegs) : nLocalVars(nLocalVars), nRegs(nRegs) {
}

MajorFrame::MajorFrame(Runtime *rt) {
	minorFrames.push_back(MinorFrame(0, 0));
}

Runtime::Runtime(RuntimeFlags flags) : _flags(flags) {
	_rootValue = new RootValue(this);
}

Runtime::~Runtime() {
	_rootValue = nullptr;
	activeContexts.clear();

	gc();

	assert(!createdValues.size());
	assert(!_szMemInUse);
}
