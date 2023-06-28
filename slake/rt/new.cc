#include <slake/runtime.h>

using namespace Slake;

/// @brief Create a new class instance.
/// @param cls Class for instance creation.
/// @return Created instance of the class.
///
/// @note This function normalizes loading-deferred classes.
ObjectValue *Slake::Runtime::_newClassInstance(ClassValue *cls) {
	ObjectValue *instance = new ObjectValue(this, (MemberValue *)cls);

	if (cls->_parentClass.isLoadingDeferred()) {
		auto ref = resolveRef(cls->_parentClass.exData.deferred, cls);
		if (!ref)
			throw ResourceNotFoundError("Parent class not found");
		cls->_parentClass = Type(ValueType::CLASS, ref);
	}

	if (cls->_parentClass.valueType == ValueType::CLASS) {
		instance->_parent = _newClassInstance((ClassValue *)cls->_parentClass.exData.customType);
	}

	for (auto i : cls->_members) {
		switch (i.second->getType().valueType) {
			case ValueType::VAR: {
				ValueRef<VarValue> var = new VarValue(
					this,
					((VarValue *)i.second)->getAccess(),
					((VarValue *)i.second)->getVarType(),
					instance,
					i.first);

				// Set value of the variable with the initial value
				auto initValue = ((VarValue *)i.second)->getValue();
				if (initValue)
					var->setValue(*initValue);

				instance->addMember(i.first, *var);
				break;
			}
			case ValueType::FN: {
				if (!((FnValue *)i.second)->isStatic())
					instance->addMember(i.first, (MemberValue *)i.second);
				break;
			}
		}
	}
	return instance;
}
