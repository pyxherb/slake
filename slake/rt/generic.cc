#include <slake/runtime.h>

using namespace slake;

void slake::Runtime::_instantiateGenericValue(Type &type, const GenericArgList &genericArgs) const {
	if (type.typeId == TypeId::GenericArg)
		type = genericArgs[type.getGenericArgExData()];
}

void slake::Runtime::_instantiateGenericValue(Value *v, const GenericArgList &genericArgs) const {
	// How to instantiate generic classes:
	// Duplicate the value, scan for references to generic parameters and
	// replace them with generic arguments.
	switch (v->getType().typeId) {
		case TypeId::Object: {
			auto value = (ObjectValue *)v;

			for (auto &i : value->scope->members)
				_instantiateGenericValue(i.second, genericArgs);

			for (auto &i : value->_genericArgs)
				_instantiateGenericValue(i, genericArgs);

			_instantiateGenericValue(value->_class, genericArgs);

			if (value->_parent)
				_instantiateGenericValue(value->_parent, genericArgs);
			break;
		}
		case TypeId::Array: {
			auto value = (ArrayValue *)v;

			_instantiateGenericValue(value->type, genericArgs);
			break;
		}
		case TypeId::Map: {
			break;
		}
		case TypeId::Class: {
			ClassValue *const value = (ClassValue *)v;

			value->_genericArgs = genericArgs;

			for (auto &i : value->scope->members)
				_instantiateGenericValue(i.second, genericArgs);

			break;
		}
		case TypeId::Interface: {
			InterfaceValue *const value = (InterfaceValue *)v;

			value->_genericArgs = genericArgs;

			for (auto &i : value->scope->members)
				_instantiateGenericValue(i.second, genericArgs);

			break;
		}
		case TypeId::Trait: {
			TraitValue *const value = (TraitValue *)v;

			for (auto &i : value->scope->members)
				_instantiateGenericValue(i.second, genericArgs);

			break;
		}
		case TypeId::Var: {
			VarValue *value = (VarValue *)v;

			_instantiateGenericValue(value->type, genericArgs);
			break;
		}
		case TypeId::Module: {
			ModuleValue *value = (ModuleValue *)v;

			for (auto &i : value->scope->members)
				_instantiateGenericValue(i.second, genericArgs);
			break;
		}
		case TypeId::Fn: {
			FnValue *value = (FnValue *)v;

			_instantiateGenericValue(value->returnType, genericArgs);

			for (auto &i : value->paramTypes)
				_instantiateGenericValue(i, genericArgs);

			for (size_t i = 0; i < value->nIns; ++i) {
				auto &ins = value->body[i];
				for (size_t j = 0; j < ins.operands.size(); ++j) {
					auto operand = ins.operands[j];
					if (operand && operand->getType() == TypeId::TypeName)
						_instantiateGenericValue(((TypeNameValue *)operand)->_data, genericArgs);
				}
			}
			break;
		}
		case TypeId::Alias: {
			AliasValue* value = (AliasValue*)v;

			value->src = instantiateGenericValue(value->src, genericArgs);
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
		case TypeId::Ref:
		case TypeId::RegRef:
		case TypeId::LocalVarRef:
		case TypeId::ArgRef:
			break;
		default:
			throw std::logic_error("Unhandled value type");
	}
}

Value *Runtime::instantiateGenericValue(const Value *v, const GenericArgList &genericArgs) const {
	// Try to look up in the cache.
	if (_genericCacheDir.count(v)) {
		auto &table = _genericCacheDir.at(v);
		if (table.count(genericArgs)) {
			// Cache hit, return.
			return table.at(genericArgs);
		}
		// Cache missed, go to the fallback.
	}

	// Cache missed, instantiate the value.
	auto value = v->duplicate();				   // Make a duplicate of the original value.
	_genericCacheDir[v][genericArgs] = value;	   // Store the instance into the cache.
	_instantiateGenericValue(value, genericArgs);  // Instantiate the value.

	return value;
}
