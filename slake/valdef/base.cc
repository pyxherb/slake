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
	reportSizeFreedToRuntime(sizeof(*this));
}

ValueRef<> Value::call(std::deque<Value *> args) const {
	return nullptr;
}

Value *Value::duplicate() const {
	throw std::logic_error("duplicate method was not implemented by the value class");
}

void Value::reportSizeAllocatedToRuntime(size_t size) {
	_rt->_szMemInUse += size;
}

void Value::reportSizeFreedToRuntime(size_t size) {
	assert(_rt->_szMemInUse >= size);
	_rt->_szMemInUse -= size;
}
