#include "array.h"

using namespace slake;

ArrayValue::ArrayValue(Runtime *rt, Type type)
	: Value(rt), type(type) {
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(Value));
}

ArrayValue::~ArrayValue() {
	for (auto i : values)
		delete i.get();
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(Value));
}
