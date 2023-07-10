#include <slake/runtime.h>

using namespace slake;

bool ClassValue::_isAbstract() const {
	for (auto i : _members) {
		switch (i.second->getType().valueType) {
			case ValueType::FN:
				if (((FnValue *)i.second)->isAbstract())
					return true;
				break;
		}
	}

	return false;
}

bool ClassValue::isAbstract() const {
	if (!(_flags & _CLS_ABSTRACT_INITED)) {
		if (_isAbstract())
			_flags |= _CLS_ABSTRACT;

		_flags |= _CLS_ABSTRACT_INITED;
	}
	return _flags & _CLS_ABSTRACT;
}

bool ClassValue::hasImplemented(const InterfaceValue *pInterface) const {
	for (auto &i : implInterfaces) {
		i.loadDeferredType(_rt);

		if (((InterfaceValue *)*(i.getCustomTypeExData()))->isDerivedFrom(pInterface))
			return true;
	}
	return false;
}

bool ClassValue::isCompatibleWith(const TraitValue *t) const {
	for (auto &i : t->_members) {
		const MemberValue *v = nullptr;	 // Corresponding member in this class.

		// Check if corresponding member presents.
		if (!(v = getMember(i.first))) {
			// Scan for parents if the member was not found.
			auto j = this;
			while (j->parentClass) {
				if (!(v = (MemberValue *)j->getMember(i.first))) {
					j->parentClass.loadDeferredType(_rt);
					j = (ClassValue*)*j->parentClass.getCustomTypeExData();
					continue;
				}
				goto found;
			}
			return false;
		}
	found:
		if (v->getType().valueType != i.second->getType().valueType)
			return false;

		// The class is incompatible if any corresponding member is private.
		if (!v->isPublic())
			return false;

		switch (v->getType().valueType) {
			case ValueType::VAR: {
				// Check for variable type.
				if (((VarValue *)v)->getVarType() != ((VarValue *)i.second)->getVarType())
					return false;
				break;
			}
			case ValueType::FN: {
				FnValue *f = (FnValue *)v, *g = (FnValue *)i.second;

				// Check for return type.
				if (f->_returnType != g->_returnType)
					return false;

				// Check for parameter number.
				if (f->_paramTypes.size() != g->_paramTypes.size())
					return false;

				// Check for parameter types.
				for (size_t i = 0; i < f->_paramTypes.size(); ++i) {
					if (f->_paramTypes[i] != g->_paramTypes[i])
						return false;
				}

				break;
			}
		}
	}

	if (t->parents.size()) {
		for (auto i : t->parents) {
			i.loadDeferredType(_rt);
			if (!isCompatibleWith((TraitValue *)*(i.getCustomTypeExData()))) {
				return false;
			}
		}
	}

	return true;
}

bool InterfaceValue::isDerivedFrom(const InterfaceValue *pInterface) const {
	if (pInterface == this)
		return true;

	for (auto &i : parents) {
		i.loadDeferredType(_rt);

		InterfaceValue *interface = (InterfaceValue *)*(i.getCustomTypeExData());

		if (interface->getType() != ValueType::INTERFACE)
			throw IncompatibleTypeError("Referenced type value is not an interface");

		if (interface->isDerivedFrom(pInterface))
			return true;
	}

	return false;
}
