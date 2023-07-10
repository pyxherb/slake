#include "../runtime.h"

void slake::Runtime::_gcWalk(Value *v) {
	if (v->_flags & VF_WALKED)
		return;

	v->_flags |= VF_WALKED;

	switch (v->getType().valueType) {
		case ValueType::OBJECT: {
			auto value = (ObjectValue *)v;
			for (auto &i : value->_members)
				_gcWalk(i.second);
			_gcWalk(value->_class);
			if (value->_parent)
				_gcWalk(*value->_parent);
			break;
		}
		case ValueType::ARRAY:
			for (auto &i : ((ArrayValue *)v)->values)
				_gcWalk(*i);
			break;
		case ValueType::MAP:
			break;
		case ValueType::CLASS: {
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

			break;
		}
		case ValueType::INTERFACE: {
			InterfaceValue *const value = (InterfaceValue *)v;

			if (value->_parent)
				_gcWalk(value->_parent);

			for (auto i : value->parents) {
				i.loadDeferredType(this);
				_gcWalk(*i.getCustomTypeExData());
			}

			for (auto &i : value->_members)
				_gcWalk(i.second);

			break;
		}
		case ValueType::TRAIT: {
			TraitValue *const value = (TraitValue *)v;

			if (value->_parent)
				_gcWalk(value->_parent);

			for (auto i : value->parents) {
				i.loadDeferredType(this);
				_gcWalk(*i.getCustomTypeExData());
			}

			for (auto &i : value->_members)
				_gcWalk(i.second);

			break;
		}
		case ValueType::STRUCT:
			break;
		case ValueType::VAR: {
			VarValue *value = (VarValue *)v;

			auto v = value->getData();
			if (v)
				_gcWalk(*v);
			break;
		}
		case ValueType::MOD: {
			ModuleValue *value = (ModuleValue *)v;

			if (value->_parent)
				_gcWalk(value->_parent);

			for (auto &i : value->_members)
				_gcWalk(i.second);
			break;
		}
		case ValueType::ROOT:
			for (auto &i : ((RootValue *)v)->_members)
				_gcWalk(*(i.second));
			break;
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
		case ValueType::BOOL:
		case ValueType::FN:
		case ValueType::REF:
			break;
		default:
			throw std::logic_error("Unhandled value type");
	}
}

void slake::Runtime::gc() {
	_isInGc = true;

	if (_rootValue)
		_gcWalk(_rootValue);

	// Walk contexts for each thread.
	for (auto &i : currentContexts) {
		auto &ctxt = i.second;
		// Walk for each major frames.
		for (auto &j : ctxt->majorFrames) {
			_gcWalk(const_cast<FnValue *>(*j.curFn));
			if (j.scopeValue)
				_gcWalk(*j.scopeValue);
			if (j.returnValue)
				_gcWalk(*j.returnValue);
			if (j.thisObject)
				_gcWalk(*j.thisObject);
			for (auto &k : j.argStack)
				_gcWalk(*k);
			for (auto &k : j.dataStack)
				_gcWalk(*k);
			// for(auto &k:j.minorFrames) {}
		}
	}

	std::unordered_set<Value *> unreachableValues;
	unreachableValues.swap(_extraGcTargets);

	if (_rootValue) {
		// Scan for GC targets
		for (auto i = _createdValues.begin(); i != _createdValues.end(); ++i) {
			if ((!((*i)->_flags & VF_WALKED)) && (!((*i)->_hostRefCount)))
				unreachableValues.insert(*i);
		}

		// Execute destructors for all destructible objects.
		destructingThreads.insert(std::this_thread::get_id());
		for (auto i : unreachableValues) {
			if (_createdValues.count(i)) {
				auto d = i->getMember("delete");
				if (d && i->getType() == ValueType::OBJECT)
					d->call(0, nullptr);
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
				if (d && i->getType() == ValueType::OBJECT)
					d->call(0, nullptr);
			}
		}
		destructingThreads.erase(std::this_thread::get_id());

		while (!_createdValues.empty()) {
			auto i = _createdValues.begin();
			if (!((*i)->_hostRefCount)) {
				delete *i;
			} else
				_createdValues.erase(i);
		}
	}

	for (auto i : _createdValues)
		i->_flags &= ~VF_WALKED;

	_szMemUsedAfterLastGc = _szMemInUse;
	_isInGc = false;
}
