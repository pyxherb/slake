#include <slake/runtime.h>

using namespace Slake;

Value::Value(Runtime *rt) : _rt(rt) {
	rt->_createdValues.insert(this);
}

Value::~Value() {
	_rt->_createdValues.erase(this);
}

void Slake::Value::reportSizeToRuntime(long size) {
	_rt->_szMemInUse += size;
}
