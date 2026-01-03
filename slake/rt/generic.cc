#include <slake/runtime.h>

using namespace slake;

struct TypeSlotGenericInstantiationTask {
	peff::RcObjectPtr<Runtime::GenericInstantiationContext> context;
	TypeRef *slot;
};

struct ValueSlotGenericInstantiationTask {
	peff::RcObjectPtr<Runtime::GenericInstantiationContext> context;
	Value *slot;
};

struct ValueInitGenericInstantiationTask {
	peff::RcObjectPtr<Runtime::GenericInstantiationContext> context;
	Reference dest;
	Value value;
};

struct ObjectGenericInstantiationTask {
	peff::RcObjectPtr<Runtime::GenericInstantiationContext> context;
	Object *obj;
};

struct Runtime::GenericInstantiationDispatcher {
	bool hasNext = false;

	peff::List<TypeSlotGenericInstantiationTask> nextWalkTypeSlots;
	peff::List<ValueSlotGenericInstantiationTask> nextWalkValueSlots;
	peff::List<ValueInitGenericInstantiationTask> nextWalkValueInits;
	peff::List<ObjectGenericInstantiationTask> nextWalkObjects;

	SLAKE_FORCEINLINE GenericInstantiationDispatcher(peff::Alloc *selfAllocator) : nextWalkTypeSlots(selfAllocator), nextWalkValueSlots(selfAllocator), nextWalkValueInits(selfAllocator), nextWalkObjects(selfAllocator) {}

	SLAKE_FORCEINLINE InternalExceptionPointer pushTypeSlot(TypeSlotGenericInstantiationTask &&slot) noexcept {
		if (!nextWalkTypeSlots.pushBack(std::move(slot))) {
			return OutOfMemoryError::alloc();
		}

		hasNext = true;

		return {};
	}

	SLAKE_FORCEINLINE InternalExceptionPointer pushTypeSlot(ValueSlotGenericInstantiationTask &&slot) noexcept {
		if (!nextWalkValueSlots.pushBack(std::move(slot))) {
			return OutOfMemoryError::alloc();
		}

		hasNext = true;

		return {};
	}

	SLAKE_FORCEINLINE InternalExceptionPointer pushValueInit(ValueInitGenericInstantiationTask &&slot) noexcept {
		if (!nextWalkValueInits.pushBack(std::move(slot))) {
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

SLAKE_API InternalExceptionPointer Runtime::setGenericCache(MemberObject *object, const peff::DynArray<Value> &genericArgs, MemberObject *instantiatedObject) {
	if (!_genericCacheDir.contains(object)) {
		if (!_genericCacheDir.insert(+object, GenericCacheTable(getFixedAlloc(), GenericArgListComparator())))
			return OutOfMemoryError::alloc();
	}
	// Store the instance into the cache.
	auto &cacheTable = _genericCacheDir.at(object);

	if (!cacheTable.contains(genericArgs)) {
		peff::DynArray<Value> copiedGenericArgs(getFixedAlloc());
		if (!copiedGenericArgs.build(genericArgs)) {
			return OutOfMemoryError::alloc();
		}

		cacheTable.insert(std::move(copiedGenericArgs), +instantiatedObject);
	} else {
		cacheTable.at(genericArgs) = instantiatedObject;
	}

	{
		peff::DynArray<Value> copiedGenericArgList(getFixedAlloc());
		if (!copiedGenericArgList.build(genericArgs)) {
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

		{
			FieldRecord curFieldRecord(tmpMod->selfAllocator.get());
			curFieldRecord.type = curOldFieldRecord.type;
			curFieldRecord.accessModifier = curOldFieldRecord.accessModifier;
			if (!curFieldRecord.name.build(curOldFieldRecord.name)) {
				return OutOfMemoryError::alloc();
			}

			if (!mod->appendFieldRecord(std::move(curFieldRecord))) {
				return OutOfMemoryError::alloc();
			}
		}

		FieldRecord &curFieldRecord = mod->fieldRecords.back();

		SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, curFieldRecord.type, instantiationContext));
		SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, Reference::makeStaticFieldRef(mod, i), readVarUnsafe(Reference::makeStaticFieldRef(tmpMod.get(), i)), instantiationContext));
	}

	return {};
}

SLAKE_API InternalExceptionPointer slake::Runtime::_instantiateGenericObject(GenericInstantiationDispatcher &dispatcher, Reference dest, Value value, GenericInstantiationContext *instantiationContext) {
	return dispatcher.pushValueInit({ instantiationContext, dest, value });
}

SLAKE_API InternalExceptionPointer slake::Runtime::_instantiateGenericObject(GenericInstantiationDispatcher &dispatcher, TypeRef &type, GenericInstantiationContext *instantiationContext) {
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
		case ValueType::Reference:
		case ValueType::RegIndex:
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
				if (!instantiationContext->mappedGenericArgs.insert(value->genericParams.at(i).name, Value(instantiationContext->genericArgs->at(i))))
					return OutOfMemoryError::alloc();
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
				if (!instantiationContext->mappedGenericArgs.insert(value->genericParams.at(i).name, Value(instantiationContext->genericArgs->at(i))))
					return OutOfMemoryError::alloc();
			}
			break;
		}
		case ObjectKind::FnOverloading:
			SLAKE_RETURN_IF_EXCEPT(_mapGenericParams(static_cast<const FnOverloadingObject *>(v), instantiationContext));
			break;
		case ObjectKind::Fn:
			break;
		default:
			std::terminate();
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
		Value copiedValue = instantiationContext->genericArgs->at(i);

		if (auto it = instantiationContext->mappedGenericArgs.find(ol->genericParams.at(i).name);
			it != instantiationContext->mappedGenericArgs.end()) {
			it.value() = copiedValue;
		} else {
			if (!instantiationContext->mappedGenericArgs.insert(ol->genericParams.at(i).name, std::move(copiedValue)))
				return OutOfMemoryError::alloc();
		}
	}
	return {};
}

SLAKE_API InternalExceptionPointer Runtime::instantiateGenericObject(MemberObject *object, MemberObject *&objectOut, GenericInstantiationContext *instantiationContext) {
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
	MemberObject *duplicatedValue = (MemberObject *)object->duplicate(&duplicator);	 // Make a duplicate of the original value.

	while (duplicator.tasks.size()) {
		if (!duplicator.exec()) {
			gc();
			return OutOfMemoryError::alloc();
		}
	}
	SLAKE_RETURN_IF_EXCEPT(_mapGenericParams(duplicatedValue, instantiationContext));

	GenericInstantiationDispatcher dispatcher(getFixedAlloc());
	// Instantiate the value.
	SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, duplicatedValue, instantiationContext));

	size_t iterateTimes = 0;

	peff::Set<CustomTypeDefObject *> deferredTypeDefs(getFixedAlloc());
	peff::Set<FnObject *> fns(getFixedAlloc());

	while (dispatcher.hasNext) {
		dispatcher.hasNext = false;

		++iterateTimes;

		{
			auto nextWalkTypeSlots = std::move(dispatcher.nextWalkTypeSlots);

			dispatcher.nextWalkTypeSlots = peff::List<TypeSlotGenericInstantiationTask>(getFixedAlloc());

			for (const auto &i : nextWalkTypeSlots) {
				TypeRef &type = *i.slot;

				switch (type.typeId) {
					/* case TypeId::Instance: {
						auto typeDef = (type.typeDef.getCustomTypeDef());

						if (typeDef->isLoadingDeferred()) {
							IdRefObject *exData = (IdRefObject *)typeDef->typeObject;
							for (auto &j : exData->entries) {
								for (auto &k : j.genericArgs) {
									SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, k, i.context.get()));
								}
							}

							if (!deferredTypeDefs.insert(+typeDef))
								return OutOfMemoryError::alloc();
						} else {
							HostObjectRef<IdRefObject> idRefObject = IdRefObject::alloc((Runtime *)this);

							if (!idRefObject) {
								return OutOfMemoryError::alloc();
							}

							if (!getFullRef(idRefObject->selfAllocator.get(), (MemberObject *)typeDef->typeObject, idRefObject->entries))
								return OutOfMemoryError::alloc();

							HostObjectRef<CustomTypeDefObject> typeDefObject = CustomTypeDefObject::alloc((Runtime *)this);

							if (!typeDefObject) {
								return OutOfMemoryError::alloc();
							}

							typeDefObject->typeObject = idRefObject.get();

							// TODO: Add HostRefHolder for idRefObject.
							type = TypeRef(type.typeId, typeDefObject.get());

							SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, type, i.context.get()));
						}
						break;
					}*/
					case TypeId::Array: {
						bool isSucceeded;
						type = type.duplicate(isSucceeded);
						if (!isSucceeded)
							return OutOfMemoryError::alloc();
						SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, (type.getArrayTypeDef())->elementType->typeRef, i.context.get()));
						if (auto td = getEqualTypeDef(type.typeDef); td)
							type.typeDef = td;
						else
							registerTypeDef(type.typeDef);
						break;
					}
					case TypeId::Ref: {
						bool isSucceeded;
						type = type.duplicate(isSucceeded);
						if (!isSucceeded)
							return OutOfMemoryError::alloc();
						SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, (type.getRefTypeDef())->referencedType->typeRef, i.context.get()));
						if (auto td = getEqualTypeDef(type.typeDef); td)
							type.typeDef = td;
						else
							registerTypeDef(type.typeDef);
						break;
					}
					case TypeId::ParamTypeList: {
						bool isSucceeded;
						type = type.duplicate(isSucceeded);
						if (!isSucceeded)
							return OutOfMemoryError::alloc();
						for (auto j : ((ParamTypeListTypeDefObject*)type.typeDef)->paramTypes)
							SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, j->typeRef, i.context.get()));
						if (auto td = getEqualTypeDef(type.typeDef); td)
							type.typeDef = td;
						else
							registerTypeDef(type.typeDef);
						break;
					}
					case TypeId::Unpacking: {
						bool isSucceeded;
						type = type.duplicate(isSucceeded);
						if (!isSucceeded)
							return OutOfMemoryError::alloc();
						SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, (type.getUnpackingTypeDef())->type->typeRef, i.context.get()));
						if (auto td = getEqualTypeDef(type.typeDef); td)
							type.typeDef = td;
						else
							registerTypeDef(type.typeDef);
						break;
					}
					case TypeId::GenericArg: {
						HostObjectRef<GenericArgTypeDefObject> typeDef = type.getGenericArgTypeDef();
						HostObjectRef<StringObject> nameObject = typeDef->nameObject;

						if (auto it = i.context->mappedGenericArgs.find(nameObject->data); it != i.context->mappedGenericArgs.end()) {
							Value fetchedArg = it.value();
							if (fetchedArg.valueType != ValueType::Invalid) {
								if (fetchedArg.valueType != ValueType::TypeName) {
									peff::String name(getFixedAlloc());
									if (!name.build(nameObject->data))
										return OutOfMemoryError::alloc();
									return allocOutOfMemoryErrorIfAllocFailed(
										GenericArgTypeError::alloc(
											const_cast<Runtime *>(this)->getFixedAlloc(),
											std::move(name)));
								}
								type = fetchedArg.getTypeName();
							}
						}
						else {
							peff::String paramName(getFixedAlloc());
							if (!paramName.build(nameObject->data)) {
								return OutOfMemoryError::alloc();
							}

							return allocOutOfMemoryErrorIfAllocFailed(
								GenericParameterNotFoundError::alloc(
									const_cast<Runtime*>(this)->getFixedAlloc(),
									std::move(paramName)));
						}
						break;
					}
				}
			}
		}

		{
			auto nextWalkValueSlots = std::move(dispatcher.nextWalkValueSlots);

			dispatcher.nextWalkTypeSlots = peff::List<TypeSlotGenericInstantiationTask>(getFixedAlloc());

			for (const auto &i : nextWalkValueSlots) {
				Value &value = *i.slot;

				switch (value.valueType) {
					case ValueType::TypeName: {
						TypeRef type = value.getTypeName();

						if (type.typeId == TypeId::GenericArg) {
							HostObjectRef<GenericArgTypeDefObject> typeDef = type.getGenericArgTypeDef();
							HostObjectRef<StringObject> nameObject = typeDef->nameObject;

							if (auto it = i.context->mappedGenericArgs.find(nameObject->data); it != i.context->mappedGenericArgs.end()) {
								if (it.value().valueType != ValueType::Invalid)
									value = it.value();
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
						}
					}
				}
			}
		}

		{
			auto nextWalkValueInits = std::move(dispatcher.nextWalkValueInits);

			dispatcher.nextWalkValueInits = peff::List<ValueInitGenericInstantiationTask>(getFixedAlloc());

			for (auto &i : nextWalkValueInits) {
				if (writeVar(i.dest, i.value)) {
					return GenericFieldInitError::alloc(const_cast<Runtime *>(this)->getFixedAlloc(), i.dest.asStaticField.moduleObject, i.dest.asStaticField.index);
				}
			}
		}

		{
			auto nextWalkObjects = std::move(dispatcher.nextWalkObjects);

			dispatcher.nextWalkObjects = peff::List<ObjectGenericInstantiationTask>(getFixedAlloc());

			for (const auto &i : nextWalkObjects) {
				Object *v = i.obj;

				switch (v->getObjectKind()) {
					case ObjectKind::Class: {
						ClassObject *const value = (ClassObject *)v;

						if (value->genericParams.size() && value != i.context->mappedObject) {
							peff::HashMap<std::string_view, Value> copiedMappedGenericArgs(getFixedAlloc());

							for (auto [k, v] : i.context->mappedGenericArgs) {
								if (!(copiedMappedGenericArgs.insert(std::string_view(k), Value(v))))
									return OutOfMemoryError::alloc();
							}

							peff::RcObjectPtr<GenericInstantiationContext> newInstantiationContext;

							if (!(newInstantiationContext = peff::allocAndConstruct<GenericInstantiationContext>(
									  getFixedAlloc(), alignof(GenericInstantiationContext),
									  getFixedAlloc(),
									  i.context->mappedObject,
									  i.context->genericArgs,
									  std::move(copiedMappedGenericArgs)))) {
								return OutOfMemoryError::alloc();
							}

							// Mark nested generic parameters which have the same name as
							// irreplaceable, thus they will not be replaced.
							for (size_t i = 0; i < value->genericParams.size(); ++i) {
								if (!newInstantiationContext->mappedGenericArgs.insert(value->genericParams.at(i).name, ValueType::Invalid))
									return OutOfMemoryError::alloc();
							}

							SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, value->baseType, newInstantiationContext.get()));

							for (auto it = value->members.begin(); it != value->members.end(); ++it) {
								SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, it.value(), newInstantiationContext.get()));
							}

							SLAKE_RETURN_IF_EXCEPT(_instantiateModuleFields(dispatcher, value, newInstantiationContext.get()));
						} else {
							// Save mapped generic arguments.
							if (value == i.context->mappedObject) {
								if (!value->genericArgs.build(*i.context->genericArgs))
									return OutOfMemoryError::alloc();
								for (auto i : i.context->mappedGenericArgs) {
									if (!value->mappedGenericArgs.insert(std::string_view(i.first), Value(i.second)))
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

						if (value->genericParams.size() && value != i.context->mappedObject) {
							peff::HashMap<std::string_view, Value> copiedMappedGenericArgs(getFixedAlloc());

							for (auto [k, v] : i.context->mappedGenericArgs) {
								if (!(copiedMappedGenericArgs.insert(std::string_view(k), Value(v))))
									return OutOfMemoryError::alloc();
							}

							peff::RcObjectPtr<GenericInstantiationContext> newInstantiationContext;

							if (!(newInstantiationContext = peff::allocAndConstruct<GenericInstantiationContext>(
									  getFixedAlloc(), alignof(GenericInstantiationContext),
									  getFixedAlloc(),
									  i.context->mappedObject,
									  i.context->genericArgs,
									  std::move(copiedMappedGenericArgs)))) {
								return OutOfMemoryError::alloc();
							}

							// Mark nested generic parameters which have the same name as
							// irreplaceable, thus they will not be replaced.
							for (size_t i = 0; i < value->genericParams.size(); ++i) {
								if (!newInstantiationContext->mappedGenericArgs.insert(value->genericParams.at(i).name, ValueType::Invalid))
									return OutOfMemoryError::alloc();
							}

							for (auto it = value->implTypes.begin(); it != value->implTypes.end(); ++it) {
								SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, *it, newInstantiationContext.get()));
							}

							for (auto it = value->members.begin(); it != value->members.end(); ++it) {
								SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, it.value(), newInstantiationContext.get()));
							}

							SLAKE_RETURN_IF_EXCEPT(_instantiateModuleFields(dispatcher, value, newInstantiationContext.get()));
						} else {
							// Save mapped generic arguments.
							if (value == i.context->mappedObject) {
								if (!value->genericArgs.build(*i.context->genericArgs))
									return OutOfMemoryError::alloc();
								for (auto [k, v] : i.context->mappedGenericArgs) {
									if (!(value->mappedGenericArgs.insert(std::string_view(k), Value(v))))
										return OutOfMemoryError::alloc();
								}
							}

							for (auto it = value->implTypes.begin(); it != value->implTypes.end(); ++it) {
								SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, *it, i.context.get()));
							}

							for (auto it = value->members.begin(); it != value->members.end(); ++it) {
								SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, it.value(), i.context.get()));
							}

							SLAKE_RETURN_IF_EXCEPT(_instantiateModuleFields(dispatcher, value, i.context.get()));
						}

						break;
					}
					case ObjectKind::Fn: {
						FnObject *value = (FnObject *)v;

						if (!fns.contains(value)) {
							if (!fns.insert(+value))
								return OutOfMemoryError::alloc();
						}

						if (i.context->mappedObject == value) {
							//
							// We expect there's only one overloading can be instantiated.
							// Uninstantiatable overloadings will be discarded.
							//
							peff::Map<FnSignature, FnOverloadingObject *, FnSignatureComparator, true> overloadings(value->overloadings.allocator());

							for (auto [signature, overloading] : value->overloadings) {
								if (overloading->genericParams.size() == instantiationContext->genericArgs->size()) {
									peff::HashMap<std::string_view, Value> copiedMappedGenericArgs(getFixedAlloc());

									for (auto [k, v] : i.context->mappedGenericArgs) {
										if (!(copiedMappedGenericArgs.insert(std::string_view(k), Value(v))))
											return OutOfMemoryError::alloc();
									}

									peff::RcObjectPtr<GenericInstantiationContext> newInstantiationContext;

									if (!(newInstantiationContext = peff::allocAndConstruct<GenericInstantiationContext>(
											  getFixedAlloc(), alignof(GenericInstantiationContext),
											  getFixedAlloc(),
											  i.context->mappedObject,
											  i.context->genericArgs,
											  std::move(copiedMappedGenericArgs)))) {
										return OutOfMemoryError::alloc();
									}

									SLAKE_RETURN_IF_EXCEPT(_mapGenericParams(overloading, newInstantiationContext.get()));

									SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, overloading, newInstantiationContext.get()));
									if (!overloadings.insert(
											{ overloading->paramTypes,
												overloading->isWithVarArgs(),
												overloading->genericParams.size(),
												overloading->overridenType },
											+overloading))
										return OutOfMemoryError::alloc();
								}
							}

							value->overloadings = std::move(overloadings);
						} else {
							for (auto j : value->overloadings) {
								size_t originalTypeSlotSize = dispatcher.nextWalkTypeSlots.size();
								if (j.second->genericParams.size()) {
									peff::HashMap<std::string_view, Value> copiedMappedGenericArgs(getFixedAlloc());

									for (auto [k, v] : i.context->mappedGenericArgs) {
										if (!(copiedMappedGenericArgs.insert(std::string_view(k), Value(v))))
											return OutOfMemoryError::alloc();
									}

									peff::RcObjectPtr<GenericInstantiationContext> newInstantiationContext;

									if (!(newInstantiationContext = peff::allocAndConstruct<GenericInstantiationContext>(
											  getFixedAlloc(), alignof(GenericInstantiationContext),
											  getFixedAlloc(),
											  i.context->mappedObject,
											  i.context->genericArgs,
											  std::move(copiedMappedGenericArgs)))) {
										return OutOfMemoryError::alloc();
									}

									// Mark nested generic parameters which have the same name as
									// irreplaceable, thus they will not be replaced.
									for (size_t i = 0; i < j.second->genericParams.size(); ++i) {
										if (!newInstantiationContext->mappedGenericArgs.insert(j.second->genericParams.at(i).name, ValueType::Invalid))
											return OutOfMemoryError::alloc();
									}

									SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, j.second, newInstantiationContext.get()));
								} else {
									SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(dispatcher, j.second, i.context.get()));
								}
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

						if (value->paramTypes.hasValue()) {
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
		}
	}

	for (auto i : fns) {
		for (auto j : i->overloadings) {
			// Expand the parameter pack.
			size_t originalParamNum;
			size_t nParams;
			do {
				originalParamNum = j.second->paramTypes.size();
				nParams = j.second->paramTypes.size();
				for (size_t k = 0; k < nParams; ++k) {
					auto &param = j.second->paramTypes.at(k);
					if (param.typeId == TypeId::Unpacking) {
						UnpackingTypeDefObject *typeDef = ((UnpackingTypeDefObject *)param.typeDef);
						ParamTypeListTypeDefObject *paramTypeList = (ParamTypeListTypeDefObject *)typeDef->type->typeRef.typeDef;

						if (paramTypeList->getTypeDefKind() != TypeDefKind::ParamTypeListTypeDef)
							std::terminate();

						if (!j.second->paramTypes.insertRangeUninitialized(k, paramTypeList->paramTypes.size() - 1))
							return OutOfMemoryError::alloc();

						for (size_t l = 0; l < paramTypeList->paramTypes.size(); ++l) {
							j.second->paramTypes.at(k + l) = paramTypeList->paramTypes.at(l)->typeRef;
						}
						nParams = j.second->paramTypes.size();
					}
				}
			} while (originalParamNum != nParams);
		}
		SLAKE_RETURN_IF_EXCEPT(i->resortOverloadings());
	}

	/* for (auto i : deferredTypeDefs) {
		slake::Reference entityRef;
		SLAKE_RETURN_IF_EXCEPT(resolveIdRef((IdRefObject*)i->typeObject, entityRef));

		if (!entityRef)
			std::terminate();

		if (entityRef.kind != ReferenceKind::ObjectRef)
			std::terminate();

		i->typeObject = entityRef.asObject;
	}*/

	SLAKE_RETURN_IF_EXCEPT(setGenericCache(object, *instantiationContext->genericArgs, duplicatedValue));

	objectOut = duplicatedValue;

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::_instantiateGenericObject(GenericInstantiationDispatcher &dispatcher, FnOverloadingObject *ol, GenericInstantiationContext *instantiationContext) {
	if (ol->genericParams.size() && ol->fnObject != instantiationContext->mappedObject) {
		peff::HashMap<std::string_view, Value> copiedMappedGenericArgs(getFixedAlloc());

		for (auto [k, v] : instantiationContext->mappedGenericArgs) {
			if (!(copiedMappedGenericArgs.insert(std::string_view(k), Value(v))))
				return OutOfMemoryError::alloc();
		}

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
			if (!newInstantiationContext.mappedGenericArgs.insert(ol->genericParams.at(i).name, ValueType::Invalid))
				return OutOfMemoryError::alloc();
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
