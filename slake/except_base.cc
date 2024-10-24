#include <slake/runtime.h>

using namespace slake;

SLAKE_API InternalException::InternalException(
	Runtime *associatedRuntime,
	ErrorKind kind)
	: associatedRuntime(associatedRuntime),
	  kind(kind) {}
SLAKE_API InternalException::~InternalException() {}
