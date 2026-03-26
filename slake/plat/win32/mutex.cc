#include <slake/runtime.h>

using namespace slake;

SLAKE_API Mutex::Mutex() {
	InitializeCriticalSection(&native_handle);
}
SLAKE_API Mutex::~Mutex() {
	DeleteCriticalSection(&native_handle);
}

SLAKE_API void Mutex::lock() {
	EnterCriticalSection(&native_handle);
}

SLAKE_API bool Mutex::try_lock() {
	return TryEnterCriticalSection(&native_handle);
}

SLAKE_API void Mutex::unlock() {
	LeaveCriticalSection(&native_handle);
}
