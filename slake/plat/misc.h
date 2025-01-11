#ifndef _SLAKE_PLAT_MISC_H_
#define _SLAKE_PLAT_MISC_H_

namespace slake {
	/// @brief Estimate current stack pointer (may not accurate).
	/// @return Estimated current stack pointer.
	void *estimateCurrentStackPointer();
}

#endif
