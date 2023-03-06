#include "value.h"

void Slake::Object::incRefCount() {
	_refCount++;
}

void Slake::Object::decRefCount() {
	_refCount--;
}

std::uint32_t Slake::Object::getRefCount() {
	return _refCount;
}
