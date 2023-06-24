#include "object.h"

#include <slake/runtime.h>

using namespace Slake;

void ObjectValue::whenRefBecomeZero() {
	if (getMember("delete"))
		_rt->_extraGcTargets.insert(this);
	else
		delete this;
}
