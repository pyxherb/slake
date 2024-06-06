#ifndef _SLAKE_DBG_SIDP_H_
#define _SLAKE_DBG_SIDP_H_

#include <cstddef>
#include <thread>
#include "adapter.h"

namespace slake {
	constexpr static char
		ACCEPTED_MAGIC[8] = { 'A', 'C', 'C', 'E', 'P', 'T', 'E', 'D' };

	constexpr static uint8_t
		SIDP_VERSION_MAJOR = 0,
		SIDP_VERSION_MINOR = 1;

	class SidpDebugAdapter : public DebugAdapter {
	private:
		bool _exit = false;
		static void _sidpListenerThreadProc(SidpDebugAdapter *adapter, uint16_t port);

	public:
		SidpDebugAdapter(uint16_t port);
		virtual ~SidpDebugAdapter();

		virtual void onExecBreakpoint(FnObject *fn, uint32_t curIns) override;
		virtual void onVarWrite(VarObject *var) override;
	};
}

#endif
