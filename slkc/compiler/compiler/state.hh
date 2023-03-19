#ifndef _SLKC_COMPILER_STATE_HH
#define _SLKC_COMPILER_STATE_HH

#include "fn.hh"

namespace Slake {
	namespace Compiler {
		struct State {
			std::unordered_map<std::string, std::shared_ptr<Fn>> fnDefs;
			std::shared_ptr<Scope> scope;
			std::uint32_t stackCur = 0, nContinueLevel = 0, nBreakLevel = 0;
			std::string currentFn;
			bool returned = false;

			inline void enterLoop() noexcept {
				nContinueLevel++, nBreakLevel++;
			}

			inline void leaveLoop() noexcept {
				nContinueLevel--, nBreakLevel--;
			}

			inline void enterSwitch() noexcept {
				nContinueLevel++;
			}

			inline void leaveSwitch() noexcept {
				nContinueLevel--;
			}

			inline State(std::shared_ptr<Scope> scope = std::shared_ptr<Scope>()) : scope(scope) {}
		};
	}
}

#endif
