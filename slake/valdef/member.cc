#include <slake/runtime.h>
#include <slake/type.h>

using namespace slake;

MemberValue::MemberValue(Runtime *rt, AccessModifier access)
	: Value(rt), AccessModified(access) {
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(Value));
}

MemberValue::~MemberValue() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(Value));
}

std::string MemberValue::getName() const {
	return _name;
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

void Scope::_getMemberChain(const std::string &name, std::deque<std::pair<Scope *, MemberValue *>> &membersOut) {
	if (auto m = getMember(name); m)
		membersOut.push_back({ this, m });

	if (parent)
		parent->_getMemberChain(name, membersOut);
}
