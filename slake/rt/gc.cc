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
		case TypeId::Object:
		case TypeId::Class:
		case TypeId::Interface:
		case TypeId::Trait:
			_gcWalk(type.getCustomTypeExData());
			break;
		case TypeId::Array:
			if (auto t = type.getArrayExData(); t)
				_gcWalk(t);
			break;
		case TypeId::Var:
		case TypeId::Module:
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
		case TypeId::String:
		case TypeId::Bool:
		case TypeId::Fn:
		case TypeId::Ref:
		case TypeId::None:
		case TypeId::GenericArg:
		case TypeId::Alias:
		case TypeId::Any:
		case TypeId::RegRef:
		case TypeId::LocalVarRef:
		case TypeId::ArgRef:
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

	if (v->scope)
		_gcWalk(v->scope);

	switch (auto typeId = v->getType().typeId; typeId) {
		case TypeId::Object: {
			auto value = (ObjectValue *)v;
			_gcWalk(value->_class);
			if (value->_parent)
				_gcWalk(value->_parent);
			break;
		}
		case TypeId::Array: {
			auto value = (ArrayValue *)v;

			_gcWalk(value->type);

			for (auto &i : value->values)
				_gcWalk(i);
			break;
		}
		case TypeId::Module:
		case TypeId::Class:
		case TypeId::Trait:
		case TypeId::Interface: {
			if (((ModuleValue *)v)->_parent)
				_gcWalk(((ModuleValue *)v)->_parent);

			for (auto &i : ((ModuleValue *)v)->imports)
				_gcWalk(i.second);

			switch (typeId) {
				case TypeId::Class:
					for (auto &i : ((ClassValue *)v)->implInterfaces) {
						i.loadDeferredType(this);
						_gcWalk(i);
					}
					((ClassValue *)v)->parentClass.loadDeferredType(this);
					if (auto p = ((ClassValue *)v)->parentClass.resolveCustomType(); p)
						_gcWalk(p);
					break;
				case TypeId::Trait:
					for (auto &i : ((TraitValue *)v)->parents) {
						i.loadDeferredType(this);
						_gcWalk(i.getCustomTypeExData());
					}
					break;
				case TypeId::Interface:
					for (auto &i : ((InterfaceValue *)v)->parents) {
						i.loadDeferredType(this);
						_gcWalk(i.getCustomTypeExData());
					}
					break;
			}

			break;
		}
		case TypeId::Var: {
			VarValue *value = (VarValue *)v;

			_gcWalk(value->type);

			if (auto v = value->getData(); v)
				_gcWalk(v);

			if (value->_parent)
				_gcWalk(value->_parent);
			break;
		}
		case TypeId::RootValue:
			break;
		case TypeId::Fn: {
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
		case TypeId::TypeName: {
			auto value = (TypeNameValue *)v;

			_gcWalk(value->_data);
			break;
		}
		case TypeId::Ref: {
			auto value = (RefValue *)v;

			for (auto &i : value->entries)
				for (auto &j : i.genericArgs) {
					_gcWalk(j);
				}
			break;
		}
		case TypeId::Alias: {
			auto value = (AliasValue *)v;

			_gcWalk(value->src);
			break;
		}
		case TypeId::Context: {
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
		case TypeId::Bool:
		case TypeId::String:
		case TypeId::RegRef:
		case TypeId::LocalVarRef:
		case TypeId::ArgRef:
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
			auto d = i->getMemberChain("delete");
			if ((d.size()) &&
				(i->getType() == TypeId::Object) &&
				(!(((ObjectValue *)i)->objectFlags & OBJECT_PARENT))) {
				for (auto &j : d) {
					_destructedValues.insert(j.first->owner);
					j.second->call(i, {});
				}
				foundDestructibleValues = true;
			}
		}
	}
	destructingThreads.erase(std::this_thread::get_id());

	for (auto i : _createdValues) {
		if (i->hostRefCount) {
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
