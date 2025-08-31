#include "runtime.h"

#include <cassert>

using namespace slake;

SLAKE_API TypeId slake::valueTypeToTypeId(ValueType valueType) {
	switch (valueType) {
		case ValueType::I8:
			return TypeId::I8;
		case ValueType::I16:
			return TypeId::I16;
		case ValueType::I32:
			return TypeId::I32;
		case ValueType::I64:
			return TypeId::I64;
		case ValueType::U8:
			return TypeId::U8;
		case ValueType::U16:
			return TypeId::U16;
		case ValueType::U32:
			return TypeId::U32;
		case ValueType::U64:
			return TypeId::U64;
		case ValueType::F32:
			return TypeId::F32;
		case ValueType::F64:
			return TypeId::F64;
		case ValueType::Bool:
			return TypeId::Bool;
		default:;
	}
	std::terminate();
}

SLAKE_API bool slake::isValueTypeCompatibleTypeId(TypeId typeId) {
	switch (typeId) {
		case TypeId::I8:
		case TypeId::I16:
		case TypeId::I32:
		case TypeId::I64:
		case TypeId::U8:
		case TypeId::U16:
		case TypeId::U32:
		case TypeId::U64:
		case TypeId::F32:
		case TypeId::F64:
		case TypeId::Bool:
			return true;
		default:;
	}
	return false;
}

SLAKE_API ValueType typeIdToValueType(TypeId typeId) {
	switch (typeId) {
		case TypeId::I8:
			return ValueType::I8;
		case TypeId::I16:
			return ValueType::I16;
		case TypeId::I32:
			return ValueType::I32;
		case TypeId::I64:
			return ValueType::I64;
		case TypeId::U8:
			return ValueType::U8;
		case TypeId::U16:
			return ValueType::U16;
		case TypeId::U32:
			return ValueType::U32;
		case TypeId::U64:
			return ValueType::U64;
		case TypeId::F32:
			return ValueType::F32;
		case TypeId::F64:
			return ValueType::F64;
		case TypeId::Bool:
			return ValueType::Bool;
		default:;
	}
	std::terminate();
}

SLAKE_API bool EntityRef::operator==(const EntityRef &rhs) const {
	if (kind != rhs.kind)
		return false;
	switch (kind) {
		case ObjectRefKind::FieldRef:
			if (asField.moduleObject != rhs.asField.moduleObject)
				return false;
			return asField.index == rhs.asField.index;
		case ObjectRefKind::ArrayElementRef:
			if (asArray.arrayObject != rhs.asArray.arrayObject)
				return false;
			return asField.index == rhs.asField.index;
		case ObjectRefKind::ObjectRef:
			return asObject.instanceObject == rhs.asObject.instanceObject;
		case ObjectRefKind::InstanceFieldRef:
			if (asObjectField.instanceObject != rhs.asObjectField.instanceObject)
				return false;
			return asObjectField.fieldIndex == rhs.asObjectField.fieldIndex;
		case ObjectRefKind::LocalVarRef:
			if (asLocalVar.context != rhs.asLocalVar.context)
				return false;
			return asLocalVar.stackOff == rhs.asLocalVar.stackOff;
		case ObjectRefKind::ArgRef:
			if (asArg.majorFrame != rhs.asArg.majorFrame)
				return false;
			return asArg.argIndex == rhs.asArg.argIndex;
		default:
			std::terminate();
	}
}

SLAKE_API bool EntityRef::operator<(const EntityRef &rhs) const {
	if (kind < rhs.kind)
		return true;
	if (kind > rhs.kind)
		return false;
	switch (kind) {
		case ObjectRefKind::FieldRef:
			if (asField.moduleObject < rhs.asField.moduleObject)
				return true;
			if (asField.moduleObject > rhs.asField.moduleObject)
				return false;
			return asField.index < rhs.asField.index;
		case ObjectRefKind::ArrayElementRef:
			if (asArray.arrayObject < rhs.asArray.arrayObject)
				return true;
			if (asArray.arrayObject > rhs.asArray.arrayObject)
				return false;
			return asField.index < rhs.asField.index;
		case ObjectRefKind::ObjectRef:
			return asObject.instanceObject < rhs.asObject.instanceObject;
		case ObjectRefKind::InstanceFieldRef:
			if (asObjectField.instanceObject < rhs.asObjectField.instanceObject)
				return true;
			if (asObjectField.instanceObject > rhs.asObjectField.instanceObject)
				return false;
			return asObjectField.fieldIndex < rhs.asObjectField.fieldIndex;
		case ObjectRefKind::LocalVarRef:
			if (asLocalVar.context < rhs.asLocalVar.context)
				return true;
			if (asLocalVar.context > rhs.asLocalVar.context)
				return false;
			return asLocalVar.stackOff < rhs.asLocalVar.stackOff;
		case ObjectRefKind::ArgRef:
			if (asArg.majorFrame < rhs.asArg.majorFrame)
				return true;
			if (asArg.majorFrame > rhs.asArg.majorFrame)
				return false;
			return asArg.argIndex < rhs.asArg.argIndex;
		case ObjectRefKind::AotPtrRef:
			return asAotPtr.ptr < rhs.asAotPtr.ptr;
		default:
			std::terminate();
	}
}

SLAKE_API Type Type::duplicate(bool &succeededOut) const {
	Type newType(*this);

	switch (typeId) {
		case TypeId::Array:
		case TypeId::Ref:
			if (!(newType.exData.typeDef = (TypeDefObject *)newType.exData.typeDef->duplicate(nullptr))) {
				succeededOut = false;
				return {};
			}
			break;
		case TypeId::GenericArg:
			if (!(newType.exData.genericArg.nameObject = (StringObject *)newType.exData.genericArg.nameObject->duplicate(nullptr))) {
				succeededOut = false;
				return {};
			}
			if (!(newType.exData.genericArg.ownerObject = newType.exData.genericArg.ownerObject->duplicate(nullptr))) {
				succeededOut = false;
				return {};
			}
			break;
		default:;
	}

	assert(verifyType(newType));

	succeededOut = true;

	return newType;
}

SLAKE_API Type &Type::getArrayExData() const {
	assert(typeId == TypeId::Array);
	assert(exData.typeDef && verifyObjectKind(exData.typeDef, ObjectKind::TypeDef));
	return exData.typeDef->type;
}
SLAKE_API Type &Type::getRefExData() const {
	assert(typeId == TypeId::Ref);
	assert(exData.typeDef && verifyObjectKind(exData.typeDef, ObjectKind::TypeDef));
	return exData.typeDef->type;
}
SLAKE_API Type &Type::getUnpackingExData() const {
	assert(typeId == TypeId::Unpacking);
	assert(exData.typeDef && verifyObjectKind(exData.typeDef, ObjectKind::ParamTypeListTypeDef));
	return exData.typeDef->type;
}

SLAKE_API bool Type::isLoadingDeferred() const noexcept {
	switch (typeId) {
		case TypeId::Instance:
			return getCustomTypeExData()->getObjectKind() == ObjectKind::IdRef;
		default:
			return false;
	}
}

SLAKE_API InternalExceptionPointer Type::loadDeferredType(Runtime *rt) {
	if (!isLoadingDeferred())
		return {};

	auto ref = (IdRefObject *)getCustomTypeExData();
	EntityRef entityRef;
	SLAKE_RETURN_IF_EXCEPT(rt->resolveIdRef(ref, entityRef));

	if (entityRef.kind != ObjectRefKind::ObjectRef)
		return allocOutOfMemoryErrorIfAllocFailed(InternalExceptionPointer(ReferencedMemberNotFoundError::alloc(rt->getFixedAlloc(), ref)));

	exData.object = entityRef.asObject.instanceObject;

	return {};
}

SLAKE_API InternalExceptionPointer slake::isCompatible(peff::Alloc *allocator, const Type &type, const Value &value, bool &resultOut) {
	if (type.typeId == TypeId::Any) {
		resultOut = true;
		return {};
	}

	switch (type.typeId) {
		case TypeId::I8:
		case TypeId::I16:
		case TypeId::I32:
		case TypeId::I64:
		case TypeId::U8:
		case TypeId::U16:
		case TypeId::U32:
		case TypeId::U64: {
			if (type.typeId != valueTypeToTypeId(value.valueType)) {
				resultOut = false;
				return {};
			}
			break;
		}
		case TypeId::String: {
			if (value.valueType != ValueType::EntityRef) {
				resultOut = false;
				return {};
			}
			const EntityRef &entityRef = value.getEntityRef();
			if (entityRef.kind != ObjectRefKind::ObjectRef) {
				resultOut = false;
				return {};
			}
			if (entityRef.asObject.instanceObject->getObjectKind() != ObjectKind::String) {
				resultOut = false;
				return {};
			}
			break;
		}
		case TypeId::Instance: {
			if (value.valueType != ValueType::EntityRef) {
				resultOut = false;
				return {};
			}

			const EntityRef &entityRef = value.getEntityRef();
			if (entityRef.kind != ObjectRefKind::ObjectRef) {
				resultOut = false;
				return {};
			}
			Object *objectPtr = entityRef.asObject.instanceObject;

			if (!objectPtr) {
				resultOut = true;
				return {};
			}

			if (auto e = const_cast<Type &>(type).loadDeferredType(objectPtr->associatedRuntime);
				e) {
				e.reset();
				resultOut = false;
				return {};
			}
			switch (type.getCustomTypeExData()->getObjectKind()) {
				case ObjectKind::Class: {
					ClassObject *thisClass = (ClassObject *)type.getCustomTypeExData();

					ClassObject *valueClass = ((InstanceObject *)objectPtr)->_class;

					if (!thisClass->isBaseOf(valueClass)) {
						resultOut = false;
						return {};
					}
					break;
				}
				case ObjectKind::Interface: {
					InterfaceObject *thisInterface = (InterfaceObject *)type.getCustomTypeExData();

					ClassObject *valueClass = ((InstanceObject *)objectPtr)->_class;

					if (!valueClass->hasImplemented(thisInterface)) {
						resultOut = false;
						return {};
					}
					break;
				}
				default:
					break;
			}

			break;
		}
		case TypeId::Array: {
			if (value.valueType != ValueType::EntityRef) {
				resultOut = false;
				return {};
			}

			const EntityRef &entityRef = value.getEntityRef();
			if (entityRef.kind != ObjectRefKind::ObjectRef) {
				resultOut = false;
				return {};
			}
			Object *objectPtr = entityRef.asObject.instanceObject;
			if (objectPtr->getObjectKind() != ObjectKind::Array) {
				resultOut = false;
				return {};
			}

			auto arrayObjectPtr = ((ArrayObject *)objectPtr);

			int result;

			SLAKE_RETURN_IF_EXCEPT(Runtime::compareType(allocator, arrayObjectPtr->elementType, type.getArrayExData(), result));

			if (result) {
				resultOut = false;
				return {};
			}
			break;
		}
		case TypeId::Ref: {
			if (value.valueType != ValueType::EntityRef) {
				resultOut = false;
				return {};
			}

			const EntityRef &entityRef = value.getEntityRef();
			Runtime *rt;
			switch (entityRef.kind) {
				case ObjectRefKind::FieldRef:
					rt = entityRef.asField.moduleObject->associatedRuntime;
					break;
				case ObjectRefKind::InstanceFieldRef:
					rt = entityRef.asObjectField.instanceObject->associatedRuntime;
					break;
				case ObjectRefKind::LocalVarRef:
					rt = entityRef.asLocalVar.context->runtime;
					break;
				case ObjectRefKind::ArrayElementRef:
					rt = entityRef.asArray.arrayObject->associatedRuntime;
					break;
				case ObjectRefKind::ArgRef:
					rt = entityRef.asArg.majorFrame->curFn->associatedRuntime;
					break;
				default:
					resultOut = false;
					return {};
			}
			Type type;

			InternalExceptionPointer e = rt->typeofVar(entityRef, type);
			if (e) {
				e.reset();
				resultOut = false;
				return {};
			}

			int result;

			SLAKE_RETURN_IF_EXCEPT(Runtime::compareType(allocator, type, type.getRefExData(), result));

			if (result) {
				resultOut = false;
				return {};
			}
			break;
		}
		default:
			resultOut = false;
			return {};
	}

	resultOut = true;
	return {};
}

SLAKE_API std::string std::to_string(const slake::Type &type, const slake::Runtime *rt) {
	switch (type.typeId) {
		case TypeId::I8:
			return "i8";
		case TypeId::I16:
			return "i16";
		case TypeId::I32:
			return "i32";
		case TypeId::I64:
			return "i64";
		case TypeId::U8:
			return "u8";
		case TypeId::U16:
			return "u16";
		case TypeId::U32:
			return "u32";
		case TypeId::U64:
			return "u64";
		case TypeId::F32:
			return "f32";
		case TypeId::F64:
			return "f64";
		case TypeId::Bool:
			return "bool";
		case TypeId::String:
			return "string";
		case TypeId::Array:
			return to_string(type.getArrayExData(), rt) + "[]";
		case TypeId::Ref:
			return to_string(type.getArrayExData(), rt) + "&";
		case TypeId::Instance: {
			if (type.isLoadingDeferred()) {
				return std::to_string((IdRefObject *)type.getCustomTypeExData());
			} else {
				peff::DynArray<slake::IdRefEntry> fullRef(peff::getDefaultAlloc());

				if (!rt->getFullRef(peff::getDefaultAlloc(), (MemberObject *)type.getCustomTypeExData(), fullRef)) {
					throw std::bad_alloc();
				}

				std::string name;

				for (size_t i = 0; i < fullRef.size(); ++i) {
					if (i) {
						name += '.';
					}

					slake::IdRefEntry &id = fullRef.at(i);

					name += id.name;

					if (id.genericArgs.size()) {
						name += '<';

						for (size_t j = 0; j < id.genericArgs.size(); ++j) {
							if (j)
								name += ",";
							name += std::to_string(id.genericArgs.at(j), rt);
						}

						name += '>';
					}
				}

				return name;
			}
		}
		case TypeId::GenericArg: {
			StringObject *nameObject = (StringObject *)type.exData.genericArg.nameObject;
			return "!" + std::string(nameObject->data);
		}
		case TypeId::Fn: {
			FnTypeDefObject *fnTypeDefObject = (FnTypeDefObject *)type.exData.typeDef;
			std::string result = "fn ";

			for (size_t i = 0; i < fnTypeDefObject->paramTypes.size(); ++i) {
				if (i)
					result += ", ";

				//result += std::to_string(fnTypeDefObject->paramTypes.at(i));
			}

			if (fnTypeDefObject->hasVarArg) {
				result += "...";
			}

			return result;
		}
		case TypeId::Any:
			return "any";
		case TypeId::Void:
			return "void";
		default:
			return "<Unknown Type>";
	}
}
