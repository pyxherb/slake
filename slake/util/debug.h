#ifndef _SLAKE_UTIL_DEBUG_H_
#define _SLAKE_UTIL_DEBUG_H_

#ifndef NDEBUG
	#ifdef _MSC_VER

		#define _CRTDBG_MAP_ALLOC
		#include <crtdbg.h>
		#include <cstdlib>
	#endif
#endif

namespace slake {
	namespace util {
		void inline setupMemoryLeakDetector() {
#ifndef NDEBUG
	#ifdef _MSC_VER
			_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	#endif
#endif
		}

		void inline dumpMemoryLeaks() {
#ifndef NDEBUG
	#ifdef _MSC_VER
			_CrtDumpMemoryLeaks();
	#endif
#endif
		}
	}
}

#endif
