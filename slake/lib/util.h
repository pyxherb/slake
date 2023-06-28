#ifndef _SLAKE_LIB_UTIL_H_
#define _SLAKE_LIB_UTIL_H_

#include <slake/runtime.h>

namespace Slake {
	namespace StdLib {
		namespace Util {
			namespace Math {
				extern ModuleValue *modMath;
				Slake::ValueRef<> sin(Slake::Runtime *rt, uint8_t nArgs, Slake::ValueRef<> *args);
				Slake::ValueRef<> cos(Slake::Runtime *rt, uint8_t nArgs, Slake::ValueRef<> *args);

				void load(Runtime *rt);
			}
			extern ModuleValue *modUtil;

			void load(Runtime *rt);
		}
	}
}

#endif
