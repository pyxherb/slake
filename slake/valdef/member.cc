#include <slake/runtime.h>

using namespace slake;

MemberValue::MemberValue(Runtime *rt, AccessModifier access)
	: Value(rt), AccessModified(access) {
	reportSizeToRuntime(sizeof(*this) - sizeof(Value));
	if (_parent)
		_parent->incRefCount();
}

MemberValue::~MemberValue() {
	if ((!getRefCount()) && !(_rt->_flags & _RT_DELETING) && _parent)
		_parent->decRefCount();
}

std::string MemberValue::getName() const { return _name; }

const Value *MemberValue::getParent() const { return _parent; }
Value *MemberValue::getParent() { return _parent; }

void MemberValue::bind(Value *parent, std::string name) {
	_parent = parent, _name = name;
}

void MemberValue::unbind() {
	if(!_parent)
		throw std::logic_error("Unbinding with unbound member value");
	_parent = nullptr;
	_name.clear();
}
