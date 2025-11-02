#ifndef _SLAKE_OPTI_REGSIMP_H_
#define _SLAKE_OPTI_REGSIMP_H_

#include "proganal.h"

namespace slake {
	namespace opti {
		InternalExceptionPointer simplifyRegularFnOverloading(
			Runtime *runtime,
			peff::Alloc *resourceAllocator,
			RegularFnOverloadingObject *fnObject,
			const ProgramAnalyzedInfo &analyzedInfo,
			HostRefHolder &hostRefHolder);
	}
}

#endif
