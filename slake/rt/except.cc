#include "../runtime.h"
using namespace slake;

bool Runtime::_findAndDispatchExceptHandler(Context *context) const {
	auto &curMajorFrame = context->majorFrames.back();
	auto &x = curMajorFrame.curExcept;
	// Find for a proper exception handler.
	for (const auto &i : curMajorFrame.minorFrames.back().exceptHandlers) {
		if (isCompatible(i.type, x)) {
			curMajorFrame.curIns = i.off;
			return true;
		}
	}
	return false;
}
