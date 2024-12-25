#include <slake/runtime.h>

using namespace slake;

SLAKE_API void Runtime::invalidateGenericCache(Object *i) {
	if (_genericCacheLookupTable.count(i)) {
		// Remove the value from generic cache if it is unreachable.
		auto &lookupEntry = _genericCacheLookupTable.at(i);

		auto &table = _genericCacheDir.at(lookupEntry.originalObject);
		table.erase(lookupEntry.genericArgs);

		if (!table.size())
			_genericCacheLookupTable.erase(lookupEntry.originalObject);

		_genericCacheLookupTable.erase(i);
	}
}

SLAKE_API void Runtime::setGenericCache(const Object *object, const GenericArgList &genericArgs, Object *instantiatedObject) {
	// Store the instance into the cache.
	_genericCacheDir[object][genericArgs] = instantiatedObject;
	_genericCacheLookupTable[instantiatedObject] = { object, genericArgs };
}

SLAKE_API InternalExceptionPointer slake::Runtime::_instantiateGenericObject(Type &type, GenericInstantiationContext &instantiationContext) {
	switch (type.typeId) {
		case TypeId::Instance: {
			if (type.isLoadingDeferred()) {
				IdRefObject *exData = (IdRefObject *)type.getCustomTypeExData();
				for (auto &i : exData->entries) {
					for (auto &j : i.genericArgs) {
						SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(j, instantiationContext));
					}
				}
			} else {
				auto idRefToResolvedType = getFullRef((MemberObject *)type.getCustomTypeExData());

				HostObjectRef<IdRefObject> idRefObject = IdRefObject::alloc((Runtime *)this);

				// TODO: If we use std::pmr version for getFullRef, remove these code and use following code:
				// idRefObject->entries = idRefToResolvedType;
				idRefObject->entries.resize(idRefToResolvedType.size(), IdRefEntry(&globalHeapPoolResource));
				for (size_t i = 0; i < idRefToResolvedType.size(); ++i)
					idRefObject->entries[i] = idRefToResolvedType[i];

				// TODO: Add HostRefHolder for idRefObject.
				type = Type(type.typeId, idRefObject.get());

				SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(type, instantiationContext));
			}
			break;
		}
		case TypeId::Array:
			type = type.duplicate();
			SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(type.getArrayExData(), instantiationContext));
			break;
		case TypeId::Ref:
			type = type.duplicate();
			SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(type.getRefExData(), instantiationContext));
			break;
		case TypeId::GenericArg: {
			HostObjectRef<StringObject> nameObject = (StringObject *)type.getCustomTypeExData();
			if (auto it = instantiationContext.mappedGenericArgs.find(nameObject->data); it != instantiationContext.mappedGenericArgs.end()) {
				if (it->second.typeId != TypeId::None)
					type = it->second;
			} else {
				std::pmr::string paramName(nameObject->data);

				SLAKE_RETURN_IF_EXCEPT(GenericParameterNotFoundError::alloc(
					const_cast<Runtime *>(this),
					std::move(paramName)));
			}
		}
	}
	return {};
}

SLAKE_API InternalExceptionPointer slake::Runtime::_instantiateGenericObject(Value &value, GenericInstantiationContext &instantiationContext) {
	switch (value.valueType) {
		case ValueType::I8:
		case ValueType::I16:
		case ValueType::I32:
		case ValueType::I64:
		case ValueType::U8:
		case ValueType::U16:
		case ValueType::U32:
		case ValueType::U64:
		case ValueType::F32:
		case ValueType::F64:
		case ValueType::Bool:
		case ValueType::ObjectRef:
		case ValueType::RegRef:
			break;
		case ValueType::TypeName:
			SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(value.getTypeName(), instantiationContext));
			break;
		case ValueType::Undefined:
			break;
		default:
			throw std::logic_error("Unhandled value type");
	}
	return {};
}

SLAKE_API InternalExceptionPointer slake::Runtime::_instantiateGenericObject(Object *v, GenericInstantiationContext &instantiationContext) {
	// How do we instantiate generic classes:
	// Duplicate the value, scan for references to generic parameters and
	// replace them with generic arguments.
	switch (v->getKind()) {
		case ObjectKind::Class: {
			ClassObject *const value = (ClassObject *)v;

			if (value->genericParams.size() && value != instantiationContext.mappedObject) {
				GenericInstantiationContext newInstantiationContext = instantiationContext;

				// Map irreplaceable parameters to corresponding generic parameter reference type
				// and thus the generic types will keep unchanged.
				for (size_t i = 0; i < value->genericParams.size(); ++i) {
					newInstantiationContext.mappedGenericArgs[value->genericParams[i].name] = TypeId::None;
				}

				newInstantiationContext.mappedObject = value;

				SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(value->parentClass, newInstantiationContext));

				SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(value, newInstantiationContext));
			} else {
				value->genericArgs = *instantiationContext.genericArgs;

				SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(value->parentClass, instantiationContext));

				for (auto &i : value->scope->members)
					SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(i.second, instantiationContext));
			}

			break;
		}
		case ObjectKind::Interface: {
			InterfaceObject *const value = (InterfaceObject *)v;

			value->genericArgs = *instantiationContext.genericArgs;

			for (auto &i : value->scope->members) {
				SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(i.second, instantiationContext));
			}

			break;
		}
		case ObjectKind::Var: {
			VarObject *value = (VarObject *)v;

			switch (value->varKind) {
				case VarKind::Regular: {
					RegularVarObject *v = (RegularVarObject *)value;
					SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(v->type, instantiationContext));
					break;
				}
				case VarKind::ArrayElementAccessor: {
					ArrayAccessorVarObject *v = (ArrayAccessorVarObject *)value;
					SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(v->arrayObject, instantiationContext));
					break;
				}
			}
			break;
		}
		case ObjectKind::Module: {
			ModuleObject *value = (ModuleObject *)v;

			for (auto &i : value->scope->members) {
				SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(i.second, instantiationContext));
			}
			break;
		}
		case ObjectKind::Fn: {
			FnObject *value = (FnObject *)v;

			if (instantiationContext.mappedObject == value) {
				FnOverloadingObject *matchedOverloading = nullptr, *matchedSpecializedOverloading = nullptr;

				for (auto i : value->overloadings) {
					if (i->genericParams.size() != instantiationContext.genericArgs->size()) {
						continue;
					}

					assert(instantiationContext.genericArgs->size() == i->specializationArgs.size());

					if (i->specializationArgs.size()) {
						for (size_t j = 0; j < i->specializationArgs.size(); ++j) {
							auto &curType = i->specializationArgs[j];

							if (curType.typeId == TypeId::GenericArg)
								throw std::runtime_error("Specialization argument must be deterministic");

							SLAKE_RETURN_IF_EXCEPT(curType.loadDeferredType(this));
							if (curType != instantiationContext.genericArgs->at(j)) {
								goto specializationArgsMismatched;
							}
						}

						SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(i, instantiationContext));
						matchedSpecializedOverloading = i;
						break;
					} else {
						SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(i, instantiationContext));
						matchedOverloading = i;
					}

				specializationArgsMismatched:;
				}

				value->overloadings.insert(
					matchedSpecializedOverloading ? matchedSpecializedOverloading : matchedOverloading);
			} else {
				for (auto i : value->overloadings) {
					SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(i, instantiationContext));
				}
			}
			break;
		}
		case ObjectKind::Alias: {
			AliasObject *value = (AliasObject *)v;

			SLAKE_RETURN_IF_EXCEPT(instantiateGenericObject(value->src, value->src, instantiationContext));
			break;
		}
		case ObjectKind::String:
		case ObjectKind::RootObject:
		case ObjectKind::IdRef:
			break;
		default:
			throw std::logic_error("Unhandled object type");
	}
	return {};
}

InternalExceptionPointer Runtime::mapGenericParams(const Object *v, GenericInstantiationContext &instantiationContext) const {
	instantiationContext.mappedObject = v;

	switch (v->getKind()) {
		case ObjectKind::Class: {
			ClassObject *value = (ClassObject *)v;

			if (instantiationContext.genericArgs->size() != value->genericParams.size()) {
				return MismatchedGenericArgumentNumberError::alloc(
					const_cast<Runtime *>(this));
			}

			for (size_t i = 0; i < value->genericParams.size(); ++i) {
				instantiationContext.mappedGenericArgs[value->genericParams[i].name] = instantiationContext.genericArgs->at(i);
			}
			break;
		}
		case ObjectKind::Interface: {
			InterfaceObject *value = (InterfaceObject *)v;

			if (instantiationContext.genericArgs->size() != value->genericParams.size()) {
				return MismatchedGenericArgumentNumberError::alloc(
					const_cast<Runtime *>(this));
			}

			for (size_t i = 0; i < value->genericParams.size(); ++i) {
				instantiationContext.mappedGenericArgs[value->genericParams[i].name] = instantiationContext.genericArgs->at(i);
			}
			break;
		}
		default:;
	}
	return {};
}

SLAKE_API InternalExceptionPointer Runtime::mapGenericParams(const FnOverloadingObject *ol, GenericInstantiationContext &instantiationContext) const {
	if (instantiationContext.genericArgs->size() != ol->genericParams.size()) {
		return MismatchedGenericArgumentNumberError::alloc(
			const_cast<Runtime *>(this));
	}

	for (size_t i = 0; i < ol->genericParams.size(); ++i) {
		instantiationContext.mappedGenericArgs[ol->genericParams[i].name] = instantiationContext.genericArgs->at(i);
	}
	return {};
}

SLAKE_API InternalExceptionPointer Runtime::instantiateGenericObject(const Object *v, Object *&objectOut, GenericInstantiationContext &instantiationContext) {
	// Try to look up in the cache.
	if (_genericCacheDir.count(v)) {
		auto &table = _genericCacheDir.at(v);
		if (auto it = table.find(*instantiationContext.genericArgs); it != table.end()) {
			// Cache hit, return.
			objectOut = it->second;
			return {};
		}
		// Cache missed, go to the fallback.
	}

	// Cache missed, instantiate the value.
	auto value = v->duplicate();									 // Make a duplicate of the original value.
	SLAKE_RETURN_IF_EXCEPT(mapGenericParams(value, instantiationContext));
	// Instantiate the value.
	SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(value, instantiationContext));

	setGenericCache(v, *instantiationContext.genericArgs, value);

	objectOut = value;

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::_instantiateGenericObject(FnOverloadingObject *ol, GenericInstantiationContext &instantiationContext) {
	if (ol->genericParams.size() && ol->fnObject != instantiationContext.mappedObject) {
		GenericInstantiationContext newInstantiationContext = instantiationContext;

		// Map irreplaceable parameters to corresponding generic parameter reference type
		// and thus the generic types will keep unchanged.
		for (size_t i = 0; i < ol->genericParams.size(); ++i) {
			newInstantiationContext.mappedGenericArgs[ol->genericParams[i].name] = TypeId::None;
		}

		newInstantiationContext.mappedObject = ol->fnObject;

		SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(ol, newInstantiationContext));
	} else {
		for (auto &i : ol->paramTypes) {
			SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(i, instantiationContext));
		}

		SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(ol->returnType, instantiationContext));

		switch (ol->overloadingKind) {
			case FnOverloadingKind::Regular: {
				RegularFnOverloadingObject *overloading = (RegularFnOverloadingObject *)ol;

				for (auto &i : overloading->instructions) {
					for (auto &j : i.operands) {
						SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(j, instantiationContext));
					}
				}

				break;
			}
			case FnOverloadingKind::Native: {
				NativeFnOverloadingObject *overloading = (NativeFnOverloadingObject *)ol;

				break;
			}
		}
	}
	return {};
}
