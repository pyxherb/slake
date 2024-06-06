#include <slake/runtime.h>
#include <slake/type.h>

using namespace slake;

MemberObject::MemberObject(Runtime *rt, AccessModifier access)
	: Object(rt), AccessModified(access) {
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(Object));
}

MemberObject::~MemberObject() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(Object));
}

std::string MemberObject::getName() const {
	return _name;
}

const Object *MemberObject::getParent() const { return _parent; }
Object *MemberObject::getParent() { return _parent; }

void MemberObject::bind(Object *parent, std::string name) {
	_parent = parent, _name = name;
}

void MemberObject::unbind() {
	if (!_parent)
		throw std::logic_error("Unbinding an unbound member value");
	_parent = nullptr;
	_name.clear();
}

void Scope::_getMemberChain(const std::string &name, std::deque<std::pair<Scope *, MemberObject *>> &membersOut) {
	if (auto m = getMember(name); m)
		membersOut.push_back({ this, m });

	if (parent)
		parent->_getMemberChain(name, membersOut);
}
