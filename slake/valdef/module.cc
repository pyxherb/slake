#include <slake/runtime.h>

using namespace slake;

ModuleValue::ModuleValue(Runtime *rt, AccessModifier access)
	: MemberValue(rt, access) {
	reportSizeToRuntime(sizeof(*this) - sizeof(MemberValue));
}

ModuleValue::~ModuleValue() {
	if ((!refCount) && !(_rt->_flags & _RT_DELETING)) {
		for (auto &i : _members) {
			i.second->unbind();
			i.second->decRefCount();
		}
	}
}

Type ModuleValue::getType() const {
	return TypeId::MOD;
}

void ModuleValue::addMember(std::string name, MemberValue *value) {
	if (_members.count(name)) {
		_members.at(name)->unbind();
		_members.at(name)->decRefCount();
	}
	_members[name] = value;
	value->incRefCount();
	value->bind(this, name);
}

MemberValue *ModuleValue::getMember(std::string name) {
	return _members.count(name) ? _members.at(name) : nullptr;
}

const MemberValue *ModuleValue::getMember(std::string name) const {
	return _members.count(name) ? _members.at(name) : nullptr;
}

Value *ModuleValue::duplicate() const {
	ModuleValue* v = new ModuleValue(_rt, getAccess());

	*v = *this;

	return (Value *)v;
}
