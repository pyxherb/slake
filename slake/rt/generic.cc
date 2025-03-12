#include <slake/runtime.h>

using namespace slake;

SLAKE_API void Runtime::invalidateGenericCache(Object *i) {
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

SLAKE_API InternalExceptionPointer Runtime::setGenericCache(const Object *object, const GenericArgList &genericArgs, Object *instantiatedObject) {
	if (!_genericCacheDir.contains(object)) {
		if (!_genericCacheDir.insert(+object, GenericCacheTable(&globalHeapPoolAlloc)))
			return OutOfMemoryError::alloc();
	}
	// Store the instance into the cache.
	auto &cacheTable = _genericCacheDir.at(object);

	if (!cacheTable.contains(genericArgs)) {
		GenericArgList copiedGenericArgs(&globalHeapPoolAlloc);
		if (!peff::copy(copiedGenericArgs, genericArgs)) {
			return OutOfMemoryError::alloc();
		}

		Object *copiedInstantiatedObject = instantiatedObject;

		cacheTable.insert(std::move(copiedGenericArgs), std::move(copiedInstantiatedObject));
	} else {
		Object *copiedInstantiatedObject = instantiatedObject;

		cacheTable.at(genericArgs) = std::move(copiedInstantiatedObject);
	}
	{
		GenericArgList copiedGenericArgList(&globalHeapPoolAlloc);
		if (!peff::copy(copiedGenericArgList, genericArgs)) {
			return OutOfMemoryError::alloc();
		}
		_genericCacheLookupTable.insert((const Object *)instantiatedObject, { object, std::move(copiedGenericArgList) });
	}

	return {};
}

SLAKE_API InternalExceptionPointer slake::Runtime::instantiateModuleFields(ModuleObject *mod, GenericInstantiationContext &instantiationContext) {
	size_t szRelocatedLocalFieldStorage = 0;
	peff::DynArray<FieldRecord> relocatedFieldRecords(&globalHeapPoolAlloc);

	for (size_t i = 0; i < mod->fieldRecords.size(); ++i) {
		FieldRecord &curOldFieldRecord = mod->fieldRecords.at(i);

		if (!mod->fieldRecordIndices.insert(curOldFieldRecord.name, +i))
			return OutOfMemoryError::alloc();

		FieldRecord curFieldRecord(&globalHeapPoolAlloc);
		if (!peff::copy(curFieldRecord.name, curOldFieldRecord.name))
			return OutOfMemoryError::alloc();
		curFieldRecord.type = curOldFieldRecord.type;
		curFieldRecord.accessModifier = curOldFieldRecord.accessModifier;
		curFieldRecord.name = std::move(curOldFieldRecord.name);

		SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(curFieldRecord.type, instantiationContext));

		switch (curFieldRecord.type.typeId) {
			case TypeId::I8:
				curFieldRecord.offset = szRelocatedLocalFieldStorage;
				szRelocatedLocalFieldStorage += sizeof(int8_t);
				break;
			case TypeId::I16:
				if (szRelocatedLocalFieldStorage & 1) {
					szRelocatedLocalFieldStorage += (2 - (szRelocatedLocalFieldStorage & 1));
				}
				curFieldRecord.offset = szRelocatedLocalFieldStorage;
				szRelocatedLocalFieldStorage += sizeof(int16_t);
				break;
			case TypeId::I32:
				if (szRelocatedLocalFieldStorage & 3) {
					szRelocatedLocalFieldStorage += (4 - (szRelocatedLocalFieldStorage & 3));
				}
				curFieldRecord.offset = szRelocatedLocalFieldStorage;
				szRelocatedLocalFieldStorage += sizeof(int32_t);
				break;
			case TypeId::I64:
				if (szRelocatedLocalFieldStorage & 7) {
					szRelocatedLocalFieldStorage += (8 - (szRelocatedLocalFieldStorage & 7));
				}
				curFieldRecord.offset = szRelocatedLocalFieldStorage;
				szRelocatedLocalFieldStorage += sizeof(int64_t);
				break;
			case TypeId::U8:
				curFieldRecord.offset = szRelocatedLocalFieldStorage;
				szRelocatedLocalFieldStorage += sizeof(uint8_t);
				break;
			case TypeId::U16:
				if (szRelocatedLocalFieldStorage & 1) {
					szRelocatedLocalFieldStorage += (2 - (szRelocatedLocalFieldStorage & 1));
				}
				curFieldRecord.offset = szRelocatedLocalFieldStorage;
				szRelocatedLocalFieldStorage += sizeof(uint16_t);
				break;
			case TypeId::U32:
				if (szRelocatedLocalFieldStorage & 3) {
					szRelocatedLocalFieldStorage += (4 - (szRelocatedLocalFieldStorage & 3));
				}
				curFieldRecord.offset = szRelocatedLocalFieldStorage;
				szRelocatedLocalFieldStorage += sizeof(uint32_t);
				break;
			case TypeId::U64:
				if (szRelocatedLocalFieldStorage & 7) {
					szRelocatedLocalFieldStorage += (8 - (szRelocatedLocalFieldStorage & 7));
				}
				curFieldRecord.offset = szRelocatedLocalFieldStorage;
				szRelocatedLocalFieldStorage += sizeof(uint64_t);
				break;
			case TypeId::Bool:
				curFieldRecord.offset = szRelocatedLocalFieldStorage;
				szRelocatedLocalFieldStorage += sizeof(bool);
				break;
			case TypeId::String:
			case TypeId::Instance:
				if (szRelocatedLocalFieldStorage & (sizeof(void *) - 1)) {
					szRelocatedLocalFieldStorage += (sizeof(void *) - (szRelocatedLocalFieldStorage & (sizeof(void *) - 1)));
				}
				curFieldRecord.offset = szRelocatedLocalFieldStorage;
				szRelocatedLocalFieldStorage += sizeof(void *);
				break;
			case TypeId::Any:
				if (szRelocatedLocalFieldStorage % sizeof(Value)) {
					szRelocatedLocalFieldStorage += (sizeof(Value) - (szRelocatedLocalFieldStorage % sizeof(Value)));
				}
				curFieldRecord.offset = szRelocatedLocalFieldStorage;
				szRelocatedLocalFieldStorage += sizeof(Value);
				break;
			case TypeId::GenericArg:
				curFieldRecord.offset = SIZE_MAX;
				break;
			default:
				std::terminate();
		}

		if (!relocatedFieldRecords.pushBack(std::move(curFieldRecord)))
			return OutOfMemoryError::alloc();
	}
	char *localFieldStorage = (char *)globalHeapPoolAlloc.alloc(szRelocatedLocalFieldStorage, sizeof(std::max_align_t));
	if (!localFieldStorage)
		return OutOfMemoryError::alloc();
	peff::ScopeGuard releaseLocalFieldStorageGuard([this, localFieldStorage, szRelocatedLocalFieldStorage]() noexcept {
		globalHeapPoolAlloc.release(localFieldStorage, szRelocatedLocalFieldStorage, sizeof(std::max_align_t));
	});

	mod->fieldRecordIndices.clear();

	peff::DynArray<FieldRecord> oldFieldRecords = std::move(mod->fieldRecords);
	mod->fieldRecords = std::move(relocatedFieldRecords);

	for (size_t i = 0; i < mod->fieldRecords.size(); ++i) {
		FieldRecord &curFieldRecord = oldFieldRecords.at(i);
		FieldRecord &curRelocatedFieldRecord = mod->fieldRecords.at(i);

		if (!mod->fieldRecordIndices.insert(curRelocatedFieldRecord.name, +i))
			throw std::bad_alloc();

		char *rawDataPtr = mod->localFieldStorage + curFieldRecord.offset,
			 *rawRelocatedDataPtr = localFieldStorage + curRelocatedFieldRecord.offset;
		switch (curFieldRecord.type.typeId) {
			case TypeId::I8:
				*((int8_t *)rawRelocatedDataPtr) = (curFieldRecord.offset != SIZE_MAX ? *((int8_t *)rawDataPtr) : 0);
				break;
			case TypeId::I16:
				*((int16_t *)rawRelocatedDataPtr) = (curFieldRecord.offset != SIZE_MAX ? *((int16_t *)rawDataPtr) : 0);
				break;
			case TypeId::I32:
				*((int32_t *)rawRelocatedDataPtr) = (curFieldRecord.offset != SIZE_MAX ? *((int32_t *)rawDataPtr) : 0);
				break;
			case TypeId::I64:
				*((int64_t *)rawRelocatedDataPtr) = (curFieldRecord.offset != SIZE_MAX ? *((int64_t *)rawDataPtr) : 0);
				break;
			case TypeId::U8:
				*((uint8_t *)rawRelocatedDataPtr) = (curFieldRecord.offset != SIZE_MAX ? *((uint8_t *)rawDataPtr) : 0);
				break;
			case TypeId::U16:
				*((uint16_t *)rawRelocatedDataPtr) = (curFieldRecord.offset != SIZE_MAX ? *((int16_t *)rawDataPtr) : 0);
				break;
			case TypeId::U32:
				*((uint32_t *)rawRelocatedDataPtr) = (curFieldRecord.offset != SIZE_MAX ? *((uint32_t *)rawDataPtr) : 0);
				break;
			case TypeId::U64:
				*((uint64_t *)rawRelocatedDataPtr) = (curFieldRecord.offset != SIZE_MAX ? *((uint64_t *)rawDataPtr) : 0);
				break;
			case TypeId::F32:
				*((float *)rawRelocatedDataPtr) = (curFieldRecord.offset != SIZE_MAX ? *((float *)rawDataPtr) : 0);
				break;
			case TypeId::F64:
				*((double *)rawRelocatedDataPtr) = (curFieldRecord.offset != SIZE_MAX ? *((double *)rawDataPtr) : 0);
				break;
			case TypeId::Bool:
				*((bool *)rawRelocatedDataPtr) = (curFieldRecord.offset != SIZE_MAX ? *((bool *)rawDataPtr) : 0);
				break;
			case TypeId::String:
			case TypeId::Instance:
				*((Object **)rawRelocatedDataPtr) = (curFieldRecord.offset != SIZE_MAX ? *((Object **)rawDataPtr) : 0);
				break;
			case TypeId::Any:
				*((Value *)rawRelocatedDataPtr) = (curFieldRecord.offset != SIZE_MAX ? *((Value *)rawDataPtr) : 0);
				break;
			case TypeId::GenericArg:
				break;
			default:
				throw LoaderError("Invalid variable type");
		}
	}

	if (mod->localFieldStorage)
		globalHeapPoolAlloc.release(mod->localFieldStorage, mod->szLocalFieldStorage, sizeof(std::max_align_t));
	mod->localFieldStorage = localFieldStorage;
	mod->szLocalFieldStorage = szRelocatedLocalFieldStorage;
	releaseLocalFieldStorageGuard.release();

	return {};
}

SLAKE_API InternalExceptionPointer slake::Runtime::_instantiateGenericObject(Type &type, GenericInstantiationContext &instantiationContext) {
	switch (type.typeId) {
		case TypeId::Instance: {
			if (type.isLoadingDeferred()) {
				IdRefObject *exData = (IdRefObject *)type.getCustomTypeExData();
				for (auto &i : exData->entries) {
					for (auto &j : i.genericArgs) {
						SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(j, instantiationContext));
					}
				}
			} else {
				peff::DynArray<IdRefEntry> idRefToResolvedType(&globalHeapPoolAlloc);

				if (!getFullRef(&globalHeapPoolAlloc, (MemberObject *)type.getCustomTypeExData(), idRefToResolvedType))
					return OutOfMemoryError::alloc();

				HostObjectRef<IdRefObject> idRefObject = IdRefObject::alloc((Runtime *)this);

				// TODO: If we use std::pmr version for getFullRef, remove these code and use following code:
				// idRefObject->entries = idRefToResolvedType;
				idRefObject->entries.resizeWith(idRefToResolvedType.size(), IdRefEntry(&globalHeapPoolAlloc));
				for (size_t i = 0; i < idRefToResolvedType.size(); ++i) {
					if (!peff::copyAssign(idRefObject->entries.at(i), idRefToResolvedType.at(i))) {
						return OutOfMemoryError::alloc();
					}
				}

				// TODO: Add HostRefHolder for idRefObject.
				type = Type(type.typeId, idRefObject.get());

				SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(type, instantiationContext));
			}
			break;
		}
		case TypeId::Array: {
			bool isSucceeded;
			type = type.duplicate(isSucceeded);
			if (!isSucceeded)
				return OutOfMemoryError::alloc();
			SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(type.getArrayExData(), instantiationContext));
			break;
		}
		case TypeId::Ref: {
			bool isSucceeded;
			type = type.duplicate(isSucceeded);
			if (!isSucceeded)
				return OutOfMemoryError::alloc();
			SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(type.getRefExData(), instantiationContext));
			break;
		}
		case TypeId::GenericArg: {
			HostObjectRef<StringObject> nameObject = (StringObject *)type.getCustomTypeExData();
			if (auto it = instantiationContext.mappedGenericArgs.find(nameObject->data); it != instantiationContext.mappedGenericArgs.end()) {
				if (it.value().typeId != TypeId::None)
					type = it.value();
			} else {
				peff::String paramName(&globalHeapPoolAlloc);
				if (!peff::copyAssign(paramName, nameObject->data)) {
					return OutOfMemoryError::alloc();
				}

				SLAKE_RETURN_IF_EXCEPT(GenericParameterNotFoundError::alloc(
					const_cast<Runtime *>(this),
					std::move(paramName)));
			}
		}
	}
	return {};
}

SLAKE_API InternalExceptionPointer slake::Runtime::_instantiateGenericObject(Value &value, GenericInstantiationContext &instantiationContext) {
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
			SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(value.getTypeName(), instantiationContext));
			break;
		case ValueType::Undefined:
			break;
		default:
			throw std::logic_error("Unhandled value type");
	}
	return {};
}

SLAKE_API InternalExceptionPointer slake::Runtime::_instantiateGenericObject(Object *v, GenericInstantiationContext &instantiationContext) {
	// How do we instantiate generic classes:
	// Duplicate the value, scan for references to generic parameters and
	// replace them with generic arguments.
	switch (v->getKind()) {
		case ObjectKind::Class: {
			ClassObject *const value = (ClassObject *)v;

			if (value->genericParams.size() && value != instantiationContext.mappedObject) {
				peff::HashMap<peff::String, Type> copiedMappedGenericArgs(&globalHeapPoolAlloc);

				if (!peff::copyAssign(copiedMappedGenericArgs, instantiationContext.mappedGenericArgs))
					return OutOfMemoryError::alloc();

				GenericInstantiationContext newInstantiationContext = {
					instantiationContext.mappedObject,
					instantiationContext.genericArgs,
					std::move(copiedMappedGenericArgs)
				};

				// Map irreplaceable parameters to corresponding generic parameter reference type
				// and thus the generic types will keep unchanged.
				for (size_t i = 0; i < value->genericParams.size(); ++i) {
					peff::String copiedName(&globalHeapPoolAlloc);
					if (!peff::copyAssign(copiedName, value->genericParams.at(i).name))
						return OutOfMemoryError::alloc();
					newInstantiationContext.mappedGenericArgs.insert(std::move(copiedName), TypeId::None);
				}

				SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(value->parentClass, newInstantiationContext));

				for (auto it = value->scope->members.begin(); it != value->scope->members.end(); ++it) {
					SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(it.value(), newInstantiationContext));
				}

				SLAKE_RETURN_IF_EXCEPT(instantiateModuleFields(value, newInstantiationContext));
			} else {
				if (value == instantiationContext.mappedObject) {
					if (!peff::copyAssign(value->genericArgs, *instantiationContext.genericArgs))
						return OutOfMemoryError::alloc();
					if (!peff::copyAssign(value->mappedGenericArgs, instantiationContext.mappedGenericArgs))
						return OutOfMemoryError::alloc();
				}

				SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(value->parentClass, instantiationContext));

				for (auto it = value->scope->members.begin(); it != value->scope->members.end(); ++it) {
					SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(it.value(), instantiationContext));
				}

				SLAKE_RETURN_IF_EXCEPT(instantiateModuleFields(value, instantiationContext));
			}

			break;
		}
		case ObjectKind::Interface: {
			InterfaceObject *const value = (InterfaceObject *)v;

			if (!peff::copyAssign(value->genericArgs, *instantiationContext.genericArgs))
				return OutOfMemoryError::alloc();
			if (!peff::copyAssign(value->mappedGenericArgs, instantiationContext.mappedGenericArgs))
				return OutOfMemoryError::alloc();

			for (auto it = value->scope->members.begin(); it != value->scope->members.end(); ++it) {
				SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(it.value(), instantiationContext));
			}

			SLAKE_RETURN_IF_EXCEPT(instantiateModuleFields(value, instantiationContext));

			break;
		}
		case ObjectKind::Module: {
			ModuleObject *value = (ModuleObject *)v;

			for (auto it = value->scope->members.begin(); it != value->scope->members.end(); ++it) {
				SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(it.value(), instantiationContext));
			}

			SLAKE_RETURN_IF_EXCEPT(instantiateModuleFields(value, instantiationContext));
			break;
		}
		case ObjectKind::Fn: {
			FnObject *value = (FnObject *)v;

			if (instantiationContext.mappedObject == value) {
				FnOverloadingObject *matchedOverloading = nullptr, *matchedSpecializedOverloading = nullptr;

				for (auto i : value->overloadings) {
					if (i->genericParams.size() != instantiationContext.genericArgs->size()) {
						continue;
					}

					assert(instantiationContext.genericArgs->size() == i->specializationArgs.size());

					if (i->specializationArgs.size()) {
						for (size_t j = 0; j < i->specializationArgs.size(); ++j) {
							auto &curType = i->specializationArgs.at(j);

							if (curType.typeId == TypeId::GenericArg)
								throw std::runtime_error("Specialization argument must be deterministic");

							SLAKE_RETURN_IF_EXCEPT(curType.loadDeferredType(this));
							if (curType != instantiationContext.genericArgs->at(j)) {
								goto specializationArgsMismatched;
							}
						}

						SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(i, instantiationContext));
						matchedSpecializedOverloading = i;
						break;
					} else {
						SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(i, instantiationContext));
						matchedOverloading = i;
					}

				specializationArgsMismatched:;
				}

				if (!value->overloadings.insert(
						matchedSpecializedOverloading ? std::move(matchedSpecializedOverloading) : std::move(matchedOverloading)))
					return OutOfMemoryError::alloc();
			} else {
				for (auto i : value->overloadings) {
					SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(i, instantiationContext));
				}
			}
			break;
		}
		case ObjectKind::String:
		case ObjectKind::RootObject:
		case ObjectKind::IdRef:
			break;
		default:
			throw std::logic_error("Unhandled object type");
	}
	return {};
}

InternalExceptionPointer Runtime::mapGenericParams(const Object *v, GenericInstantiationContext &instantiationContext) const {
	instantiationContext.mappedObject = v;

	switch (v->getKind()) {
		case ObjectKind::Class: {
			ClassObject *value = (ClassObject *)v;

			if (instantiationContext.genericArgs->size() != value->genericParams.size()) {
				return MismatchedGenericArgumentNumberError::alloc(
					const_cast<Runtime *>(this));
			}

			for (size_t i = 0; i < value->genericParams.size(); ++i) {
				peff::String copiedName(&globalHeapPoolAlloc);

				if (!peff::copyAssign(copiedName, value->genericParams.at(i).name)) {
					return OutOfMemoryError::alloc();
				}

				Type copiedType = instantiationContext.genericArgs->at(i);

				instantiationContext.mappedGenericArgs.insert(std::move(copiedName), std::move(copiedType));
			}
			break;
		}
		case ObjectKind::Interface: {
			InterfaceObject *value = (InterfaceObject *)v;

			if (instantiationContext.genericArgs->size() != value->genericParams.size()) {
				return MismatchedGenericArgumentNumberError::alloc(
					const_cast<Runtime *>(this));
			}

			for (size_t i = 0; i < value->genericParams.size(); ++i) {
				peff::String copiedName(&globalHeapPoolAlloc);
				if (!peff::copyAssign(copiedName, value->genericParams.at(i).name))
					return OutOfMemoryError::alloc();
				Type copiedType = instantiationContext.genericArgs->at(i);
				instantiationContext.mappedGenericArgs.insert(std::move(copiedName), std::move(copiedType));
			}
			break;
		}
		default:;
	}
	return {};
}

SLAKE_API InternalExceptionPointer Runtime::mapGenericParams(const FnOverloadingObject *ol, GenericInstantiationContext &instantiationContext) const {
	if (instantiationContext.genericArgs->size() != ol->genericParams.size()) {
		return MismatchedGenericArgumentNumberError::alloc(
			const_cast<Runtime *>(this));
	}

	for (size_t i = 0; i < ol->genericParams.size(); ++i) {
		peff::String copiedName(&globalHeapPoolAlloc);

		if (!peff::copyAssign(copiedName, ol->genericParams.at(i).name)) {
			return OutOfMemoryError::alloc();
		}

		Type copiedType = instantiationContext.genericArgs->at(i);

		instantiationContext.mappedGenericArgs.insert(std::move(copiedName), std::move(copiedType));
	}
	return {};
}

SLAKE_API InternalExceptionPointer Runtime::instantiateGenericObject(const Object *v, Object *&objectOut, GenericInstantiationContext &instantiationContext) {
	// Try to look up in the cache.
	if (_genericCacheDir.contains(v)) {
		auto &table = _genericCacheDir.at(v);
		if (auto it = table.find(*instantiationContext.genericArgs); it != table.end()) {
			// Cache hit, return.
			objectOut = it.value();
			return {};
		}
		// Cache missed, go to the fallback.
	}

	// Cache missed, instantiate the value.
	auto value = v->duplicate();  // Make a duplicate of the original value.
	SLAKE_RETURN_IF_EXCEPT(mapGenericParams(value, instantiationContext));
	// Instantiate the value.
	SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(value, instantiationContext));

	SLAKE_RETURN_IF_EXCEPT(setGenericCache(v, *instantiationContext.genericArgs, value));

	objectOut = value;

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::_instantiateGenericObject(FnOverloadingObject *ol, GenericInstantiationContext &instantiationContext) {
	if (ol->genericParams.size() && ol->fnObject != instantiationContext.mappedObject) {
		peff::HashMap<peff::String, Type> copiedMappedGenericArgs(&globalHeapPoolAlloc);

		if (!peff::copyAssign(copiedMappedGenericArgs, instantiationContext.mappedGenericArgs))
			return OutOfMemoryError::alloc();

		GenericInstantiationContext newInstantiationContext = {
			instantiationContext.mappedObject,
			instantiationContext.genericArgs,
			std::move(copiedMappedGenericArgs)
		};

		// Map irreplaceable parameters to corresponding generic parameter reference type
		// and thus the generic types will keep unchanged.
		for (size_t i = 0; i < ol->genericParams.size(); ++i) {
			peff::String copiedName(&globalHeapPoolAlloc);
			if (!peff::copyAssign(copiedName, ol->genericParams.at(i).name))
				return OutOfMemoryError::alloc();
			newInstantiationContext.mappedGenericArgs.insert(std::move(copiedName), TypeId::None);
		}

		newInstantiationContext.mappedObject = ol->fnObject;

		SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(ol, newInstantiationContext));
	} else {
		for (auto &i : ol->paramTypes) {
			SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(i, instantiationContext));
		}

		SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(ol->returnType, instantiationContext));

		switch (ol->overloadingKind) {
			case FnOverloadingKind::Regular: {
				RegularFnOverloadingObject *overloading = (RegularFnOverloadingObject *)ol;

				for (auto &i : overloading->instructions) {
					for (size_t j = 0; j < i.nOperands; ++j) {
						SLAKE_RETURN_IF_EXCEPT(_instantiateGenericObject(i.operands[j], instantiationContext));
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
