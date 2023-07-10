#ifndef _SLAKE_LIB_STD_H_
#define _SLAKE_LIB_STD_H_

#include <slake/runtime.h>

namespace slake {
	namespace stdlib {
		extern ModuleValue *modStd;
		void load(Runtime *rt);
	}
}

#endif
