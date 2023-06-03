#include "../runtime.h"

void Slake::Runtime::_gcWalk(Value *v, uint8_t gcStat) {
	uint8_t valueStat = v->flags & VF_GCSTAT ? 1 : 0;
	if (valueStat == gcStat)
		return;

	if (gcStat)
		v->flags |= VF_GCSTAT;
	else
		v->flags &= ~VF_GCSTAT;

	switch (v->getType().valueType) {
		case ValueType::OBJECT: {
			auto value = (ObjectValue *)v;
			for (auto &i : value->_members)
				_gcWalk(i.second, gcStat);
			_gcWalk(value->_type, gcStat);
			break;
		}
		case ValueType::ARRAY:
			for (auto &i : ((ArrayValue *)v)->values)
				_gcWalk(*i, gcStat);
			break;
		case ValueType::MAP:
			break;
		case ValueType::CLASS: {
			ClassValue *const value = (ClassValue *)v;

			if (value->_parent)
				_gcWalk(value->_parent, gcStat);

			{
				auto p = value->_parentClass.resolveCustomType();
				if (p)
					_gcWalk(p, gcStat);
			}

			for (auto &i : value->_members)
				_gcWalk(i.second, gcStat);

			break;
		}
		case ValueType::STRUCT:
			break;
		case ValueType::VAR: {
			VarValue *value = (VarValue *)v;

			auto v = value->getValue();
			if (v)
				_gcWalk(*v, gcStat);
			break;
		}
		case ValueType::MOD: {
			ModuleValue *value = (ModuleValue *)v;

			if (value->_parent)
				_gcWalk(value->_parent, gcStat);

			for (auto &i : value->_members)
				_gcWalk(i.second, gcStat);
			break;
		}
		case ValueType::ROOT:
			for (auto &i : ((RootValue *)v)->_members)
				_gcWalk(*(i.second), gcStat);
			break;
	}
}

void Slake::Runtime::gc() {
	_isInGc = true;

	uint8_t gcStat = _internalFlags & RT_LASTGCSTAT ? 1 : 0;
	if (_rootValue)
		_gcWalk(_rootValue, gcStat);

	// Walk contexts for each thread.
	for (auto &i : threadCurrentContexts) {
		auto &ctxt = i.second;
		// Walk for each major frames.
		for (auto &j : ctxt->majorFrames) {
			_gcWalk(*j.curFn, gcStat);
			if (j.scopeValue)
				_gcWalk(*j.scopeValue, gcStat);
			if (j.returnValue)
				_gcWalk(*j.returnValue, gcStat);
			if (j.thisObject)
				_gcWalk(*j.thisObject, gcStat);
			for (auto &k : j.argStack)
				_gcWalk(*k, gcStat);
			for (auto &k : j.dataStack)
				_gcWalk(*k, gcStat);
			// Walking for minor frames are currently unneeded.
		}
	}

	
	if (_rootValue) {
		for (auto i = _createdValues.begin(); i != _createdValues.end();) {
			if ((((*i)->flags & VF_GCSTAT ? 1 : 0) != gcStat) && (!((*i)->_hostRefCount)))
				delete *(i++);
			else
				++i;
		}
	} else {
		while (!_createdValues.empty()) {
			auto i = _createdValues.begin();
			if ((((*i)->flags & VF_GCSTAT ? 1 : 0) != gcStat) && (!((*i)->_hostRefCount)))
				delete *i;
			else
				_createdValues.erase(i);
		}
	}

	if (gcStat)
		_internalFlags &= ~RT_LASTGCSTAT;
	else
		_internalFlags |= RT_LASTGCSTAT;

	_szMemUsedAfterLastGc = _szMemInUse;
	_isInGc = false;
}
