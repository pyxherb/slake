#include <slake/runtime.h>

using namespace slake;

void Scope::putMember(const std::string &name, MemberObject *value) {
	members[name] = value;
	value->setParent(owner);
	value->setName(name.c_str());
}

void Scope::removeMember(const std::string &name) {
	if (auto it = members.find(name); it != members.end()) {
		it->second->setParent(nullptr);
		it->second->setName("");
		members.erase(it);
	}

	throw std::logic_error("No such member");
}

Scope *Scope::duplicate() {
	std::unique_ptr<Scope> newScope = std::make_unique<Scope>(owner, parent);

	for (auto i : members) {
		newScope->putMember(i.first, (MemberObject *)i.second->duplicate());
	}

	return newScope.release();
}
