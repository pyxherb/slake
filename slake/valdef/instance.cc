#include "instance.h"

#include <slake/runtime.h>

using namespace slake;

Object* InstanceObject::duplicate() const {
	InstanceObject* v = new InstanceObject(_rt, _class);

	*v = *this;

	return (Object *)v;
}
