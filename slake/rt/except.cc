#include "../runtime.h"
using namespace slake;

SLAKE_API InternalExceptionPointer Runtime::_findAndDispatchExceptHandler(const Value &curExcept, const MinorFrame &minorFrame, uint32_t &offsetOut) const {
	// Find a proper exception handler.
	for (const auto &i : minorFrame.exceptHandlers) {
		if (isCompatible(i.type, curExcept)) {
			offsetOut = i.off;
			return {};
		}
	}

	offsetOut = UINT32_MAX;
	return {};
}
