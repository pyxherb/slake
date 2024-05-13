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

			value->_genericArgs = *instantiationContext.genericArgs;

			if (value->genericParams.size() && value != instantiationContext.mappedValue) {
				GenericInstantiationContext newInstantiationContext = instantiationContext;

				// Map irreplaceable parameters to corresponding generic parameter reference type
				// and thus the generic types will keep unchanged.
				for (size_t i = 0; i < value->genericParams.size(); ++i) {
					newInstantiationContext.mappedGenericArgs[value->genericParams[i].name] = Type(value->genericParams[i].name);
				}

				for (auto &i : value->scope->members)
					_instantiateGenericValue(i.second, newInstantiationContext);
			} else {
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
			VarValue *value = (VarValue *)v;

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
			BasicFnValue *value = (BasicFnValue *)v;

			if (value->genericParams.size() && value != instantiationContext.mappedValue) {
				GenericInstantiationContext newInstantiationContext = instantiationContext;

				// Map irreplaceable parameters to corresponding generic parameter reference type
				// and thus the generic types will keep unchanged.
				for (size_t i = 0; i < value->genericParams.size(); ++i) {
					newInstantiationContext.mappedGenericArgs[value->genericParams[i].name] = Type(value->genericParams[i].name);
				}

				_instantiateGenericValue(value->returnType, newInstantiationContext);

				for (auto &i : value->paramTypes)
					_instantiateGenericValue(i, newInstantiationContext);

				if (value->isNative()) {
					((NativeFnValue *)value)->mappedGenericArgs = newInstantiationContext.mappedGenericArgs;
				} else {
					for (size_t i = 0; i < (((FnValue *)value))->nIns; ++i) {
						auto &ins = (((FnValue *)value))->body[i];
						for (size_t j = 0; j < ins.operands.size(); ++j) {
							auto operand = ins.operands[j];
							if (operand && operand->getType() == TypeId::TypeName)
								_instantiateGenericValue(((TypeNameValue *)operand)->_data, newInstantiationContext);
						}
					}
				}
			} else {
				_instantiateGenericValue(value->returnType, instantiationContext);

				for (auto &i : value->paramTypes)
					_instantiateGenericValue(i, instantiationContext);

				if (value->isNative()) {
					((NativeFnValue *)value)->mappedGenericArgs = instantiationContext.mappedGenericArgs;
				} else {
					for (size_t i = 0; i < (((FnValue *)value))->nIns; ++i) {
						auto &ins = (((FnValue *)value))->body[i];
						for (size_t j = 0; j < ins.operands.size(); ++j) {
							auto operand = ins.operands[j];
							if (operand && operand->getType() == TypeId::TypeName)
								_instantiateGenericValue(((TypeNameValue *)operand)->_data, instantiationContext);
						}
					}
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
		case TypeId::Fn: {
			FnValue *value = (FnValue *)v;

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
	auto value = v->duplicate();									// Make a duplicate of the original value.
	_genericCacheDir[v][*instantiationContext.genericArgs] = value;	// Store the instance into the cache.
	mapGenericParams(value, instantiationContext);
	_instantiateGenericValue(value, instantiationContext);	// Instantiate the value.

	return value;
}
