#include "../runtime.h"
using namespace slake;

SLAKE_API uint32_t Runtime::_findAndDispatchExceptHandler(const Value &curExcept, const MinorFrame &minorFrame) const {
	// Find a proper exception handler.
	for (const auto &i : minorFrame.exceptHandlers) {
		if (isCompatible(i.type, curExcept)) {
			return i.off;
		}
	}
	return UINT32_MAX;
}
