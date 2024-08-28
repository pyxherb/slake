#include <slake/runtime.h>

using namespace slake;

Object::Object(Runtime *rt) : _rt(rt) {
}

Object::~Object() {
	if (scope) {
		if (!(_flags & VF_ALIAS))
			delete scope;
	}
	_rt->invalidateGenericCache(this);
}

Object *Object::duplicate() const {
	throw std::logic_error("duplicate method was not implemented by the object class");
}

MemberObject *slake::Object::getMember(const std::string &name) {
	if (methodTable) {
		if (auto m = methodTable->getMethod(name); m)
			return m;
	}

	if (scope) {
		if (auto m = scope->getMember(name); m)
			return m;
	}

	return nullptr;
}

std::deque<std::pair<Scope *, MemberObject *>> slake::Object::getMemberChain(const std::string &name) {
	return scope ? scope->getMemberChain(name) : std::deque<std::pair<Scope *, MemberObject *>>();
}

