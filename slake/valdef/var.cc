#include "var.h"
#include <slake/runtime.h>

using namespace slake;

Value* VarValue::duplicate() const {
	VarValue* v = new VarValue(_rt, 0, type);

	*v = *this;

	return (Value *)v;
}
