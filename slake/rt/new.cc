#include <slake/runtime.h>

using namespace slake;

static InstanceObject *_defaultClassInstantiator(Runtime *runtime, ClassObject *cls);

/// @brief Create a new class instance.
/// @param cls Class for instance creation.
/// @return Created instance of the class.
///
/// @note This function normalizes loading-deferred types.
InstanceObject *slake::Runtime::newClassInstance(ClassObject *cls) {
	if (cls->customInstantiator)
		return cls->customInstantiator(this, cls);
	return _defaultClassInstantiator(this, cls);
}

ArrayObject *slake::Runtime::newArrayInstance(Type type, uint32_t size) {
	ArrayObject *instance = new ArrayObject(this, type);

	instance->values.resize(size);

	for (size_t i = 0; i < size; ++i) {
		instance->values[i] = new VarObject(this, ACCESS_PUB, type);
	}

	return instance;
}

static InstanceObject *_defaultClassInstantiator(Runtime *runtime, ClassObject *cls) {
	InstanceObject *parent = nullptr;

	if (cls->parentClass.typeId == TypeId::Class) {
		cls->parentClass.loadDeferredType(runtime);

		Object *parentClass = (ClassObject *)cls->parentClass.getCustomTypeExData();
		assert(parentClass->getType() == TypeId::Class);

		parent = runtime->newClassInstance((ClassObject *)parentClass);
	}

	InstanceObject *instance = new InstanceObject(runtime, cls, parent);

	for (auto i : cls->scope->members) {
		switch (i.second->getType().typeId) {
			case TypeId::Var: {
				VarObject *var = new VarObject(
					runtime,
					((BasicVarObject *)i.second)->getAccess(),
					((BasicVarObject *)i.second)->getVarType());

				// Initialize the variable if initial value is set
				var->setData(((VarObject *)i.second)->getData());

				instance->scope->addMember(i.first, var);
				break;
			}
			case TypeId::Fn: {
				std::deque<FnOverloadingObject *> overloadings;

				for (auto j : ((FnObject *)i.second)->overloadings) {
					if ((!(j->access & ACCESS_STATIC)) &&
						(j->overloadingFlags & OL_VIRTUAL))
						overloadings.push_back(j);
				}

				if (overloadings.size()) {
					// Link the method with method inherited from the parent.
					FnObject *fn = new FnObject(runtime);

					fn->overloadings = std::move(overloadings);

					for (InstanceObject *j = parent; j; j = j->_parent) {
						if (auto f = j->scope->getMember(i.first);
							f && (f->getType() == TypeId::Fn))
							fn->parentFn = (FnObject *)f;
					}

					instance->scope->addMember(i.first, fn);
				}

				break;
			}
		}
	}
	return instance;
}
