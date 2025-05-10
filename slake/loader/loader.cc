#include "loader.h"

using namespace slake;
using namespace slake::loader;

SLAKE_API InternalExceptionPointer loader::loadModule(Runtime *runtime, Reader *reader, ModuleObject *&moduleObjectOut) noexcept {
	slxfmt::ImgHeader imh = { 0 };

	if (memcmp(imh.magic, slxfmt::IMH_MAGIC, sizeof(imh.magic))) {
		return allocOutOfMemoryErrorIfAllocFailed(BadMagicError::alloc(&runtime->globalHeapPoolAlloc));
	}

	return {};
}
