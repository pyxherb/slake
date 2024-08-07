#include <slake/runtime.h>

using namespace slake;

Value::Value(const Type &type) {
	this->data = new Type(type);
	valueType = ValueType::TypeName;
}

void Value::_setObjectRef(Object* objectPtr, bool isHostRef) {
	if (isHostRef) {
		if (objectPtr)
			++objectPtr->hostRefCount;
	}
	this->data = ObjectRefValueExData{ objectPtr, isHostRef };
}

void Value::_reset() {
	auto savedData = data;
	switch (valueType) {
		case ValueType::U8:
		case ValueType::U16:
		case ValueType::U32:
		case ValueType::U64:
		case ValueType::I8:
		case ValueType::I16:
		case ValueType::I32:
		case ValueType::I64:
		case ValueType::F32:
		case ValueType::F64:
		case ValueType::Bool:
		case ValueType::RegRef:
			break;
		case ValueType::ObjectRef: {
			ObjectRefValueExData &exData = std::get<ObjectRefValueExData>(data);

			if (exData.isHostRef) {
				if (exData.objectPtr)
					--exData.objectPtr->hostRefCount;
			}
			break;
		}
		case ValueType::TypeName:
			if (auto t = std::get<Type *>(data); t)
				delete t;
			data = {};
			break;
		case ValueType::Undefined:
			break;
		default:
			throw std::logic_error("Unhandled object type");
	}
	data = {};

	valueType = ValueType::Undefined;
}

Value::~Value() {
	_reset();
}

Type &Value::getTypeName() {
	return *std::get<Type *>(data);
}

const Type &Value::getTypeName() const {
	return ((Value *)this)->getTypeName();
}

const ObjectRefValueExData &Value::getObjectRef() const {
	return std::get<ObjectRefValueExData>(data);
}

Value &Value::operator=(const Value &other) {
	_reset();

	switch (other.valueType) {
		case ValueType::U8:
		case ValueType::U16:
		case ValueType::U32:
		case ValueType::U64:
		case ValueType::I8:
		case ValueType::I16:
		case ValueType::I32:
		case ValueType::I64:
		case ValueType::F32:
		case ValueType::F64:
		case ValueType::Bool:
		case ValueType::RegRef:
			this->data = other.data;
			break;
		case ValueType::ObjectRef:
			this->data = other.data;
			std::get<ObjectRefValueExData>(this->data).isHostRef = false;
			break;
		case ValueType::TypeName:
			this->data = new Type(other.getTypeName());
			break;
		case ValueType::Undefined:
			break;
		default:
			throw std::logic_error("Unhandled object type");
	}
	valueType = other.valueType;

	return *this;
}
