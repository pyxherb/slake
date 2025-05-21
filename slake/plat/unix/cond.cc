#include <slake/runtime.h>

using namespace slake;

SLAKE_API Cond::Cond() {
	nativeHandle = PTHREAD_COND_INITIALIZER;
}
SLAKE_API Cond::~Cond() {
}

SLAKE_API void Cond::wait() {
	pthread_cond_wait(&nativeHandle, &internalMutex.nativeHandle);
}

SLAKE_API void Cond::notify() {
	pthread_cond_signal(&nativeHandle);
}
