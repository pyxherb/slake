#ifndef _SLKC_BASEDEFS_H_
#define _SLKC_BASEDEFS_H_

#include <slake/basedefs.h>

#if SLKC_BUILD_SHARED
	#if SLKC_IS_BUILDING
		#define SLKC_API SLAKE_DLLEXPORT
	#else
		#define SLKC_API SLAKE_DLLIMPORT
	#endif
#else
	#define SLKC_API
#endif

namespace slkc {
	class Document;
}

#endif
