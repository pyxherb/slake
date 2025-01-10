#ifndef _SLKAOT_RT_STACKOP_H_
#define _SLKAOT_RT_STACKOP_H_

#include <slake/basedefs.h>
#include <cstddef>

namespace slake {
	namespace aot_rt {
		constexpr size_t CALLFRAME_RESERVED_SPACE_SIZE = 0;

		SLAKE_FORCEINLINE void *getStackPtr() {
			return __builtin_frame_address(0);
		}
	}
}

#endif
