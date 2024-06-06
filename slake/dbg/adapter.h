#ifndef _SLAKE_DBG_ADAPTER_H_
#define _SLAKE_DBG_ADAPTER_H_

#include <cstddef>
#include <thread>
#include <slake/runtime.h>

namespace slake {
	class DebugAdapter {
	public:
		virtual ~DebugAdapter() = default;

		virtual void onExecBreakpoint(FnObject* fn, uint32_t curIns) = 0;
		virtual void onVarWrite(VarObject* var) = 0;
	};
}

#endif
