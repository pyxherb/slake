#include "runtime.h"

using namespace slake;

MinorFrame::MinorFrame(uint32_t nLocalVars, uint32_t nRegs) : nLocalVars(nLocalVars), nRegs(nRegs) {
}

MajorFrame::MajorFrame(Runtime *rt) {
	minorFrames.push_back(MinorFrame(0, 0));
}

Runtime::Runtime(RuntimeFlags flags) : _flags(flags) {
	_rootObject = new RootObject(this);
}

Runtime::~Runtime() {
	_rootObject = nullptr;
	activeContexts.clear();

	gc();

	assert(!createdObjects.size());
	assert(!_szMemInUse);
}
