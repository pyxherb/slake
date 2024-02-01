#include <slake/runtime.h>

using namespace slake;

bool slake::_isRuntimeInDestruction(Runtime* runtime) {
	return runtime->_flags & _RT_DELETING;
}

Value::Value(Runtime *rt) : _rt(rt) {
	rt->_createdValues.insert(this);
	reportSizeAllocatedToRuntime(sizeof(*this));
}

Value::~Value() {
	_rt->invalidateGenericCache(this);
	_rt->_createdValues.erase(this);
	reportSizeFreedToRuntime(sizeof(*this));
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

void Value::reportSizeAllocatedToRuntime(size_t size) {
	_rt->_szMemInUse += size;
}

void Value::reportSizeFreedToRuntime(size_t size) {
	assert(_rt->_szMemInUse >= size);
	_rt->_szMemInUse -= size;
}
