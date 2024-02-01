#include "operand.h"

using namespace slake;

slake::LocalVarRefValue::LocalVarRefValue(Runtime *rt, int32_t index, bool unwrapValue)
	: Value(rt), index(index), unwrapValue(unwrapValue) {
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(Value));
}

LocalVarRefValue::~LocalVarRefValue() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(Value));
}

Value *LocalVarRefValue::duplicate() const {
	return new LocalVarRefValue(_rt, index, unwrapValue);
}

RegRefValue::~RegRefValue() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(Value));
}

Value *RegRefValue::duplicate() const {
	return new RegRefValue(_rt, index, unwrapValue);
}

ArgRefValue::~ArgRefValue() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(Value));
}

Value *ArgRefValue::duplicate() const {
	return new ArgRefValue(_rt, index, unwrapValue);
}
