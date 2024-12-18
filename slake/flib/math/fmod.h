#ifndef _SLAKE_FLIB_FMOD_H_
#define _SLAKE_FLIB_FMOD_H_

#include <slake/basedefs.h>

namespace slake {
	namespace flib {
		SLAKE_API float fmodf(float n, float d);
		SLAKE_API double fmod(double n, double d);
	}
}

#endif
