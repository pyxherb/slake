#include <slake/runtime.h>
#include <algorithm>

using namespace slake;

SLAKE_API ArrayAccessorVarObject::ArrayAccessorVarObject(
	Runtime *rt,
	const Type &elementType,
	ArrayObject *arrayObject)
	: VarObject(rt), arrayObject(arrayObject) {
}

SLAKE_API ArrayAccessorVarObject::ArrayAccessorVarObject(const ArrayAccessorVarObject &other) : VarObject(other) {
	arrayObject = other.arrayObject;
}

SLAKE_API ArrayAccessorVarObject::~ArrayAccessorVarObject() {}

SLAKE_API Type ArrayAccessorVarObject::getVarType(const VarRefContext &context) const {
	return Type::makeArrayTypeName(_rt, elementType);
}

SLAKE_API ArrayObject::ArrayObject(Runtime *rt, const Type &elementType, ArrayAccessorVarObject *accessor)
	: Object(rt),
	  elementType(elementType),
	  accessor(accessor) {
}

SLAKE_API ArrayObject::ArrayObject(const ArrayObject &x) : Object(x) {
	length = x.length;
	elementType = x.elementType;
	accessor = x.accessor;
}

SLAKE_API ArrayObject::~ArrayObject() {
}

SLAKE_API ObjectKind ArrayObject::getKind() const { return ObjectKind::Array; }

//
// U8ArrayObject
//

SLAKE_API U8ArrayAccessorVarObject::U8ArrayAccessorVarObject(Runtime *rt, U8ArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, Type(ValueType::U8), arrayObject) {
}

SLAKE_API U8ArrayAccessorVarObject::~U8ArrayAccessorVarObject() {}

SLAKE_API void U8ArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
	if (value.valueType != ValueType::U8)
		throw MismatchedTypeError("Mismatched array element type");

	U8ArrayObject *arrayObject = ((U8ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	arrayObject->data[index] = value.getU8();
}

SLAKE_API Value U8ArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	U8ArrayObject *arrayObject = ((U8ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

SLAKE_API HostObjectRef<U8ArrayAccessorVarObject> slake::U8ArrayAccessorVarObject::alloc(Runtime *rt, U8ArrayObject *arrayObject) {
	using Alloc = std::pmr::polymorphic_allocator<U8ArrayAccessorVarObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<U8ArrayAccessorVarObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, arrayObject);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::U8ArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<U8ArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API void U8ArrayObject::_resizeUnchecked(size_t newLength) {
	uint8_t *newData = (uint8_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(uint8_t) * newLength);

	data = newData;
	length = newLength;
}

SLAKE_API void U8ArrayObject::clear() {
	if (data) {
		_rt->globalHeapPoolResource.deallocate(
			data,
			length * sizeof(uint8_t));
		data = nullptr;
		length = 0;
	}
}

SLAKE_API void U8ArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	if (value.valueType != ValueType::U8)
		throw MismatchedTypeError("Mismatched array element type");
	memset(data + beginIndex, value.getU8(), length * sizeof(uint8_t));
}

SLAKE_API void U8ArrayObject::resize(size_t newLength) {
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

SLAKE_API U8ArrayObject::U8ArrayObject(Runtime *rt, size_t length)
	: ArrayObject(rt, Type(ValueType::U8), U8ArrayAccessorVarObject::alloc(rt, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(uint8_t));
}

SLAKE_API U8ArrayObject::U8ArrayObject(const U8ArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(uint8_t));
}

SLAKE_API U8ArrayObject::~U8ArrayObject() {
	clear();
}

Object *U8ArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<U8ArrayObject> slake::U8ArrayObject::alloc(Runtime *rt, size_t length) {
	using Alloc = std::pmr::polymorphic_allocator<U8ArrayObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<U8ArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, length);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<U8ArrayObject> slake::U8ArrayObject::alloc(const U8ArrayObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<U8ArrayObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<U8ArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::U8ArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<U8ArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

//
// U16ArrayObject
//

SLAKE_API U16ArrayAccessorVarObject::U16ArrayAccessorVarObject(Runtime *rt, U16ArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, Type(ValueType::U16), arrayObject) {
}

SLAKE_API U16ArrayAccessorVarObject::~U16ArrayAccessorVarObject() {}

SLAKE_API void U16ArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
	if (value.valueType != ValueType::U16)
		throw MismatchedTypeError("Mismatched array element type");

	U16ArrayObject *arrayObject = ((U16ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	arrayObject->data[index] = value.getU16();
}

SLAKE_API Value U16ArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	U16ArrayObject *arrayObject = ((U16ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

SLAKE_API HostObjectRef<U16ArrayAccessorVarObject> slake::U16ArrayAccessorVarObject::alloc(Runtime *rt, U16ArrayObject *arrayObject) {
	using Alloc = std::pmr::polymorphic_allocator<U16ArrayAccessorVarObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<U16ArrayAccessorVarObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, arrayObject);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::U16ArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<U16ArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API void U16ArrayObject::_resizeUnchecked(size_t newLength) {
	uint16_t *newData = (uint16_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(uint16_t) * newLength);

	data = newData;
	length = newLength;
}

SLAKE_API void U16ArrayObject::clear() {
	if (data) {
		_rt->globalHeapPoolResource.deallocate(
			data,
			length * sizeof(uint16_t));
		data = nullptr;
		length = 0;
	}
}

SLAKE_API void U16ArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	if (value.valueType != ValueType::U16)
		throw MismatchedTypeError("Mismatched array element type");
	std::fill_n(data + beginIndex, length, value.getU16());
}

SLAKE_API void U16ArrayObject::resize(size_t newLength) {
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

SLAKE_API U16ArrayObject::U16ArrayObject(Runtime *rt, size_t length)
	: ArrayObject(rt, Type(ValueType::U16), U16ArrayAccessorVarObject::alloc(rt, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(uint16_t));
}

SLAKE_API U16ArrayObject::U16ArrayObject(const U16ArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(uint16_t));
}

SLAKE_API U16ArrayObject::~U16ArrayObject() {
	clear();
}

Object *U16ArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<U16ArrayObject> slake::U16ArrayObject::alloc(Runtime *rt, size_t length) {
	using Alloc = std::pmr::polymorphic_allocator<U16ArrayObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<U16ArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, length);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<U16ArrayObject> slake::U16ArrayObject::alloc(const U16ArrayObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<U16ArrayObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<U16ArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::U16ArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<U16ArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

//
// U32ArrayObject
//

SLAKE_API U32ArrayAccessorVarObject::U32ArrayAccessorVarObject(Runtime *rt, U32ArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, Type(ValueType::U32), arrayObject) {
}

SLAKE_API U32ArrayAccessorVarObject::~U32ArrayAccessorVarObject() {}

SLAKE_API void U32ArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
	if (value.valueType != ValueType::U32)
		throw MismatchedTypeError("Mismatched array element type");

	U32ArrayObject *arrayObject = ((U32ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	arrayObject->data[index] = value.getU32();
}

SLAKE_API Value U32ArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	U32ArrayObject *arrayObject = ((U32ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

SLAKE_API HostObjectRef<U32ArrayAccessorVarObject> slake::U32ArrayAccessorVarObject::alloc(Runtime *rt, U32ArrayObject *arrayObject) {
	using Alloc = std::pmr::polymorphic_allocator<U32ArrayAccessorVarObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<U32ArrayAccessorVarObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, arrayObject);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::U32ArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<U32ArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API void U32ArrayObject::_resizeUnchecked(size_t newLength) {
	uint32_t *newData = (uint32_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(uint32_t) * newLength);

	data = newData;
	length = newLength;
}

SLAKE_API void U32ArrayObject::clear() {
	if (data) {
		_rt->globalHeapPoolResource.deallocate(
			data,
			length * sizeof(uint32_t));
		data = nullptr;
		length = 0;
	}
}

SLAKE_API void U32ArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	if (value.valueType != ValueType::U32)
		throw MismatchedTypeError("Mismatched array element type");
	std::fill_n(data + beginIndex, length, value.getU32());
}

SLAKE_API void U32ArrayObject::resize(size_t newLength) {
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

SLAKE_API U32ArrayObject::U32ArrayObject(Runtime *rt, size_t length)
	: ArrayObject(rt, Type(ValueType::U32), U32ArrayAccessorVarObject::alloc(rt, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(uint32_t));
}

SLAKE_API U32ArrayObject::U32ArrayObject(const U32ArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(uint32_t));
}

SLAKE_API U32ArrayObject::~U32ArrayObject() {
	clear();
}

Object *U32ArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<U32ArrayObject> slake::U32ArrayObject::alloc(Runtime *rt, size_t length) {
	using Alloc = std::pmr::polymorphic_allocator<U32ArrayObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<U32ArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, length);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<U32ArrayObject> slake::U32ArrayObject::alloc(const U32ArrayObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<U32ArrayObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<U32ArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::U32ArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<U32ArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

//
// U64ArrayObject
//

SLAKE_API U64ArrayAccessorVarObject::U64ArrayAccessorVarObject(Runtime *rt, U64ArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, Type(ValueType::U64), arrayObject) {
}

SLAKE_API U64ArrayAccessorVarObject::~U64ArrayAccessorVarObject() {}

SLAKE_API void U64ArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
	if (value.valueType != ValueType::U64)
		throw MismatchedTypeError("Mismatched array element type");

	U64ArrayObject *arrayObject = ((U64ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	arrayObject->data[index] = value.getU64();
}

SLAKE_API Value U64ArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	U64ArrayObject *arrayObject = ((U64ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

SLAKE_API HostObjectRef<U64ArrayAccessorVarObject> slake::U64ArrayAccessorVarObject::alloc(Runtime *rt, U64ArrayObject *arrayObject) {
	using Alloc = std::pmr::polymorphic_allocator<U64ArrayAccessorVarObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<U64ArrayAccessorVarObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, arrayObject);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::U64ArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<U64ArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API void U64ArrayObject::_resizeUnchecked(size_t newLength) {
	uint64_t *newData = (uint64_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(uint64_t) * newLength);

	data = newData;
	length = newLength;
}

SLAKE_API void U64ArrayObject::clear() {
	if (data) {
		_rt->globalHeapPoolResource.deallocate(
			data,
			length * sizeof(uint64_t));
		data = nullptr;
		length = 0;
	}
}

SLAKE_API void U64ArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	if (value.valueType != ValueType::U64)
		throw MismatchedTypeError("Mismatched array element type");
	std::fill_n(data + beginIndex, length, value.getU64());
}

SLAKE_API void U64ArrayObject::resize(size_t newLength) {
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

SLAKE_API U64ArrayObject::U64ArrayObject(Runtime *rt, size_t length)
	: ArrayObject(rt, Type(ValueType::U64), U64ArrayAccessorVarObject::alloc(rt, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(uint64_t));
}

SLAKE_API U64ArrayObject::U64ArrayObject(const U64ArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(uint64_t));
}

SLAKE_API U64ArrayObject::~U64ArrayObject() {
	clear();
}

Object *U64ArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<U64ArrayObject> slake::U64ArrayObject::alloc(Runtime *rt, size_t length) {
	using Alloc = std::pmr::polymorphic_allocator<U64ArrayObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<U64ArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, length);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<U64ArrayObject> slake::U64ArrayObject::alloc(const U64ArrayObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<U64ArrayObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<U64ArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::U64ArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<U64ArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

//
// I8ArrayObject
//

SLAKE_API I8ArrayAccessorVarObject::I8ArrayAccessorVarObject(Runtime *rt, I8ArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, Type(ValueType::I8), arrayObject) {
}

SLAKE_API I8ArrayAccessorVarObject::~I8ArrayAccessorVarObject() {}

SLAKE_API void I8ArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
	if (value.valueType != ValueType::I8)
		throw MismatchedTypeError("Mismatched array element type");

	I8ArrayObject *arrayObject = ((I8ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	arrayObject->data[index] = value.getI8();
}

SLAKE_API Value I8ArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	I8ArrayObject *arrayObject = ((I8ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

SLAKE_API HostObjectRef<I8ArrayAccessorVarObject> slake::I8ArrayAccessorVarObject::alloc(Runtime *rt, I8ArrayObject *arrayObject) {
	using Alloc = std::pmr::polymorphic_allocator<I8ArrayAccessorVarObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<I8ArrayAccessorVarObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, arrayObject);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::I8ArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<I8ArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API void I8ArrayObject::_resizeUnchecked(size_t newLength) {
	int8_t *newData = (int8_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(int8_t) * newLength);

	data = newData;
	length = newLength;
}

SLAKE_API void I8ArrayObject::clear() {
	if (data) {
		_rt->globalHeapPoolResource.deallocate(
			data,
			length * sizeof(int8_t));
		data = nullptr;
		length = 0;
	}
}

SLAKE_API void I8ArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	if (value.valueType != ValueType::I8)
		throw MismatchedTypeError("Mismatched array element type");

	int8_t v = value.getI8();
	memset(data + beginIndex, *(uint8_t *)&v, length * sizeof(int8_t));
}

SLAKE_API void I8ArrayObject::resize(size_t newLength) {
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

SLAKE_API I8ArrayObject::I8ArrayObject(Runtime *rt, size_t length)
	: ArrayObject(rt, Type(ValueType::I8), I8ArrayAccessorVarObject::alloc(rt, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(int8_t));
}

SLAKE_API I8ArrayObject::I8ArrayObject(const I8ArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(int8_t));
}

SLAKE_API I8ArrayObject::~I8ArrayObject() {
	clear();
}

Object *I8ArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<I8ArrayObject> slake::I8ArrayObject::alloc(Runtime *rt, size_t length) {
	using Alloc = std::pmr::polymorphic_allocator<I8ArrayObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<I8ArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, length);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<I8ArrayObject> slake::I8ArrayObject::alloc(const I8ArrayObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<I8ArrayObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<I8ArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::I8ArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<I8ArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

//
// I16ArrayObject
//

SLAKE_API I16ArrayAccessorVarObject::I16ArrayAccessorVarObject(Runtime *rt, I16ArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, Type(ValueType::I16), arrayObject) {
}

SLAKE_API I16ArrayAccessorVarObject::~I16ArrayAccessorVarObject() {}

SLAKE_API void I16ArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
	if (value.valueType != ValueType::I16)
		throw MismatchedTypeError("Mismatched array element type");

	I16ArrayObject *arrayObject = ((I16ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	arrayObject->data[index] = value.getI16();
}

SLAKE_API Value I16ArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	I16ArrayObject *arrayObject = ((I16ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

SLAKE_API HostObjectRef<I16ArrayAccessorVarObject> slake::I16ArrayAccessorVarObject::alloc(Runtime *rt, I16ArrayObject *arrayObject) {
	using Alloc = std::pmr::polymorphic_allocator<I16ArrayAccessorVarObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<I16ArrayAccessorVarObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, arrayObject);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::I16ArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<I16ArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API void I16ArrayObject::_resizeUnchecked(size_t newLength) {
	int16_t *newData = (int16_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(int16_t) * newLength);

	data = newData;
	length = newLength;
}

SLAKE_API void I16ArrayObject::clear() {
	if (data) {
		_rt->globalHeapPoolResource.deallocate(
			data,
			length * sizeof(int16_t));
		data = nullptr;
		length = 0;
	}
}

SLAKE_API void I16ArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	if (value.valueType != ValueType::I16)
		throw MismatchedTypeError("Mismatched array element type");
	std::fill_n(data + beginIndex, length, value.getI16());
}

SLAKE_API void I16ArrayObject::resize(size_t newLength) {
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

SLAKE_API I16ArrayObject::I16ArrayObject(Runtime *rt, size_t length)
	: ArrayObject(rt, Type(ValueType::I16), I16ArrayAccessorVarObject::alloc(rt, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(int16_t));
}

SLAKE_API I16ArrayObject::I16ArrayObject(const I16ArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(int16_t));
}

SLAKE_API I16ArrayObject::~I16ArrayObject() {
	clear();
}

Object *I16ArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<I16ArrayObject> slake::I16ArrayObject::alloc(Runtime *rt, size_t length) {
	using Alloc = std::pmr::polymorphic_allocator<I16ArrayObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<I16ArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, length);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<I16ArrayObject> slake::I16ArrayObject::alloc(const I16ArrayObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<I16ArrayObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<I16ArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::I16ArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<I16ArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

//
// I32ArrayObject
//

SLAKE_API I32ArrayAccessorVarObject::I32ArrayAccessorVarObject(Runtime *rt, I32ArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, Type(ValueType::I32), arrayObject) {
}

SLAKE_API I32ArrayAccessorVarObject::~I32ArrayAccessorVarObject() {}

SLAKE_API void I32ArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
	if (value.valueType != ValueType::I32)
		throw MismatchedTypeError("Mismatched array element type");

	I32ArrayObject *arrayObject = ((I32ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	arrayObject->data[index] = value.getI32();
}

SLAKE_API Value I32ArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	I32ArrayObject *arrayObject = ((I32ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

SLAKE_API HostObjectRef<I32ArrayAccessorVarObject> slake::I32ArrayAccessorVarObject::alloc(Runtime *rt, I32ArrayObject *arrayObject) {
	using Alloc = std::pmr::polymorphic_allocator<I32ArrayAccessorVarObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<I32ArrayAccessorVarObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, arrayObject);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::I32ArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<I32ArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API void I32ArrayObject::_resizeUnchecked(size_t newLength) {
	int32_t *newData = (int32_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(int32_t) * newLength);

	data = newData;
	length = newLength;
}

SLAKE_API void I32ArrayObject::clear() {
	if (data) {
		_rt->globalHeapPoolResource.deallocate(
			data,
			length * sizeof(int32_t));
		data = nullptr;
		length = 0;
	}
}

SLAKE_API void I32ArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	if (value.valueType != ValueType::I32)
		throw MismatchedTypeError("Mismatched array element type");
	std::fill_n(data + beginIndex, length, value.getI32());
}

SLAKE_API void I32ArrayObject::resize(size_t newLength) {
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

SLAKE_API I32ArrayObject::I32ArrayObject(Runtime *rt, size_t length)
	: ArrayObject(rt, Type(ValueType::I32), I32ArrayAccessorVarObject::alloc(rt, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(int32_t));
}

SLAKE_API I32ArrayObject::I32ArrayObject(const I32ArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(int32_t));
}

SLAKE_API I32ArrayObject::~I32ArrayObject() {
	clear();
}

Object *I32ArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<I32ArrayObject> slake::I32ArrayObject::alloc(Runtime *rt, size_t length) {
	using Alloc = std::pmr::polymorphic_allocator<I32ArrayObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<I32ArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, length);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<I32ArrayObject> slake::I32ArrayObject::alloc(const I32ArrayObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<I32ArrayObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<I32ArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::I32ArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<I32ArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

//
// I64ArrayObject
//

SLAKE_API I64ArrayAccessorVarObject::I64ArrayAccessorVarObject(Runtime *rt, I64ArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, Type(ValueType::I64), arrayObject) {
}

SLAKE_API I64ArrayAccessorVarObject::~I64ArrayAccessorVarObject() {}

SLAKE_API void I64ArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
	if (value.valueType != ValueType::I64)
		throw MismatchedTypeError("Mismatched array element type");

	I64ArrayObject *arrayObject = ((I64ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	arrayObject->data[index] = value.getI64();
}

SLAKE_API Value I64ArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	I64ArrayObject *arrayObject = ((I64ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

SLAKE_API HostObjectRef<I64ArrayAccessorVarObject> slake::I64ArrayAccessorVarObject::alloc(Runtime *rt, I64ArrayObject *arrayObject) {
	using Alloc = std::pmr::polymorphic_allocator<I64ArrayAccessorVarObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<I64ArrayAccessorVarObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, arrayObject);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::I64ArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<I64ArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API void I64ArrayObject::_resizeUnchecked(size_t newLength) {
	int64_t *newData = (int64_t *)_rt->globalHeapPoolResource.allocate(
		sizeof(int64_t) * newLength);

	data = newData;
	length = newLength;
}

SLAKE_API void I64ArrayObject::clear() {
	if (data) {
		_rt->globalHeapPoolResource.deallocate(
			data,
			length * sizeof(int64_t));
		data = nullptr;
		length = 0;
	}
}

SLAKE_API void I64ArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	if (value.valueType != ValueType::I64)
		throw MismatchedTypeError("Mismatched array element type");
	std::fill_n(data + beginIndex, length, value.getI64());
}

SLAKE_API void I64ArrayObject::resize(size_t newLength) {
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

SLAKE_API I64ArrayObject::I64ArrayObject(Runtime *rt, size_t length)
	: ArrayObject(rt, Type(ValueType::I64), I64ArrayAccessorVarObject::alloc(rt, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(int64_t));
}

SLAKE_API I64ArrayObject::I64ArrayObject(const I64ArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(int64_t));
}

SLAKE_API I64ArrayObject::~I64ArrayObject() {
	clear();
}

Object *I64ArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<I64ArrayObject> slake::I64ArrayObject::alloc(Runtime *rt, size_t length) {
	using Alloc = std::pmr::polymorphic_allocator<I64ArrayObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<I64ArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, length);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<I64ArrayObject> slake::I64ArrayObject::alloc(const I64ArrayObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<I64ArrayObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<I64ArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::I64ArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<I64ArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

//
// F32ArrayObject
//

SLAKE_API F32ArrayAccessorVarObject::F32ArrayAccessorVarObject(Runtime *rt, F32ArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, Type(ValueType::F32), arrayObject) {
}

SLAKE_API F32ArrayAccessorVarObject::~F32ArrayAccessorVarObject() {}

SLAKE_API void F32ArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
	if (value.valueType != ValueType::F32)
		throw MismatchedTypeError("Mismatched array element type");

	F32ArrayObject *arrayObject = ((F32ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	arrayObject->data[index] = value.getF32();
}

SLAKE_API Value F32ArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	F32ArrayObject *arrayObject = ((F32ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

SLAKE_API HostObjectRef<F32ArrayAccessorVarObject> slake::F32ArrayAccessorVarObject::alloc(Runtime *rt, F32ArrayObject *arrayObject) {
	using Alloc = std::pmr::polymorphic_allocator<F32ArrayAccessorVarObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<F32ArrayAccessorVarObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, arrayObject);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::F32ArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<F32ArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API void F32ArrayObject::_resizeUnchecked(size_t newLength) {
	float *newData = (float *)_rt->globalHeapPoolResource.allocate(
		sizeof(float) * newLength);

	data = newData;
	length = newLength;
}

SLAKE_API void F32ArrayObject::clear() {
	if (data) {
		_rt->globalHeapPoolResource.deallocate(
			data,
			length * sizeof(float));
		data = nullptr;
		length = 0;
	}
}

SLAKE_API void F32ArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	if (value.valueType != ValueType::F32)
		throw MismatchedTypeError("Mismatched array element type");
	std::fill_n(data + beginIndex, length, value.getF32());
}

SLAKE_API void F32ArrayObject::resize(size_t newLength) {
	float *newData = (float *)_rt->globalHeapPoolResource.allocate(
		sizeof(float) * newLength);

	if (length < newLength) {
		memcpy(newData, data, length * sizeof(float));
		memset(newData + newLength, 0, (newLength - length) * sizeof(float));
	} else {
		memcpy(newData, data, newLength * sizeof(float));
	}

	clear();

	data = newData;
	length = newLength;
}

SLAKE_API F32ArrayObject::F32ArrayObject(Runtime *rt, size_t length)
	: ArrayObject(rt, Type(ValueType::F32), F32ArrayAccessorVarObject::alloc(rt, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(float));
}

SLAKE_API F32ArrayObject::F32ArrayObject(const F32ArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(float));
}

SLAKE_API F32ArrayObject::~F32ArrayObject() {
	clear();
}

Object *F32ArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<F32ArrayObject> slake::F32ArrayObject::alloc(Runtime *rt, size_t length) {
	using Alloc = std::pmr::polymorphic_allocator<F32ArrayObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<F32ArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, length);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<F32ArrayObject> slake::F32ArrayObject::alloc(const F32ArrayObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<F32ArrayObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<F32ArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::F32ArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<F32ArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

//
// F64ArrayObject
//

SLAKE_API F64ArrayAccessorVarObject::F64ArrayAccessorVarObject(Runtime *rt, F64ArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, Type(ValueType::F64), arrayObject) {
}

SLAKE_API F64ArrayAccessorVarObject::~F64ArrayAccessorVarObject() {}

SLAKE_API void F64ArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
	if (value.valueType != ValueType::F64)
		throw MismatchedTypeError("Mismatched array element type");

	F64ArrayObject *arrayObject = ((F64ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	arrayObject->data[index] = value.getF64();
}

SLAKE_API Value F64ArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	F64ArrayObject *arrayObject = ((F64ArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

SLAKE_API HostObjectRef<F64ArrayAccessorVarObject> slake::F64ArrayAccessorVarObject::alloc(Runtime *rt, F64ArrayObject *arrayObject) {
	using Alloc = std::pmr::polymorphic_allocator<F64ArrayAccessorVarObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<F64ArrayAccessorVarObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, arrayObject);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::F64ArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<F64ArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API void F64ArrayObject::_resizeUnchecked(size_t newLength) {
	double *newData = (double *)_rt->globalHeapPoolResource.allocate(
		sizeof(double) * newLength);

	data = newData;
	length = newLength;
}

SLAKE_API void F64ArrayObject::clear() {
	if (data) {
		_rt->globalHeapPoolResource.deallocate(
			data,
			length * sizeof(double));
		data = nullptr;
		length = 0;
	}
}

SLAKE_API void F64ArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	if (value.valueType != ValueType::F64)
		throw MismatchedTypeError("Mismatched array element type");
	std::fill_n(data + beginIndex, length, value.getF64());
}

SLAKE_API void F64ArrayObject::resize(size_t newLength) {
	double *newData = (double *)_rt->globalHeapPoolResource.allocate(
		sizeof(double) * newLength);

	if (length < newLength) {
		memcpy(newData, data, length * sizeof(double));
		memset(newData + newLength, 0, (newLength - length) * sizeof(double));
	} else {
		memcpy(newData, data, newLength * sizeof(double));
	}

	clear();

	data = newData;
	length = newLength;
}

SLAKE_API F64ArrayObject::F64ArrayObject(Runtime *rt, size_t length)
	: ArrayObject(rt, Type(ValueType::F64), F64ArrayAccessorVarObject::alloc(rt, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(double));
}

SLAKE_API F64ArrayObject::F64ArrayObject(const F64ArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(double));
}

SLAKE_API F64ArrayObject::~F64ArrayObject() {
	clear();
}

Object *F64ArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<F64ArrayObject> slake::F64ArrayObject::alloc(Runtime *rt, size_t length) {
	using Alloc = std::pmr::polymorphic_allocator<F64ArrayObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<F64ArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, length);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<F64ArrayObject> slake::F64ArrayObject::alloc(const F64ArrayObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<F64ArrayObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<F64ArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::F64ArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<F64ArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

//
// BoolArrayObject
//

SLAKE_API BoolArrayAccessorVarObject::BoolArrayAccessorVarObject(Runtime *rt, BoolArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, Type(ValueType::Bool), arrayObject) {
}

SLAKE_API BoolArrayAccessorVarObject::~BoolArrayAccessorVarObject() {}

SLAKE_API void BoolArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
	if (value.valueType != ValueType::Bool)
		throw MismatchedTypeError("Mismatched array element type");

	BoolArrayObject *arrayObject = ((BoolArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	arrayObject->data[index] = value.getBool();
}

SLAKE_API Value BoolArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	BoolArrayObject *arrayObject = ((BoolArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

SLAKE_API HostObjectRef<BoolArrayAccessorVarObject> slake::BoolArrayAccessorVarObject::alloc(Runtime *rt, BoolArrayObject *arrayObject) {
	using Alloc = std::pmr::polymorphic_allocator<BoolArrayAccessorVarObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<BoolArrayAccessorVarObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, arrayObject);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::BoolArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<BoolArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API void BoolArrayObject::_resizeUnchecked(size_t newLength) {
	bool *newData = (bool *)_rt->globalHeapPoolResource.allocate(
		sizeof(bool) * newLength);

	data = newData;
	length = newLength;
}

SLAKE_API void BoolArrayObject::clear() {
	if (data) {
		_rt->globalHeapPoolResource.deallocate(
			data,
			length * sizeof(bool));
		data = nullptr;
		length = 0;
	}
}

SLAKE_API void BoolArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	if (value.valueType != ValueType::Bool)
		throw MismatchedTypeError("Mismatched array element type");
	std::fill_n(data + beginIndex, length, value.getBool());
}

SLAKE_API void BoolArrayObject::resize(size_t newLength) {
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

SLAKE_API BoolArrayObject::BoolArrayObject(Runtime *rt, size_t length)
	: ArrayObject(rt, Type(ValueType::Bool), BoolArrayAccessorVarObject::alloc(rt, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(bool));
}

SLAKE_API BoolArrayObject::BoolArrayObject(const BoolArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(bool));
}

SLAKE_API BoolArrayObject::~BoolArrayObject() {
	clear();
}

Object *BoolArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<BoolArrayObject> slake::BoolArrayObject::alloc(Runtime *rt, size_t length) {
	using Alloc = std::pmr::polymorphic_allocator<BoolArrayObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<BoolArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, length);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<BoolArrayObject> slake::BoolArrayObject::alloc(const BoolArrayObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<BoolArrayObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<BoolArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::BoolArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<BoolArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

//
// ObjectRefArrayObject
//

SLAKE_API ObjectRefArrayAccessorVarObject::ObjectRefArrayAccessorVarObject(Runtime *rt, const Type &elementType, ObjectRefArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, elementType, arrayObject) {
}

SLAKE_API ObjectRefArrayAccessorVarObject::~ObjectRefArrayAccessorVarObject() {}

SLAKE_API void ObjectRefArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
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

SLAKE_API Value ObjectRefArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	ObjectRefArrayObject *arrayObject = ((ObjectRefArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

SLAKE_API HostObjectRef<ObjectRefArrayAccessorVarObject> slake::ObjectRefArrayAccessorVarObject::alloc(Runtime *rt, const Type &elementType, ObjectRefArrayObject *arrayObject) {
	using Alloc = std::pmr::polymorphic_allocator<ObjectRefArrayAccessorVarObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<ObjectRefArrayAccessorVarObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, elementType, arrayObject);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::ObjectRefArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<ObjectRefArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API void ObjectRefArrayObject::_resizeUnchecked(size_t newLength) {
	Object **newData = (Object **)_rt->globalHeapPoolResource.allocate(
		sizeof(Object *) * newLength);

	data = newData;
	length = newLength;
}

SLAKE_API void ObjectRefArrayObject::clear() {
	if (data) {
		_rt->globalHeapPoolResource.deallocate(
			data,
			length * sizeof(Object *));
		data = nullptr;
		length = 0;
	}
}

SLAKE_API void ObjectRefArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	if (value.valueType != ValueType::ObjectRef)
		throw MismatchedTypeError("Mismatched array element type");
	if (!isCompatible(elementType, value))
		throw MismatchedTypeError("Mismatched array element type");
	std::fill_n(data + beginIndex, length, value.getObjectRef());
}

SLAKE_API void ObjectRefArrayObject::resize(size_t newLength) {
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

SLAKE_API ObjectRefArrayObject::ObjectRefArrayObject(Runtime *rt, const Type &elementType, size_t length)
	: ArrayObject(rt, elementType, ObjectRefArrayAccessorVarObject::alloc(rt, elementType, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(Object *));
}

SLAKE_API ObjectRefArrayObject::ObjectRefArrayObject(const ObjectRefArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(Object *));
}

SLAKE_API ObjectRefArrayObject::~ObjectRefArrayObject() {
	clear();
}

Object *ObjectRefArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<ObjectRefArrayObject> slake::ObjectRefArrayObject::alloc(Runtime *rt, const Type &elementType, size_t length) {
	using Alloc = std::pmr::polymorphic_allocator<ObjectRefArrayObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<ObjectRefArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, elementType, length);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<ObjectRefArrayObject> slake::ObjectRefArrayObject::alloc(const ObjectRefArrayObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<ObjectRefArrayObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<ObjectRefArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::ObjectRefArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<ObjectRefArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

//
// AnyArrayObject
//

SLAKE_API AnyArrayAccessorVarObject::AnyArrayAccessorVarObject(Runtime *rt, AnyArrayObject *arrayObject)
	: ArrayAccessorVarObject(rt, Type(TypeId::Any), arrayObject) {
}

SLAKE_API AnyArrayAccessorVarObject::~AnyArrayAccessorVarObject() {}

SLAKE_API void AnyArrayAccessorVarObject::setData(const VarRefContext &varRefContext, const Value &value) {
	AnyArrayObject *arrayObject = ((AnyArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	arrayObject->data[index] = value;
}

SLAKE_API Value AnyArrayAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	AnyArrayObject *arrayObject = ((AnyArrayObject *)this->arrayObject);
	size_t index = varRefContext.asArray.index;
	if (index > arrayObject->length)
		throw OutOfRangeError();

	return Value(arrayObject->data[index]);
}

SLAKE_API HostObjectRef<AnyArrayAccessorVarObject> slake::AnyArrayAccessorVarObject::alloc(Runtime *rt, AnyArrayObject *arrayObject) {
	using Alloc = std::pmr::polymorphic_allocator<AnyArrayAccessorVarObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<AnyArrayAccessorVarObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, arrayObject);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::AnyArrayAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<AnyArrayAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API void AnyArrayObject::_resizeUnchecked(size_t newLength) {
	Value *newData = (Value *)_rt->globalHeapPoolResource.allocate(
		sizeof(Value) * newLength);

	data = newData;
	length = newLength;
}

SLAKE_API void AnyArrayObject::clear() {
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

SLAKE_API void AnyArrayObject::fill(size_t beginIndex, size_t length, const Value &value) {
	if (beginIndex + length > this->length)
		throw OutOfRangeError();
	std::fill_n(data + beginIndex, length, value);
}

SLAKE_API void AnyArrayObject::resize(size_t newLength) {
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

SLAKE_API AnyArrayObject::AnyArrayObject(Runtime *rt, size_t length)
	: ArrayObject(rt, Type(TypeId::Any), AnyArrayAccessorVarObject::alloc(rt, this).get()) {
	_resizeUnchecked(length);
	memset(data, 0, length * sizeof(Value));
}

SLAKE_API AnyArrayObject::AnyArrayObject(const AnyArrayObject &x) : ArrayObject(x) {
	_resizeUnchecked(x.length);
	memcpy(data, x.data, length * sizeof(Value));
}

SLAKE_API AnyArrayObject::~AnyArrayObject() {
	clear();
}

Object *AnyArrayObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<AnyArrayObject> slake::AnyArrayObject::alloc(Runtime *rt, size_t length) {
	using Alloc = std::pmr::polymorphic_allocator<AnyArrayObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<AnyArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, length);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<AnyArrayObject> slake::AnyArrayObject::alloc(const AnyArrayObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<AnyArrayObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<AnyArrayObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::AnyArrayObject::dealloc() {
	std::pmr::polymorphic_allocator<AnyArrayObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
