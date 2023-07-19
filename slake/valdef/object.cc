#include "object.h"

#include <slake/runtime.h>

using namespace slake;

void ObjectValue::onRefZero() {
	if (getMember("delete"))
		_rt->_extraGcTargets.insert(this);
	else
		delete this;
}

Value* ObjectValue::duplicate() const {
	ObjectValue* v = new ObjectValue(_rt, _class);

	*v = *this;

	return (Value *)v;
}
