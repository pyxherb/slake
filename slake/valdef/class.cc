#include <slake/runtime.h>

using namespace slake;

bool ClassValue::_isAbstract() const {
	for (auto i : _members) {
		switch (i.second->getType().typeId) {
			case TypeId::FN:
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

bool ClassValue::consistsOf(const TraitValue *t) const {
	for (auto &i : t->_members) {
		const MemberValue *v = nullptr;	 // Corresponding member in this class.

		// Check if corresponding member presents.
		if (!(v = getMember(i.first))) {
			// Scan for parents if the member was not found.
			auto j = this;
			while (j->parentClass) {
				if (!(v = (MemberValue *)j->getMember(i.first))) {
					j->parentClass.loadDeferredType(_rt);
					j = (ClassValue *)*j->parentClass.getCustomTypeExData();
					continue;
				}
				goto found;
			}
			return false;
		}
	found:
		if (v->getType().typeId != i.second->getType().typeId)
			return false;

		// The class is incompatible if any corresponding member is private.
		if (!v->isPublic())
			return false;

		switch (v->getType().typeId) {
			case TypeId::VAR: {
				// Check variable type.
				if (((VarValue *)v)->getVarType() != ((VarValue *)i.second)->getVarType())
					return false;
				break;
			}
			case TypeId::FN: {
				FnValue *f = (FnValue *)v, *g = (FnValue *)i.second;

				// Check return type.
				if (f->returnType != g->returnType)
					return false;

				// Check parameter number.
				if (f->paramTypes.size() != g->paramTypes.size())
					return false;

				// Check parameter types.
				for (size_t i = 0; i < f->paramTypes.size(); ++i) {
					if (f->paramTypes[i] != g->paramTypes[i])
						return false;
				}

				break;
			}
		}
	}

	if (t->parents.size()) {
		for (auto &i : t->parents) {
			i.loadDeferredType(_rt);
			if (!consistsOf((TraitValue *)*(i.getCustomTypeExData()))) {
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

		if (interface->getType() != TypeId::INTERFACE)
			throw IncompatibleTypeError("Referenced type value is not an interface");

		if (interface->isDerivedFrom(pInterface))
			return true;
	}

	return false;
}

Value *ClassValue::duplicate() const {
	ClassValue *v = new ClassValue(_rt, 0, {});
	*v = *this;

	return (Value *)v;
}

Value *InterfaceValue::duplicate() const {
	InterfaceValue *v = new InterfaceValue(_rt, 0);
	*v = *this;

	return (Value *)v;
}

Value *TraitValue::duplicate() const {
	TraitValue *v = new TraitValue(_rt, 0);
	*v = *this;

	return (Value *)v;
}
