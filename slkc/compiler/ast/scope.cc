#include "scope.h"
#include "member.h"

using namespace slake::slkc;

Scope *Scope::duplicate() {
	unique_ptr<Scope> newScope = make_unique<Scope>();

	newScope->owner = owner;
	newScope->parent = parent;

	for (const auto &i : members) {
		newScope->members[i.first] = i.second->duplicate<MemberNode>();
	}

	return newScope.release();
}
