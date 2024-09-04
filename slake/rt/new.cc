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

HostObjectRef<ArrayObject> Runtime::newArrayInstance(Runtime *rt, const Type &type, size_t length) {
	switch (type.typeId) {
		case TypeId::Value:
			switch (type.getValueTypeExData()) {
				case ValueType::I8:
					return I8ArrayObject::alloc(rt, length).get();
				case ValueType::I16:
					return I16ArrayObject::alloc(rt, length).get();
				case ValueType::I32:
					return I32ArrayObject::alloc(rt, length).get();
				case ValueType::I64:
					return I64ArrayObject::alloc(rt, length).get();
				case ValueType::U8:
					return U8ArrayObject::alloc(rt, length).get();
				case ValueType::U16:
					return U16ArrayObject::alloc(rt, length).get();
				case ValueType::U32:
					return U32ArrayObject::alloc(rt, length).get();
				case ValueType::U64:
					return U64ArrayObject::alloc(rt, length).get();
				case ValueType::F32:
					return F32ArrayObject::alloc(rt, length).get();
				case ValueType::F64:
					return F64ArrayObject::alloc(rt, length).get();
				case ValueType::Bool:
					return BoolArrayObject::alloc(rt, length).get();
			}
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			return ObjectRefArrayObject::alloc(rt, type, length).get();
		case TypeId::Any:
			return AnyArrayObject::alloc(rt, length).get();
		default:
			throw IncompatibleTypeError("Specified type is not constructible");
	}
}

static InstanceObject *_defaultClassInstantiator(Runtime *runtime, ClassObject *cls) {
	InstanceObject *parent = nullptr;

	HostObjectRef<InstanceObject> instance;

	if (cls->parentClass.typeId == TypeId::Instance) {
		cls->parentClass.loadDeferredType(runtime);

		Object *parentClass = (ClassObject *)cls->parentClass.getCustomTypeExData();
		assert(parentClass->getKind() == ObjectKind::Class);

		parent = runtime->newClassInstance((ClassObject *)parentClass);
	}

	if (parent) {
		instance = parent;
	} else {
		instance = InstanceObject::alloc(runtime);
	}

	instance->_class = cls;

	MethodTable *methodTable = nullptr;
	bool isMethodTablePresent = true;

	if (parent) {
		methodTable = parent->methodTable;
	} else {
		methodTable = cls->cachedInstantiatedMethodTable;
	}

	if (!methodTable) {
		methodTable = new MethodTable(instance.get());
		cls->cachedInstantiatedMethodTable = methodTable;
		isMethodTablePresent = false;
	}
	instance->methodTable = methodTable;

	for (const auto &i : cls->scope->members) {
		switch (i.second->getKind()) {
			case ObjectKind::Var: {
				HostObjectRef<RegularVarObject> var = RegularVarObject::alloc(
					runtime,
					((RegularVarObject *)i.second)->accessModifier,
					((RegularVarObject *)i.second)->getVarType());

				VarRefContext placeholderVarContext = {};
				// Initialize the variable if initial value is set
				var->setData(placeholderVarContext, ((RegularVarObject *)i.second)->getData(placeholderVarContext));

				instance->scope->addMember(i.first, var.release());
				break;
			}
			case ObjectKind::Fn: {
				if (!isMethodTablePresent) {
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
				}

				break;
			}
		}
	}

	return instance.release();
}
