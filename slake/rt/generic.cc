#include <slake/runtime.h>

using namespace slake;

void slake::Runtime::_instantiateGenericObject(Type &type, GenericInstantiationContext &instantiationContext) const {
	switch (type.typeId) {
		case TypeId::Class:
		case TypeId::Interface:
		case TypeId::Trait:
		case TypeId::Instance: {
			if (type.isLoadingDeferred()) {
				IdRefObject *exData = (IdRefObject *)type.getCustomTypeExData();
				for (auto &i : exData->entries) {
					for (auto &j : i.genericArgs) {
						_instantiateGenericObject(j, instantiationContext);
					}
				}
			} else {
				auto idRefToResolvedType = getFullRef((MemberObject *)type.getCustomTypeExData());

				HostObjectRef<IdRefObject> idRefObject = new IdRefObject((Runtime *)this);
				idRefObject->entries = idRefToResolvedType;

				type = Type(type.typeId, idRefObject.release());

				_instantiateGenericObject(type, instantiationContext);
			}
			break;
		}
		case TypeId::Array:
			_instantiateGenericObject(type.getArrayExData(), instantiationContext);
			break;
		case TypeId::Ref:
			_instantiateGenericObject(type.getRefExData(), instantiationContext);
			break;
		case TypeId::Var:
			_instantiateGenericObject(type.getVarExData(), instantiationContext);
			break;
		case TypeId::GenericArg:
			if (auto it = instantiationContext.mappedGenericArgs.find(type.getGenericArgExData()); it != instantiationContext.mappedGenericArgs.end()) {
				type = it->second;
			} else
				throw GenericInstantiationError("No such generic parameter named " + type.getGenericArgExData());
	}
}

void slake::Runtime::_instantiateGenericObject(Value& value, GenericInstantiationContext &instantiationContext) const {
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
		case ValueType::String:
		case ValueType::ObjectRef:
		case ValueType::RegRef:
		case ValueType::ArgRef:
		case ValueType::LocalVarRef:
			break;
		case ValueType::TypeName:
			_instantiateGenericObject(value.getTypeName(), instantiationContext);
			break;
		case ValueType::Undefined:
			break;
		default:
			throw std::logic_error("Unhandled value type");
	}
}

void slake::Runtime::_instantiateGenericObject(Object *v, GenericInstantiationContext &instantiationContext) const {
	// How to instantiate generic classes:
	// Duplicate the value, scan for references to generic parameters and
	// replace them with generic arguments.
	switch (v->getType().typeId) {
		case TypeId::Instance: {
			auto value = (InstanceObject *)v;

			for (auto &i : value->scope->members)
				_instantiateGenericObject(i.second, instantiationContext);

			for (auto &i : value->_genericArgs)
				_instantiateGenericObject(i, instantiationContext);

			_instantiateGenericObject(value->_class, instantiationContext);

			if (value->_parent)
				_instantiateGenericObject(value->_parent, instantiationContext);
			break;
		}
		case TypeId::Array: {
			auto value = (ArrayObject *)v;

			_instantiateGenericObject(value->type, instantiationContext);
			break;
		}
		case TypeId::Class: {
			ClassObject *const value = (ClassObject *)v;

			if (value->genericParams.size() && value != instantiationContext.mappedObject) {
				GenericInstantiationContext newInstantiationContext = instantiationContext;

				// Map irreplaceable parameters to corresponding generic parameter reference type
				// and thus the generic types will keep unchanged.
				for (size_t i = 0; i < value->genericParams.size(); ++i) {
					newInstantiationContext.mappedGenericArgs[value->genericParams[i].name] = Type(value->genericParams[i].name);
				}

				newInstantiationContext.mappedObject = value;

				_instantiateGenericObject(value->parentClass, newInstantiationContext);

				_instantiateGenericObject(value, newInstantiationContext);
			} else {
				value->_genericArgs = *instantiationContext.genericArgs;

				_instantiateGenericObject(value->parentClass, instantiationContext);

				for (auto &i : value->scope->members)
					_instantiateGenericObject(i.second, instantiationContext);
			}

			break;
		}
		case TypeId::Interface: {
			InterfaceObject *const value = (InterfaceObject *)v;

			value->_genericArgs = *instantiationContext.genericArgs;

			for (auto &i : value->scope->members)
				_instantiateGenericObject(i.second, instantiationContext);

			break;
		}
		case TypeId::Trait: {
			TraitObject *const value = (TraitObject *)v;

			for (auto &i : value->scope->members)
				_instantiateGenericObject(i.second, instantiationContext);

			break;
		}
		case TypeId::Var: {
			BasicVarObject *value = (BasicVarObject *)v;

			_instantiateGenericObject(value->type, instantiationContext);
			break;
		}
		case TypeId::Module: {
			ModuleObject *value = (ModuleObject *)v;

			for (auto &i : value->scope->members)
				_instantiateGenericObject(i.second, instantiationContext);
			break;
		}
		case TypeId::Fn: {
			FnObject *value = (FnObject *)v;

			if (instantiationContext.mappedObject == value) {
				//
				// We expect there's only one overloading can be instantiated.
				// Uninstantiatable overloadings will be discarded.
				//
				FnOverloadingObject *matchedOverloading = nullptr;

				for (auto &i : value->overloadings) {
					if (i->genericParams.size() == instantiationContext.genericArgs->size()) {
						matchedOverloading = i;
						break;
					}
				}

				value->overloadings.clear();

				if (matchedOverloading) {
					_instantiateGenericObject(matchedOverloading, instantiationContext);
					value->overloadings.push_back(matchedOverloading);
				}
			} else {
				for (auto i : value->overloadings) {
					_instantiateGenericObject(i, instantiationContext);
				}
			}
			break;
		}
		case TypeId::Alias: {
			AliasObject *value = (AliasObject *)v;

			value->src = instantiateGenericObject(value->src, instantiationContext);
			break;
		}
		case TypeId::RootObject:
		case TypeId::IdRef:
			break;
		default:
			throw std::logic_error("Unhandled object type");
	}
}

void Runtime::mapGenericParams(const Object *v, GenericInstantiationContext &instantiationContext) const {
	instantiationContext.mappedObject = v;

	switch (v->getType().typeId) {
		case TypeId::Class: {
			ClassObject *value = (ClassObject *)v;

			if (instantiationContext.genericArgs->size() != value->genericParams.size())
				throw GenericInstantiationError("Number of generic parameter does not match");

			for (size_t i = 0; i < value->genericParams.size(); ++i) {
				instantiationContext.mappedGenericArgs[value->genericParams[i].name] = instantiationContext.genericArgs->at(i);
			}
			break;
		}
		case TypeId::Interface: {
			InterfaceObject *value = (InterfaceObject *)v;

			if (instantiationContext.genericArgs->size() != value->genericParams.size())
				throw GenericInstantiationError("Number of generic parameter does not match");

			for (size_t i = 0; i < value->genericParams.size(); ++i) {
				instantiationContext.mappedGenericArgs[value->genericParams[i].name] = instantiationContext.genericArgs->at(i);
			}
			break;
		}
		case TypeId::Trait: {
			TraitObject *value = (TraitObject *)v;

			if (instantiationContext.genericArgs->size() != value->genericParams.size())
				throw GenericInstantiationError("Number of generic parameter does not match");

			for (size_t i = 0; i < value->genericParams.size(); ++i) {
				instantiationContext.mappedGenericArgs[value->genericParams[i].name] = instantiationContext.genericArgs->at(i);
			}
			break;
		}
		default:;
	}
}

void Runtime::mapGenericParams(const FnOverloadingObject *ol, GenericInstantiationContext &instantiationContext) const {
	if (instantiationContext.genericArgs->size() != ol->genericParams.size())
		throw GenericInstantiationError("Number of generic parameter does not match");

	for (size_t i = 0; i < ol->genericParams.size(); ++i) {
		instantiationContext.mappedGenericArgs[ol->genericParams[i].name] = instantiationContext.genericArgs->at(i);
	}
}

Object *Runtime::instantiateGenericObject(const Object *v, GenericInstantiationContext &instantiationContext) const {
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
	mapGenericParams(value, instantiationContext);
	_instantiateGenericObject(value, instantiationContext);	// Instantiate the value.

	_genericCacheLookupTable[value] = { v, *instantiationContext.genericArgs };

	return value;
}

void Runtime::_instantiateGenericObject(FnOverloadingObject *ol, GenericInstantiationContext &instantiationContext) const {
	if (ol->genericParams.size() && ol->fnObject != instantiationContext.mappedObject) {
		GenericInstantiationContext newInstantiationContext = instantiationContext;

		// Map irreplaceable parameters to corresponding generic parameter reference type
		// and thus the generic types will keep unchanged.
		for (size_t i = 0; i < ol->genericParams.size(); ++i) {
			newInstantiationContext.mappedGenericArgs[ol->genericParams[i].name] = Type(ol->genericParams[i].name);
		}

		newInstantiationContext.mappedObject = ol->fnObject;

		_instantiateGenericObject(ol, newInstantiationContext);
	} else {
		for (auto &i : ol->paramTypes)
			_instantiateGenericObject(i, instantiationContext);

		_instantiateGenericObject(ol->returnType, instantiationContext);

		switch (ol->getOverloadingKind()) {
			case FnOverloadingKind::Regular: {
				RegularFnOverloadingObject *overloading = (RegularFnOverloadingObject *)ol;

				for (auto &i : overloading->instructions) {
					for (auto &j : i.operands) {
						_instantiateGenericObject(j, instantiationContext);
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
}
