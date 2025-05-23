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
	SleepConditionVariableCS(&nativeHandle, &criticalSection, INFINITE);
}

SLAKE_API void Cond::notify() {
	WakeConditionVariable(&nativeHandle);
}

SLAKE_API void Cond::notifyAll() {
	WakeAllConditionVariable(&nativeHandle);
}
