#include "instance.h"

#include <slake/runtime.h>

using namespace slake;

SLAKE_API InstanceMemberAccessorVarObject::InstanceMemberAccessorVarObject(
	Runtime *rt,
	InstanceObject *instanceObject)
	: VarObject(rt), instanceObject(instanceObject) {
}

SLAKE_API InstanceMemberAccessorVarObject::~InstanceMemberAccessorVarObject() {
}

SLAKE_API Type InstanceMemberAccessorVarObject::getVarType(const VarRefContext &context) const {
	return instanceObject->objectLayout->fieldRecords[context.asInstance.fieldIndex].type;
}

SLAKE_API InternalExceptionPointer InstanceMemberAccessorVarObject::setData(const VarRefContext &context, const Value &value) {
	ObjectFieldRecord &fieldRecord =
		instanceObject->objectLayout->fieldRecords.at(
			context.asInstance.fieldIndex);

	if (!isCompatible(fieldRecord.type, value)) {
		return raiseMismatchedVarTypeError(associatedRuntime);
	}

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

	return {};
}

SLAKE_API InternalExceptionPointer InstanceMemberAccessorVarObject::getData(const VarRefContext &varRefContext, Value &valueOut) const {
	ObjectFieldRecord &fieldRecord =
		instanceObject->objectLayout->fieldRecords.at(
			varRefContext.asInstance.fieldIndex);

	char *rawFieldPtr = instanceObject->rawFieldData + fieldRecord.offset;

	switch (fieldRecord.type.typeId) {
		case TypeId::Value:
			switch (fieldRecord.type.getValueTypeExData()) {
				case ValueType::I8:
					valueOut = Value(*((int8_t *)rawFieldPtr));
					break;
				case ValueType::I16:
					valueOut = Value(*((int16_t *)rawFieldPtr));
					break;
				case ValueType::I32:
					valueOut = Value(*((int32_t *)rawFieldPtr));
					break;
				case ValueType::I64:
					valueOut = Value(*((int64_t *)rawFieldPtr));
					break;
				case ValueType::U8:
					valueOut = Value(*((uint8_t *)rawFieldPtr));
					break;
				case ValueType::U16:
					valueOut = Value(*((uint16_t *)rawFieldPtr));
					break;
				case ValueType::U32:
					valueOut = Value(*((uint32_t *)rawFieldPtr));
					break;
				case ValueType::U64:
					valueOut = Value(*((uint64_t *)rawFieldPtr));
					break;
				case ValueType::F32:
					valueOut = Value(*((float *)rawFieldPtr));
					break;
				case ValueType::F64:
					valueOut = Value(*((double *)rawFieldPtr));
					break;
				case ValueType::Bool:
					valueOut = Value(*((bool *)rawFieldPtr));
					break;
			}
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			valueOut = Value(*((Object **)rawFieldPtr));
			break;
		default:
			// All fields should be checked during the instantiation.
			throw std::logic_error("Unhandled value type");
	}
	return {};
}

SLAKE_API HostObjectRef<InstanceMemberAccessorVarObject> slake::InstanceMemberAccessorVarObject::alloc(Runtime *rt, InstanceObject *instanceObject) {
	using Alloc = std::pmr::polymorphic_allocator<InstanceMemberAccessorVarObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<InstanceMemberAccessorVarObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, instanceObject);

	rt->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::InstanceMemberAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<InstanceMemberAccessorVarObject> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API InstanceObject::InstanceObject(Runtime *rt)
	: Object(rt) {
	memberAccessor = InstanceMemberAccessorVarObject::alloc(rt, this).get();
}

SLAKE_API InstanceObject::InstanceObject(const InstanceObject &x) : Object(x) {
	_class = x._class;
	objectLayout = x.objectLayout;
	methodTable = x.methodTable;
	// TODO: Copy the rawFieldData.
}

SLAKE_API InstanceObject::~InstanceObject() {
	if (rawFieldData)
		delete[] rawFieldData;

	// DO NOT DELETE THE OBJECT LAYOUT AND THE METHOD TABLE!!!
	// They are borrowed from the class.
}

SLAKE_API ObjectKind InstanceObject::getKind() const { return ObjectKind::Instance; }

SLAKE_API Object *InstanceObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API MemberObject *InstanceObject::getMember(
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

SLAKE_API HostObjectRef<InstanceObject> slake::InstanceObject::alloc(Runtime *rt) {
	using Alloc = std::pmr::polymorphic_allocator<InstanceObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<InstanceObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt);

	rt->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<InstanceObject> slake::InstanceObject::alloc(const InstanceObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<InstanceObject>;
	Alloc allocator(&other->associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<InstanceObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->associatedRuntime->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::InstanceObject::dealloc() {
	std::pmr::polymorphic_allocator<InstanceObject> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
