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
	HostObjectRef<ArrayObject> instance = ArrayObject::alloc(this, type);

	instance->values.resize(size);

	for (size_t i = 0; i < size; ++i) {
		instance->values[i] = VarObject::alloc(this, ACCESS_PUB, type).release();
	}

	return instance.release();
}

static InstanceObject *_defaultClassInstantiator(Runtime *runtime, ClassObject *cls) {
	InstanceObject *parent = nullptr;

	if (cls->parentClass.typeId == TypeId::Class) {
		cls->parentClass.loadDeferredType(runtime);

		Object *parentClass = (ClassObject *)cls->parentClass.getCustomTypeExData();
		assert(parentClass->getType() == TypeId::Class);

		parent = runtime->newClassInstance((ClassObject *)parentClass);
	}

	HostObjectRef<InstanceObject> instance = InstanceObject::alloc(runtime, cls, parent);

	for (auto i : cls->scope->members) {
		switch (i.second->getType().typeId) {
			case TypeId::Var: {
				HostObjectRef<VarObject> var = VarObject::alloc(
					runtime,
					((BasicVarObject *)i.second)->getAccess(),
					((BasicVarObject *)i.second)->getVarType());

				// Initialize the variable if initial value is set
				var->setData(((VarObject *)i.second)->getData());

				instance->scope->addMember(i.first, var.release());
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
					HostObjectRef<FnObject> fn = FnObject::alloc(runtime);

					fn->overloadings = std::move(overloadings);

					for (InstanceObject *j = parent; j; j = j->_parent) {
						if (auto f = j->scope->getMember(i.first);
							f && (f->getType() == TypeId::Fn)) {
							fn->parentFn = (FnObject *)f;
							((FnObject *)f)->descentFn = fn.get();
						}
					}

					instance->scope->addMember(i.first, fn.release());
				}

				break;
			}
		}
	}
	return instance.release();
}
