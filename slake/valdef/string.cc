#include "string.h"

using namespace slake;

void slake::StringObject::_setData(const char *str, size_t size) {
	reportSizeFreedToRuntime(data.size());

	if (!size)
		data.clear();
	else
		data = std::string(str, size);

	reportSizeAllocatedToRuntime(size);
}

slake::StringObject::StringObject(Runtime *rt, const char *s, size_t size) : Object(rt) {
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(Object));
	_setData(s, size);
}

slake::StringObject::StringObject(Runtime *rt, std::string &&s) : Object(rt) {
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(Object));
	data = std::move(s);
	reportSizeAllocatedToRuntime(data.size());
}

StringObject::~StringObject() {
	_setData(nullptr, 0);
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(Object));
}

Object *StringObject::duplicate() const {
	StringObject *v = new StringObject(_rt, nullptr, 0);
	*v = *this;

	return (Object *)v;
}
