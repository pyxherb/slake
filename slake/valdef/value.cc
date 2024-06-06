#include <slake/runtime.h>

using namespace slake;

bool slake::_isRuntimeInDestruction(Runtime *runtime) {
	return runtime->_flags & _RT_DELETING;
}

Value::Value(Runtime *rt) : _rt(rt) {
	rt->createdValues.insert(this);
	reportSizeAllocatedToRuntime(sizeof(*this));
}

Value::~Value() {
	if (scope) {
		if (!(_flags & VF_ALIAS))
			delete scope;
	}
	_rt->invalidateGenericCache(this);
	reportSizeFreedToRuntime(sizeof(*this));
	if (!(_rt->_flags & _RT_INGC))
		_rt->createdValues.erase(this);
}

Value *Value::duplicate() const {
	throw std::logic_error("duplicate method was not implemented by the value class");
}

MemberValue *slake::Value::getMember(const std::string &name) {
	return scope ? scope->getMember(name) : nullptr;
}

std::deque<std::pair<Scope *, MemberValue *>> slake::Value::getMemberChain(const std::string &name) {
	return scope ? scope->getMemberChain(name) : std::deque<std::pair<Scope *, MemberValue *>>();
}

Value &slake::Value::operator=(const Value &x) {
	if (scope) {
		if (!(_flags & VF_ALIAS))
			delete scope;
	}

	_rt = x._rt;
	_flags = x._flags & ~VF_WALKED;
	scope = x.scope ? x.scope->duplicate() : nullptr;

	return *this;
}

void Value::reportSizeAllocatedToRuntime(size_t size) {
	_rt->_szMemInUse += size;
}

void Value::reportSizeFreedToRuntime(size_t size) {
	assert(_rt->_szMemInUse >= size);
	_rt->_szMemInUse -= size;
}
