#include <slake/runtime.h>

using namespace slake;

Object::Object(Runtime *rt) : _rt(rt) {
	rt->createdObjects.insert(this);
}

Object::~Object() {
	if (scope) {
		if (!(_flags & VF_ALIAS))
			delete scope;
	}
	_rt->invalidateGenericCache(this);
	if (!(_rt->_flags & _RT_INGC))
		_rt->createdObjects.erase(this);
}

Object *Object::duplicate() const {
	throw std::logic_error("duplicate method was not implemented by the object class");
}

MemberObject *slake::Object::getMember(const std::string &name) {
	return scope ? scope->getMember(name) : nullptr;
}

std::deque<std::pair<Scope *, MemberObject *>> slake::Object::getMemberChain(const std::string &name) {
	return scope ? scope->getMemberChain(name) : std::deque<std::pair<Scope *, MemberObject *>>();
}

Object &slake::Object::operator=(const Object &x) {
	if (scope) {
		if (!(_flags & VF_ALIAS))
			delete scope;
	}

	_rt = x._rt;
	_flags = x._flags & ~VF_WALKED;
	scope = x.scope ? x.scope->duplicate() : nullptr;

	return *this;
}
