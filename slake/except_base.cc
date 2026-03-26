#include <slake/runtime.h>

using namespace slake;

SLAKE_API InternalException::InternalException(
	peff::Alloc *self_allocator,
	ErrorKind kind)
	: self_allocator(self_allocator),
	  kind(kind) {}
SLAKE_API InternalException::~InternalException() {}
