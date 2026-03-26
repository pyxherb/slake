#include "var.h"
#include <slake/runtime.h>

using namespace slake;

InternalExceptionPointer slake::raise_mismatched_var_type_error(Runtime *rt) {
	return alloc_out_of_memory_error_if_alloc_failed(MismatchedVarTypeError::alloc(rt->get_fixed_alloc()));
}
