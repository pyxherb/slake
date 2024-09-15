#include <slake/runtime.h>
#include <algorithm>

using namespace slake;

ArrayAccessorVarObject::ArrayAccessorVarObject(
	Runtime *rt,
	const Type &elementType,
	ArrayObject *arrayObject)
	: VarObject(rt), arrayObject(arrayObject) {
}

ArrayAccessorVarObject::ArrayAccessorVarObject(const ArrayAccessorVarObject &other) : VarObject(other) {
	arrayObject = other.arrayObject;
}

ArrayAccessorVarObject::~ArrayAccessorVarObject() {}

Type ArrayAccessorVarObject::getVarType(const VarRefContext &context) const {
	return Type::makeArrayTypeName(_rt, elementType);
}

ArrayObject::ArrayObject(Runtime *rt, const Type &elementType, ArrayAccessorVarObject *accessor)
	: Object(rt),
	  elementType(elementType),
	  accessor(accessor) {
}

ArrayObject::ArrayObject(const ArrayObject &x) : Object(x) {
	length = x.length;
	elementType = x.elementType;
	accessor = x.accessor;
}

ArrayObject::~ArrayObject() {
}

ObjectKind ArrayObject::getKind() const { return ObjectKind::Array; }

//
// U8ArrayObject
//

U8ArrayAccessorVarObject::U8ArrayAccessorVarObject(Runtime *rt, U8ArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, Type(ValueType::U8), arrayObject) {
}

U8ArrayAccessorVarObject::~U8ArrayAccessorVarObject() {}

void U8ArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
	if (value.valueType != ValueType::U8)
		throw MismatchedTypeError("Mismatched array element type");

	U8ArrayObject *arrayObject = ((U8ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	arrayObject->data[index] = value.getU8();
}

Value U8ArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	U8ArrayObject *arrayObject = ((U8ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

HostObjectRef<U8ArrayAccessorVarObject> slake::U8ArrayAccessorVarObject::alloc(Runtime *rt, U8ArrayObject *arrayObject) {
	std::pmr::polymorphic_allocator<U8ArrayAccessorVarObject> allocator(&rt->globalHeapPoolResource);

	U8ArrayAccessorVarObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, arrayObject);

	rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::U8ArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<U8ArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

void U8ArrayObject::_resizeUnchecked(size_t newLength) {
	uint8_t *newData = (uint8_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(uint8_t) * newLength);

	data = newData;
	length = newLength;
}

void U8ArrayObject::clear() {
	if (data) {
		_rt->globalHeapPoolResource.deallocate(
			data,
			length * sizeof(uint8_t));
		data = nullptr;
		length = 0;
	}
}

void U8ArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	if (value.valueType != ValueType::U8)
		throw MismatchedTypeError("Mismatched array element type");
	memset(data + beginIndex, value.getU8(), length * sizeof(uint8_t));
}

void U8ArrayObject::resize(size_t newLength) {
	uint8_t *newData = (uint8_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(uint8_t) * newLength);

	if (length < newLength) {
		memcpy(newData, data, length * sizeof(uint8_t));
		memset(newData + newLength, 0, (newLength - length) * sizeof(uint8_t));
	} else {
		memcpy(newData, data, newLength * sizeof(uint8_t));
	}

	clear();

	data = newData;
	length = newLength;
}

U8ArrayObject::U8ArrayObject(Runtime *rt, size_t length)
	: ArrayObject(rt, Type(ValueType::U8), U8ArrayAccessorVarObject::alloc(rt, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(uint8_t));
}

U8ArrayObject::U8ArrayObject(const U8ArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(int8_t));
}

U8ArrayObject::~U8ArrayObject() {
	clear();
}

Object *U8ArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

HostObjectRef<U8ArrayObject> slake::U8ArrayObject::alloc(Runtime *rt, size_t length) {
	std::pmr::polymorphic_allocator<U8ArrayObject> allocator(&rt->globalHeapPoolResource);

	U8ArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, length);

	rt->createdObjects.insert(ptr);

	return ptr;
}

HostObjectRef<U8ArrayObject> slake::U8ArrayObject::alloc(const U8ArrayObject *other) {
	std::pmr::polymorphic_allocator<U8ArrayObject> allocator(&other->_rt->globalHeapPoolResource);

	U8ArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, *other);

	other->_rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::U8ArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<U8ArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

//
// U16ArrayObject
//

U16ArrayAccessorVarObject::U16ArrayAccessorVarObject(Runtime *rt, U16ArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, Type(ValueType::U16), arrayObject) {
}

U16ArrayAccessorVarObject::~U16ArrayAccessorVarObject() {}

void U16ArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
	if (value.valueType != ValueType::U16)
		throw MismatchedTypeError("Mismatched array element type");

	U16ArrayObject *arrayObject = ((U16ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	arrayObject->data[index] = value.getU16();
}

Value U16ArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	U16ArrayObject *arrayObject = ((U16ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

HostObjectRef<U16ArrayAccessorVarObject> slake::U16ArrayAccessorVarObject::alloc(Runtime *rt, U16ArrayObject *arrayObject) {
	std::pmr::polymorphic_allocator<U16ArrayAccessorVarObject> allocator(&rt->globalHeapPoolResource);

	U16ArrayAccessorVarObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, arrayObject);

	rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::U16ArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<U16ArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

void U16ArrayObject::_resizeUnchecked(size_t newLength) {
	uint16_t *newData = (uint16_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(uint16_t) * newLength);

	data = newData;
	length = newLength;
}

void U16ArrayObject::clear() {
	if (data) {
		_rt->globalHeapPoolResource.deallocate(
			data,
			length * sizeof(uint16_t));
		data = nullptr;
		length = 0;
	}
}

void U16ArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	if (value.valueType != ValueType::U16)
		throw MismatchedTypeError("Mismatched array element type");
	std::fill_n(data + beginIndex, length, value.getU16());
}

void U16ArrayObject::resize(size_t newLength) {
	uint16_t *newData = (uint16_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(uint16_t) * newLength);

	if (length < newLength) {
		memcpy(newData, data, length * sizeof(uint16_t));
		memset(newData + newLength, 0, (newLength - length) * sizeof(uint16_t));
	} else {
		memcpy(newData, data, newLength * sizeof(uint16_t));
	}

	clear();

	data = newData;
	length = newLength;
}

U16ArrayObject::U16ArrayObject(Runtime *rt, size_t length)
	: ArrayObject(rt, Type(ValueType::U16), U16ArrayAccessorVarObject::alloc(rt, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(uint16_t));
}

U16ArrayObject::U16ArrayObject(const U16ArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(uint16_t));
}

U16ArrayObject::~U16ArrayObject() {
	clear();
}

Object *U16ArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

HostObjectRef<U16ArrayObject> slake::U16ArrayObject::alloc(Runtime *rt, size_t length) {
	std::pmr::polymorphic_allocator<U16ArrayObject> allocator(&rt->globalHeapPoolResource);

	U16ArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, length);

	rt->createdObjects.insert(ptr);

	return ptr;
}

HostObjectRef<U16ArrayObject> slake::U16ArrayObject::alloc(const U16ArrayObject *other) {
	std::pmr::polymorphic_allocator<U16ArrayObject> allocator(&other->_rt->globalHeapPoolResource);

	U16ArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, *other);

	other->_rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::U16ArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<U16ArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

//
// U32ArrayObject
//

U32ArrayAccessorVarObject::U32ArrayAccessorVarObject(Runtime *rt, U32ArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, Type(ValueType::U32), arrayObject) {
}

U32ArrayAccessorVarObject::~U32ArrayAccessorVarObject() {}

void U32ArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
	if (value.valueType != ValueType::U32)
		throw MismatchedTypeError("Mismatched array element type");

	U32ArrayObject *arrayObject = ((U32ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	arrayObject->data[index] = value.getU32();
}

Value U32ArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	U32ArrayObject *arrayObject = ((U32ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

HostObjectRef<U32ArrayAccessorVarObject> slake::U32ArrayAccessorVarObject::alloc(Runtime *rt, U32ArrayObject *arrayObject) {
	std::pmr::polymorphic_allocator<U32ArrayAccessorVarObject> allocator(&rt->globalHeapPoolResource);

	U32ArrayAccessorVarObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, arrayObject);

	rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::U32ArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<U32ArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

void U32ArrayObject::_resizeUnchecked(size_t newLength) {
	uint32_t *newData = (uint32_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(uint32_t) * newLength);

	data = newData;
	length = newLength;
}

void U32ArrayObject::clear() {
	if (data) {
		_rt->globalHeapPoolResource.deallocate(
			data,
			length * sizeof(uint32_t));
		data = nullptr;
		length = 0;
	}
}

void U32ArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	if (value.valueType != ValueType::U32)
		throw MismatchedTypeError("Mismatched array element type");
	std::fill_n(data + beginIndex, length, value.getU32());
}

void U32ArrayObject::resize(size_t newLength) {
	uint32_t *newData = (uint32_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(uint32_t) * newLength);

	if (length < newLength) {
		memcpy(newData, data, length * sizeof(uint32_t));
		memset(newData + newLength, 0, (newLength - length) * sizeof(uint32_t));
	} else {
		memcpy(newData, data, newLength * sizeof(uint32_t));
	}

	clear();

	data = newData;
	length = newLength;
}

U32ArrayObject::U32ArrayObject(Runtime *rt, size_t length)
	: ArrayObject(rt, Type(ValueType::U32), U32ArrayAccessorVarObject::alloc(rt, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(uint32_t));
}

U32ArrayObject::U32ArrayObject(const U32ArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(uint32_t));
}

U32ArrayObject::~U32ArrayObject() {
	clear();
}

Object *U32ArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

HostObjectRef<U32ArrayObject> slake::U32ArrayObject::alloc(Runtime *rt, size_t length) {
	std::pmr::polymorphic_allocator<U32ArrayObject> allocator(&rt->globalHeapPoolResource);

	U32ArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, length);

	rt->createdObjects.insert(ptr);

	return ptr;
}

HostObjectRef<U32ArrayObject> slake::U32ArrayObject::alloc(const U32ArrayObject *other) {
	std::pmr::polymorphic_allocator<U32ArrayObject> allocator(&other->_rt->globalHeapPoolResource);

	U32ArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, *other);

	other->_rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::U32ArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<U32ArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

//
// U64ArrayObject
//

U64ArrayAccessorVarObject::U64ArrayAccessorVarObject(Runtime *rt, U64ArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, Type(ValueType::U64), arrayObject) {
}

U64ArrayAccessorVarObject::~U64ArrayAccessorVarObject() {}

void U64ArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
	if (value.valueType != ValueType::U64)
		throw MismatchedTypeError("Mismatched array element type");

	U64ArrayObject *arrayObject = ((U64ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	arrayObject->data[index] = value.getU64();
}

Value U64ArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	U64ArrayObject *arrayObject = ((U64ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

HostObjectRef<U64ArrayAccessorVarObject> slake::U64ArrayAccessorVarObject::alloc(Runtime *rt, U64ArrayObject *arrayObject) {
	std::pmr::polymorphic_allocator<U64ArrayAccessorVarObject> allocator(&rt->globalHeapPoolResource);

	U64ArrayAccessorVarObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, arrayObject);

	rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::U64ArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<U64ArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

void U64ArrayObject::_resizeUnchecked(size_t newLength) {
	uint64_t *newData = (uint64_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(uint64_t) * newLength);

	data = newData;
	length = newLength;
}

void U64ArrayObject::clear() {
	if (data) {
		_rt->globalHeapPoolResource.deallocate(
			data,
			length * sizeof(uint64_t));
		data = nullptr;
		length = 0;
	}
}

void U64ArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	if (value.valueType != ValueType::U64)
		throw MismatchedTypeError("Mismatched array element type");
	std::fill_n(data + beginIndex, length, value.getU64());
}

void U64ArrayObject::resize(size_t newLength) {
	uint64_t *newData = (uint64_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(uint64_t) * newLength);

	if (length < newLength) {
		memcpy(newData, data, length * sizeof(uint64_t));
		memset(newData + newLength, 0, (newLength - length) * sizeof(uint64_t));
	} else {
		memcpy(newData, data, newLength * sizeof(uint64_t));
	}

	clear();

	data = newData;
	length = newLength;
}

U64ArrayObject::U64ArrayObject(Runtime *rt, size_t length)
	: ArrayObject(rt, Type(ValueType::U64), U64ArrayAccessorVarObject::alloc(rt, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(uint64_t));
}

U64ArrayObject::U64ArrayObject(const U64ArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(uint64_t));
}

U64ArrayObject::~U64ArrayObject() {
	clear();
}

Object *U64ArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

HostObjectRef<U64ArrayObject> slake::U64ArrayObject::alloc(Runtime *rt, size_t length) {
	std::pmr::polymorphic_allocator<U64ArrayObject> allocator(&rt->globalHeapPoolResource);

	U64ArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, length);

	rt->createdObjects.insert(ptr);

	return ptr;
}

HostObjectRef<U64ArrayObject> slake::U64ArrayObject::alloc(const U64ArrayObject *other) {
	std::pmr::polymorphic_allocator<U64ArrayObject> allocator(&other->_rt->globalHeapPoolResource);

	U64ArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, *other);

	other->_rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::U64ArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<U64ArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

//
// I8ArrayObject
//

I8ArrayAccessorVarObject::I8ArrayAccessorVarObject(Runtime *rt, I8ArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, Type(ValueType::I8), arrayObject) {
}

I8ArrayAccessorVarObject::~I8ArrayAccessorVarObject() {}

void I8ArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
	if (value.valueType != ValueType::I8)
		throw MismatchedTypeError("Mismatched array element type");

	I8ArrayObject *arrayObject = ((I8ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	arrayObject->data[index] = value.getI8();
}

Value I8ArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	I8ArrayObject *arrayObject = ((I8ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

HostObjectRef<I8ArrayAccessorVarObject> slake::I8ArrayAccessorVarObject::alloc(Runtime *rt, I8ArrayObject *arrayObject) {
	std::pmr::polymorphic_allocator<I8ArrayAccessorVarObject> allocator(&rt->globalHeapPoolResource);

	I8ArrayAccessorVarObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, arrayObject);

	rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::I8ArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<I8ArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

void I8ArrayObject::_resizeUnchecked(size_t newLength) {
	int8_t *newData = (int8_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(int8_t) * newLength);

	data = newData;
	length = newLength;
}

void I8ArrayObject::clear() {
	if (data) {
		_rt->globalHeapPoolResource.deallocate(
			data,
			length * sizeof(int8_t));
		data = nullptr;
		length = 0;
	}
}

void I8ArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	if (value.valueType != ValueType::I8)
		throw MismatchedTypeError("Mismatched array element type");

	int8_t v = value.getI8();
	memset(data + beginIndex, *(uint8_t *)&v, length * sizeof(int8_t));
}

void I8ArrayObject::resize(size_t newLength) {
	int8_t *newData = (int8_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(int8_t) * newLength);

	if (length < newLength) {
		memcpy(newData, data, length * sizeof(int8_t));
		memset(newData + newLength, 0, (newLength - length) * sizeof(int8_t));
	} else {
		memcpy(newData, data, newLength * sizeof(int8_t));
	}

	clear();

	data = newData;
	length = newLength;
}

I8ArrayObject::I8ArrayObject(Runtime *rt, size_t length)
	: ArrayObject(rt, Type(ValueType::I8), I8ArrayAccessorVarObject::alloc(rt, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(int8_t));
}

I8ArrayObject::I8ArrayObject(const I8ArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(int8_t));
}

I8ArrayObject::~I8ArrayObject() {
	clear();
}

Object *I8ArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

HostObjectRef<I8ArrayObject> slake::I8ArrayObject::alloc(Runtime *rt, size_t length) {
	std::pmr::polymorphic_allocator<I8ArrayObject> allocator(&rt->globalHeapPoolResource);

	I8ArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, length);

	rt->createdObjects.insert(ptr);

	return ptr;
}

HostObjectRef<I8ArrayObject> slake::I8ArrayObject::alloc(const I8ArrayObject *other) {
	std::pmr::polymorphic_allocator<I8ArrayObject> allocator(&other->_rt->globalHeapPoolResource);

	I8ArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, *other);

	other->_rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::I8ArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<I8ArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

//
// I16ArrayObject
//

I16ArrayAccessorVarObject::I16ArrayAccessorVarObject(Runtime *rt, I16ArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, Type(ValueType::I16), arrayObject) {
}

I16ArrayAccessorVarObject::~I16ArrayAccessorVarObject() {}

void I16ArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
	if (value.valueType != ValueType::I16)
		throw MismatchedTypeError("Mismatched array element type");

	I16ArrayObject *arrayObject = ((I16ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	arrayObject->data[index] = value.getI16();
}

Value I16ArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	I16ArrayObject *arrayObject = ((I16ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

HostObjectRef<I16ArrayAccessorVarObject> slake::I16ArrayAccessorVarObject::alloc(Runtime *rt, I16ArrayObject *arrayObject) {
	std::pmr::polymorphic_allocator<I16ArrayAccessorVarObject> allocator(&rt->globalHeapPoolResource);

	I16ArrayAccessorVarObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, arrayObject);

	rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::I16ArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<I16ArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

void I16ArrayObject::_resizeUnchecked(size_t newLength) {
	int16_t *newData = (int16_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(int16_t) * newLength);

	data = newData;
	length = newLength;
}

void I16ArrayObject::clear() {
	if (data) {
		_rt->globalHeapPoolResource.deallocate(
			data,
			length * sizeof(int16_t));
		data = nullptr;
		length = 0;
	}
}

void I16ArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	if (value.valueType != ValueType::I16)
		throw MismatchedTypeError("Mismatched array element type");
	std::fill_n(data + beginIndex, length, value.getI16());
}

void I16ArrayObject::resize(size_t newLength) {
	int16_t *newData = (int16_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(int16_t) * newLength);

	if (length < newLength) {
		memcpy(newData, data, length * sizeof(int16_t));
		memset(newData + newLength, 0, (newLength - length) * sizeof(int16_t));
	} else {
		memcpy(newData, data, newLength * sizeof(int16_t));
	}

	clear();

	data = newData;
	length = newLength;
}

I16ArrayObject::I16ArrayObject(Runtime *rt, size_t length)
	: ArrayObject(rt, Type(ValueType::I16), I16ArrayAccessorVarObject::alloc(rt, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(int16_t));
}

I16ArrayObject::I16ArrayObject(const I16ArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(int16_t));
}

I16ArrayObject::~I16ArrayObject() {
	clear();
}

Object *I16ArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

HostObjectRef<I16ArrayObject> slake::I16ArrayObject::alloc(Runtime *rt, size_t length) {
	std::pmr::polymorphic_allocator<I16ArrayObject> allocator(&rt->globalHeapPoolResource);

	I16ArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, length);

	rt->createdObjects.insert(ptr);

	return ptr;
}

HostObjectRef<I16ArrayObject> slake::I16ArrayObject::alloc(const I16ArrayObject *other) {
	std::pmr::polymorphic_allocator<I16ArrayObject> allocator(&other->_rt->globalHeapPoolResource);

	I16ArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, *other);

	other->_rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::I16ArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<I16ArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

//
// I32ArrayObject
//

I32ArrayAccessorVarObject::I32ArrayAccessorVarObject(Runtime *rt, I32ArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, Type(ValueType::I32), arrayObject) {
}

I32ArrayAccessorVarObject::~I32ArrayAccessorVarObject() {}

void I32ArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
	if (value.valueType != ValueType::I32)
		throw MismatchedTypeError("Mismatched array element type");

	I32ArrayObject *arrayObject = ((I32ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	arrayObject->data[index] = value.getI32();
}

Value I32ArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	I32ArrayObject *arrayObject = ((I32ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

HostObjectRef<I32ArrayAccessorVarObject> slake::I32ArrayAccessorVarObject::alloc(Runtime *rt, I32ArrayObject *arrayObject) {
	std::pmr::polymorphic_allocator<I32ArrayAccessorVarObject> allocator(&rt->globalHeapPoolResource);

	I32ArrayAccessorVarObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, arrayObject);

	rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::I32ArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<I32ArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

void I32ArrayObject::_resizeUnchecked(size_t newLength) {
	int32_t *newData = (int32_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(int32_t) * newLength);

	data = newData;
	length = newLength;
}

void I32ArrayObject::clear() {
	if (data) {
		_rt->globalHeapPoolResource.deallocate(
			data,
			length * sizeof(int32_t));
		data = nullptr;
		length = 0;
	}
}

void I32ArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	if (value.valueType != ValueType::I32)
		throw MismatchedTypeError("Mismatched array element type");
	std::fill_n(data + beginIndex, length, value.getI32());
}

void I32ArrayObject::resize(size_t newLength) {
	int32_t *newData = (int32_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(int32_t) * newLength);

	if (length < newLength) {
		memcpy(newData, data, length * sizeof(int32_t));
		memset(newData + newLength, 0, (newLength - length) * sizeof(int32_t));
	} else {
		memcpy(newData, data, newLength * sizeof(int32_t));
	}

	clear();

	data = newData;
	length = newLength;
}

I32ArrayObject::I32ArrayObject(Runtime *rt, size_t length)
	: ArrayObject(rt, Type(ValueType::I32), I32ArrayAccessorVarObject::alloc(rt, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(int32_t));
}

I32ArrayObject::I32ArrayObject(const I32ArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(int32_t));
}

I32ArrayObject::~I32ArrayObject() {
	clear();
}

Object *I32ArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

HostObjectRef<I32ArrayObject> slake::I32ArrayObject::alloc(Runtime *rt, size_t length) {
	std::pmr::polymorphic_allocator<I32ArrayObject> allocator(&rt->globalHeapPoolResource);

	I32ArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, length);

	rt->createdObjects.insert(ptr);

	return ptr;
}

HostObjectRef<I32ArrayObject> slake::I32ArrayObject::alloc(const I32ArrayObject *other) {
	std::pmr::polymorphic_allocator<I32ArrayObject> allocator(&other->_rt->globalHeapPoolResource);

	I32ArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, *other);

	other->_rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::I32ArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<I32ArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

//
// I64ArrayObject
//

I64ArrayAccessorVarObject::I64ArrayAccessorVarObject(Runtime *rt, I64ArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, Type(ValueType::I64), arrayObject) {
}

I64ArrayAccessorVarObject::~I64ArrayAccessorVarObject() {}

void I64ArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
	if (value.valueType != ValueType::I64)
		throw MismatchedTypeError("Mismatched array element type");

	I64ArrayObject *arrayObject = ((I64ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	arrayObject->data[index] = value.getI64();
}

Value I64ArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	I64ArrayObject *arrayObject = ((I64ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

HostObjectRef<I64ArrayAccessorVarObject> slake::I64ArrayAccessorVarObject::alloc(Runtime *rt, I64ArrayObject *arrayObject) {
	std::pmr::polymorphic_allocator<I64ArrayAccessorVarObject> allocator(&rt->globalHeapPoolResource);

	I64ArrayAccessorVarObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, arrayObject);

	rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::I64ArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<I64ArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

void I64ArrayObject::_resizeUnchecked(size_t newLength) {
	int64_t *newData = (int64_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(int64_t) * newLength);

	data = newData;
	length = newLength;
}

void I64ArrayObject::clear() {
	if (data) {
		_rt->globalHeapPoolResource.deallocate(
			data,
			length * sizeof(int64_t));
		data = nullptr;
		length = 0;
	}
}

void I64ArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	if (value.valueType != ValueType::I64)
		throw MismatchedTypeError("Mismatched array element type");
	std::fill_n(data + beginIndex, length, value.getI64());
}

void I64ArrayObject::resize(size_t newLength) {
	int64_t *newData = (int64_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(int64_t) * newLength);

	if (length < newLength) {
		memcpy(newData, data, length * sizeof(int64_t));
		memset(newData + newLength, 0, (newLength - length) * sizeof(int64_t));
	} else {
		memcpy(newData, data, newLength * sizeof(int64_t));
	}

	clear();

	data = newData;
	length = newLength;
}

I64ArrayObject::I64ArrayObject(Runtime *rt, size_t length)
	: ArrayObject(rt, Type(ValueType::I64), I64ArrayAccessorVarObject::alloc(rt, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(int64_t));
}

I64ArrayObject::I64ArrayObject(const I64ArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(int64_t));
}

I64ArrayObject::~I64ArrayObject() {
	clear();
}

Object *I64ArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

HostObjectRef<I64ArrayObject> slake::I64ArrayObject::alloc(Runtime *rt, size_t length) {
	std::pmr::polymorphic_allocator<I64ArrayObject> allocator(&rt->globalHeapPoolResource);

	I64ArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, length);

	rt->createdObjects.insert(ptr);

	return ptr;
}

HostObjectRef<I64ArrayObject> slake::I64ArrayObject::alloc(const I64ArrayObject *other) {
	std::pmr::polymorphic_allocator<I64ArrayObject> allocator(&other->_rt->globalHeapPoolResource);

	I64ArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, *other);

	other->_rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::I64ArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<I64ArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

//
// F32ArrayObject
//

F32ArrayAccessorVarObject::F32ArrayAccessorVarObject(Runtime *rt, F32ArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, Type(ValueType::F32), arrayObject) {
}

F32ArrayAccessorVarObject::~F32ArrayAccessorVarObject() {}

void F32ArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
	if (value.valueType != ValueType::F32)
		throw MismatchedTypeError("Mismatched array element type");

	F32ArrayObject *arrayObject = ((F32ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	arrayObject->data[index] = value.getF32();
}

Value F32ArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	F32ArrayObject *arrayObject = ((F32ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

HostObjectRef<F32ArrayAccessorVarObject> slake::F32ArrayAccessorVarObject::alloc(Runtime *rt, F32ArrayObject *arrayObject) {
	std::pmr::polymorphic_allocator<F32ArrayAccessorVarObject> allocator(&rt->globalHeapPoolResource);

	F32ArrayAccessorVarObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, arrayObject);

	rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::F32ArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<F32ArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

void F32ArrayObject::_resizeUnchecked(size_t newLength) {
	float_t *newData = (float_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(float_t) * newLength);

	data = newData;
	length = newLength;
}

void F32ArrayObject::clear() {
	if (data) {
		_rt->globalHeapPoolResource.deallocate(
			data,
			length * sizeof(float_t));
		data = nullptr;
		length = 0;
	}
}

void F32ArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	if (value.valueType != ValueType::F32)
		throw MismatchedTypeError("Mismatched array element type");
	std::fill_n(data + beginIndex, length, value.getF32());
}

void F32ArrayObject::resize(size_t newLength) {
	float_t *newData = (float_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(float_t) * newLength);

	if (length < newLength) {
		memcpy(newData, data, length * sizeof(float_t));
		memset(newData + newLength, 0, (newLength - length) * sizeof(float_t));
	} else {
		memcpy(newData, data, newLength * sizeof(float_t));
	}

	clear();

	data = newData;
	length = newLength;
}

F32ArrayObject::F32ArrayObject(Runtime *rt, size_t length)
	: ArrayObject(rt, Type(ValueType::F32), F32ArrayAccessorVarObject::alloc(rt, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(int8_t));
}

F32ArrayObject::F32ArrayObject(const F32ArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(int8_t));
}

F32ArrayObject::~F32ArrayObject() {
	clear();
}

Object *F32ArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

HostObjectRef<F32ArrayObject> slake::F32ArrayObject::alloc(Runtime *rt, size_t length) {
	std::pmr::polymorphic_allocator<F32ArrayObject> allocator(&rt->globalHeapPoolResource);

	F32ArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, length);

	rt->createdObjects.insert(ptr);

	return ptr;
}

HostObjectRef<F32ArrayObject> slake::F32ArrayObject::alloc(const F32ArrayObject *other) {
	std::pmr::polymorphic_allocator<F32ArrayObject> allocator(&other->_rt->globalHeapPoolResource);

	F32ArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, *other);

	other->_rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::F32ArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<F32ArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

//
// F64ArrayObject
//

F64ArrayAccessorVarObject::F64ArrayAccessorVarObject(Runtime *rt, F64ArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, Type(ValueType::F64), arrayObject) {
}

F64ArrayAccessorVarObject::~F64ArrayAccessorVarObject() {}

void F64ArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
	if (value.valueType != ValueType::F64)
		throw MismatchedTypeError("Mismatched array element type");

	F64ArrayObject *arrayObject = ((F64ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	arrayObject->data[index] = value.getF64();
}

Value F64ArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	F64ArrayObject *arrayObject = ((F64ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

HostObjectRef<F64ArrayAccessorVarObject> slake::F64ArrayAccessorVarObject::alloc(Runtime *rt, F64ArrayObject *arrayObject) {
	std::pmr::polymorphic_allocator<F64ArrayAccessorVarObject> allocator(&rt->globalHeapPoolResource);

	F64ArrayAccessorVarObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, arrayObject);

	rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::F64ArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<F64ArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

void F64ArrayObject::_resizeUnchecked(size_t newLength) {
	double_t *newData = (double_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(double_t) * newLength);

	data = newData;
	length = newLength;
}

void F64ArrayObject::clear() {
	if (data) {
		_rt->globalHeapPoolResource.deallocate(
			data,
			length * sizeof(double_t));
		data = nullptr;
		length = 0;
	}
}

void F64ArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	if (value.valueType != ValueType::F64)
		throw MismatchedTypeError("Mismatched array element type");
	std::fill_n(data + beginIndex, length, value.getF64());
}

void F64ArrayObject::resize(size_t newLength) {
	double_t *newData = (double_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(double_t) * newLength);

	if (length < newLength) {
		memcpy(newData, data, length * sizeof(double_t));
		memset(newData + newLength, 0, (newLength - length) * sizeof(double_t));
	} else {
		memcpy(newData, data, newLength * sizeof(double_t));
	}

	clear();

	data = newData;
	length = newLength;
}

F64ArrayObject::F64ArrayObject(Runtime *rt, size_t length)
	: ArrayObject(rt, Type(ValueType::F64), F64ArrayAccessorVarObject::alloc(rt, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(int8_t));
}

F64ArrayObject::F64ArrayObject(const F64ArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(int8_t));
}

F64ArrayObject::~F64ArrayObject() {
	clear();
}

Object *F64ArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

HostObjectRef<F64ArrayObject> slake::F64ArrayObject::alloc(Runtime *rt, size_t length) {
	std::pmr::polymorphic_allocator<F64ArrayObject> allocator(&rt->globalHeapPoolResource);

	F64ArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, length);

	rt->createdObjects.insert(ptr);

	return ptr;
}

HostObjectRef<F64ArrayObject> slake::F64ArrayObject::alloc(const F64ArrayObject *other) {
	std::pmr::polymorphic_allocator<F64ArrayObject> allocator(&other->_rt->globalHeapPoolResource);

	F64ArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, *other);

	other->_rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::F64ArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<F64ArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

//
// BoolArrayObject
//

BoolArrayAccessorVarObject::BoolArrayAccessorVarObject(Runtime *rt, BoolArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, Type(ValueType::Bool), arrayObject) {
}

BoolArrayAccessorVarObject::~BoolArrayAccessorVarObject() {}

void BoolArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
	if (value.valueType != ValueType::Bool)
		throw MismatchedTypeError("Mismatched array element type");

	BoolArrayObject *arrayObject = ((BoolArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	arrayObject->data[index] = value.getBool();
}

Value BoolArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	BoolArrayObject *arrayObject = ((BoolArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

HostObjectRef<BoolArrayAccessorVarObject> slake::BoolArrayAccessorVarObject::alloc(Runtime *rt, BoolArrayObject *arrayObject) {
	std::pmr::polymorphic_allocator<BoolArrayAccessorVarObject> allocator(&rt->globalHeapPoolResource);

	BoolArrayAccessorVarObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, arrayObject);

	rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::BoolArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<BoolArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

void BoolArrayObject::_resizeUnchecked(size_t newLength) {
	bool *newData = (bool *)_rt->globalHeapPoolResource.allocate(
		sizeof(bool) * newLength);

	data = newData;
	length = newLength;
}

void BoolArrayObject::clear() {
	if (data) {
		_rt->globalHeapPoolResource.deallocate(
			data,
			length * sizeof(bool));
		data = nullptr;
		length = 0;
	}
}

void BoolArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	if (value.valueType != ValueType::Bool)
		throw MismatchedTypeError("Mismatched array element type");
	memset(data, value.getBool(), length * sizeof(bool));
}

void BoolArrayObject::resize(size_t newLength) {
	bool *newData = (bool *)_rt->globalHeapPoolResource.allocate(
		sizeof(bool) * newLength);

	if (length < newLength) {
		memcpy(newData, data, length * sizeof(bool));
		memset(newData + newLength, 0, (newLength - length) * sizeof(bool));
	} else {
		memcpy(newData, data, newLength * sizeof(bool));
	}

	clear();

	data = newData;
	length = newLength;
}

BoolArrayObject::BoolArrayObject(Runtime *rt, size_t length)
	: ArrayObject(rt, Type(ValueType::Bool), BoolArrayAccessorVarObject::alloc(rt, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(bool));
}

BoolArrayObject::BoolArrayObject(const BoolArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(bool));
}

BoolArrayObject::~BoolArrayObject() {
	clear();
}

Object *BoolArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

HostObjectRef<BoolArrayObject> slake::BoolArrayObject::alloc(Runtime *rt, size_t length) {
	std::pmr::polymorphic_allocator<BoolArrayObject> allocator(&rt->globalHeapPoolResource);

	BoolArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, length);

	rt->createdObjects.insert(ptr);

	return ptr;
}

HostObjectRef<BoolArrayObject> slake::BoolArrayObject::alloc(const BoolArrayObject *other) {
	std::pmr::polymorphic_allocator<BoolArrayObject> allocator(&other->_rt->globalHeapPoolResource);

	BoolArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, *other);

	other->_rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::BoolArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<BoolArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

//
// ObjectRefArrayObject
//

ObjectRefArrayAccessorVarObject::ObjectRefArrayAccessorVarObject(Runtime *rt, const Type &elementType, ObjectRefArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, elementType, arrayObject) {
}

ObjectRefArrayAccessorVarObject::~ObjectRefArrayAccessorVarObject() {}

void ObjectRefArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
	if (value.valueType != ValueType::ObjectRef)
		throw MismatchedTypeError("Mismatched array element type");

	ObjectRefArrayObject *arrayObject = ((ObjectRefArrayObject *)this->arrayObject);

	if (!isCompatible(arrayObject->elementType, value))
		throw MismatchedTypeError("Mismatched array element type");

	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	arrayObject->data[index] = value.getObjectRef();
}

Value ObjectRefArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	ObjectRefArrayObject *arrayObject = ((ObjectRefArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

HostObjectRef<ObjectRefArrayAccessorVarObject> slake::ObjectRefArrayAccessorVarObject::alloc(Runtime *rt, const Type &elementType, ObjectRefArrayObject *arrayObject) {
	std::pmr::polymorphic_allocator<ObjectRefArrayAccessorVarObject> allocator(&rt->globalHeapPoolResource);

	ObjectRefArrayAccessorVarObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, elementType, arrayObject);

	rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::ObjectRefArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<ObjectRefArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

void ObjectRefArrayObject::_resizeUnchecked(size_t newLength) {
	Object **newData = (Object **)_rt->globalHeapPoolResource.allocate(
		sizeof(Object *) * newLength);

	data = newData;
	length = newLength;
}

void ObjectRefArrayObject::clear() {
	if (data) {
		_rt->globalHeapPoolResource.deallocate(
			data,
			length * sizeof(Object *));
		data = nullptr;
		length = 0;
	}
}

void ObjectRefArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	if (value.valueType != ValueType::ObjectRef)
		throw MismatchedTypeError("Mismatched array element type");
	if (!isCompatible(elementType, value))
		throw MismatchedTypeError("Mismatched array element type");
	std::fill_n(data + beginIndex, length, value.getObjectRef());
}

void ObjectRefArrayObject::resize(size_t newLength) {
	Object **newData = (Object **)_rt->globalHeapPoolResource.allocate(
		sizeof(Object *) * newLength);

	if (length < newLength) {
		memcpy(newData, data, length * sizeof(Object *));
		memset(newData + newLength, 0, (newLength - length) * sizeof(Object *));
	} else {
		memcpy(newData, data, newLength * sizeof(Object *));
	}

	clear();

	data = newData;
	length = newLength;
}

ObjectRefArrayObject::ObjectRefArrayObject(Runtime *rt, const Type &elementType, size_t length)
	: ArrayObject(rt, elementType, ObjectRefArrayAccessorVarObject::alloc(rt, elementType, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(Object *));
}

ObjectRefArrayObject::ObjectRefArrayObject(const ObjectRefArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(Object *));
}

ObjectRefArrayObject::~ObjectRefArrayObject() {
	clear();
}

Object *ObjectRefArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

HostObjectRef<ObjectRefArrayObject> slake::ObjectRefArrayObject::alloc(Runtime *rt, const Type &elementType, size_t length) {
	std::pmr::polymorphic_allocator<ObjectRefArrayObject> allocator(&rt->globalHeapPoolResource);

	ObjectRefArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, elementType, length);

	rt->createdObjects.insert(ptr);

	return ptr;
}

HostObjectRef<ObjectRefArrayObject> slake::ObjectRefArrayObject::alloc(const ObjectRefArrayObject *other) {
	std::pmr::polymorphic_allocator<ObjectRefArrayObject> allocator(&other->_rt->globalHeapPoolResource);

	ObjectRefArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, *other);

	other->_rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::ObjectRefArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<ObjectRefArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

//
// AnyArrayObject
//

AnyArrayAccessorVarObject::AnyArrayAccessorVarObject(Runtime *rt, AnyArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, Type(TypeId::Any), arrayObject) {
}

AnyArrayAccessorVarObject::~AnyArrayAccessorVarObject() {}

void AnyArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
	AnyArrayObject *arrayObject = ((AnyArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	arrayObject->data[index] = value;
}

Value AnyArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	AnyArrayObject *arrayObject = ((AnyArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

HostObjectRef<AnyArrayAccessorVarObject> slake::AnyArrayAccessorVarObject::alloc(Runtime *rt, AnyArrayObject *arrayObject) {
	std::pmr::polymorphic_allocator<AnyArrayAccessorVarObject> allocator(&rt->globalHeapPoolResource);

	AnyArrayAccessorVarObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, arrayObject);

	rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::AnyArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<AnyArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

void AnyArrayObject::_resizeUnchecked(size_t newLength) {
	Value *newData = (Value *)_rt->globalHeapPoolResource.allocate(
		sizeof(Value) * newLength);

	data = newData;
	length = newLength;
}

void AnyArrayObject::clear() {
	if (data) {
		for (size_t i = 0; i < length; ++i)
			std::destroy_at(&data[i]);
		_rt->globalHeapPoolResource.deallocate(
			data,
			length * sizeof(Value));
		data = nullptr;
		length = 0;
	}
}

void AnyArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	std::fill_n(data + beginIndex, length, value);
}

void AnyArrayObject::resize(size_t newLength) {
	Value *newData = (Value *)_rt->globalHeapPoolResource.allocate(
		sizeof(Value) * newLength);

	if (length < newLength) {
		std::uninitialized_move_n(data, length, newData);
		std::uninitialized_value_construct_n(newData + newLength, (newLength - length));
	} else {
		std::uninitialized_move_n(data, newLength, newData);
	}

	clear();

	data = newData;
	length = newLength;
}

AnyArrayObject::AnyArrayObject(Runtime *rt, size_t length)
	: ArrayObject(rt, Type(TypeId::Any), AnyArrayAccessorVarObject::alloc(rt, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(Value));
}

AnyArrayObject::AnyArrayObject(const AnyArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(Value));
}

AnyArrayObject::~AnyArrayObject() {
	clear();
}

Object *AnyArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

HostObjectRef<AnyArrayObject> slake::AnyArrayObject::alloc(Runtime *rt, size_t length) {
	std::pmr::polymorphic_allocator<AnyArrayObject> allocator(&rt->globalHeapPoolResource);

	AnyArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt, length);

	rt->createdObjects.insert(ptr);

	return ptr;
}

HostObjectRef<AnyArrayObject> slake::AnyArrayObject::alloc(const AnyArrayObject *other) {
	std::pmr::polymorphic_allocator<AnyArrayObject> allocator(&other->_rt->globalHeapPoolResource);

	AnyArrayObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, *other);

	other->_rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::AnyArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<AnyArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
