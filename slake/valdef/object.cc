#include "object.h"

#include <slake/runtime.h>

using namespace slake;

void ObjectValue::onRefZero() {
	if (getMember("delete"))
		_rt->_extraGcTargets.insert(this);
	else
		delete this;
}
