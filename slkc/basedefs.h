#ifndef _SLKC_BASEDEFS_H_
#define _SLKC_BASEDEFS_H_

#include <slake/basedefs.h>
#include <config.h>

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
