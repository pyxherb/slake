#include <slake/runtime.h>

using namespace slake;

/// @brief Create a new class instance.
/// @param cls Class for instance creation.
/// @return Created instance of the class.
///
/// @note This function normalizes loading-deferred types.
ObjectValue *slake::Runtime::_newClassInstance(ClassValue *cls) {
	ObjectValue *instance = new ObjectValue(this, cls);

	if (cls->parentClass.typeId == TypeId::CLASS) {
		cls->parentClass.loadDeferredType(this);
		instance->_parent = _newClassInstance((ClassValue *)*cls->parentClass.getCustomTypeExData());
	}

	for (auto i : cls->_members) {
		switch (i.second->getType().typeId) {
			case TypeId::VAR: {
				ValueRef<VarValue> var = new VarValue(
					this,
					((VarValue *)i.second)->getAccess(),
					((VarValue *)i.second)->getVarType());

				// Initialize the variable if initial value is set
				if (auto initValue = ((VarValue *)i.second)->getData(); initValue)
					var->setData(initValue);

				instance->addMember(i.first, *var);
				break;
			}
			case TypeId::FN: {
				if (!((FnValue *)i.second)->isStatic())
					instance->addMember(i.first, (MemberValue *)i.second->duplicate());
				break;
			}
		}
	}
	return instance;
}

ObjectValue *slake::Runtime::_newGenericClassInstance(ClassValue *cls, std::deque<Type> &genericArgs) {
	ObjectValue *instance = new ObjectValue(this, cls);

	if (cls->parentClass.typeId == TypeId::CLASS) {
		cls->parentClass.loadDeferredType(this);
		instance->_parent = _newClassInstance((ClassValue *)*cls->parentClass.getCustomTypeExData());
	}

	for (auto i : cls->_members) {
		switch (i.second->getType().typeId) {
			case TypeId::VAR: {
				ValueRef<VarValue> var = new VarValue(
					this,
					((VarValue *)i.second)->getAccess(),
					((VarValue *)i.second)->getVarType());

				// Set value of the variable with the initial value
				if (auto initValue = ((VarValue *)i.second)->getData(); initValue)
					var->setData(initValue);

				instance->addMember(i.first, *var);
				break;
			}
			case TypeId::FN: {
				if (!((FnValue *)i.second)->isStatic())
					instance->addMember(i.first, (MemberValue *)i.second);
				break;
			}
		}
	}

	instance->_genericArgs = genericArgs;
	return instance;
}
