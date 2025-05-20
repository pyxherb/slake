#include <slake/runtime.h>

using namespace slake;

SLAKE_API Mutex::Mutex() {
	InitializeCriticalSection(&nativeHandle);
}
SLAKE_API Mutex::~Mutex() {
	DeleteCriticalSection(&nativeHandle);
}

SLAKE_API void Mutex::lock() {
	EnterCriticalSection(&nativeHandle);
}

SLAKE_API bool Mutex::tryLock() {
	return TryEnterCriticalSection(&nativeHandle);
}

SLAKE_API void Mutex::unlock() {
	LeaveCriticalSection(&nativeHandle);
}
