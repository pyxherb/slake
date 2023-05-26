#include "value.h"

#include "runtime.h"

using namespace Slake;

Type::Type(RefValue *ref) : valueType(ValueType::OBJECT) {
	exData.deferred = ref;
}

Type::~Type() {
	switch (valueType) {
		case ValueType::ARRAY:
			if (exData.array)
				delete exData.array;
			break;
		case ValueType::OBJECT:
			if (exData.deferred)
				break;
			break;
		case ValueType::MAP:
			if (exData.map.k)
				delete exData.map.k;
			if (exData.map.v)
				delete exData.map.v;
			break;
	}
}


bool Type::isDeferred() noexcept {
	return valueType == ValueType::OBJECT &&
		   ((Value *)exData.customType)->getType() == ValueType::REF;
}

Value::Value(Runtime *rt) : _rt(rt) {
	rt->_createdValues.insert(this);
}

Value::~Value() {
	_rt->_createdValues.erase(this);
}

ValueRef<> Slake::FnValue::call(uint8_t nArgs, ValueRef<> *args) {
	std::shared_ptr<Context> context = std::make_unique<Context>(ExecContext(_parent, this, 0));
	getRuntime()->threadCurrentContexts[std::this_thread::get_id()] = context;

	context->frames.push_back(Frame(0, UINT32_MAX - 1));
	context->callingStack.push_back(ExecContext(_parent, this, UINT32_MAX - 1));
	while (context->execContext.curIns != UINT32_MAX) {
		if (context->execContext.curIns >= _nIns)
			throw std::runtime_error("Out of function body");
		getRuntime()->_execIns(context.get(), context->execContext.fn->_body[context->execContext.curIns]);
	}

	getRuntime()->threadCurrentContexts.erase(std::this_thread::get_id());
	return context->retValue;
}

std::string Slake::FnValue::toString() const {
	std::string s = Value::toString() + ",\"instructions\":[";

	for (size_t i = 0; i < _nIns; i++) {
		s += (i ? "," : "") + std::to_string(_body[i]);
	}

	s += "]";

	return s;
}

void Slake::Value::reportSizeToRuntime(long size) {
	_rt->szInUse += size;
}
