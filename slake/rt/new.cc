#include <slake/runtime.h>

using namespace slake;

/// @brief Create a new class instance.
/// @param cls Class for instance creation.
/// @return Created instance of the class.
///
/// @note This function normalizes loading-deferred types.
ObjectValue *slake::Runtime::_newClassInstance(ClassValue *cls) {
	ObjectValue *parent = nullptr;

	if (cls->parentClass.typeId == TypeId::Class) {
		cls->parentClass.loadDeferredType(this);
		parent = _newClassInstance((ClassValue *)cls->parentClass.getCustomTypeExData());
	}

	ObjectValue *instance = new ObjectValue(this, cls, parent);

	for (auto i : cls->scope->members) {
		switch (i.second->getType().typeId) {
			case TypeId::Var: {
				ValueRef<VarValue> var = new VarValue(
					this,
					((VarValue *)i.second)->getAccess(),
					((VarValue *)i.second)->getVarType());

				// Initialize the variable if initial value is set
				if (auto initValue = ((VarValue *)i.second)->getData(); initValue)
					var->setData(initValue);

				instance->scope->addMember(i.first, var.get());
				break;
			}
			case TypeId::Fn: {
				if (!((FnValue *)i.second)->isStatic())
					instance->scope->addMember(i.first, (MemberValue *)i.second->duplicate());
				break;
			}
		}
	}
	return instance;
}
