#ifndef _SLKC_COMPILER_STATE_HH
#define _SLKC_COMPILER_STATE_HH

#include "fn.hh"

namespace Slake {
	namespace Compiler {
		struct Context {
			std::unordered_map<std::string, LocalVar> lvars;
			std::uint32_t stackCur = 0, nContinueLevel = 0, nBreakLevel = 0;
			bool returned = false;

			inline Context() { }
			inline Context(const Context& context) { *this = context; }
			inline Context(const Context&& context) { *this = context; }

			inline Context& operator=(const Context& x) {
				lvars = x.lvars;
				stackCur = x.stackCur, nContinueLevel = x.nContinueLevel, nBreakLevel = x.nBreakLevel;
				returned = x.returned;
				return *this;
			}
			inline Context& operator=(const Context&& x) {
				lvars = x.lvars;
				stackCur = x.stackCur, nContinueLevel = x.nContinueLevel, nBreakLevel = x.nBreakLevel;
				returned = x.returned;
				return *this;
			}
		};

		enum class OptimizeLevel {

		};

		struct State {
			std::unordered_map<std::string, std::shared_ptr<Fn>> fnDefs;
			std::unordered_map<std::string, std::shared_ptr<RefExpr>> imports;
			std::shared_ptr<Scope> scope;
			Context context;
			std::string currentFn;

			inline void enterLoop() noexcept {
				context.nContinueLevel++, context.nBreakLevel++;
			}

			inline void leaveLoop() noexcept {
				context.nContinueLevel--, context.nBreakLevel--;
			}

			inline void enterSwitch() noexcept {
				context.nContinueLevel++;
			}

			inline void leaveSwitch() noexcept {
				context.nContinueLevel--;
			}

			inline State(std::shared_ptr<Scope> scope = std::shared_ptr<Scope>()) : scope(scope) {}
		};
	}
}

#endif
