#ifndef _SLAKE_OPTI_OPTIMIZER_H_
#define _SLAKE_OPTI_OPTIMIZER_H_

#include "../runtime.h"

namespace slake {
	bool isConstantValue(const Value &value);
	bool isFnSSACompatible(RegularFnOverloadingObject *fn);
	void trimFnInstructions(RegularFnOverloadingObject *fn);
}

#endif
