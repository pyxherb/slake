#include <slake/runtime.h>

using namespace slake;

SLAKE_API Mutex::Mutex() {
	native_handle = PTHREAD_MUTEX_INITIALIZER;
}
SLAKE_API Mutex::~Mutex() {
}

SLAKE_API void Mutex::lock() {
	pthread_mutex_lock(&native_handle);
}

SLAKE_API bool Mutex::try_lock() {
	return pthread_mutex_trylock(&native_handle);
}

SLAKE_API void Mutex::unlock() {
	pthread_mutex_unlock(&native_handle);
}
