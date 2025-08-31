#ifndef _SLAKE_BASEDEFS_H_
#define _SLAKE_BASEDEFS_H_

#ifdef _MSC_VER

	#define SLAKE_DLLEXPORT __declspec(dllexport)
	#define SLAKE_DLLIMPORT __declspec(dllimport)
	#define SLAKE_FORCEINLINE __forceinline
	#define SLAKE_NOINLINE __declspec(noinline)

#elif defined(__GNUC__)

	#define SLAKE_DLLEXPORT __attribute__((visibility("default")))
	#define SLAKE_DLLIMPORT __attribute__((visibility("default")))
	#define SLAKE_FORCEINLINE __attribute__((__always_inline__)) inline
	#define SLAKE_NOINLINE

#endif

#if SLAKE_BUILD_SHARED
	#if SLAKE_IS_BUILDING

		#if SLAKE_BUILD_SHARED
			#define SLAKE_API SLAKE_DLLEXPORT
		#endif

	#else

		#if SLAKE_BUILD_SHARED
			#define SLAKE_API SLAKE_DLLIMPORT
		#endif

	#endif
#else
	#define SLAKE_API
#endif

#define SLAKE_REFERENCED_PARAM(n) (n)

#endif
