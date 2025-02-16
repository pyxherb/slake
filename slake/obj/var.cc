#include "var.h"
#include <slake/runtime.h>

using namespace slake;

MismatchedVarTypeError *slake::raiseMismatchedVarTypeError(Runtime *rt) {
	return MismatchedVarTypeError::alloc(rt);
}
