#include <slake/runtime.h>

using namespace slake;

Value::Value(Runtime *rt) : _rt(rt) {
	rt->_createdValues.insert(this);
	reportSizeToRuntime(sizeof(*this));
}

Value::~Value() {
	_rt->_createdValues.erase(this);
}

MemberValue *Value::getMember(std::string name) {
	return nullptr;
}
const MemberValue *Value::getMember(std::string name) const {
	return nullptr;
}

ValueRef<> Value::call(uint8_t nArgs, ValueRef<> *args) const {
	return nullptr;
}

Value *Value::copy() const {
	throw std::logic_error("Not implemented yet");
}

void Value::onRefZero() {
	delete this;
}

void Value::reportSizeToRuntime(long size) {
	_rt->_szMemInUse += size;
}
