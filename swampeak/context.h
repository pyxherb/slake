#ifndef _SWAMPEAK_CONTEXT_H_
#define _SWAMPEAK_CONTEXT_H_

#include "types.h"
#include "value.h"
#include <deque>

namespace Swampeak {
	constexpr std::size_t STACK_MAX = 131072;
	struct Context final {
		Addr_t execCur;
		Addr_t curStackFrame;
		std::deque<Addr_t> callingStack;
		std::deque<std::shared_ptr<IValue>> dataStack;
	};
}

#endif
