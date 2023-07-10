#include <slake/runtime.h>

using namespace slake;

slake::NotFoundError::NotFoundError(std::string msg, ValueRef<RefValue> ref)
	: RuntimeExecError(msg), ref(ref) {
}
