#include <slake/runtime.h>

using namespace slake;

void slake::Runtime::_instantiateGenericValue(Type &type, GenericInstantiationContext &instantiationContext) const {
	if (type.typeId == TypeId::GenericArg) {
		if (auto it = instantiationContext.mappedGenericArgs.find(type.getGenericArgExData()); it != instantiationContext.mappedGenericArgs.end()) {
			type = it->second;
		} else
			throw GenericInstantiationError("No such generic parameter named " + type.getGenericArgExData());
	}
}

void slake::Runtime::_instantiateGenericValue(Value *v, GenericInstantiationContext &instantiationContext) const {
	// How to instantiate generic classes:
	// Duplicate the value, scan for references to generic parameters and
	// replace them with generic arguments.
	switch (v->getType().typeId) {
		case TypeId::Object: {
			auto value = (ObjectValue *)v;

			for (auto &i : value->scope->members)
				_instantiateGenericValue(i.second, instantiationContext);

			for (auto &i : value->_genericArgs)
				_instantiateGenericValue(i, instantiationContext);

			_instantiateGenericValue(value->_class, instantiationContext);

			if (value->_parent)
				_instantiateGenericValue(value->_parent, instantiationContext);
			break;
		}
		case TypeId::Array: {
			auto value = (ArrayValue *)v;

			_instantiateGenericValue(value->type, instantiationContext);
			break;
		}
		case TypeId::Class: {
			ClassValue *const value = (ClassValue *)v;

			if (value->genericParams.size() && value != instantiationContext.mappedValue) {
				GenericInstantiationContext newInstantiationContext = instantiationContext;

				// Map irreplaceable parameters to corresponding generic parameter reference type
				// and thus the generic types will keep unchanged.
				for (size_t i = 0; i < value->genericParams.size(); ++i) {
					newInstantiationContext.mappedGenericArgs[value->genericParams[i].name] = Type(value->genericParams[i].name);
				}

				newInstantiationContext.mappedValue = value;

				_instantiateGenericValue(value, newInstantiationContext);
			} else {
				value->_genericArgs = *instantiationContext.genericArgs;

				for (auto &i : value->scope->members)
					_instantiateGenericValue(i.second, instantiationContext);
			}

			break;
		}
		case TypeId::Interface: {
			InterfaceValue *const value = (InterfaceValue *)v;

			value->_genericArgs = *instantiationContext.genericArgs;

			for (auto &i : value->scope->members)
				_instantiateGenericValue(i.second, instantiationContext);

			break;
		}
		case TypeId::Trait: {
			TraitValue *const value = (TraitValue *)v;

			for (auto &i : value->scope->members)
				_instantiateGenericValue(i.second, instantiationContext);

			break;
		}
		case TypeId::Var: {
			BasicVarValue *value = (BasicVarValue *)v;

			_instantiateGenericValue(value->type, instantiationContext);
			break;
		}
		case TypeId::Module: {
			ModuleValue *value = (ModuleValue *)v;

			for (auto &i : value->scope->members)
				_instantiateGenericValue(i.second, instantiationContext);
			break;
		}
		case TypeId::Fn: {
			FnValue *value = (FnValue *)v;

			if (instantiationContext.mappedValue == value) {
				//
				// We expect there's only one overloading can be instantiated.
				// Uninstantiatable overloadings will be discarded.
				//
				FnOverloadingValue* matchedOverloading = nullptr;

				for (auto& i : value->overloadings) {
					if (i->genericParams.size() == instantiationContext.genericArgs->size()) {
						matchedOverloading = i;
						break;
					}
				}

				value->overloadings.clear();

				if (matchedOverloading) {
					_instantiateGenericValue(matchedOverloading, instantiationContext);
					value->overloadings.push_back(matchedOverloading);
				}
			} else {
				for (auto i : value->overloadings) {
					_instantiateGenericValue(i, instantiationContext);
				}
			}
			break;
		}
		case TypeId::Alias: {
			AliasValue *value = (AliasValue *)v;

			value->src = instantiateGenericValue(value->src, instantiationContext);
			break;
		}
		case TypeId::RootValue:
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
		case TypeId::IdRef:
		case TypeId::RegRef:
		case TypeId::LocalVarRef:
		case TypeId::ArgRef:
			break;
		default:
			throw std::logic_error("Unhandled value type");
	}
}

void Runtime::mapGenericParams(const Value *v, GenericInstantiationContext &instantiationContext) const {
	instantiationContext.mappedValue = v;

	switch (v->getType().typeId) {
		case TypeId::Class: {
			ClassValue *value = (ClassValue *)v;

			if (instantiationContext.genericArgs->size() != value->genericParams.size())
				throw GenericInstantiationError("Number of generic parameter does not match");

			for (size_t i = 0; i < value->genericParams.size(); ++i) {
				instantiationContext.mappedGenericArgs[value->genericParams[i].name] = instantiationContext.genericArgs->at(i);
			}
			break;
		}
		case TypeId::Interface: {
			InterfaceValue *value = (InterfaceValue *)v;

			if (instantiationContext.genericArgs->size() != value->genericParams.size())
				throw GenericInstantiationError("Number of generic parameter does not match");

			for (size_t i = 0; i < value->genericParams.size(); ++i) {
				instantiationContext.mappedGenericArgs[value->genericParams[i].name] = instantiationContext.genericArgs->at(i);
			}
			break;
		}
		case TypeId::Trait: {
			TraitValue *value = (TraitValue *)v;

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

void Runtime::mapGenericParams(const FnOverloadingValue *ol, GenericInstantiationContext &instantiationContext) const {
	if (instantiationContext.genericArgs->size() != ol->genericParams.size())
		throw GenericInstantiationError("Number of generic parameter does not match");

	for (size_t i = 0; i < ol->genericParams.size(); ++i) {
		instantiationContext.mappedGenericArgs[ol->genericParams[i].name] = instantiationContext.genericArgs->at(i);
	}
}

Value *Runtime::instantiateGenericValue(const Value *v, GenericInstantiationContext &instantiationContext) const {
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
	_instantiateGenericValue(value, instantiationContext);	// Instantiate the value.

	return value;
}

void Runtime::_instantiateGenericValue(FnOverloadingValue *ol, GenericInstantiationContext &instantiationContext) const {
	if (ol->genericParams.size() && ol->fnValue != instantiationContext.mappedValue) {
		GenericInstantiationContext newInstantiationContext = instantiationContext;

		// Map irreplaceable parameters to corresponding generic parameter reference type
		// and thus the generic types will keep unchanged.
		for (size_t i = 0; i < ol->genericParams.size(); ++i) {
			newInstantiationContext.mappedGenericArgs[ol->genericParams[i].name] = Type(ol->genericParams[i].name);
		}

		newInstantiationContext.mappedValue = ol->fnValue;

		_instantiateGenericValue(ol, newInstantiationContext);
	} else {
		for (auto &i : ol->paramTypes)
			_instantiateGenericValue(i, instantiationContext);

		_instantiateGenericValue(ol->returnType, instantiationContext);

		switch (ol->getOverloadingKind()) {
			case FnOverloadingKind::Regular: {
				RegularFnOverloadingValue *overloading = (RegularFnOverloadingValue *)ol;

				for (auto& i : overloading->instructions) {
					for (auto& j : i.operands) {
						if (j) {
							if (j->getType() == TypeId::TypeName)
								_instantiateGenericValue(((TypeNameValue *)j)->_data, instantiationContext);
						}
					}
				}

				break;
			}
			case FnOverloadingKind::Native: {
				NativeFnOverloadingValue *overloading = (NativeFnOverloadingValue *)ol;

				break;
			}
		}
	}
}
