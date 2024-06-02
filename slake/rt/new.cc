#include <slake/runtime.h>

using namespace slake;

static ObjectValue *_defaultClassInstantiator(Runtime *runtime, ClassValue *cls);

/// @brief Create a new class instance.
/// @param cls Class for instance creation.
/// @return Created instance of the class.
///
/// @note This function normalizes loading-deferred types.
ObjectValue *slake::Runtime::newClassInstance(ClassValue *cls) {
	if (cls->customInstantiator)
		return cls->customInstantiator(this, cls);
	return _defaultClassInstantiator(this, cls);
}

ArrayValue* slake::Runtime::newArrayInstance(Type type, uint32_t size) {
	ArrayValue *instance = new ArrayValue(this, type);

	instance->values.resize(size);

	for (size_t i = 0; i < size; ++i) {
		instance->values[i] = new VarValue(this, ACCESS_PUB, type);
	}

	return instance;
}

static ObjectValue *_defaultClassInstantiator(Runtime *runtime, ClassValue *cls) {
	ObjectValue *parent = nullptr;

	if (cls->parentClass.typeId == TypeId::Class) {
		cls->parentClass.loadDeferredType(runtime);

		Value *parentClass = (ClassValue *)cls->parentClass.getCustomTypeExData();
		assert(parentClass->getType() == TypeId::Class);

		parent = runtime->newClassInstance((ClassValue *)parentClass);
	}

	ObjectValue *instance = new ObjectValue(runtime, cls, parent);

	for (auto i : cls->scope->members) {
		switch (i.second->getType().typeId) {
			case TypeId::Var: {
				ValueRef<VarValue> var = new VarValue(
					runtime,
					((BasicVarValue *)i.second)->getAccess(),
					((BasicVarValue *)i.second)->getVarType());

				// Initialize the variable if initial value is set
				if (auto initValue = ((VarValue *)i.second)->getData(); initValue)
					var->setData(initValue);

				instance->scope->addMember(i.first, var.get());
				break;
			}
			case TypeId::Fn: {
				FnValue *fn = new FnValue(runtime);

				for (auto j : ((FnValue *)i.second)->overloadings) {
					if (!(j->access & ACCESS_STATIC))
						fn->overloadings.push_back(j);
				}

				for (ObjectValue* j = parent; j; j=j->_parent) {
					if (auto f = j->scope->getMember(i.first);
						f && (f->getType() == TypeId::Fn))
						fn->parentFn = (FnValue*)f;
				}

				instance->scope->addMember(i.first, fn);

				break;
			}
		}
	}
	return instance;
}
