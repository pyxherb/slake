#include <slake/runtime.h>

using namespace slake;

SLAKE_API Cond::Cond() {
	nativeHandle = PTHREAD_COND_INITIALIZER;
}
SLAKE_API Cond::~Cond() {
}

SLAKE_API bool Cond::init() {
}

SLAKE_API void Cond::wait() {
	assert(isValid);
	pthread_cond_wait(&nativeHandle, &internalMutex.nativeHandle);
}

SLAKE_API void Cond::notify() {
	assert(isValid);
	pthread_cond_signal(&nativeHandle);
}
