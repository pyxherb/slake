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
			for (auto i : value->_members)
				_gcWalk(i.second, gcStat);
			_gcWalk(value->_type, gcStat);
			break;
		}
		case ValueType::ARRAY:
			for (auto i : ((ArrayValue *)v)->values)
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

			for (auto i : value->_members)
				_gcWalk(*(i.second), gcStat);

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

			for (auto i : value->_members)
				_gcWalk(*(i.second), gcStat);
			break;
		}
		case ValueType::ROOT:
			for (auto i : ((RootValue *)v)->_members)
				_gcWalk(*(i.second), gcStat);
			break;
	}
}

void Slake::Runtime::_gcWalkForExecContext(ExecContext &ctxt, uint8_t gcStat) {
	for (auto i : ctxt.args)
		_gcWalk(*i, gcStat);
	_gcWalk(*ctxt.fn, gcStat);
	if (ctxt.pThis)
		_gcWalk(*ctxt.pThis, gcStat);
	if (ctxt.scopeValue)
		_gcWalk(*ctxt.scopeValue, gcStat);
}

void Slake::Runtime::gc() {
	isInGc = true;

	uint8_t gcStat = _internalFlags & RTI_LASTGCSTAT ? 1 : 0;
	if (_rootValue)
		_gcWalk(_rootValue, gcStat);

	for (auto i : threadCurrentContexts) {
		auto &ctxt = i.second;
		if (ctxt->retValue)
			_gcWalk(*ctxt->retValue, gcStat);
		_gcWalkForExecContext(ctxt->execContext, gcStat);
		for (auto j : ctxt->callingStack)
			_gcWalkForExecContext(j, gcStat);
		for (auto j : ctxt->dataStack)
			_gcWalk(*j, gcStat);
	}

	for (auto i = _createdValues.begin(); i != _createdValues.end();) {
		if ((((*i)->flags & VF_GCSTAT ? 1 : 0) != gcStat) && (!((*i)->_hostRefCount)))
			delete *(i++);
		else
			++i;
	}

	if (gcStat)
		_internalFlags &= ~RTI_LASTGCSTAT;
	else
		_internalFlags |= RTI_LASTGCSTAT;

	szLastGcMemUsed = szInUse;
	isInGc = false;
}
