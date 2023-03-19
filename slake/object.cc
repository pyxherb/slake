#include "object.h"

void Slake::Object::incRefCount() {
	_refCount++;
}

void Slake::Object::decRefCount() {
	if (!--_refCount)
		delete this;
}

std::uint32_t Slake::Object::getRefCount() {
	return _refCount;
}

Slake::RefObject::RefObject(std::string name, RefObject* next) : name(name), next(next) {
}

Slake::RefObject::~RefObject() {
	delete next;
}
