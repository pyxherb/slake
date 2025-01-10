#ifndef _SLKAOT_RT_DEFAULT_STACKOP_H_
#define _SLKAOT_RT_DEFAULT_STACKOP_H_

#include <cstddef>

namespace slake {
	namespace aot_rt {
		constexpr size_t CALLFRAME_RESERVED_SPACE_SIZE = 4096;

		void *getStackPtr();
	}
}

#endif
