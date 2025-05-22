#include <slake/runtime.h>

using namespace slake;

SLAKE_API Cond::Cond() {
	InitializeCriticalSection(&criticalSection);
	InitializeConditionVariable(&nativeHandle);
}
SLAKE_API Cond::~Cond() {
	DeleteCriticalSection(&criticalSection);
}

SLAKE_API void Cond::wait() {
	EnterCriticalSection(&criticalSection);
	SleepConditionVariableCS(&nativeHandle, &criticalSection, INFINITE);
	LeaveCriticalSection(&criticalSection);
}

SLAKE_API void Cond::notify() {
	WakeConditionVariable(&nativeHandle);
}

SLAKE_API void Cond::notifyAll() {
	WakeAllConditionVariable(&nativeHandle);
}
