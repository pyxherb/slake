#ifndef _SLAKE_OBJ_VAR_H_
#define _SLAKE_OBJ_VAR_H_

#include "member.h"
#include <slake/except.h>
#include <slake/type.h>

namespace slake {
	struct MajorFrame;
	struct Context;

	struct LocalVarRecord {
		size_t stackOffset;
		Type type;
	};

	[[nodiscard]] MismatchedVarTypeError *raiseMismatchedVarTypeError(Runtime *rt);
}

#endif
