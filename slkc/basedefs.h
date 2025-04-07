#ifndef _SLAKE_BASEDEFS_H_
#define _SLAKE_BASEDEFS_H_

#include <slake/basedefs.h>

#if SLKC_BUILD_SHARED
	#if SLKC_IS_BUILDING

		#if SLKC_BUILD_SHARED
			#define SLKC_API SLAKE_DLLEXPORT
		#endif

	#else

		#if SLKC_BUILD_SHARED
			#define SLKC_API SLAKE_DLLIMPORT
		#endif

	#endif
#else
	#define SLKC_API
#endif

#endif
