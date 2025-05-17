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

SLAKE_API Type Type::makeArrayTypeName(Runtime *runtime, const Type &elementType) {
	Type type;

	type.typeId = TypeId::Array;
	type.exData.typeDef = TypeDefObject::alloc(runtime, elementType).get();

	return type;
}

SLAKE_API Type Type::makeRefTypeName(Runtime *runtime, const Type &elementType) {
	Type type;

	type.typeId = TypeId::Ref;
	type.exData.typeDef = TypeDefObject::alloc(runtime, elementType).get();

	return type;
}

SLAKE_API Type Type::duplicate(bool &succeededOut) const {
	Type newType(*this);

	switch (typeId) {
		case TypeId::Array:
		case TypeId::Ref:
			if (!(newType.exData.typeDef = (TypeDefObject *)newType.exData.typeDef->duplicate())) {
				succeededOut = false;
				return {};
			}
			break;
		default:;
	}

	succeededOut = true;

	return newType;
}

SLAKE_API Type &Type::getArrayExData() const { return exData.typeDef->type; }
SLAKE_API Type &Type::getRefExData() const { return exData.typeDef->type; }

SLAKE_API bool Type::isLoadingDeferred() const noexcept {
	switch (typeId) {
		case TypeId::Instance:
			return getCustomTypeExData()->getKind() == ObjectKind::IdRef;
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
		return allocOutOfMemoryErrorIfAllocFailed(ReferencedMemberNotFoundError::alloc(&rt->globalHeapPoolAlloc, ref));

	exData.object = entityRef.asObject.instanceObject;

	return {};
}

SLAKE_API bool Type::operator<(const Type &rhs) const {
	if (typeId < rhs.typeId)
		return true;
	else if (typeId > rhs.typeId)
		return false;
	else {
		switch (rhs.typeId) {
			case TypeId::Instance: {
				auto lhsType = getCustomTypeExData(), rhsType = rhs.getCustomTypeExData();

				// TODO: Use comparison instead of the simple assert.
				assert(lhsType->getKind() == rhsType->getKind());
				switch (lhsType->getKind()) {
					case ObjectKind::IdRef: {
						IdRefObject *lhsRef = (IdRefObject *)lhsType,
									*rhsRef = (IdRefObject *)rhsType;

						if (lhsRef->entries.size() < rhsRef->entries.size())
							return true;
						if (lhsRef->entries.size() > rhsRef->entries.size())
							return false;

						GenericArgListComparator genericArgListComparator;

						for (size_t i = 0; i < lhsRef->entries.size(); ++i) {
							auto &curLhsRefEntry = lhsRef->entries.at(i),
								 &curRhsRefEntry = rhsRef->entries.at(i);

							if (curLhsRefEntry.name < curRhsRefEntry.name)
								return true;
							if (curLhsRefEntry.name > curRhsRefEntry.name)
								return false;

							if (genericArgListComparator(
									curLhsRefEntry.genericArgs,
									curRhsRefEntry.genericArgs))
								return true;
						}

						return false;
					}
				}

				return lhsType < rhsType;
			}
			case TypeId::Array:
				return getArrayExData() < rhs.getArrayExData();
			case TypeId::Ref:
				return getRefExData() < rhs.getRefExData();
			default:;
		}
	}

	return false;
}

SLAKE_API bool Type::operator==(const Type &rhs) const {
	if (rhs.typeId != typeId)
		return false;

	switch (rhs.typeId) {
		case TypeId::None:
		case TypeId::I8:
		case TypeId::I16:
		case TypeId::I32:
		case TypeId::I64:
		case TypeId::U8:
		case TypeId::U16:
		case TypeId::U32:
		case TypeId::U64:
			break;
		case TypeId::Instance: {
			auto lhsType = getCustomTypeExData(), rhsType = rhs.getCustomTypeExData();

			assert(lhsType->getKind() == rhsType->getKind());
			switch (lhsType->getKind()) {
				case ObjectKind::IdRef: {
					IdRefObject *lhsRef = (IdRefObject *)lhsType,
								*rhsRef = (IdRefObject *)rhsType;

					if (lhsRef->entries.size() != rhsRef->entries.size())
						return false;

					GenericArgListEqComparator genericArgListComparator;

					for (size_t i = 0; i < lhsRef->entries.size(); ++i) {
						auto &curLhsRefEntry = lhsRef->entries.at(i),
							 &curRhsRefEntry = rhsRef->entries.at(i);

						if (curLhsRefEntry.name != curRhsRefEntry.name)
							return false;

						if (!genericArgListComparator(
								curLhsRefEntry.genericArgs,
								curRhsRefEntry.genericArgs))
							return false;
					}

					break;
				}
				default:;
			}

			return lhsType == rhsType;
		}
		case TypeId::GenericArg: {
			auto lhsOwnerObject = exData.genericArg.ownerObject,
				 rhsOwnerObject = rhs.exData.genericArg.ownerObject;

			auto lhsObjectKind = lhsOwnerObject->getKind();

			auto lhsName = exData.genericArg.nameObject,
				 rhsName = rhs.exData.genericArg.nameObject;

			Object *lhsOwnerOut, *rhsOwnerOut;

			GenericParam *lhsGenericParam = getGenericParam(lhsOwnerObject, lhsName->data, &lhsOwnerOut),
						 *rhsGenericParam = getGenericParam(rhsOwnerObject, rhsName->data, &rhsOwnerOut);

			if ((!lhsGenericParam) ||
				(!rhsGenericParam))
				return false;

			switch (lhsOwnerOut->getKind()) {
				case ObjectKind::Class:
				case ObjectKind::Interface:
					return lhsGenericParam == rhsGenericParam;
				case ObjectKind::FnOverloading: {
					auto l = (FnOverloadingObject *)lhsOwnerOut;

					switch (rhsOwnerOut->getKind()) {
						case ObjectKind::Class:
						case ObjectKind::Interface:
							return false;
						case ObjectKind::FnOverloading: {
							auto r = (FnOverloadingObject *)rhsOwnerOut;
							return getGenericParamIndex(l->genericParams, lhsName->data) == getGenericParamIndex(r->genericParams, rhsName->data);
						}
					}

					break;
				}
				default:;
			}

			return false;
		}
		case TypeId::Array:
			return getArrayExData() == rhs.getArrayExData();
		case TypeId::Ref:
			return getRefExData() == rhs.getRefExData();
		default:;
	}
	return true;
}

SLAKE_API bool slake::isCompatible(const Type &type, const Value &value) {
	if (type.typeId == TypeId::Any)
		return true;

	switch (type.typeId) {
		case TypeId::I8:
		case TypeId::I16:
		case TypeId::I32:
		case TypeId::I64:
		case TypeId::U8:
		case TypeId::U16:
		case TypeId::U32:
		case TypeId::U64: {
			if (type.typeId != valueTypeToTypeId(value.valueType))
				return false;
			break;
		}
		case TypeId::String: {
			if (value.valueType != ValueType::EntityRef)
				return false;
			const EntityRef &entityRef = value.getEntityRef();
			if (entityRef.kind != ObjectRefKind::ObjectRef)
				return false;
			if (entityRef.asObject.instanceObject->getKind() != ObjectKind::String)
				return false;
			break;
		}
		case TypeId::Instance: {
			if (value.valueType != ValueType::EntityRef)
				return false;

			const EntityRef &entityRef = value.getEntityRef();
			if (entityRef.kind != ObjectRefKind::ObjectRef)
				return false;
			Object *objectPtr = entityRef.asObject.instanceObject;

			if (!objectPtr) {
				return true;
			}

			if (auto e = const_cast<Type &>(type).loadDeferredType(objectPtr->associatedRuntime);
				e) {
				e.reset();
				return false;
			}
			switch (type.getCustomTypeExData()->getKind()) {
				case ObjectKind::Class: {
					ClassObject *thisClass = (ClassObject *)type.getCustomTypeExData();

					ClassObject *valueClass = ((InstanceObject *)objectPtr)->_class;

					if (!thisClass->isBaseOf(valueClass))
						return false;
					break;
				}
				case ObjectKind::Interface: {
					InterfaceObject *thisInterface = (InterfaceObject *)type.getCustomTypeExData();

					ClassObject *valueClass = ((InstanceObject *)objectPtr)->_class;

					if (!valueClass->hasImplemented(thisInterface))
						return false;
					break;
				}
				default:
					break;
			}

			break;
		}
		case TypeId::Array: {
			if (value.valueType != ValueType::EntityRef)
				return false;

			const EntityRef &entityRef = value.getEntityRef();
			if (entityRef.kind != ObjectRefKind::ObjectRef)
				return false;
			Object *objectPtr = entityRef.asObject.instanceObject;
			if (objectPtr->getKind() != ObjectKind::Array)
				return false;

			auto arrayObjectPtr = ((ArrayObject *)objectPtr);

			if (arrayObjectPtr->elementType != type.getArrayExData())
				return false;
			break;
		}
		case TypeId::Ref: {
			if (value.valueType != ValueType::EntityRef)
				return false;

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
					break;
				case ObjectRefKind::ArrayElementRef:
					rt = entityRef.asArray.arrayObject->associatedRuntime;
					break;
				case ObjectRefKind::ArgRef:
					rt = entityRef.asArg.majorFrame->curFn->associatedRuntime;
					break;
				default:
					return false;
			}
			Type type;

			InternalExceptionPointer e = rt->typeofVar(entityRef, type);
			if (e) {
				e.reset();
				return false;
			}

			if (type != type.getRefExData())
				return false;
			break;
		}
		default:
			return false;
	}

	return true;
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
		case TypeId::FnDelegate: {
			FnTypeDefObject *fnTypeDefObject = (FnTypeDefObject *)type.exData.typeDef;
			std::string result = "fn ";

			for (size_t i = 0; i < fnTypeDefObject->paramTypes.size(); ++i) {
				if (i)
					result += ", ";

				result += std::to_string(fnTypeDefObject->paramTypes.at(i));
			}

			if (fnTypeDefObject->hasVarArg) {
				result += "...";
			}

			return result;
		}
		case TypeId::Any:
			return "any";
		case TypeId::None:
			return "void";
		default:
			return "<Unknown Type>";
	}
}
