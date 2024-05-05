#include "array.h"

using namespace slake;

ArrayValue::ArrayValue(Runtime *rt, Type type)
	: Value(rt), type(type) {
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(Value));
}

ArrayValue::~ArrayValue() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(Value));
}

Value *ArrayValue::duplicate() const {
	ArrayValue *v = new ArrayValue(_rt, type);

	*v = *this;

	return (Value *)v;
}
