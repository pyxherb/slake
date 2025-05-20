#include <slake/runtime.h>

using namespace slake;

SLAKE_API Mutex::Mutex() {
	nativeHandle = PTHREAD_MUTEX_INITIALIZER;
}
SLAKE_API Mutex::~Mutex() {
}

SLAKE_API void Mutex::lock() {
	assert(isValid);
	pthread_mutex_lock(&nativeHandle);
}

SLAKE_API bool Mutex::tryLock() {
	assert(isValid);
	pthread_mutex_trylock(&nativeHandle);
}

SLAKE_API void Mutex::unlock() {
	assert(isValid);
	pthread_mutex_unlock(&nativeHandle);
}
