#ifndef _SLAKE_BASE_DEBUG_H
#define _SLAKE_BASE_DEBUG_H

#if defined(DEBUG) || defined(_DEBUG) || defined(DBG)
	#ifdef _WIN32

		#define _CRTDBG_MAP_ALLOC
		#include <crtdbg.h>
		#include <cstdlib>

		// Memory leak detection
		#define malloc(x) _malloc_dbg(x, _NORMAL_BLOCK, __FILE__, __LINE__)
		#define free(x) _free_dbg(x, _NORMAL_BLOCK)
		#define __DBG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
		#define new __DBG_NEW

	#endif
#endif

namespace Vzc {
	namespace Debug {
		void inline setupMemoryLeakDetector() {
#if defined(_MSC_VER) && (defined(DEBUG) || defined(_DEBUG) || defined(DBG))
			_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
		}

		void inline dumpMemoryLeaks() {
#if defined(_MSC_VER) && (defined(DEBUG) || defined(_DEBUG) || defined(DBG))
			_CrtDumpMemoryLeaks();
#endif
		}
	}
}

#endif
