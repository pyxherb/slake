#include <slake/runtime.h>

using namespace slake;

Object::Object(Runtime *rt) : _rt(rt) {
}

Object::~Object() {
	_rt->invalidateGenericCache(this);
}

Object *Object::duplicate() const {
	throw std::logic_error("duplicate() method is not supported");
}

MemberObject *Object::getMember(
	const std::string &name,
	VarRefContext *varRefContextOut) const {
	return nullptr;
}

