#ifndef _SLAKE_BASEDEFS_H_
#define _SLAKE_BASEDEFS_H_

#include <peff/base/basedefs.h>

#if SLAKE_BUILD_SHARED
	#if SLAKE_IS_BUILDING
		#define SLAKE_API PEFF_DLLEXPORT
	#else
		#define SLAKE_API PEFF_DLLIMPORT
	#endif
#else
	#define SLAKE_API
#endif

#define SLAKE_FORCEINLINE PEFF_FORCEINLINE

#define SLAKE_REFERENCED_PARAM(n) ((void)n)
#define SLAKE_REFERENCED_VAR(n) ((void)n)

#define SLAKE_REQUIRES_CONCEPT(...) PEFF_REQUIRES_CONCEPT(__VA_ARGS__)

#define SLAKE_UNREACHABLE() PEFF_UNREACHABLE()

#define SLAKE_RESTRICT PEFF_RESTRICT

#if __cplusplus >= 202002L
	#define SLAKE_LIKELY(...) (__VA_ARGS__) [[likely]]
#elif defined(__GNUC__)
	#define SLAKE_LIKELY(...) (__builtin_expect(static_cast<bool>(__VA_ARGS__), 1))
#else
	#define SLAKE_LIKELY(...) (__VA_ARGS__)
#endif

#if __cplusplus >= 202002L
	#define SLAKE_UNLIKELY(...) (__VA_ARGS__) [[unlikely]]
#elif defined(__GNUC__)
	#define SLAKE_UNLIKELY(...) (__builtin_expect(static_cast<bool>(__VA_ARGS__), 0))
#else
	#define SLAKE_UNLIKELY(...) (__VA_ARGS__)
#endif

#if __cplusplus >= 202002L
	#define SLAKE_LIKELY_CASE(...) \
		case __VA_ARGS__:          \
			[[likely]]
#else
	#define SLAKE_LIKELY_CASE(...) case __VA_ARGS__:
#endif

#if __cplusplus >= 202302L
	#define SLAKE_ASSUME(...) [[assume(__VA_ARGS__)]]
#elif defined(__clang__)
	#define SLAKE_ASSUME(...) __builtin_assume(__VA_ARGS__)
#elif defined(__GNUC__)
	#define SLAKE_ASSUME(...) \
		if (__VA_ARGS__) {    \
		} else                \
			__builtin_unreachable()
#else
	#define SLAKE_ASSUME(...)
#endif

#endif
