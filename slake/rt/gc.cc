#include "../runtime.h"

using namespace slake;

void Runtime::_gcWalk(Type &type) {
	switch (type.typeId) {
		case TypeId::OBJECT:
		case TypeId::CLASS:
		case TypeId::INTERFACE:
		case TypeId::TRAIT:
			_gcWalk(*type.getCustomTypeExData());
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
	if (v->_flags & VF_WALKED)
		return;

	v->_flags |= VF_WALKED;

	switch (v->getType().typeId) {
		case TypeId::OBJECT: {
			auto value = (ObjectValue *)v;
			for (auto &i : value->_members)
				_gcWalk(i.second);
			_gcWalk(value->_class);
			if (value->_parent)
				_gcWalk(*value->_parent);
			break;
		}
		case TypeId::ARRAY:
			for (auto &i : ((ArrayValue *)v)->values)
				_gcWalk(*i);
			break;
		case TypeId::MAP:
			break;
		case TypeId::CLASS: {
			ClassValue *const value = (ClassValue *)v;

			if (value->_parent)
				_gcWalk(value->_parent);

			{
				auto p = value->parentClass.resolveCustomType();
				if (p)
					_gcWalk(p);
			}

			for (auto &i : value->_members)
				_gcWalk(i.second);

			value->parentClass.loadDeferredType(this);
			_gcWalk(value->parentClass);

			for (auto &i : value->implInterfaces) {
				i.loadDeferredType(this);
				_gcWalk(i);
			}

			break;
		}
		case TypeId::INTERFACE: {
			InterfaceValue *const value = (InterfaceValue *)v;

			if (value->_parent)
				_gcWalk(value->_parent);

			for (auto &i : value->parents) {
				i.loadDeferredType(this);
				_gcWalk(*i.getCustomTypeExData());
			}

			for (auto &i : value->_members)
				_gcWalk(i.second);

			break;
		}
		case TypeId::TRAIT: {
			TraitValue *const value = (TraitValue *)v;

			if (value->_parent)
				_gcWalk(value->_parent);

			for (auto &i : value->parents) {
				i.loadDeferredType(this);
				_gcWalk(*i.getCustomTypeExData());
			}

			for (auto &i : value->_members)
				_gcWalk(i.second);

			break;
		}
		case TypeId::VAR: {
			VarValue *value = (VarValue *)v;

			_gcWalk(value->type);

			auto v = value->getData();
			if (v)
				_gcWalk(*v);

			if (value->_parent)
				_gcWalk(value->_parent);
			break;
		}
		case TypeId::MOD: {
			ModuleValue *value = (ModuleValue *)v;

			if (value->_parent)
				_gcWalk(value->_parent);

			for (auto &i : value->_members)
				_gcWalk(i.second);
			break;
		}
		case TypeId::ROOT:
			for (auto &i : ((RootValue *)v)->_members)
				_gcWalk(*(i.second));
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
					for (size_t j = 0; j < ins.operands.size(); ++j) {
						auto operand = *ins.operands[j];
						if (operand)
							_gcWalk(operand);
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

			_gcWalk(*value->_src);
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
			auto t = v->getType().typeId;
			throw std::logic_error("Unhandled value type");
	}
}

void Runtime::_gcWalk(Context& ctxt) {
	for (auto &j : ctxt.majorFrames) {
		_gcWalk(const_cast<FnValue *>(*j.curFn));
		if (j.scopeValue)
			_gcWalk(*j.scopeValue);
		if (j.returnValue)
			_gcWalk(*j.returnValue);
		if (j.thisObject)
			_gcWalk(*j.thisObject);
		if (j.curExcept)
			_gcWalk(*j.curExcept);
		for (auto &k : j.argStack)
			_gcWalk(*k);
		for (auto &k : j.nextArgStack)
			_gcWalk(*k);
		for (auto &k : j.localVars)
			_gcWalk(*k);
		for (auto &k : j.minorFrames) {
			for (auto &l : k.exceptHandlers)
				_gcWalk(l.type);
			for (auto &l : k.gpRegs)
				_gcWalk(*l);
			for (auto &l : k.tmpRegs)
				_gcWalk(*l);
		}
	}
}

void Runtime::gc() {
	_flags |= _RT_INGC;

	if (_rootValue)
		_gcWalk(_rootValue);

	// Walk contexts for each thread.
	for (auto &i : activeContexts)
		_gcWalk(*i.second);

	std::unordered_set<Value *> unreachableValues;
	unreachableValues.swap(_extraGcTargets);

	if (_rootValue) {
		// Scan for GC targets
		for (auto i = _createdValues.begin(); i != _createdValues.end(); ++i) {
			auto type = (*i)->getType();
			if ((!((*i)->_flags & VF_WALKED)) && (!((*i)->hostRefCount)))
				unreachableValues.insert(*i);
		}

		// Execute destructors for all destructible objects.
		destructingThreads.insert(std::this_thread::get_id());
		for (auto i : unreachableValues) {
			if (_createdValues.count(i)) {
				auto d = i->getMember("delete");
				if (d && i->getType() == TypeId::OBJECT)
					d->call({});
			}
		}
		destructingThreads.erase(std::this_thread::get_id());

		for (auto i : unreachableValues) {
			if (_createdValues.count(i))
				delete i;
		}
	} else {
		destructingThreads.insert(std::this_thread::get_id());
		for (auto i : _createdValues) {
			if (_createdValues.count(i)) {
				auto d = i->getMember("delete");
				if (d && i->getType() == TypeId::OBJECT)
					d->call({});
			}
		}
		destructingThreads.erase(std::this_thread::get_id());

		while (!_createdValues.empty()) {
			auto i = _createdValues.begin();
			if (!((*i)->hostRefCount)) {
				delete *i;
			} else
				_createdValues.erase(i);
		}
	}

	for (auto i : _createdValues)
		i->_flags &= ~VF_WALKED;

	_szMemUsedAfterLastGc = _szMemInUse;
	_flags &= ~_RT_INGC;
}
