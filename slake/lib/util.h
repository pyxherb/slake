#ifndef _SLAKE_LIB_UTIL_H_
#define _SLAKE_LIB_UTIL_H_

#include <slake/runtime.h>

namespace slake {
	namespace stdlib {
		namespace util {
			namespace Math {
				extern ModuleValue *modMath;
				void load(Runtime *rt);
			}
			extern ModuleValue *modUtil;

			void load(Runtime *rt);
		}
	}
}

#endif
