#include <slake/runtime.h>

using namespace slake;

struct TypeSlotGenericInstantiationTask {
	peff::RcObjectPtr<Runtime::GenericInstantiationContext> context;
	Type *slot;
};

struct ObjectGenericInstantiationTask {
	peff::RcObjectPtr<Runtime::GenericInstantiationContext> context;
	Object *obj;
};

struct Runtime::GenericInstantiationDispatcher {
	bool hasNext = false;

	peff::List<TypeSlotGenericInstantiationTask> nextWalkTypeSlots;
	peff::List<ObjectGenericInstantiationTask> nextWalkObjects;

	SLAKE_FORCEINLINE GenericInstantiationDispatcher(peff::Alloc *selfAllocator) : nextWalkTypeSlots(selfAllocator), nextWalkObjects(selfAllocator) {}

	SLAKE_FORCEINLINE InternalExceptionPointer pushTypeSlot(TypeSlotGenericInstantiationTask &&slot) noexcept {
		assert(verifyType(*slot.slot));

		if (!nextWalkTypeSlots.pushBack(std::move(slot))) {
			return OutOfMemoryError::alloc();
		}

		hasNext = true;

		return {};
	}

	SLAKE_FORCEINLINE InternalExceptionPointer pushObject(ObjectGenericInstantiationTask &&slot) noexcept {
		if (!nextWalkObjects.pushBack(std::move(slot))) {
			return OutOfMemoryError::alloc();
		}

		hasNext = true;

		return {};
	}
};

SLAKE_API void Runtime::invalidateGenericCache(MemberObject *i) {
	if (_genericCacheLookupTable.contains(i)) {
		// Remove the value from generic cache if it is unreachable.
		auto &lookupEntry = _genericCacheLookupTable.at(i);

		auto &table = _genericCacheDir.at(lookupEntry.originalObject);
		table.remove(lookupEntry.genericArgs);

		if (!table.size())
			_genericCacheDir.remove(lookupEntry.originalObject);

		_genericCacheLookupTable.remove(i);
	}
}

SLAKE_API InternalExceptionPointer Runtime::setGenericCache(const MemberObject *object, const GenericArgList &genericArgs, MemberObject *instantiatedObject) {
	if (!_genericCacheDir.contains(object)) {
		if (!_genericCacheDir.insert(+object, GenericCacheTable(getFixedAlloc())))
			return OutOfMemoryError::alloc();
	}
	// Store the instance into the cache.
	auto &cacheTable = _genericCacheDir.at(object);

	if (!cacheTable.contains(genericArgs)) {
		GenericArgList copiedGenericArgs(getFixedAlloc());
		if (!peff::copy(copiedGenericArgs, genericArgs)) {
			return OutOfMemoryError::alloc();
		}

		MemberObject *copiedInstantiatedObject = instantiatedObject;

		cacheTable.insert(std::move(copiedGenericArgs), std::move(copiedInstantiatedObject));
	} else {
		MemberObject *copiedInstantiatedObject = instantiatedObject;

		cacheTable.at(genericArgs) = std::move(copiedInstantiatedObject);
	}

	{
		GenericArgList copiedGenericArgList(getFixedAlloc());
		if (!peff::copy(copiedGenericArgList, genericArgs)) {
			return OutOfMemoryError::alloc();
		}
		_genericCacheLookupTable.insert(+instantiatedObject, { object, std::move(copiedGenericArgList) });
	}

	return {};
}

SLAKE_API InternalExceptionPointer slake::Runtime::_instantiateModuleFields(GenericInstantiationDispatcher &dispatcher, ModuleObject *mod, GenericInstantiationContext *instantiationContext) {
	size_t szRelocatedLocalFieldStorage = 0;

	HostObjectRef<ModuleObject> tmpMod;

	if (!(tmpMod = ModuleObject::alloc(this))) {
		return OutOfMemoryError::alloc();
	}

	const size_t nRecords = mod->fieldRecords.size();

	tmpMod->fieldRecords = std::move(mod->fieldRecords);
	tmpMod->localFieldStorage = std::move(mod->localFieldStorage);

	mod->fieldRecordIndices.clear();

	for (size_t i = 0; i < nRecords; ++i) {
		const FieldRecord &curOldFieldRecord = tmpMod->fieldRecords.at(i);

		FieldRecord curFieldRecord(tmpMod->selfAllocator.get());
		curFieldRecord.type = curOldFieldRecord.type;
		curFieldRecord.accessModifier = curOldFieldRecord.accessModifier;
		if (!curFieldRecord.name.build(curOldFieldRecord.name)) {
			return OutOfMemoryError::alloc();
		}

		SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, curFieldRecord.type, instantiationContext));

		if (!mod->appendFieldRecord(std::move(curFieldRecord))) {
			return OutOfMemoryError::alloc();
		}

		if (writeVar(EntityRef::makeFieldRef(mod, i), readVarUnsafe(EntityRef::makeFieldRef(tmpMod.get(), i)))) {
			return GenericFieldInitError::alloc(const_cast<Runtime *>(this)->getFixedAlloc(), mod, i);
		}
	}

	return {};
}

SLAKE_API InternalExceptionPointer slake::Runtime::_instantiateGenericObject(GenericInstantiationDispatcher &dispatcher, Type &type, GenericInstantiationContext *instantiationContext) {
	return dispatcher.pushTypeSlot({ instantiationContext, &type });
}

SLAKE_API InternalExceptionPointer slake::Runtime::_instantiateGenericObject(GenericInstantiationDispatcher &dispatcher, Value &value, GenericInstantiationContext *instantiationContext) {
	switch (value.valueType) {
		case ValueType::I8:
		case ValueType::I16:
		case ValueType::I32:
		case ValueType::I64:
		case ValueType::U8:
		case ValueType::U16:
		case ValueType::U32:
		case ValueType::U64:
		case ValueType::F32:
		case ValueType::F64:
		case ValueType::Bool:
		case ValueType::EntityRef:
		case ValueType::RegRef:
			break;
		case ValueType::TypeName:
			SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, value.getTypeName(), instantiationContext));
			break;
		case ValueType::Undefined:
			break;
		default:
			throw std::logic_error("Unhandled value type");
	}
	return {};
}

SLAKE_API InternalExceptionPointer slake::Runtime::_instantiateGenericObject(GenericInstantiationDispatcher &dispatcher, Object *v, GenericInstantiationContext *instantiationContext) {
	return dispatcher.pushObject({ instantiationContext, v });
}

InternalExceptionPointer Runtime::_mapGenericParams(const Object *v, GenericInstantiationContext *instantiationContext) const {
	instantiationContext->mappedObject = v;

	switch (v->getObjectKind()) {
		case ObjectKind::Class: {
			ClassObject *value = (ClassObject *)v;

			if (instantiationContext->genericArgs->size() != value->genericParams.size()) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MismatchedGenericArgumentNumberError::alloc(
						const_cast<Runtime *>(this)->getFixedAlloc()));
			}

			for (size_t i = 0; i < value->genericParams.size(); ++i) {
				peff::String copiedName(const_cast<Runtime *>(this)->getFixedAlloc());

				if (!copiedName.build(value->genericParams.at(i).name)) {
					return OutOfMemoryError::alloc();
				}

				Type copiedType = instantiationContext->genericArgs->at(i);

				instantiationContext->mappedGenericArgs.insert(std::move(copiedName), std::move(copiedType));
			}
			break;
		}
		case ObjectKind::Interface: {
			InterfaceObject *value = (InterfaceObject *)v;

			if (instantiationContext->genericArgs->size() != value->genericParams.size()) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MismatchedGenericArgumentNumberError::alloc(
						const_cast<Runtime *>(this)->getFixedAlloc()));
			}

			for (size_t i = 0; i < value->genericParams.size(); ++i) {
				peff::String copiedName(const_cast<Runtime *>(this)->getFixedAlloc());
				if (!peff::copyAssign(copiedName, value->genericParams.at(i).name))
					return OutOfMemoryError::alloc();
				Type copiedType = instantiationContext->genericArgs->at(i);
				instantiationContext->mappedGenericArgs.insert(std::move(copiedName), std::move(copiedType));
			}
			break;
		}
		default:;
	}
	return {};
}

SLAKE_API InternalExceptionPointer Runtime::_mapGenericParams(const FnOverloadingObject *ol, GenericInstantiationContext *instantiationContext) const {
	if (instantiationContext->genericArgs->size() != ol->genericParams.size()) {
		return allocOutOfMemoryErrorIfAllocFailed(
			MismatchedGenericArgumentNumberError::alloc(
				const_cast<Runtime *>(this)->getFixedAlloc()));
	}

	for (size_t i = 0; i < ol->genericParams.size(); ++i) {
		peff::String copiedName(const_cast<Runtime *>(this)->getFixedAlloc());

		if (!peff::copyAssign(copiedName, ol->genericParams.at(i).name)) {
			return OutOfMemoryError::alloc();
		}

		Type copiedType = instantiationContext->genericArgs->at(i);

		instantiationContext->mappedGenericArgs.insert(std::move(copiedName), std::move(copiedType));
	}
	return {};
}

SLAKE_API InternalExceptionPointer Runtime::instantiateGenericObject(const MemberObject *object, MemberObject *&objectOut, GenericInstantiationContext *instantiationContext) {
	// Try to look up in the cache.
	if (_genericCacheDir.contains(object)) {
		auto &table = _genericCacheDir.at(object);
		if (auto it = table.find(*instantiationContext->genericArgs); it != table.end()) {
			// Cache hit, return.
			objectOut = it.value();
			return {};
		}
		// Cache missed, go to the fallback.
	}

	Duplicator duplicator(this, getCurGenAlloc());

	// Cache missed, instantiate the value.
	MemberObject *value = (MemberObject *)object->duplicate(&duplicator);  // Make a duplicate of the original value.

	while (duplicator.tasks.size()) {
		if (!duplicator.exec()) {
			gc();
			return OutOfMemoryError::alloc();
		}
	}
	SLAKE_RETURN_IF_EXCEPT(_mapGenericParams(value, instantiationContext));

	GenericInstantiationDispatcher dispatcher(getFixedAlloc());
	// Instantiate the value.
	SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, value, instantiationContext));

	while (dispatcher.hasNext) {
		dispatcher.hasNext = false;

		for (auto &i : dispatcher.nextWalkTypeSlots) {
			Type &type = *i.slot;

			switch (type.typeId) {
				case TypeId::Instance: {
					if (type.isLoadingDeferred()) {
						IdRefObject *exData = (IdRefObject *)type.getCustomTypeExData();
						for (auto &j : exData->entries) {
							for (auto &k : j.genericArgs) {
								SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, k, i.context.get()));
							}
						}
					} else {
						HostObjectRef<IdRefObject> idRefObject = IdRefObject::alloc((Runtime *)this);

						if (!getFullRef(idRefObject->selfAllocator.get(), (MemberObject *)type.getCustomTypeExData(), idRefObject->entries))
							return OutOfMemoryError::alloc();

						// TODO: Add HostRefHolder for idRefObject.
						type = Type(type.typeId, idRefObject.get());

						SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, type, i.context.get()));
					}
					break;
				}
				case TypeId::Array: {
					bool isSucceeded;
					type = type.duplicate(isSucceeded);
					if (!isSucceeded)
						return OutOfMemoryError::alloc();
					if (!dispatcher.pushTypeSlot(TypeSlotGenericInstantiationTask{ i.context, &type.getArrayExData() }))
						return OutOfMemoryError::alloc();
					break;
				}
				case TypeId::Ref: {
					bool isSucceeded;
					type = type.duplicate(isSucceeded);
					if (!isSucceeded)
						return OutOfMemoryError::alloc();
					SLAKE_RETURN_IF_EXCEPT(dispatcher.pushTypeSlot(TypeSlotGenericInstantiationTask{ i.context, &type.getRefExData() }));
					break;
				}
				case TypeId::GenericArg: {
					HostObjectRef<StringObject> nameObject = (StringObject *)type.getGenericArgNameObject();

					if (auto it = i.context->mappedGenericArgs.find(nameObject->data); it != i.context->mappedGenericArgs.end()) {
						if (it.value().typeId != TypeId::Void)
							type = it.value();
					} else {
						peff::String paramName(getFixedAlloc());
						if (!paramName.build(nameObject->data)) {
							return OutOfMemoryError::alloc();
						}

						return allocOutOfMemoryErrorIfAllocFailed(
							GenericParameterNotFoundError::alloc(
								const_cast<Runtime *>(this)->getFixedAlloc(),
								std::move(paramName)));
					}
					break;
				}
			}
		}

		dispatcher.nextWalkTypeSlots.clear();

		for (auto &i : dispatcher.nextWalkObjects) {
			Object *v = i.obj;

			switch (v->getObjectKind()) {
				case ObjectKind::Class: {
					ClassObject *const value = (ClassObject *)v;

					if (value->genericParams.size() && value != i.context->mappedObject) {
						peff::HashMap<peff::String, Type> copiedMappedGenericArgs(getFixedAlloc());

						if (!peff::copyAssign(copiedMappedGenericArgs, i.context->mappedGenericArgs))
							return OutOfMemoryError::alloc();

						peff::RcObjectPtr<GenericInstantiationContext> newInstantiationContext;

						if (!(newInstantiationContext = peff::allocAndConstruct<GenericInstantiationContext>(
								  getFixedAlloc(), alignof(GenericInstantiationContext),
								  getFixedAlloc(),
								  i.context->mappedObject,
								  i.context->genericArgs,
								  std::move(copiedMappedGenericArgs)))) {
							return OutOfMemoryError::alloc();
						}

						// Map irreplaceable parameters to corresponding generic parameter reference type
						// and thus the generic types will keep unchanged.
						for (size_t i = 0; i < value->genericParams.size(); ++i) {
							peff::String copiedName(getFixedAlloc());
							if (!copiedName.build(value->genericParams.at(i).name))
								return OutOfMemoryError::alloc();
							newInstantiationContext->mappedGenericArgs.insert(std::move(copiedName), TypeId::Void);
						}

						SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, value->baseType, newInstantiationContext.get()));

						for (auto it = value->members.begin(); it != value->members.end(); ++it) {
							SLAKE_RETURN_IF_EXCEPT(dispatcher.pushObject({ newInstantiationContext, it.value() }));
						}

						SLAKE_RETURN_IF_EXCEPT(_instantiateModuleFields(dispatcher, value, newInstantiationContext.get()));
					} else {
						if (value == i.context->mappedObject) {
							if (!peff::copyAssign(value->genericArgs, *i.context->genericArgs))
								return OutOfMemoryError::alloc();
							for (auto i : i.context->mappedGenericArgs) {
								peff::String name(value->selfAllocator.get());
								Type type;

								if (!name.build(i.first)) {
									return OutOfMemoryError::alloc();
								}

								type = i.second;

								if (!value->mappedGenericArgs.insert(std::move(name), std::move(type)))
									return OutOfMemoryError::alloc();
							}
						}

						SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, value->baseType, i.context.get()));

						for (auto it = value->members.begin(); it != value->members.end(); ++it) {
							SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, it.value(), i.context.get()));
						}

						SLAKE_RETURN_IF_EXCEPT(_instantiateModuleFields(dispatcher, value, i.context.get()));
					}

					break;
				}
				case ObjectKind::Interface: {
					InterfaceObject *const value = (InterfaceObject *)v;

					if (!peff::copyAssign(value->genericArgs, *i.context->genericArgs))
						return OutOfMemoryError::alloc();
					if (!peff::copyAssign(value->mappedGenericArgs, i.context->mappedGenericArgs))
						return OutOfMemoryError::alloc();

					for (auto it = value->members.begin(); it != value->members.end(); ++it) {
						SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, it.value(), i.context.get()));
					}

					SLAKE_RETURN_IF_EXCEPT(_instantiateModuleFields(dispatcher, value, i.context.get()));

					break;
				}
				case ObjectKind::Fn: {
					FnObject *value = (FnObject *)v;

					if (i.context->mappedObject == value) {
						//
						// We expect there's only one overloading can be instantiated.
						// Uninstantiatable overloadings will be discarded.
						//
						FnOverloadingObject *matchedOverloading = nullptr;

						for (auto &i : value->overloadings) {
							if (i->genericParams.size() == instantiationContext->genericArgs->size()) {
								matchedOverloading = i;
								break;
							}
						}

						value->overloadings.clear();

						if (matchedOverloading) {
							SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, matchedOverloading, instantiationContext));
							if(!value->overloadings.insert(+matchedOverloading))
								return OutOfMemoryError::alloc();
						}
					} else {
						for (auto j : value->overloadings) {
							SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, j, i.context.get()));
						}
					}
					break;
				}
				case ObjectKind::String:
					break;
				case ObjectKind::IdRef: {
					IdRefObject *value = (IdRefObject *)v;

					for (auto &j : value->entries) {
						for (auto &k : j.genericArgs) {
							SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, k, i.context.get()));
						}
					}

					if (value->paramTypes) {
						for (auto &j : *value->paramTypes) {
							SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, j, i.context.get()));
						}
					}

					break;
				}
				default:
					throw std::logic_error("Unhandled object type");
			}
		}

		dispatcher.nextWalkObjects.clear();
	}

	SLAKE_RETURN_IF_EXCEPT(setGenericCache(object, *instantiationContext->genericArgs, value));

	objectOut = value;

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::_instantiateGenericObject(GenericInstantiationDispatcher &dispatcher, FnOverloadingObject *ol, GenericInstantiationContext *instantiationContext) {
	if (ol->genericParams.size() && ol->fnObject != instantiationContext->mappedObject) {
		peff::HashMap<peff::String, Type> copiedMappedGenericArgs(getFixedAlloc());

		if (!peff::copyAssign(copiedMappedGenericArgs, instantiationContext->mappedGenericArgs))
			return OutOfMemoryError::alloc();

		peff::NullAlloc tmpAlloc;
		GenericInstantiationContext newInstantiationContext = {
			&tmpAlloc,
			instantiationContext->mappedObject,
			instantiationContext->genericArgs,
			std::move(copiedMappedGenericArgs)
		};

		// Map irreplaceable parameters to corresponding generic parameter reference type
		// and thus the generic types will keep unchanged.
		for (size_t i = 0; i < ol->genericParams.size(); ++i) {
			peff::String copiedName(getFixedAlloc());
			if (!peff::copyAssign(copiedName, ol->genericParams.at(i).name))
				return OutOfMemoryError::alloc();
			newInstantiationContext.mappedGenericArgs.insert(std::move(copiedName), TypeId::Void);
		}

		newInstantiationContext.mappedObject = ol->fnObject;

		SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, ol, &newInstantiationContext));
	} else {
		for (auto &i : ol->paramTypes) {
			SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, i, instantiationContext));
		}

		SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, ol->returnType, instantiationContext));

		switch (ol->overloadingKind) {
			case FnOverloadingKind::Regular: {
				RegularFnOverloadingObject *overloading = (RegularFnOverloadingObject *)ol;

				for (auto &i : overloading->instructions) {
					for (size_t j = 0; j < i.nOperands; ++j) {
						SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, i.operands[j], instantiationContext));
					}
				}

				break;
			}
			case FnOverloadingKind::Native: {
				NativeFnOverloadingObject *overloading = (NativeFnOverloadingObject *)ol;

				break;
			}
		}
	}
	return {};
}
