#ifndef _SLAKE_DBG_ADAPTER_H_
#define _SLAKE_DBG_ADAPTER_H_

#include <cstddef>
#include <thread>
#include <slake/runtime.h>

namespace slake {
	class DebugAdapter {
	public:
		virtual ~DebugAdapter() = default;

		virtual void onExecBreakpoint(FnValue* fn, uint32_t curIns) = 0;
		virtual void onVarWrite(VarValue* var) = 0;
	};
}

#endif
