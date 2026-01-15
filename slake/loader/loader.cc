#include "loader.h"

using namespace slake;
using namespace slake::loader;

SLAKE_API LoaderContext::LoaderContext(peff::Alloc *allocator)
	: allocator(allocator),
	  loadedIdRefs(allocator),
	  loadedCustomTypeDefs(allocator),
	  loadedInterfaces(allocator),
	  loadedStructs(allocator),
	  loadedClasses(allocator),
	  loadedFns(allocator),
	  hostRefHolder(allocator) {
}

SLAKE_API LoaderContext::~LoaderContext() {
}

SLAKE_FORCEINLINE static InternalExceptionPointer _normalizeReadResult(Runtime *runtime, ReadResult readResult) {
	switch (readResult) {
		case ReadResult::Succeeded:
			break;
		case ReadResult::ReadError:
			return allocOutOfMemoryErrorIfAllocFailed(ReadError::alloc(runtime->getFixedAlloc()));
		default:
			std::terminate();
	}

	return {};
}

SLAKE_API InternalExceptionPointer loader::loadType(LoaderContext &context, Runtime *runtime, Reader *reader, Object *member, TypeRef &typeOut) noexcept {
	slxfmt::TypeId typeId;

	SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU8((uint8_t &)typeId)));

	switch (typeId) {
		case slake::slxfmt::TypeId::Void:
			typeOut = TypeId::Void;
			break;
		case slake::slxfmt::TypeId::Any:
			typeOut = TypeId::Any;
			break;
		case slake::slxfmt::TypeId::I8:
			typeOut = TypeId::I8;
			break;
		case slake::slxfmt::TypeId::I16:
			typeOut = TypeId::I16;
			break;
		case slake::slxfmt::TypeId::I32:
			typeOut = TypeId::I32;
			break;
		case slake::slxfmt::TypeId::I64:
			typeOut = TypeId::I64;
			break;
		case slake::slxfmt::TypeId::U8:
			typeOut = TypeId::U8;
			break;
		case slake::slxfmt::TypeId::U16:
			typeOut = TypeId::U16;
			break;
		case slake::slxfmt::TypeId::U32:
			typeOut = TypeId::U32;
			break;
		case slake::slxfmt::TypeId::U64:
			typeOut = TypeId::U64;
			break;
		case slake::slxfmt::TypeId::F32:
			typeOut = TypeId::F32;
			break;
		case slake::slxfmt::TypeId::F64:
			typeOut = TypeId::F64;
			break;
		case slake::slxfmt::TypeId::String:
			typeOut = TypeId::String;
			break;
		case slake::slxfmt::TypeId::Bool:
			typeOut = TypeId::Bool;
			break;
		case slake::slxfmt::TypeId::Array: {
			HostObjectRef<ArrayTypeDefObject> typeDef;

			if (!(typeDef = ArrayTypeDefObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			HostObjectRef<HeapTypeObject> heapType;

			if (!(heapType = HeapTypeObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			SLAKE_RETURN_IF_EXCEPT(loadType(context, runtime, reader, member, heapType->typeRef));

			typeDef->elementType = heapType.get();

			if (auto td = runtime->getEqualTypeDef(typeDef.get()); td) {
				typeOut = TypeRef(TypeId::Array, td);
			} else {
				typeOut = TypeRef(TypeId::Array, typeDef.get());
				SLAKE_RETURN_IF_EXCEPT(runtime->registerTypeDef(typeDef.get()));
			}
			break;
		}
		case slake::slxfmt::TypeId::Object: {
			HostObjectRef<CustomTypeDefObject> typeDef;

			if (!(typeDef = CustomTypeDefObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}
			HostObjectRef<IdRefObject> idRef;

			SLAKE_RETURN_IF_EXCEPT(loadIdRef(context, runtime, reader, member, idRef));

			typeDef->typeObject = idRef.get();

			if (auto td = runtime->getEqualTypeDef(typeDef.get()); td) {
				typeOut = TypeRef(TypeId::Instance, td);
			} else {
				typeOut = TypeRef(TypeId::Instance, typeDef.get());
				if (!context.loadedCustomTypeDefs.insert(typeDef.get()))
					return OutOfMemoryError::alloc();
				SLAKE_RETURN_IF_EXCEPT(runtime->registerTypeDef(typeDef.get()));
			}
			break;
		}
		case slake::slxfmt::TypeId::Struct: {
			HostObjectRef<CustomTypeDefObject> typeDef;

			if (!(typeDef = CustomTypeDefObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}
			HostObjectRef<IdRefObject> idRef;

			SLAKE_RETURN_IF_EXCEPT(loadIdRef(context, runtime, reader, member, idRef));

			typeDef->typeObject = idRef.get();

			if (auto td = runtime->getEqualTypeDef(typeDef.get()); td) {
				typeOut = TypeRef(TypeId::StructInstance, td);
			} else {
				typeOut = TypeRef(TypeId::StructInstance, typeDef.get());
				if (!context.loadedCustomTypeDefs.insert(typeDef.get()))
					return OutOfMemoryError::alloc();
				SLAKE_RETURN_IF_EXCEPT(runtime->registerTypeDef(typeDef.get()));
			}
			break;
		}
		case slake::slxfmt::TypeId::GenericArg: {
			HostObjectRef<StringObject> nameObject;

			if (!(nameObject = StringObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			uint32_t nameLen;

			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(nameLen)));

			if (!nameObject->data.resize(nameLen)) {
				return OutOfMemoryError::alloc();
			}
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read(nameObject->data.data(), nameLen)));

			HostObjectRef<GenericArgTypeDefObject> typeDef;

			if (!(typeDef = GenericArgTypeDefObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			typeDef->ownerObject = member;
			typeDef->nameObject = nameObject.get();

			if (auto td = runtime->getEqualTypeDef(typeDef.get()); td) {
				typeOut = TypeRef(TypeId::GenericArg, td);
			} else {
				typeOut = TypeRef(TypeId::GenericArg, typeDef.get());
				SLAKE_RETURN_IF_EXCEPT(runtime->registerTypeDef(typeDef.get()));
			}
			break;
		}
		case slake::slxfmt::TypeId::Ref: {
			HostObjectRef<RefTypeDefObject> typeDef;

			if (!(typeDef = RefTypeDefObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			HostObjectRef<HeapTypeObject> heapType;

			if (!(heapType = HeapTypeObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			SLAKE_RETURN_IF_EXCEPT(loadType(context, runtime, reader, member, heapType->typeRef));

			typeDef->referencedType = heapType.get();

			if (auto td = runtime->getEqualTypeDef(typeDef.get()); td) {
				typeOut = TypeRef(TypeId::Ref, td);
			} else {
				typeOut = TypeRef(TypeId::Ref, typeDef.get());
				SLAKE_RETURN_IF_EXCEPT(runtime->registerTypeDef(typeDef.get()));
			}
			break;
		}
		case slake::slxfmt::TypeId::ParamTypeList: {
			HostObjectRef<ParamTypeListTypeDefObject> typeDef;

			if (!(typeDef = ParamTypeListTypeDefObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			uint32_t nParams;

			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(nParams)));

			if (!typeDef->paramTypes.resize(nParams)) {
				return OutOfMemoryError::alloc();
			}

			for (uint32_t i = 0; i < nParams; ++i) {
				HostObjectRef<HeapTypeObject> heapType;

				if (!(heapType = HeapTypeObject::alloc(runtime))) {
					return OutOfMemoryError::alloc();
				}

				SLAKE_RETURN_IF_EXCEPT(loadType(context, runtime, reader, member, heapType->typeRef));

				typeDef->paramTypes.at(i) = heapType.get();
			}

			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readBool(typeDef->hasVarArg)));

			if (auto td = runtime->getEqualTypeDef(typeDef.get()); td) {
				typeOut = TypeRef(TypeId::ParamTypeList, td);
			} else {
				typeOut = TypeRef(TypeId::ParamTypeList, typeDef.get());
				SLAKE_RETURN_IF_EXCEPT(runtime->registerTypeDef(typeDef.get()));
			}
			break;
		}
		case slake::slxfmt::TypeId::Unpacking: {
			HostObjectRef<UnpackingTypeDefObject> typeDef;

			if (!(typeDef = UnpackingTypeDefObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			HostObjectRef<HeapTypeObject> heapType;

			if (!(heapType = HeapTypeObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			SLAKE_RETURN_IF_EXCEPT(loadType(context, runtime, reader, member, heapType->typeRef));

			typeDef->type = heapType.get();

			if (auto td = runtime->getEqualTypeDef(typeDef.get()); td) {
				typeOut = TypeRef(TypeId::Unpacking, td);
			} else {
				typeOut = TypeRef(TypeId::Unpacking, typeDef.get());
				SLAKE_RETURN_IF_EXCEPT(runtime->registerTypeDef(typeDef.get()));
			}
			break;
		}
		default:
			std::terminate();
	}

	uint8_t modifier;
	SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU8(modifier)));

	if (modifier & slxfmt::TYPE_FINAL)
		typeOut.typeModifier |= TYPE_FINAL;

	return {};
}

SLAKE_API InternalExceptionPointer loader::loadGenericParam(LoaderContext &context, Runtime *runtime, Reader *reader, Object *member, GenericParam &genericParamOut) noexcept {
	uint32_t lenName;

	SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(lenName)));
	if (!genericParamOut.name.resize(lenName)) {
		return OutOfMemoryError::alloc();
	}
	SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read(genericParamOut.name.data(), lenName)));

	bool b;
	SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readBool(b)));

	if (b) {
		SLAKE_RETURN_IF_EXCEPT(loadType(context, runtime, reader, member, genericParamOut.inputType));
	} else {
		SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readBool(b)));
		if (b) {
			SLAKE_RETURN_IF_EXCEPT(loadType(context, runtime, reader, member, genericParamOut.baseType));
		}

		uint32_t nImplInterfaces;

		SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(nImplInterfaces)));

		if (!genericParamOut.interfaces.resize(nImplInterfaces)) {
			return OutOfMemoryError::alloc();
		}
		for (size_t i = 0; i < nImplInterfaces; ++i) {
			SLAKE_RETURN_IF_EXCEPT(loadType(context, runtime, reader, member, genericParamOut.interfaces.at(i)));
		}
	}

	return {};
}

SLAKE_API InternalExceptionPointer loader::loadValue(LoaderContext &context, Runtime *runtime, Reader *reader, Object *member, Value &valueOut) noexcept {
	slxfmt::ValueType vt;

	SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU8((uint8_t &)vt)));

	switch (vt) {
		case slake::slxfmt::ValueType::None: {
			valueOut = Value(Reference::makeObjectRef(nullptr));
			break;
		}
		case slake::slxfmt::ValueType::I8: {
			int8_t data;
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readI8(data)));
			valueOut = Value((int8_t)data);
			break;
		}
		case slake::slxfmt::ValueType::I16: {
			int16_t data;
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readI16(data)));
			valueOut = Value((int16_t)data);
			break;
		}
		case slake::slxfmt::ValueType::I32: {
			int32_t data;
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readI32(data)));
			valueOut = Value((int32_t)data);
			break;
		}
		case slake::slxfmt::ValueType::I64: {
			int64_t data;
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readI64(data)));
			valueOut = Value((int64_t)data);
			break;
		}
		case slake::slxfmt::ValueType::U8: {
			uint8_t data;
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU8(data)));
			valueOut = Value((uint8_t)data);
			break;
		}
		case slake::slxfmt::ValueType::U16: {
			uint16_t data;
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU16(data)));
			valueOut = Value((uint16_t)data);
			break;
		}
		case slake::slxfmt::ValueType::U32: {
			uint32_t data;
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(data)));
			valueOut = Value((uint32_t)data);
			break;
		}
		case slake::slxfmt::ValueType::U64: {
			uint64_t data;
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU64(data)));
			valueOut = Value((uint64_t)data);
			break;
		}
		case slake::slxfmt::ValueType::F32: {
			float data;
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readF32(data)));
			valueOut = Value((float)data);
			break;
		}
		case slake::slxfmt::ValueType::F64: {
			double data;
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readF64(data)));
			valueOut = Value((double)data);
			break;
		}
		case slake::slxfmt::ValueType::Bool: {
			bool data;
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readBool(data)));
			valueOut = Value((bool)data);
			break;
		}
		case slake::slxfmt::ValueType::TypeName: {
			TypeRef type;

			SLAKE_RETURN_IF_EXCEPT(loadType(context, runtime, reader, member, type));

			valueOut = Value(type);
			break;
		}
		case slake::slxfmt::ValueType::Reg: {
			uint32_t data;
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(data)));
			valueOut = Value(ValueType::RegIndex, (uint32_t)data);
			break;
		}
		case slake::slxfmt::ValueType::String: {
			HostObjectRef<StringObject> strObj;

			if (!(strObj = StringObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			uint32_t lenName;
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(lenName)));
			if (!strObj->data.resize(lenName)) {
				return OutOfMemoryError::alloc();
			}

			if (lenName) {
				SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read(strObj->data.data(), lenName)));
			}

			valueOut = Value(Reference::makeObjectRef(strObj.get()));
			break;
		}
		case slake::slxfmt::ValueType::IdRef: {
			HostObjectRef<IdRefObject> idRefObj;

			SLAKE_RETURN_IF_EXCEPT(loadIdRef(context, runtime, reader, member, idRefObj));

			valueOut = Value(Reference::makeObjectRef(idRefObj.get()));
			break;
		}
		case slake::slxfmt::ValueType::Array: {
			HostObjectRef<ArrayObject> a;

			TypeRef elementType;

			SLAKE_RETURN_IF_EXCEPT(loadType(context, runtime, reader, member, elementType));

			uint32_t nElements;

			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(nElements)));

			if (!(a = runtime->newArrayInstance(runtime, elementType, nElements))) {
				return OutOfMemoryError::alloc();
			}

			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read((char *)a->data, a->elementSize * nElements)));

			valueOut = a.get();
			break;
		}
		default:
			// TODO: Use InvalidValueTypeError.
			return ReadError::alloc(runtime->getFixedAlloc());
	}

	return {};
}

SLAKE_API InternalExceptionPointer loader::loadIdRefEntries(LoaderContext &context, Runtime *runtime, Reader *reader, Object *member, peff::DynArray<IdRefEntry> &entriesOut) noexcept {
	uint32_t nEntries;

	SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(nEntries)));
	for (size_t i = 0; i < nEntries; ++i) {
		IdRefEntry curEntry(runtime->getCurGenAlloc());

		uint32_t lenName;
		SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(lenName)));
		if (!curEntry.name.resize(lenName)) {
			return OutOfMemoryError::alloc();
		}

		SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read(curEntry.name.data(), lenName)));

		uint8_t nGenericArgs;
		SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU8(nGenericArgs)));

		if (!curEntry.genericArgs.resize(nGenericArgs)) {
			return OutOfMemoryError::alloc();
		}
		for (size_t j = 0; j < nGenericArgs; ++j) {
			SLAKE_RETURN_IF_EXCEPT(loadValue(context, runtime, reader, member, curEntry.genericArgs.at(j)));
		}

		if (!entriesOut.pushBack(std::move(curEntry))) {
			return OutOfMemoryError::alloc();
		}
	}

	return {};
}

SLAKE_API InternalExceptionPointer loader::loadIdRef(LoaderContext &context, Runtime *runtime, Reader *reader, Object *member, HostObjectRef<IdRefObject> &idRefOut) noexcept {
	if (!(idRefOut = IdRefObject::alloc(runtime))) {
		return OutOfMemoryError::alloc();
	}

	SLAKE_RETURN_IF_EXCEPT(loadIdRefEntries(context, runtime, reader, member, idRefOut->entries));

	uint32_t nParamTypes;

	SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(nParamTypes)));

	if (nParamTypes != UINT32_MAX) {
		peff::DynArray<TypeRef> paramTypes(idRefOut->selfAllocator.get());

		if (!paramTypes.resize(nParamTypes)) {
			return OutOfMemoryError::alloc();
		}
		for (size_t i = 0; i < nParamTypes; ++i) {
			SLAKE_RETURN_IF_EXCEPT(loadType(context, runtime, reader, member, paramTypes.at(i)));
		}

		idRefOut->paramTypes = std::move(paramTypes);
	}

	SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readBool(idRefOut->hasVarArgs)));

	SLAKE_RETURN_IF_EXCEPT(loadType(context, runtime, reader, member, idRefOut->overridenType));

	if (auto it = context.loadedIdRefs.find(idRefOut.get()); it != context.loadedIdRefs.end()) {
		idRefOut = *it;
	} else {
		if (!(context.loadedIdRefs.insert(idRefOut.get()))) {
			return OutOfMemoryError::alloc();
		}
	}

	return {};
}

SLAKE_API InternalExceptionPointer loader::loadModuleMembers(LoaderContext &context, Runtime *runtime, Reader *reader, BasicModuleObject *moduleObject) noexcept {
	{
		uint32_t nClasses;

		SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(nClasses)));
		for (size_t i = 0; i < nClasses; ++i) {
			slake::slxfmt::ClassTypeDesc desc;

			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read((char *)&desc, sizeof(desc))));

			HostObjectRef<ClassObject> clsObject;

			if (!(clsObject = ClassObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			AccessModifier access = 0;

			if (desc.flags & slxfmt::CTD_PUB) {
				access |= ACCESS_PUBLIC;
			}
			if (desc.flags & slxfmt::CTD_FINAL) {
				access |= ACCESS_FINAL;
			}

			if (!clsObject->resizeName(desc.lenName)) {
				return OutOfMemoryError::alloc();
			}
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read(clsObject->getNameRawPtr(), desc.lenName)));

			for (size_t j = 0; j < desc.nGenericParams; ++j) {
				GenericParam gp(clsObject->selfAllocator.get());

				SLAKE_RETURN_IF_EXCEPT(loadGenericParam(context, runtime, reader, moduleObject, gp));

				if (!clsObject->genericParams.pushBack(std::move(gp)))
					return OutOfMemoryError::alloc();
				if (!clsObject->mappedGenericParams.insert(clsObject->genericParams.back().name, +j))
					return OutOfMemoryError::alloc();
			}

			if (desc.flags & slxfmt::CTD_DERIVED) {
				SLAKE_RETURN_IF_EXCEPT(loadType(context, runtime, reader, moduleObject, clsObject->baseType));
			}

			if (!clsObject->implTypes.resize(desc.nImpls)) {
				return OutOfMemoryError::alloc();
			}
			for (size_t j = 0; j < desc.nImpls; ++j) {
				SLAKE_RETURN_IF_EXCEPT(loadType(context, runtime, reader, moduleObject, clsObject->implTypes.at(j)));
			}

			SLAKE_RETURN_IF_EXCEPT(loadModuleMembers(context, runtime, reader, clsObject.get()));

			if (!context.loadedClasses.insert(clsObject.get())) {
				return OutOfMemoryError::alloc();
			}

			clsObject->setParent(moduleObject);

			if (!moduleObject->addMember(clsObject.get())) {
				return OutOfMemoryError::alloc();
			}
		}
	}

	{
		uint32_t nInterfaces;

		SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(nInterfaces)));
		for (size_t i = 0; i < nInterfaces; ++i) {
			slake::slxfmt::InterfaceTypeDesc desc;

			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read((char *)&desc, sizeof(desc))));

			HostObjectRef<InterfaceObject> interfaceObject;

			if (!(interfaceObject = InterfaceObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			AccessModifier access = 0;

			if (desc.flags & slxfmt::ITD_PUB) {
				access |= ACCESS_PUBLIC;
			}

			if (!interfaceObject->resizeName(desc.lenName)) {
				return OutOfMemoryError::alloc();
			}
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read(interfaceObject->getNameRawPtr(), desc.lenName)));

			for (size_t j = 0; j < desc.nGenericParams; ++j) {
				GenericParam gp(interfaceObject->selfAllocator.get());

				SLAKE_RETURN_IF_EXCEPT(loadGenericParam(context, runtime, reader, moduleObject, gp));

				if (!interfaceObject->genericParams.pushBack(std::move(gp))) {
					return OutOfMemoryError::alloc();
				}
				if (!interfaceObject->mappedGenericParams.insert(interfaceObject->genericParams.back().name, +j))
					return OutOfMemoryError::alloc();
			}

			if (!interfaceObject->implTypes.resize(desc.nParents)) {
				return OutOfMemoryError::alloc();
			}
			for (size_t j = 0; j < desc.nParents; ++j) {
				SLAKE_RETURN_IF_EXCEPT(loadType(context, runtime, reader, moduleObject, interfaceObject->implTypes.at(j)));
			}

			SLAKE_RETURN_IF_EXCEPT(loadModuleMembers(context, runtime, reader, interfaceObject.get()));

			if (!context.loadedInterfaces.insert(interfaceObject.get())) {
				return OutOfMemoryError::alloc();
			}

			interfaceObject->setParent(moduleObject);

			if (!moduleObject->addMember(interfaceObject.get())) {
				return OutOfMemoryError::alloc();
			}
		}
	}

	{
		uint32_t nStructs;

		SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(nStructs)));
		for (size_t i = 0; i < nStructs; ++i) {
			slake::slxfmt::StructTypeDesc desc;

			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read((char *)&desc, sizeof(desc))));

			HostObjectRef<StructObject> structObject;

			if (!(structObject = StructObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			AccessModifier access = 0;

			if (desc.flags & slxfmt::STD_PUB) {
				access |= ACCESS_PUBLIC;
			}

			if (!structObject->resizeName(desc.lenName)) {
				return OutOfMemoryError::alloc();
			}
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read(structObject->getNameRawPtr(), desc.lenName)));

			for (size_t j = 0; j < desc.nGenericParams; ++j) {
				GenericParam gp(structObject->selfAllocator.get());

				SLAKE_RETURN_IF_EXCEPT(loadGenericParam(context, runtime, reader, moduleObject, gp));

				if (!structObject->genericParams.pushBack(std::move(gp))) {
					return OutOfMemoryError::alloc();
				}
				if (!structObject->mappedGenericParams.insert(structObject->genericParams.back().name, +j))
					return OutOfMemoryError::alloc();
			}

			SLAKE_RETURN_IF_EXCEPT(loadModuleMembers(context, runtime, reader, structObject.get()));

			if (!context.loadedStructs.insert(structObject.get())) {
				return OutOfMemoryError::alloc();
			}

			structObject->setParent(moduleObject);

			if (!moduleObject->addMember(structObject.get())) {
				return OutOfMemoryError::alloc();
			}
		}
	}

	{
		uint32_t nFns;

		SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(nFns)));
		for (size_t i = 0; i < nFns; ++i) {
			HostObjectRef<FnObject> fnObject;

			if (!(fnObject = FnObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			uint32_t lenName;
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(lenName)));
			if (!fnObject->resizeName(lenName)) {
				return OutOfMemoryError::alloc();
			}
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read(fnObject->getNameRawPtr(), lenName)));

			uint32_t nOverloadings;

			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(nOverloadings)));
			for (size_t j = 0; j < nOverloadings; ++j) {
				HostObjectRef<RegularFnOverloadingObject> fnOverloadingObject;

				if (!(fnOverloadingObject = RegularFnOverloadingObject::alloc(fnObject.get()))) {
					return OutOfMemoryError::alloc();
				}

				slake::slxfmt::FnDesc fnd;

				SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read((char *)&fnd, sizeof(fnd))));

				if (fnd.flags & slxfmt::FND_PUB) {
					fnOverloadingObject->access |= ACCESS_PUBLIC;
				}
				if (fnd.flags & slxfmt::FND_FINAL) {
					fnOverloadingObject->access |= ACCESS_FINAL;
				}
				if (fnd.flags & slxfmt::FND_STATIC) {
					fnOverloadingObject->access |= ACCESS_STATIC;
				}
				if (fnd.flags & slxfmt::FND_NATIVE) {
					fnOverloadingObject->access |= ACCESS_NATIVE;
				}

				if (fnd.flags & slxfmt::FND_VARG) {
					fnOverloadingObject->setVarArgs();
				}
				if (fnd.flags & slxfmt::FND_GENERATOR) {
					fnOverloadingObject->setCoroutine();
				}
				if (fnd.flags & slxfmt::FND_VIRTUAL) {
					fnOverloadingObject->setVirtualFlag();
				}

				fnOverloadingObject->setRegisterNumber(fnd.nRegisters);

				for (size_t k = 0; k < fnd.nGenericParams; ++k) {
					GenericParam gp(fnOverloadingObject->selfAllocator.get());

					SLAKE_RETURN_IF_EXCEPT(loadGenericParam(context, runtime, reader, fnOverloadingObject.get(), gp));

					if (!fnOverloadingObject->genericParams.pushBack(std::move(gp))) {
						return OutOfMemoryError::alloc();
					}
					if (!fnOverloadingObject->mappedGenericParams.insert(fnOverloadingObject->genericParams.back().name, +j))
						return OutOfMemoryError::alloc();
				}

				if (!fnOverloadingObject->paramTypes.resize(fnd.nParams)) {
					return OutOfMemoryError::alloc();
				}
				for (size_t k = 0; k < fnd.nParams; ++k) {
					SLAKE_RETURN_IF_EXCEPT(loadType(context, runtime, reader, fnOverloadingObject.get(), fnOverloadingObject->paramTypes.at(k)));
				}

				// stub
				fnOverloadingObject->overridenType = TypeId::Void;

				for (size_t k = 0; k < fnd.lenBody; ++k) {
					Opcode opcode;

					SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU8((uint8_t &)opcode)));

					uint32_t output;

					SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(output)));

					uint32_t nOperands;

					SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(nOperands)));

					Instruction ins;

					ins.opcode = opcode;
					ins.output = output;
					if (!ins.reserveOperands(fnOverloadingObject->selfAllocator.get(), nOperands)) {
						return OutOfMemoryError::alloc();
					}

					for (size_t l = 0; l < nOperands; ++l) {
						SLAKE_RETURN_IF_EXCEPT(loadValue(context, runtime, reader, fnOverloadingObject.get(), ins.operands[l]));
					}

					if (!fnOverloadingObject->instructions.pushBack(std::move(ins))) {
						return OutOfMemoryError::alloc();
					}
				}

				if (!fnObject->overloadings.insert(
						FnSignature{ fnOverloadingObject->paramTypes,
							fnOverloadingObject->isWithVarArgs(),
							fnOverloadingObject->genericParams.size(),
							fnOverloadingObject->overridenType },
						fnOverloadingObject.get())) {
					return OutOfMemoryError::alloc();
				}
			}

			if (!context.loadedFns.insert(fnObject.get()))
				return OutOfMemoryError::alloc();

			fnObject->setParent(moduleObject);

			if (!moduleObject->addMember(fnObject.get())) {
				return OutOfMemoryError::alloc();
			}
		}
	}

	// TODO: Load scoped enumerations.

	{
		uint32_t nFields;

		SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(nFields)));

		for (size_t i = 0; i < nFields; ++i) {
			slake::slxfmt::VarDesc vad;

			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read((char *)&vad, sizeof(vad))));

			FieldRecord fr(moduleObject->selfAllocator.get());

			AccessModifier access = 0;

			if (vad.flags & slxfmt::VAD_PUB) {
				access |= ACCESS_PUBLIC;
			}

			if (vad.flags & slxfmt::VAD_FINAL) {
				access |= ACCESS_FINAL;
			}

			if (vad.flags & slxfmt::VAD_STATIC) {
				access |= ACCESS_STATIC;
			}

			if (vad.flags & slxfmt::VAD_NATIVE) {
				access |= ACCESS_NATIVE;
			}

			fr.accessModifier = access;

			if (!fr.name.resize(vad.lenName)) {
				return OutOfMemoryError::alloc();
			}
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read(fr.name.data(), vad.lenName)));
			SLAKE_RETURN_IF_EXCEPT(loadType(context, runtime, reader, moduleObject, fr.type));

			if (!moduleObject->appendFieldRecord(std::move(fr))) {
				return OutOfMemoryError::alloc();
			}

			Value initialValue;
			SLAKE_RETURN_IF_EXCEPT(loadValue(context, runtime, reader, moduleObject, initialValue));

			SLAKE_RETURN_IF_EXCEPT(runtime->writeVar(Reference::makeStaticFieldRef(moduleObject, i), initialValue));
		}
	}

	return {};
}

SLAKE_API InternalExceptionPointer loader::loadModule(LoaderContext &context, Runtime *runtime, Reader *reader, HostObjectRef<ModuleObject> &moduleObjectOut) noexcept {
	slxfmt::ImgHeader imh = { 0 };

	SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read((char *)&imh, sizeof(imh))));

	if (memcmp(imh.magic, slxfmt::IMH_MAGIC, sizeof(imh.magic))) {
		return allocOutOfMemoryErrorIfAllocFailed(BadMagicError::alloc(runtime->getFixedAlloc()));
	}

	if (!(moduleObjectOut = ModuleObject::alloc(runtime))) {
		return OutOfMemoryError::alloc();
	}

	{
		peff::DynArray<IdRefEntry> moduleFullName(runtime->getCurGenAlloc());
		SLAKE_RETURN_IF_EXCEPT(loadIdRefEntries(context, runtime, reader, moduleObjectOut.get(), moduleFullName));

		for (size_t i = 0; i < imh.nImports; ++i) {
			HostObjectRef<IdRefObject> path;

			SLAKE_RETURN_IF_EXCEPT(loadIdRef(context, runtime, reader, moduleObjectOut.get(), path));

			if (path->paramTypes.hasValue()) {
				std::terminate();
			}

			for (size_t i = 0; i < path->entries.size(); ++i) {
				if (path->entries.at(i).genericArgs.size()) {
					std::terminate();
				}
			}

			if (!moduleObjectOut->unnamedImports.pushBack(path.get())) {
				return OutOfMemoryError::alloc();
			}
		}

		SLAKE_RETURN_IF_EXCEPT(loadModuleMembers(context, runtime, reader, moduleObjectOut.get()));

		SLAKE_RETURN_IF_EXCEPT(completeParentNamespaces(context, runtime, moduleObjectOut.get(), moduleFullName));
	}

	for (auto i : context.loadedCustomTypeDefs) {
		runtime->unregisterTypeDef(i);
	}

	for (auto i : context.loadedCustomTypeDefs) {
		SLAKE_RETURN_IF_EXCEPT(runtime->loadDeferredCustomTypeDef(i));

		SLAKE_RETURN_IF_EXCEPT(runtime->registerTypeDef(i));
	}

	context.loadedCustomTypeDefs.clear();

	for (auto i : context.loadedInterfaces) {
		SLAKE_RETURN_IF_EXCEPT(i->updateInheritanceRelationship(runtime->getFixedAlloc()));
	}

	context.loadedInterfaces.clear();

	for (auto i : context.loadedStructs) {
		SLAKE_RETURN_IF_EXCEPT(i->isRecursed(runtime->getFixedAlloc()));
	}

	context.loadedInterfaces.clear();

	for (auto i : context.loadedFns) {
		SLAKE_RETURN_IF_EXCEPT(i->resortOverloadings());
	}

	runtime->gc();

	return {};
}

SLAKE_API InternalExceptionPointer slake::loader::completeParentNamespaces(LoaderContext &context, Runtime *runtime, BasicModuleObject *moduleObject, const peff::DynArray<IdRefEntry> &ref) noexcept {
	HostObjectRef<ModuleObject> mod = runtime->getRootObject();

	for (size_t i = 0; i < ref.size() - 1; ++i) {
		std::string_view name = ref.at(i).name;

		if (auto m = mod->members.find(name); m != mod->members.end()) {
			if (m.value()->getObjectKind() != ObjectKind::Module)
				std::terminate();
			mod = (ModuleObject *)m.value();
		} else {
			HostObjectRef<ModuleObject> newMod;

			if (!(newMod = ModuleObject::alloc(runtime)))
				return OutOfMemoryError::alloc();

			if (!newMod->setName(name))
				return OutOfMemoryError::alloc();

			if (!mod->addMember(newMod.get()))
				return OutOfMemoryError::alloc();

			mod = newMod.get();
		}
	}

	if (!moduleObject->setName(ref.at(ref.size() - 1).name))
		return OutOfMemoryError::alloc();
	if (!mod->addMember(moduleObject))
		return OutOfMemoryError::alloc();

	return {};
}
