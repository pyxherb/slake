#include "var.h"
#include <slake/runtime.h>

using namespace slake;

SLAKE_API VarObject::VarObject(Runtime *rt) : MemberObject(rt) {
}

SLAKE_API VarObject::VarObject(const VarObject &x) : MemberObject(x) {
}

SLAKE_API VarObject::~VarObject() {
}

SLAKE_API ObjectKind VarObject::getKind() const { return ObjectKind::Var; }

SLAKE_API slake::RegularVarObject::RegularVarObject(Runtime *rt, AccessModifier access, const Type &type)
	: VarObject(rt), type(type) {
	this->accessModifier = access;
}

SLAKE_API RegularVarObject::RegularVarObject(const RegularVarObject &other) : VarObject(other) {
	value = other.value;
	type = other.type;

	name = other.name;
	parent = other.parent;
}

SLAKE_API RegularVarObject::~RegularVarObject() {
}

SLAKE_API Object *RegularVarObject::duplicate() const {
	return (Object *)(VarObject *)alloc(this).get();
}

SLAKE_API const char *RegularVarObject::getName() const {
	return name.c_str();
}

SLAKE_API void RegularVarObject::setName(const char *name) {
	this->name = name;
}

SLAKE_API Object *RegularVarObject::getParent() const {
	return parent;
}

SLAKE_API void RegularVarObject::setParent(Object *parent) {
	this->parent = parent;
}

SLAKE_API void slake::RegularVarObject::dealloc() {
	std::pmr::polymorphic_allocator<RegularVarObject> allocator(&VarObject::associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API bool RegularVarObject::getData(const VarRefContext &context, Value &valueOut) const {
	valueOut = value;
	return true;
}

SLAKE_API bool RegularVarObject::setData(const VarRefContext &context, const Value &value) {
	if (!isCompatible(type, value)) {
		raiseMismatchedVarTypeError(associatedRuntime);
		return false;
	}
	this->value = value;
	return true;
}

SLAKE_API ObjectKind RegularVarObject::getKind() const { return ObjectKind::Var; }

SLAKE_API HostObjectRef<RegularVarObject> slake::RegularVarObject::alloc(Runtime *rt, AccessModifier access, const Type &type) {
	using Alloc = std::pmr::polymorphic_allocator<RegularVarObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<RegularVarObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, access, type);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<RegularVarObject> slake::RegularVarObject::alloc(const RegularVarObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<RegularVarObject>;
	Alloc allocator(&other->associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<RegularVarObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->Object::associatedRuntime->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API LocalVarAccessorVarObject::LocalVarAccessorVarObject(
	Runtime *rt,
	Context *context,
	MajorFrame *majorFrame)
	: VarObject(rt), context(context), majorFrame(majorFrame) {
}

SLAKE_API LocalVarAccessorVarObject::~LocalVarAccessorVarObject() {
}

SLAKE_API Type LocalVarAccessorVarObject::getVarType(const VarRefContext &context) const {
	return majorFrame->localVarRecords[context.asLocalVar.localVarIndex].type;
}

SLAKE_API VarKind LocalVarAccessorVarObject::getVarKind() const { return VarKind::LocalVarAccessor; }

SLAKE_API bool LocalVarAccessorVarObject::setData(const VarRefContext &context, const Value &value) {
	LocalVarRecord &localVarRecord =
		majorFrame->localVarRecords[context.asLocalVar.localVarIndex];

	if (!isCompatible(localVarRecord.type, value)) {
		raiseMismatchedVarTypeError(associatedRuntime);
		return false;
	}

	char *rawDataPtr = this->context->dataStack + localVarRecord.stackOffset;

	switch (localVarRecord.type.typeId) {
		case TypeId::Value:
			switch (localVarRecord.type.getValueTypeExData()) {
				case ValueType::I8:
					*((int8_t *)rawDataPtr) = value.getI8();
					break;
				case ValueType::I16:
					*((int16_t *)rawDataPtr) = value.getI16();
					break;
				case ValueType::I32:
					*((int32_t *)rawDataPtr) = value.getI32();
					break;
				case ValueType::I64:
					*((int64_t *)rawDataPtr) = value.getI64();
					break;
				case ValueType::U8:
					*((uint8_t *)rawDataPtr) = value.getU8();
					break;
				case ValueType::U16:
					*((uint16_t *)rawDataPtr) = value.getU16();
					break;
				case ValueType::U32:
					*((uint32_t *)rawDataPtr) = value.getU32();
					break;
				case ValueType::U64:
					*((uint64_t *)rawDataPtr) = value.getU64();
					break;
				case ValueType::F32:
					*((float *)rawDataPtr) = value.getF32();
					break;
				case ValueType::F64:
					*((double *)rawDataPtr) = value.getF64();
					break;
				case ValueType::Bool:
					*((bool *)rawDataPtr) = value.getBool();
					break;
			}
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			*((Object **)rawDataPtr) = value.getObjectRef();
			break;
		default:
			// All fields should be checked during the instantiation.
			throw std::logic_error("Unhandled value type");
	}
	return true;
}

SLAKE_API bool LocalVarAccessorVarObject::getData(const VarRefContext &varRefContext, Value &valueOut) const {
	LocalVarRecord &localVarRecord =
		majorFrame->localVarRecords[varRefContext.asLocalVar.localVarIndex];

	char *rawDataPtr = this->context->dataStack + localVarRecord.stackOffset;

	switch (localVarRecord.type.typeId) {
		case TypeId::Value:
			switch (localVarRecord.type.getValueTypeExData()) {
				case ValueType::I8:
					valueOut = Value(*((int8_t *)rawDataPtr));
					break;
				case ValueType::I16:
					valueOut = Value(*((int16_t *)rawDataPtr));
					break;
				case ValueType::I32:
					valueOut = Value(*((int32_t *)rawDataPtr));
					break;
				case ValueType::I64:
					valueOut = Value(*((int64_t *)rawDataPtr));
					break;
				case ValueType::U8:
					valueOut = Value(*((uint8_t *)rawDataPtr));
					break;
				case ValueType::U16:
					valueOut = Value(*((uint16_t *)rawDataPtr));
					break;
				case ValueType::U32:
					valueOut = Value(*((uint32_t *)rawDataPtr));
					break;
				case ValueType::U64:
					valueOut = Value(*((uint64_t *)rawDataPtr));
					break;
				case ValueType::F32:
					valueOut = Value(*((float *)rawDataPtr));
					break;
				case ValueType::F64:
					valueOut = Value(*((double *)rawDataPtr));
					break;
				case ValueType::Bool:
					valueOut = Value(*((bool *)rawDataPtr));
					break;
			}
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			valueOut = Value(*((Object **)rawDataPtr));
			break;
		default:
			// All fields should be checked during the instantiation.
			throw std::logic_error("Unhandled value type");
	}

	return true;
}

SLAKE_API HostObjectRef<LocalVarAccessorVarObject> slake::LocalVarAccessorVarObject::alloc(
	Runtime *rt,
	Context *context,
	MajorFrame *majorFrame) {
	using Alloc = std::pmr::polymorphic_allocator<LocalVarAccessorVarObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<LocalVarAccessorVarObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, context, majorFrame);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::LocalVarAccessorVarObject::dealloc() {
	std::pmr::polymorphic_allocator<LocalVarAccessorVarObject> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

void slake::raiseMismatchedVarTypeError(Runtime *rt) {
	rt->setThreadLocalInternalException(
		std::this_thread::get_id(),
		MismatchedVarTypeError::alloc(rt));
}
