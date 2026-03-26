#include <slake/runtime.h>

using namespace slake;

SLAKE_API Cond::Cond() {
	native_handle = PTHREAD_COND_INITIALIZER;
}
SLAKE_API Cond::~Cond() {
}

SLAKE_API void Cond::wait() {
	pthread_cond_wait(&native_handle, &internal_mutex.native_handle);
}

SLAKE_API void Cond::notify() {
	pthread_cond_signal(&native_handle);
}

SLAKE_API void Cond::notify_all() {
	pthread_cond_broadcast(&native_handle);
}