#include <slake/runtime.h>

using namespace slake;

bool VarRef::operator<(const VarRef &rhs) const {
	if (varPtr < rhs.varPtr)
		return true;
	if (varPtr > rhs.varPtr)
		return false;

	if (!varPtr)
		return false;

	switch (varPtr->getVarKind()) {
		case VarKind::ArrayElement:
			if (context.asArray.index < rhs.context.asArray.index)
				return true;
			break;
		case VarKind::Regular:
			break;
	}

	return false;
}

Value::Value(const Type &type) {
	*((Type *)data.asType) = type;
	valueType = ValueType::TypeName;
}

void Value::_setObjectRef(Object* objectPtr, bool isHostRef) {
	if (isHostRef) {
		if (objectPtr)
			++objectPtr->hostRefCount;
	}
	this->data.asObjectRef = ObjectRefValueExData{ objectPtr, isHostRef };
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
		case ValueType::VarRef:
			break;
		case ValueType::ObjectRef: {
			ObjectRefValueExData &exData = data.asObjectRef;

			if (exData.isHostRef) {
				if (exData.objectPtr)
					--exData.objectPtr->hostRefCount;
			}
			break;
		}
		case ValueType::TypeName:
			*((Type *)data.asType) = {};
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
	return *((Type *)data.asType);
}

const Type &Value::getTypeName() const {
	return ((Value *)this)->getTypeName();
}

const ObjectRefValueExData &Value::getObjectRef() const {
	return data.asObjectRef;
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
		case ValueType::VarRef:
			this->data = other.data;
			break;
		case ValueType::ObjectRef:
			this->data = other.data;
			this->data.asObjectRef.isHostRef = false;
			break;
		case ValueType::TypeName:
			*((Type *)data.asType) = other.getTypeName();
			break;
		case ValueType::Undefined:
			break;
		default:
			throw std::logic_error("Unhandled object type");
	}
	valueType = other.valueType;

	return *this;
}

bool Value::operator==(const Value &rhs) const {
	if (valueType != rhs.valueType)
		return false;

	switch (valueType) {
		case ValueType::I8:
			return data.asI8 == rhs.data.asI8;
		case ValueType::I16:
			return data.asI16 == rhs.data.asI16;
		case ValueType::I32:
			return data.asI32 == rhs.data.asI32;
		case ValueType::I64:
			return data.asI64 == rhs.data.asI64;
		case ValueType::U8:
			return data.asU8 == rhs.data.asU8;
		case ValueType::U16:
			return data.asU16 == rhs.data.asU16;
		case ValueType::U32:
			return data.asU32 == rhs.data.asU32;
		case ValueType::U64:
			return data.asU64 == rhs.data.asU64;
		case ValueType::Bool:
			return data.asBool == rhs.data.asBool;
		case ValueType::ObjectRef: {
			if (data.asObjectRef.isHostRef != rhs.data.asObjectRef.isHostRef)
				return false;
			return data.asObjectRef.objectPtr == data.asObjectRef.objectPtr;
		}
		case ValueType::RegRef:
			return data.asU32 == rhs.data.asU32;
		case ValueType::TypeName:
			return getTypeName() == rhs.getTypeName();
		case ValueType::Undefined:
			return true;
	}
}

bool Value::operator<(const Value &rhs) const {
	if (valueType < rhs.valueType)
		return true;
	if (valueType > rhs.valueType)
		return false;

	switch (valueType) {
		case ValueType::I8:
			return data.asI8 < rhs.data.asI8;
		case ValueType::I16:
			return data.asI16 < rhs.data.asI16;
		case ValueType::I32:
			return data.asI32 < rhs.data.asI32;
		case ValueType::I64:
			return data.asI64 < rhs.data.asI64;
		case ValueType::U8:
			return data.asU8 < rhs.data.asU8;
		case ValueType::U16:
			return data.asU16 < rhs.data.asU16;
		case ValueType::U32:
			return data.asU32 < rhs.data.asU32;
		case ValueType::U64:
			return data.asU64 < rhs.data.asU64;
		case ValueType::Bool:
			return data.asBool < rhs.data.asBool;
		case ValueType::ObjectRef:
			return data.asObjectRef.objectPtr < rhs.data.asObjectRef.objectPtr;
		case ValueType::RegRef:
			return data.asU32 < rhs.data.asU32;
		case ValueType::TypeName:
			return getTypeName() < rhs.getTypeName();
		case ValueType::VarRef:
			return data.asVarRef < rhs.data.asVarRef;
		case ValueType::Undefined:
			return false;
	}
}
