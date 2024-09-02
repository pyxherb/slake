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

	if (cls->parentClass.typeId == TypeId::Instance) {
		cls->parentClass.loadDeferredType(runtime);

		Object *parentClass = (ClassObject *)cls->parentClass.getCustomTypeExData();
		assert(parentClass->getKind() == ObjectKind::Class);

		parent = runtime->newClassInstance((ClassObject *)parentClass);
	}

	HostObjectRef<InstanceObject> instance = InstanceObject::alloc(runtime, cls, parent);

	MethodTable *methodTable;

	if (parent) {
		methodTable = parent->methodTable;
	} else {
		methodTable = cls->cachedInstantiatedMethodTable;
	}

	if (!methodTable) {
		methodTable = new MethodTable(instance.get());
		cls->cachedInstantiatedMethodTable = methodTable;
	}
	instance->methodTable = methodTable;

	for (const auto &i : cls->scope->members) {
		switch (i.second->getKind()) {
			case ObjectKind::Var: {
				HostObjectRef<VarObject> var = VarObject::alloc(
					runtime,
					((BasicVarObject *)i.second)->accessModifier,
					((BasicVarObject *)i.second)->getVarType());

				VarRefContext placeholderVarContext = {};
				// Initialize the variable if initial value is set
				var->setData(placeholderVarContext, ((VarObject *)i.second)->getData(placeholderVarContext));

				instance->scope->addMember(i.first, var.release());
				break;
			}
			case ObjectKind::Fn: {
				std::deque<FnOverloadingObject *> overloadings;

				for (auto j : ((FnObject *)i.second)->overloadings) {
					if ((!(j->access & ACCESS_STATIC)) &&
						(j->overloadingFlags & OL_VIRTUAL))
						overloadings.push_back(j);
				}

				if (i.first == "delete") {
					std::deque<Type> destructorParamTypes;
					GenericParamList destructorGenericParamList;

					for (auto &j : overloadings) {
						if (isDuplicatedOverloading(j, destructorParamTypes, destructorGenericParamList, false)) {
							methodTable->destructors.push_front(j);
							break;
						}
					}
				} else {
					if (overloadings.size()) {
						// Link the method with method inherited from the parent.
						HostObjectRef<FnObject> fn;
						auto &methodSlot = methodTable->methods[i.first];

						if (auto m = methodTable->getMethod(i.first); m)
							fn = m;
						else {
							fn = FnObject::alloc(runtime);
							methodSlot = fn.get();
						}

						for (auto &j : overloadings) {
							for (auto &k : methodSlot->overloadings) {
								if (isDuplicatedOverloading(
										k,
										j->paramTypes,
										j->genericParams,
										j->overloadingFlags & OL_VARG)) {
									methodSlot->overloadings.erase(k);
									break;
								}
							}
							methodSlot->overloadings.insert(j);
						}
					}
				}

				break;
			}
		}
	}

	return instance.release();
}
