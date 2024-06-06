#include <slake/runtime.h>

using namespace slake;

slake::NotFoundError::NotFoundError(std::string msg, IdRefObject *ref)
	: RuntimeExecError(msg), ref(ref) {
}
