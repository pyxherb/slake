#include "../runtime.h"
using namespace slake;

SLAKE_API InternalExceptionPointer Runtime::_findAndDispatchExceptHandler(const Value &curExcept, const MinorFrame &minorFrame, uint32_t &offsetOut) const {
	// Find a proper exception handler.
	bool result;

	for (const auto &i : minorFrame.exceptHandlers) {
		SLAKE_RETURN_IF_EXCEPT(isCompatible(getFixedAlloc(), i.type, curExcept, result));
		if (result) {
			offsetOut = i.off;
			return {};
		}
	}

	offsetOut = UINT32_MAX;
	return {};
}
