#include <slake/runtime.h>

using namespace slake;

/// @brief Create a new class instance.
/// @param cls Class for instance creation.
/// @return Created instance of the class.
///
/// @note This function normalizes loading-deferred classes.
ObjectValue *slake::Runtime::_newClassInstance(ClassValue *cls) {
	ObjectValue *instance = new ObjectValue(this, cls);

	if (cls->parentClass.valueType == ValueType::CLASS) {
		cls->parentClass.loadDeferredType(this);
		instance->_parent = _newClassInstance((ClassValue *)*cls->parentClass.getCustomTypeExData());
	}

	for (auto i : cls->_members) {
		switch (i.second->getType().valueType) {
			case ValueType::VAR: {
				ValueRef<VarValue> var = new VarValue(
					this,
					((VarValue *)i.second)->getAccess(),
					((VarValue *)i.second)->getVarType());

				// Set value of the variable with the initial value
				auto initValue = ((VarValue *)i.second)->getData();
				if (initValue)
					var->setData(*initValue);

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
