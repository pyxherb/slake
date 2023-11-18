#include "object.h"

#include <slake/runtime.h>

using namespace slake;

void ObjectValue::addMember(std::string name, MemberValue *value) {
	if (_members.count(name)) {
		_members.at(name)->unbind();
		_members.at(name)->decRefCount();
	}
	_members[name] = value;
	value->incRefCount();
	value->bind(this, name);
}

MemberValue *ObjectValue::getMember(std::string name) {
	if (_members.count(name))
		return _members.at(name);
	return _parent ? _parent->getMember(name) : nullptr;
}

const MemberValue *ObjectValue::getMember(std::string name) const {
	return ((ObjectValue *)this)->getMember(name);
}

void ObjectValue::onRefZero() {
	if (getMember("delete"))
		_rt->_extraGcTargets.insert(this);
	else
		delete this;
}

Value* ObjectValue::duplicate() const {
	ObjectValue* v = new ObjectValue(_rt, _class);

	*v = *this;

	return (Value *)v;
}
