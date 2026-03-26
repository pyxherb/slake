#include <slake/runtime.h>

using namespace slake;

SLAKE_API Cond::Cond() {
	InitializeCriticalSection(&critical_section);
	InitializeConditionVariable(&native_handle);
}
SLAKE_API Cond::~Cond() {
	DeleteCriticalSection(&critical_section);
}

SLAKE_API void Cond::wait() {
	SleepConditionVariableCS(&native_handle, &critical_section, INFINITE);
}

SLAKE_API void Cond::notify() {
	WakeConditionVariable(&native_handle);
}

SLAKE_API void Cond::notify_all() {
	WakeAllConditionVariable(&native_handle);
}
