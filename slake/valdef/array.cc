#include "array.h"

using namespace slake;

ArrayObject::ArrayObject(Runtime *rt, Type type)
	: Object(rt), type(type) {
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(Object));
}

ArrayObject::~ArrayObject() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(Object));
}

Object *ArrayObject::duplicate() const {
	ArrayObject *v = new ArrayObject(_rt, type);

	*v = *this;

	return (Object *)v;
}
