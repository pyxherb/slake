#include "../runtime.h"

using namespace slake;

void Runtime::_gcWalk(Scope *scope) {
	for (auto &i : scope->members) {
		_gcWalk(i.second);
	}

	if (scope->owner)
		_gcWalk(scope->owner);

	if (scope->parent)
		_gcWalk(scope->parent);
}

void Runtime::_gcWalk(Type &type) {
	switch (type.typeId) {
		case TypeId::OBJECT:
		case TypeId::CLASS:
		case TypeId::INTERFACE:
		case TypeId::TRAIT:
			_gcWalk(type.getCustomTypeExData());
			break;
		case TypeId::ARRAY:
			_gcWalk(type.getArrayExData());
			break;
		case TypeId::MAP: {
			_gcWalk(*type.getMapExData().first);
			_gcWalk(*type.getMapExData().second);
			break;
		}
		case TypeId::VAR:
		case TypeId::MOD:
		case TypeId::ROOT:
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
		case TypeId::STRING:
		case TypeId::BOOL:
		case TypeId::FN:
		case TypeId::REF:
		case TypeId::NONE:
		case TypeId::GENERIC_ARG:
		case TypeId::ALIAS:
		case TypeId::ANY:
		case TypeId::REG_REF:
		case TypeId::LVAR_REF:
		case TypeId::ARG_REF:
			break;
		default:
			throw std::logic_error("Unhandled value type");
	}
}

void Runtime::_gcWalk(Value *v) {
	if (_walkedValues.count(v))
		return;

	_walkedValues.insert(v);
	_createdValues.erase(v);

	switch (auto typeId = v->getType().typeId; typeId) {
		case TypeId::OBJECT: {
			auto value = (ObjectValue *)v;
			_gcWalk(value->scope.get());
			_gcWalk(value->_class);
			if (value->_parent)
				_gcWalk(value->_parent);
			break;
		}
		case TypeId::ARRAY:
			for (auto &i : ((ArrayValue *)v)->values)
				_gcWalk(i);
			break;
		case TypeId::MAP:
			break;
		case TypeId::MOD:
		case TypeId::CLASS:
		case TypeId::TRAIT:
		case TypeId::INTERFACE: {
			if (((ModuleValue *)v)->_parent)
				_gcWalk(((ModuleValue *)v)->_parent);

			_gcWalk(((ModuleValue *)v)->scope.get());

			for (auto &i : ((ModuleValue *)v)->imports)
				_gcWalk(i.second);

			switch (typeId) {
				case TypeId::CLASS:
					for (auto &i : ((ClassValue *)v)->implInterfaces) {
						i.loadDeferredType(this);
						_gcWalk(i);
					}
					((ClassValue *)v)->parentClass.loadDeferredType(this);
					if (auto p = ((ClassValue *)v)->parentClass.resolveCustomType(); p)
						_gcWalk(p);
					break;
				case TypeId::TRAIT:
					for (auto &i : ((TraitValue *)v)->parents) {
						i.loadDeferredType(this);
						_gcWalk(i.getCustomTypeExData());
					}
					break;
				case TypeId::INTERFACE:
					for (auto &i : ((InterfaceValue *)v)->parents) {
						i.loadDeferredType(this);
						_gcWalk(i.getCustomTypeExData());
					}
					break;
			}

			break;
		}
		case TypeId::VAR: {
			VarValue *value = (VarValue *)v;

			_gcWalk(value->type);

			if (auto v = value->getData(); v)
				_gcWalk(v);

			if (value->_parent)
				_gcWalk(value->_parent);
			break;
		}
		case TypeId::ROOT:
			_gcWalk(((RootValue *)v)->scope.get());
			break;
		case TypeId::FN: {
			auto basicFn = (BasicFnValue *)v;

			if (basicFn->_parent)
				_gcWalk(basicFn->_parent);

			_gcWalk(basicFn->returnType);
			for (auto &i : basicFn->paramTypes)
				_gcWalk(i);

			if (!((BasicFnValue *)v)->isNative()) {
				auto value = (FnValue *)basicFn;
				for (size_t i = 0; i < value->nIns; ++i) {
					auto &ins = value->body[i];
					for (auto j : ins.operands) {
						if (j)
							_gcWalk(j);
					}
				}
			}
			break;
		}
		case TypeId::TYPENAME: {
			auto value = (TypeNameValue *)v;

			_gcWalk(value->_data);
			break;
		}
		case TypeId::REF: {
			auto value = (RefValue *)v;

			for (auto &i : value->entries)
				for (auto &j : i.genericArgs) {
					_gcWalk(j);
				}
			break;
		}
		case TypeId::ALIAS: {
			auto value = (AliasValue *)v;

			_gcWalk(value->src);
			break;
		}
		case TypeId::CONTEXT: {
			auto value = (ContextValue *)v;

			_gcWalk(*value->_context);
			break;
		}
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
		case TypeId::BOOL:
		case TypeId::STRING:
		case TypeId::REG_REF:
		case TypeId::LVAR_REF:
		case TypeId::ARG_REF:
			break;
		default:
			throw std::logic_error("Unhandled value type");
	}
}

void Runtime::_gcWalk(Context &ctxt) {
	for (auto &j : ctxt.majorFrames) {
		_gcWalk(const_cast<FnValue *>(j.curFn));
		if (j.scopeValue)
			_gcWalk(j.scopeValue);
		if (j.returnValue)
			_gcWalk(j.returnValue);
		if (j.thisObject)
			_gcWalk(j.thisObject);
		if (j.curExcept)
			_gcWalk(j.curExcept);
		for (auto &k : j.argStack)
			_gcWalk(k);
		for (auto &k : j.nextArgStack)
			_gcWalk(k);
		for (auto &k : j.localVars)
			_gcWalk(k);
		for (auto &k : j.regs)
			_gcWalk(k);
		for (auto &k : j.minorFrames) {
			for (auto &l : k.exceptHandlers)
				_gcWalk(l.type);
		}
	}
}

void Runtime::gc() {
	_flags |= _RT_INGC;

	bool foundDestructibleValues = false;

rescan:
	if (_rootValue)
		_gcWalk(_rootValue);

	// Walk contexts for each thread.
	for (auto &i : activeContexts)
		_gcWalk(*i.second);

	// Execute destructors for all destructible objects.
	destructingThreads.insert(std::this_thread::get_id());
	for (auto i : _createdValues) {
		if (!i->hostRefCount) {
			auto d = memberChainOf(i, "delete");
			if (d.size() && i->getType() == TypeId::OBJECT && !(((ObjectValue*)i)->objectFlags & OBJECT_PARENT)) {
				for(auto j : d) {
					_destructedValues.insert(j.first->owner);
					j.second->call({});
				}
				foundDestructibleValues = true;
			}
		}
	}
	destructingThreads.erase(std::this_thread::get_id());

	for (auto i : _createdValues) {
		if(i->hostRefCount) {
			_walkedValues.insert(i);
		} else
			delete i;
	}

	_createdValues.swap(_walkedValues);
	_walkedValues.clear();
	_destructedValues.clear();

	if (foundDestructibleValues) {
		foundDestructibleValues = false;
		goto rescan;
	}

	_szMemUsedAfterLastGc = _szMemInUse;
	_flags &= ~_RT_INGC;
}
