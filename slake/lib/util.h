#ifndef _SLAKE_LIB_UTIL_H_
#define _SLAKE_LIB_UTIL_H_

#include <slake/runtime.h>

namespace Slake {
	namespace StdLib {
		namespace Util {
			namespace Concurrent {
				Slake::ValueRef<> threadNew(Slake::Runtime *rt, uint8_t nArgs, Slake::ValueRef<> *args);
				Slake::ValueRef<> threadDelete(Slake::Runtime *rt, uint8_t nArgs, Slake::ValueRef<> *args);
				Slake::ValueRef<> threadKill(Slake::Runtime *rt, uint8_t nArgs, Slake::ValueRef<> *args);
				Slake::ValueRef<> threadSuspend(Slake::Runtime *rt, uint8_t nArgs, Slake::ValueRef<> *args);
				Slake::ValueRef<> threadResume(Slake::Runtime *rt, uint8_t nArgs, Slake::ValueRef<> *args);
				Slake::ValueRef<> threadJoin(Slake::Runtime *rt, uint8_t nArgs, Slake::ValueRef<> *args);
				Slake::ValueRef<> threadRun(Slake::Runtime *rt, uint8_t nArgs, Slake::ValueRef<> *args);
			}
		}
	}
}

#endif
