#ifndef _SLAKE_LIB_STD_MATH_H_
#define _SLAKE_LIB_STD_MATH_H_

#include <slake/runtime.h>

namespace slake {
	namespace stdlib {
		namespace math {
			extern ModuleValue *modMath;

			void load(Runtime *rt);
		}
	}
}

#endif
