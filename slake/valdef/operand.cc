#include "operand.h"

using namespace slake;

Value *LocalVarRefValue::duplicate() const {
	return new LocalVarRefValue(_rt, index);
}

Value *RegRefValue::duplicate() const {
	return new RegRefValue(_rt, reg);
}

Value *ArgRefValue::duplicate() const {
	return new ArgRefValue(_rt, index);
}
