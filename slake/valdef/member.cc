#include <slake/runtime.h>
#include <slake/type.h>

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

std::string MemberValue::getName() const {
	auto s = _name;
	if (_genericArgs.size()) {
		s += "<";
		for (size_t i = 0; i < _genericArgs.size(); ++i) {
			if (i)
				s += ", ";
			s += std::to_string(_genericArgs[i], _rt);
		}
		s += ">";
	}
	return s;
}

const Value *MemberValue::getParent() const { return _parent; }
Value *MemberValue::getParent() { return _parent; }

void MemberValue::bind(Value *parent, std::string name) {
	_parent = parent, _name = name;
}

void MemberValue::unbind() {
	if (!_parent)
		throw std::logic_error("Unbinding an unbound member value");
	_parent = nullptr;
	_name.clear();
}
