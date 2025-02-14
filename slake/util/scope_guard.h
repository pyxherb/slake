#ifndef _SLAKE_UTIL_SCOPE_GUARD_H_
#define _SLAKE_UTIL_SCOPE_GUARD_H_

#include <functional>

namespace slake {
	namespace util {
		using ScopeGuardCallback = std::function<void()>;

		struct ScopeGuard {
			ScopeGuardCallback callback;

			ScopeGuard() = default;
			inline ScopeGuard(ScopeGuardCallback &&callback) : callback(callback) {}
			inline ~ScopeGuard() {
				if (callback)
					callback();
			}

			inline void release() {
				callback = {};
			}

			inline ScopeGuard &operator=(ScopeGuardCallback &&callback) {
				this->callback = callback;
				return *this;
			}
		};
	}
}

#endif
