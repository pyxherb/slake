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

SLAKE_API bool slake::Runtime::_instantiateGenericObject(Type &type, GenericInstantiationContext &instantiationContext) const {
	switch (type.typeId) {
		case TypeId::Instance: {
			if (type.isLoadingDeferred()) {
				IdRefObject *exData = (IdRefObject *)type.getCustomTypeExData();
				for (auto &i : exData->entries) {
					for (auto &j : i.genericArgs) {
						if (!_instantiateGenericObject(j, instantiationContext))
							return false;
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

				if (!_instantiateGenericObject(type, instantiationContext))
					return false;
			}
			break;
		}
		case TypeId::Array:
			type = type.duplicate();
			if (!_instantiateGenericObject(type.getArrayExData(), instantiationContext))
				return false;
			break;
		case TypeId::Ref:
			type = type.duplicate();
			if (!_instantiateGenericObject(type.getRefExData(), instantiationContext))
				return false;
			break;
		case TypeId::GenericArg: {
			HostObjectRef<StringObject> nameObject = (StringObject *)type.getCustomTypeExData();
			if (auto it = instantiationContext.mappedGenericArgs.find(nameObject->data); it != instantiationContext.mappedGenericArgs.end()) {
				if (it->second.typeId != TypeId::None)
					type = it->second;
			} else {
				std::pmr::string paramName(nameObject->data);

				setThreadLocalInternalException(
					std::this_thread::get_id(),
					GenericParameterNotFoundError::alloc(
						const_cast<Runtime *>(this),
						std::move(paramName)));
				return false;
			}
		}
	}
	return true;
}

SLAKE_API bool slake::Runtime::_instantiateGenericObject(Value &value, GenericInstantiationContext &instantiationContext) const {
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
			if (!_instantiateGenericObject(value.getTypeName(), instantiationContext))
				return false;
			break;
		case ValueType::Undefined:
			break;
		default:
			throw std::logic_error("Unhandled value type");
	}
	return true;
}

SLAKE_API bool slake::Runtime::_instantiateGenericObject(Object *v, GenericInstantiationContext &instantiationContext) const {
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

				if (!_instantiateGenericObject(value->parentClass, newInstantiationContext))
					return false;

				if (!_instantiateGenericObject(value, newInstantiationContext))
					return false;
			} else {
				value->genericArgs = *instantiationContext.genericArgs;

				if (!_instantiateGenericObject(value->parentClass, instantiationContext))
					return false;

				for (auto &i : value->scope->members)
					if (!_instantiateGenericObject(i.second, instantiationContext))
						return false;
			}

			break;
		}
		case ObjectKind::Interface: {
			InterfaceObject *const value = (InterfaceObject *)v;

			value->genericArgs = *instantiationContext.genericArgs;

			for (auto &i : value->scope->members) {
				if (!_instantiateGenericObject(i.second, instantiationContext))
					return false;
			}

			break;
		}
		case ObjectKind::Var: {
			VarObject *value = (VarObject *)v;

			switch (value->getVarKind()) {
				case VarKind::Regular: {
					RegularVarObject *v = (RegularVarObject *)value;
					if (!_instantiateGenericObject(v->type, instantiationContext))
						return false;
					break;
				}
				case VarKind::ArrayElementAccessor: {
					ArrayAccessorVarObject *v = (ArrayAccessorVarObject *)value;
					if (!_instantiateGenericObject(v->arrayObject, instantiationContext))
						return false;
					break;
				}
			}
			break;
		}
		case ObjectKind::Module: {
			ModuleObject *value = (ModuleObject *)v;

			for (auto &i : value->scope->members) {
				if (!_instantiateGenericObject(i.second, instantiationContext))
					return false;
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

							if (!curType.loadDeferredType(this))
								return false;
							if (curType != instantiationContext.genericArgs->at(j)) {
								goto specializationArgsMismatched;
							}
						}

						if (!_instantiateGenericObject(i, instantiationContext))
							return false;
						matchedSpecializedOverloading = i;
						break;
					} else {
						if (!_instantiateGenericObject(i, instantiationContext))
							return false;
						matchedOverloading = i;
					}

				specializationArgsMismatched:;
				}

				value->overloadings.insert(
					matchedSpecializedOverloading ? matchedSpecializedOverloading : matchedOverloading);
			} else {
				for (auto i : value->overloadings) {
					if (!_instantiateGenericObject(i, instantiationContext))
						return false;
				}
			}
			break;
		}
		case ObjectKind::Alias: {
			AliasObject *value = (AliasObject *)v;

			value->src = instantiateGenericObject(value->src, instantiationContext);
			break;
		}
		case ObjectKind::String:
		case ObjectKind::RootObject:
		case ObjectKind::IdRef:
			break;
		default:
			throw std::logic_error("Unhandled object type");
	}
	return true;
}

bool Runtime::mapGenericParams(const Object *v, GenericInstantiationContext &instantiationContext) const {
	instantiationContext.mappedObject = v;

	switch (v->getKind()) {
		case ObjectKind::Class: {
			ClassObject *value = (ClassObject *)v;

			if (instantiationContext.genericArgs->size() != value->genericParams.size()) {
				setThreadLocalInternalException(
					std::this_thread::get_id(),
					MismatchedGenericArgumentNumberError::alloc(
						const_cast<Runtime *>(this)));
				return false;
			}

			for (size_t i = 0; i < value->genericParams.size(); ++i) {
				instantiationContext.mappedGenericArgs[value->genericParams[i].name] = instantiationContext.genericArgs->at(i);
			}
			break;
		}
		case ObjectKind::Interface: {
			InterfaceObject *value = (InterfaceObject *)v;

			if (instantiationContext.genericArgs->size() != value->genericParams.size()) {
				setThreadLocalInternalException(
					std::this_thread::get_id(),
					MismatchedGenericArgumentNumberError::alloc(
						const_cast<Runtime *>(this)));
				return false;
			}

			for (size_t i = 0; i < value->genericParams.size(); ++i) {
				instantiationContext.mappedGenericArgs[value->genericParams[i].name] = instantiationContext.genericArgs->at(i);
			}
			break;
		}
		default:;
	}
	return true;
}

SLAKE_API bool Runtime::mapGenericParams(const FnOverloadingObject *ol, GenericInstantiationContext &instantiationContext) const {
	if (instantiationContext.genericArgs->size() != ol->genericParams.size()) {
		setThreadLocalInternalException(
			std::this_thread::get_id(),
			MismatchedGenericArgumentNumberError::alloc(
				const_cast<Runtime *>(this)));
		return false;
	}

	for (size_t i = 0; i < ol->genericParams.size(); ++i) {
		instantiationContext.mappedGenericArgs[ol->genericParams[i].name] = instantiationContext.genericArgs->at(i);
	}
	return true;
}

SLAKE_API Object *Runtime::instantiateGenericObject(const Object *v, GenericInstantiationContext &instantiationContext) const {
	// Try to look up in the cache.
	if (_genericCacheDir.count(v)) {
		auto &table = _genericCacheDir.at(v);
		if (auto it = table.find(*instantiationContext.genericArgs); it != table.end()) {
			// Cache hit, return.
			return it->second;
		}
		// Cache missed, go to the fallback.
	}

	// Cache missed, instantiate the value.
	auto value = v->duplicate();									 // Make a duplicate of the original value.
	_genericCacheDir[v][*instantiationContext.genericArgs] = value;	 // Store the instance into the cache.
	if(!mapGenericParams(value, instantiationContext))
		return nullptr;
	// Instantiate the value.
	if(!_instantiateGenericObject(value, instantiationContext))
		return nullptr;

	_genericCacheLookupTable[value] = { v, *instantiationContext.genericArgs };

	return value;
}

SLAKE_API bool Runtime::_instantiateGenericObject(FnOverloadingObject *ol, GenericInstantiationContext &instantiationContext) const {
	if (ol->genericParams.size() && ol->fnObject != instantiationContext.mappedObject) {
		GenericInstantiationContext newInstantiationContext = instantiationContext;

		// Map irreplaceable parameters to corresponding generic parameter reference type
		// and thus the generic types will keep unchanged.
		for (size_t i = 0; i < ol->genericParams.size(); ++i) {
			newInstantiationContext.mappedGenericArgs[ol->genericParams[i].name] = TypeId::None;
		}

		newInstantiationContext.mappedObject = ol->fnObject;

		if (!_instantiateGenericObject(ol, newInstantiationContext))
			return false;
	} else {
		for (auto &i : ol->paramTypes) {
			if (!_instantiateGenericObject(i, instantiationContext)) {
				return false;
			}
		}

		if (!_instantiateGenericObject(ol->returnType, instantiationContext))
			return false;

		switch (ol->getOverloadingKind()) {
			case FnOverloadingKind::Regular: {
				RegularFnOverloadingObject *overloading = (RegularFnOverloadingObject *)ol;

				for (auto &i : overloading->instructions) {
					for (auto &j : i.operands) {
						if (!_instantiateGenericObject(j, instantiationContext))
							return false;
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
	return true;
}
