#include <slake/runtime.h>

using namespace slake;

SLAKE_API void Runtime::invalidateGenericCache(Object *i) {
	if (_genericCacheLookupTable.contains(i)) {
		// Remove the value from generic cache if it is unreachable.
		auto &lookupEntry = _genericCacheLookupTable.at(i);

		auto &table = _genericCacheDir.at(lookupEntry.originalObject);
		table.remove(lookupEntry.genericArgs);

		if (!table.size())
			_genericCacheDir.remove(lookupEntry.originalObject);

		_genericCacheLookupTable.remove(i);
	}
}

SLAKE_API void Runtime::setGenericCache(const Object *object, const GenericArgList &genericArgs, Object *instantiatedObject) {
	if (!_genericCacheDir.contains(object)) {
		const Object *copiedObject = object;
		if (!_genericCacheDir.insert(std::move(copiedObject), GenericCacheTable(&globalHeapPoolAlloc)))
			std::terminate();
	}
	// Store the instance into the cache.
	auto &cacheTable = _genericCacheDir.at(object);

	if (!cacheTable.contains(genericArgs)) {
		GenericArgList copiedGenericArgs;
		if (!peff::copy(copiedGenericArgs, genericArgs)) {
			std::terminate();
		}

		Object *copiedInstantiatedObject = instantiatedObject;

		cacheTable.insert(std::move(copiedGenericArgs), std::move(copiedInstantiatedObject));
	} else {
		Object *copiedInstantiatedObject = instantiatedObject;

		cacheTable.at(genericArgs) = std::move(copiedInstantiatedObject);
	}
	{
		GenericArgList copiedGenericArgList;
		if (!peff::copy(copiedGenericArgList, genericArgs)) {
			std::terminate();
		}
		_genericCacheLookupTable.insert((const Object*)instantiatedObject, { object, std::move(copiedGenericArgList) });
	}
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
				peff::DynArray<IdRefEntry> idRefToResolvedType(&globalHeapPoolAlloc);

				if (!getFullRef((MemberObject *)type.getCustomTypeExData(), idRefToResolvedType))
					std::terminate();

				HostObjectRef<IdRefObject> idRefObject = IdRefObject::alloc((Runtime *)this);

				// TODO: If we use std::pmr version for getFullRef, remove these code and use following code:
				// idRefObject->entries = idRefToResolvedType;
				idRefObject->entries.resizeWith(idRefToResolvedType.size(), IdRefEntry(&globalHeapPoolAlloc));
				for (size_t i = 0; i < idRefToResolvedType.size(); ++i) {
					if (!peff::copyAssign(idRefObject->entries.at(i), idRefToResolvedType.at(i))) {
						std::terminate();
					}
				}

				// TODO: Add HostRefHolder for idRefObject.
				type = Type(type.typeId, idRefObject.get());

				SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(type, instantiationContext));
			}
			break;
		}
		case TypeId::Array: {
			bool isSucceeded;
			type = type.duplicate(isSucceeded);
			if (!isSucceeded)
				std::terminate();
			SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(type.getArrayExData(), instantiationContext));
			break;
		}
		case TypeId::Ref: {
			bool isSucceeded;
			type = type.duplicate(isSucceeded);
			if (!isSucceeded)
				std::terminate();
			SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(type.getRefExData(), instantiationContext));
			break;
		}
		case TypeId::GenericArg: {
			HostObjectRef<StringObject> nameObject = (StringObject *)type.getCustomTypeExData();
			if (auto it = instantiationContext.mappedGenericArgs.find(nameObject->data); it != instantiationContext.mappedGenericArgs.end()) {
				if (it.value().typeId != TypeId::None)
					type = it.value();
			} else {
				peff::String paramName(&globalHeapPoolAlloc);
				if (!peff::copyAssign(paramName, nameObject->data)) {
					std::terminate();
				}

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
				peff::HashMap<peff::String, Type> copiedMappedGenericArgs(&globalHeapPoolAlloc);

				if (!peff::copyAssign(copiedMappedGenericArgs, instantiationContext.mappedGenericArgs))
					std::terminate();

				GenericInstantiationContext newInstantiationContext = {
					instantiationContext.mappedObject,
					instantiationContext.genericArgs,
					std::move(copiedMappedGenericArgs)
				};

				// Map irreplaceable parameters to corresponding generic parameter reference type
				// and thus the generic types will keep unchanged.
				for (size_t i = 0; i < value->genericParams.size(); ++i) {
					peff::String copiedName(&globalHeapPoolAlloc);
					if (!peff::copyAssign(copiedName, value->genericParams.at(i).name))
						std::terminate();
					newInstantiationContext.mappedGenericArgs.insert(std::move(copiedName), TypeId::None);
				}

				SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(value->parentClass, newInstantiationContext));

				for (auto it = value->scope->members.begin(); it != value->scope->members.end(); ++it) {
					SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(it.value(), newInstantiationContext));
				}
			} else {
				if (value == instantiationContext.mappedObject) {
					if (!peff::copyAssign(value->genericArgs, *instantiationContext.genericArgs))
						std::terminate();
				}

				SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(value->parentClass, instantiationContext));

				for (auto it = value->scope->members.begin(); it != value->scope->members.end(); ++it) {
					SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(it.value(), instantiationContext));
				}
			}

			break;
		}
		case ObjectKind::Interface: {
			InterfaceObject *const value = (InterfaceObject *)v;

			if (!peff::copyAssign(value->genericArgs, *instantiationContext.genericArgs))
				std::terminate();

			for (auto it = value->scope->members.begin(); it != value->scope->members.end(); ++it) {
				SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(it.value(), instantiationContext));
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

			for (auto it = value->scope->members.begin(); it != value->scope->members.end(); ++it) {
				SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(it.value(), instantiationContext));
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
							auto &curType = i->specializationArgs.at(j);

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
					matchedSpecializedOverloading ? std::move(matchedSpecializedOverloading) : std::move(matchedOverloading));
			} else {
				for (auto i : value->overloadings) {
					SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(i, instantiationContext));
				}
			}
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
				peff::String copiedName(&globalHeapPoolAlloc);

				if (!peff::copyAssign(copiedName, value->genericParams.at(i).name)) {
					std::terminate();
				}

				Type copiedType = instantiationContext.genericArgs->at(i);

				instantiationContext.mappedGenericArgs.insert(std::move(copiedName), std::move(copiedType));
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
				peff::String copiedName(&globalHeapPoolAlloc);
				if (!peff::copyAssign(copiedName, value->genericParams.at(i).name))
					std::terminate();
				Type copiedType = instantiationContext.genericArgs->at(i);
				instantiationContext.mappedGenericArgs.insert(std::move(copiedName), std::move(copiedType));
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
		peff::String copiedName(&globalHeapPoolAlloc);

		if (!peff::copyAssign(copiedName, ol->genericParams.at(i).name)) {
			std::terminate();
		}

		Type copiedType = instantiationContext.genericArgs->at(i);

		instantiationContext.mappedGenericArgs.insert(std::move(copiedName), std::move(copiedType));
	}
	return {};
}

SLAKE_API InternalExceptionPointer Runtime::instantiateGenericObject(const Object *v, Object *&objectOut, GenericInstantiationContext &instantiationContext) {
	// Try to look up in the cache.
	if (_genericCacheDir.contains(v)) {
		auto &table = _genericCacheDir.at(v);
		if (auto it = table.find(*instantiationContext.genericArgs); it != table.end()) {
			// Cache hit, return.
			objectOut = it.value();
			return {};
		}
		// Cache missed, go to the fallback.
	}

	// Cache missed, instantiate the value.
	auto value = v->duplicate();  // Make a duplicate of the original value.
	SLAKE_RETURN_IF_EXCEPT(mapGenericParams(value, instantiationContext));
	// Instantiate the value.
	SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(value, instantiationContext));

	setGenericCache(v, *instantiationContext.genericArgs, value);

	objectOut = value;

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::_instantiateGenericObject(FnOverloadingObject *ol, GenericInstantiationContext &instantiationContext) {
	if (ol->genericParams.size() && ol->fnObject != instantiationContext.mappedObject) {
		peff::HashMap<peff::String, Type> copiedMappedGenericArgs(&globalHeapPoolAlloc);

		if (!peff::copyAssign(copiedMappedGenericArgs, instantiationContext.mappedGenericArgs))
			std::terminate();

		GenericInstantiationContext newInstantiationContext = {
			instantiationContext.mappedObject,
			instantiationContext.genericArgs,
			std::move(copiedMappedGenericArgs)
		};

		// Map irreplaceable parameters to corresponding generic parameter reference type
		// and thus the generic types will keep unchanged.
		for (size_t i = 0; i < ol->genericParams.size(); ++i) {
			peff::String copiedName(&globalHeapPoolAlloc);
			if (!peff::copyAssign(copiedName, ol->genericParams.at(i).name))
				std::terminate();
			newInstantiationContext.mappedGenericArgs.insert(std::move(copiedName), TypeId::None);
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
