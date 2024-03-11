#include "object.h"

#include <slake/runtime.h>

using namespace slake;

Value* ObjectValue::duplicate() const {
	ObjectValue* v = new ObjectValue(_rt, _class);

	*v = *this;

	return (Value *)v;
}
