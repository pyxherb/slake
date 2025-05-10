#include <slake/runtime.h>

using namespace slake;

SLAKE_API InternalException::InternalException(
	peff::Alloc *selfAllocator,
	ErrorKind kind)
	: selfAllocator(selfAllocator),
	  kind(kind) {}
SLAKE_API InternalException::~InternalException() {}
