#include <slake/runtime.h>

using namespace slake;

Value::Value(Runtime *rt) : _rt(rt) {
	rt->_createdValues.insert(this);
	reportSizeToRuntime(sizeof(*this));
}

Value::~Value() {
	_rt->invalidateGenericCache(this);
	_rt->_createdValues.erase(this);
}

MemberValue *Value::getMember(std::string name) {
	return nullptr;
}
const MemberValue *Value::getMember(std::string name) const {
	return nullptr;
}

ValueRef<> Value::call(std::deque<ValueRef<>> args) const {
	return nullptr;
}

Value *Value::duplicate() const {
	throw std::logic_error("duplicate method was not implemented by the value class");
}

void Value::onRefZero() {
	delete this;
}

void Value::reportSizeToRuntime(long size) {
	_rt->_szMemInUse += size;
}
