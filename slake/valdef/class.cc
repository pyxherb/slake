#include <slake/type.h>
#include <slake/value.h>

using namespace Slake;

bool ClassValue::hasImplemented(const InterfaceValue *pInterface) const {
	// Check if the class has implemented the interface.
	for (auto &i : _interfaces) {
		assert(i.exData.customType->getType() == ValueType::INTERFACE);
		auto j = (InterfaceValue *)i.exData.customType;
		if (j == pInterface)
			return true;

		// Check if parents of the interface implements the input interface.
		for (auto k = (InterfaceValue *)j->_parent;
			 k; k = (InterfaceValue *)k->_parentClass.exData.customType) {
			if (k == pInterface)
				return true;
		}
	}
	return false;
}

/// @brief Check if the class is compatible with a trait.
/// @param t Trait to check.
/// @return true if compatible, false otherwise.
bool ClassValue::isCompatibleWith(const TraitValue *t) const {
	for (auto &i : t->_members) {
		MemberValue *v = nullptr;  // Corresponding member of this class.

		// Check if corresponding member presents.
		if (!_members.count(i.first)) {
			// Scan for parents if the member was not found.
			for (auto j = (ClassValue *)_parentClass.exData.customType;
				 j; j = (ClassValue *)j->_parentClass.exData.customType) {
				// Continue if the member still does not present.
				if (!(v = (MemberValue *)j->getMember(i.first)))
					continue;
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
				if (f->_params.size() != g->_params.size())
					return false;

				// Check for parameter types.
				for (size_t i = 0; i < f->_params.size(); ++i) {
					if (f->_params[i] != g->_params[i])
						return false;
				}

				break;
			}
		}
	}

	return true;
}
