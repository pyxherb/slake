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

SLAKE_API Value RegularVarObject::getData(const VarRefContext &context) const { return value; }

SLAKE_API void RegularVarObject::setData(const VarRefContext &context, const Value &value) {
	if (!isCompatible(type, value))
		throw MismatchedTypeError("Mismatched variable type");
	this->value = value;
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

SLAKE_API void LocalVarAccessorVarObject::setData(const VarRefContext &context, const Value &value) {
	LocalVarRecord &localVarRecord =
		majorFrame->localVarRecords[context.asLocalVar.localVarIndex];

	if (!isCompatible(localVarRecord.type, value))
		throw MismatchedTypeError("Mismatched variable type");

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
}

SLAKE_API Value LocalVarAccessorVarObject::getData(const VarRefContext &varRefContext) const {
	LocalVarRecord &localVarRecord =
		majorFrame->localVarRecords[varRefContext.asLocalVar.localVarIndex];

	char *rawDataPtr = this->context->dataStack + localVarRecord.stackOffset;

	switch (localVarRecord.type.typeId) {
		case TypeId::Value:
			switch (localVarRecord.type.getValueTypeExData()) {
				case ValueType::I8:
					return Value(*((int8_t *)rawDataPtr));
				case ValueType::I16:
					return Value(*((int16_t *)rawDataPtr));
				case ValueType::I32:
					return Value(*((int32_t *)rawDataPtr));
				case ValueType::I64:
					return Value(*((int64_t *)rawDataPtr));
				case ValueType::U8:
					return Value(*((uint8_t *)rawDataPtr));
				case ValueType::U16:
					return Value(*((uint16_t *)rawDataPtr));
				case ValueType::U32:
					return Value(*((uint32_t *)rawDataPtr));
				case ValueType::U64:
					return Value(*((uint64_t *)rawDataPtr));
				case ValueType::F32:
					return Value(*((float *)rawDataPtr));
				case ValueType::F64:
					return Value(*((double *)rawDataPtr));
				case ValueType::Bool:
					return Value(*((bool *)rawDataPtr));
			}
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			return Value(*((Object **)rawDataPtr));
		default:
			// All fields should be checked during the instantiation.
			;
	}
	throw std::logic_error("Unhandled value type");
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
