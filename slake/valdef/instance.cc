#include "instance.h"

#include <slake/runtime.h>

using namespace slake;

InstanceMemberAccessorVarObject::InstanceMemberAccessorVarObject(
	Runtime *rt,
	InstanceObject *instanceObject)
	: VarObject(rt), instanceObject(instanceObject) {
}

InstanceMemberAccessorVarObject::~InstanceMemberAccessorVarObject() {
}

Type InstanceMemberAccessorVarObject::getVarType(const VarRefContext &context) const {
	return instanceObject->objectLayout->fieldRecords[context.asInstance.fieldIndex].type;
}

void InstanceMemberAccessorVarObject::setData(const VarRefContext &context, const Value &value) {
	ObjectFieldRecord &fieldRecord =
		instanceObject->objectLayout->fieldRecords.at(
			context.asInstance.fieldIndex);

	if (!isCompatible(fieldRecord.type, value))
		throw MismatchedTypeError("Mismatched variable type");

	char *rawFieldPtr = instanceObject->rawFieldData + fieldRecord.offset;

	switch (fieldRecord.type.typeId) {
		case TypeId::Value:
			switch (fieldRecord.type.getValueTypeExData()) {
				case ValueType::I8:
					*((int8_t *)rawFieldPtr) = value.getI8();
					break;
				case ValueType::I16:
					*((int16_t *)rawFieldPtr) = value.getI16();
					break;
				case ValueType::I32:
					*((int32_t *)rawFieldPtr) = value.getI32();
					break;
				case ValueType::I64:
					*((int64_t *)rawFieldPtr) = value.getI64();
					break;
				case ValueType::U8:
					*((uint8_t *)rawFieldPtr) = value.getU8();
					break;
				case ValueType::U16:
					*((uint16_t *)rawFieldPtr) = value.getU16();
					break;
				case ValueType::U32:
					*((uint32_t *)rawFieldPtr) = value.getU32();
					break;
				case ValueType::U64:
					*((uint64_t *)rawFieldPtr) = value.getU64();
					break;
				case ValueType::F32:
					*((float *)rawFieldPtr) = value.getF32();
					break;
				case ValueType::F64:
					*((double *)rawFieldPtr) = value.getF64();
					break;
				case ValueType::Bool:
					*((bool *)rawFieldPtr) = value.getBool();
					break;
			}
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			*((Object **)rawFieldPtr) = value.getObjectRef();
			break;
		default:
			// All fields should be checked during the instantiation.
			assert(false);
	}
}

Value InstanceMemberAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	ObjectFieldRecord &fieldRecord =
		instanceObject->objectLayout->fieldRecords.at(
			varRefContext.asInstance.fieldIndex);

	char *rawFieldPtr = instanceObject->rawFieldData + fieldRecord.offset;

	switch (fieldRecord.type.typeId) {
		case TypeId::Value:
			switch (fieldRecord.type.getValueTypeExData()) {
				case ValueType::I8:
					return Value(*((int8_t *)rawFieldPtr));
				case ValueType::I16:
					return Value(*((int16_t *)rawFieldPtr));
				case ValueType::I32:
					return Value(*((int32_t *)rawFieldPtr));
				case ValueType::I64:
					return Value(*((int64_t *)rawFieldPtr));
				case ValueType::U8:
					return Value(*((uint8_t *)rawFieldPtr));
				case ValueType::U16:
					return Value(*((uint16_t *)rawFieldPtr));
				case ValueType::U32:
					return Value(*((uint32_t *)rawFieldPtr));
				case ValueType::U64:
					return Value(*((uint64_t *)rawFieldPtr));
				case ValueType::F32:
					return Value(*((float *)rawFieldPtr));
				case ValueType::F64:
					return Value(*((double *)rawFieldPtr));
				case ValueType::Bool:
					return Value(*((bool *)rawFieldPtr));
			}
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			return Value(*((Object **)rawFieldPtr));
		default:
			// All fields should be checked during the instantiation.
			assert(false);
	}
}

HostObjectRef<InstanceMemberAccessorVarObject> slake::InstanceMemberAccessorVarObject::alloc(Runtime *rt, InstanceObject *instanceObject) {
	using Alloc = std::pmr::polymorphic_allocator<InstanceMemberAccessorVarObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<InstanceMemberAccessorVarObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, instanceObject);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

void slake::InstanceMemberAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<InstanceMemberAccessorVarObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

InstanceObject::InstanceObject(Runtime *rt)
	: Object(rt) {
	memberAccessor = InstanceMemberAccessorVarObject::alloc(rt, this).get();
}

InstanceObject::InstanceObject(const InstanceObject &x) : Object(x) {
	_class = x._class;
	objectLayout = x.objectLayout;
	methodTable = x.methodTable;
	// TODO: Copy the rawFieldData.
}

InstanceObject::~InstanceObject() {
	if (rawFieldData)
		delete[] rawFieldData;

	// DO NOT DELETE THE OBJECT LAYOUT AND THE METHOD TABLE!!!
	// They are borrowed from the class.
}

ObjectKind InstanceObject::getKind() const { return ObjectKind::Instance; }

Object *InstanceObject::duplicate() const {
	return (Object *)alloc(this).get();
}

MemberObject *InstanceObject::getMember(
	const std::pmr::string &name,
	VarRefContext *varRefContextOut) const {
	if (auto it = methodTable->methods.find(name);
		it != methodTable->methods.end())
		return it->second;

	if (auto it = objectLayout->fieldNameMap.find(name);
		it != objectLayout->fieldNameMap.end()) {
		if (varRefContextOut) {
			*varRefContextOut = VarRefContext::makeInstanceContext(it->second);
		}

		return memberAccessor;
	}

	return nullptr;
}

HostObjectRef<InstanceObject> slake::InstanceObject::alloc(Runtime *rt) {
	using Alloc = std::pmr::polymorphic_allocator<InstanceObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<InstanceObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

HostObjectRef<InstanceObject> slake::InstanceObject::alloc(const InstanceObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<InstanceObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<InstanceObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

void slake::InstanceObject::dealloc() {
	std::pmr::polymorphic_allocator<InstanceObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
