#include "loader.h"

using namespace slake;
using namespace slake::loader;

#define RETURN_IO_ERROR_IF_READ_FAILED(e) if (!e)

SLAKE_FORCEINLINE static InternalExceptionPointer _normalizeReadResult(Runtime *runtime, ReadResult readResult) {
	switch (readResult) {
		case ReadResult::Succeeded:
			break;
		case ReadResult::Eof:
		case ReadResult::ReadError:
			return allocOutOfMemoryErrorIfAllocFailed(ReadError::alloc(&runtime->globalHeapPoolAlloc));
		default:
			std::terminate();
	}

	return {};
}

SLAKE_API InternalExceptionPointer loader::loadType(Runtime *runtime, Reader *reader, Object *member, Type &typeOut) noexcept {
	slxfmt::TypeId typeId;

	SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU8((uint8_t &)typeId)));

	switch (typeId) {
		case slake::slxfmt::TypeId::None:
			typeOut = TypeId::None;
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
			Type elementType;

			SLAKE_RETURN_IF_EXCEPT(loadType(runtime, reader, member, elementType));

			HostObjectRef<TypeDefObject> typeDef;

			if (!(typeDef = TypeDefObject::alloc(runtime, elementType))) {
				return OutOfMemoryError::alloc();
			}

			typeOut = Type(TypeId::Array, typeDef.get());
			break;
		}
		case slake::slxfmt::TypeId::Object: {
			HostObjectRef<IdRefObject> idRef;

			SLAKE_RETURN_IF_EXCEPT(loadIdRef(runtime, reader, member, idRef));

			typeOut = Type(TypeId::Instance, idRef.get());
			break;
		}
		case slake::slxfmt::TypeId::GenericArg: {
			peff::String name(&runtime->globalHeapPoolAlloc);

			uint32_t nameLen;

			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(nameLen)));

			if (!name.resize(nameLen)) {
				return OutOfMemoryError::alloc();
			}
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read(name.data(), nameLen)));

			HostObjectRef<StringObject> nameObject;

			if (!(nameObject = StringObject::alloc(runtime, std::move(name)))) {
				return OutOfMemoryError::alloc();
			}

			typeOut = Type(nameObject.get(), member);
			break;
		}
		case slake::slxfmt::TypeId::Ref: {
			Type elementType;

			SLAKE_RETURN_IF_EXCEPT(loadType(runtime, reader, member, elementType));

			HostObjectRef<TypeDefObject> typeDef;

			if (!(typeDef = TypeDefObject::alloc(runtime, elementType))) {
				return OutOfMemoryError::alloc();
			}

			typeOut = Type(TypeId::Ref, typeDef.get());
			break;
		}
		default:
			std::terminate();
	}

	return {};
}

SLAKE_API InternalExceptionPointer loader::loadGenericParam(Runtime *runtime, Reader *reader, Object *member, GenericParam &genericParamOut) noexcept {
	uint32_t lenName;

	SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(lenName)));
	if (!genericParamOut.name.resize(lenName)) {
		return OutOfMemoryError::alloc();
	}
	SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read(genericParamOut.name.data(), lenName)));

	bool b;
	SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readBool(b)));

	if (b) {
		SLAKE_RETURN_IF_EXCEPT(loadType(runtime, reader, member, genericParamOut.baseType));
	}

	uint32_t nImplInterfaces;

	SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(nImplInterfaces)));

	if (!genericParamOut.interfaces.resize(nImplInterfaces)) {
		return OutOfMemoryError::alloc();
	}
	for (size_t i = 0; i < nImplInterfaces; ++i) {
		SLAKE_RETURN_IF_EXCEPT(loadType(runtime, reader, member, genericParamOut.interfaces.at(i)));
	}

	return {};
}

SLAKE_API InternalExceptionPointer loader::loadValue(Runtime *runtime, Reader *reader, Object *member, Value &valueOut) noexcept {
	slxfmt::ValueType vt;

	SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU8((uint8_t &)vt)));

	switch (vt) {
		case slake::slxfmt::ValueType::None: {
			valueOut = Value(EntityRef::makeObjectRef(nullptr));
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
			Type type;

			SLAKE_RETURN_IF_EXCEPT(loadType(runtime, reader, member, type));

			valueOut = Value(type);
			break;
		}
		case slake::slxfmt::ValueType::Reg: {
			uint32_t data;
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(data)));
			valueOut = Value(ValueType::RegRef, (uint32_t)data);
			break;
		}
		case slake::slxfmt::ValueType::String: {
			peff::String s(&runtime->globalHeapPoolAlloc);

			uint32_t lenName;
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(lenName)));
			if (!s.resize(lenName)) {
				return OutOfMemoryError::alloc();
			}

			if (lenName) {
				SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read(s.data(), lenName)));
			}

			HostObjectRef<StringObject> strObj;

			if (!(strObj = StringObject::alloc(runtime, std::move(s)))) {
				return OutOfMemoryError::alloc();
			}

			valueOut = Value(EntityRef::makeObjectRef(strObj.get()));
			break;
		}
		case slake::slxfmt::ValueType::IdRef: {
			HostObjectRef<IdRefObject> idRefObj;

			SLAKE_RETURN_IF_EXCEPT(loadIdRef(runtime, reader, member, idRefObj));

			valueOut = Value(EntityRef::makeObjectRef(idRefObj.get()));
			break;
		}
		case slake::slxfmt::ValueType::Array: {
			HostObjectRef<ArrayObject> a;

			Type elementType;

			SLAKE_RETURN_IF_EXCEPT(loadType(runtime, reader, member, elementType));

			uint32_t nElements;

			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(nElements)));

			if (!(a = runtime->newArrayInstance(runtime, elementType, nElements))) {
				return OutOfMemoryError::alloc();
			}

			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read((char *)a->data, a->elementSize * nElements)));
			break;
		}
	}

	return {};
}

SLAKE_API InternalExceptionPointer loader::loadIdRefEntries(Runtime *runtime, Reader *reader, Object *member, peff::DynArray<IdRefEntry> &entriesOut) noexcept {
	uint32_t nEntries;

	SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(nEntries)));
	for (size_t i = 0; i < nEntries; ++i) {
		IdRefEntry curEntry(&runtime->globalHeapPoolAlloc);

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
			SLAKE_RETURN_IF_EXCEPT(loadType(runtime, reader, member, curEntry.genericArgs.at(j)));
		}

		if (!entriesOut.pushBack(std::move(curEntry))) {
			return OutOfMemoryError::alloc();
		}
	}

	return {};
}

SLAKE_API InternalExceptionPointer loader::loadIdRef(Runtime *runtime, Reader *reader, Object *member, HostObjectRef<IdRefObject> &idRefOut) noexcept {
	if (!(idRefOut = IdRefObject::alloc(runtime))) {
		return OutOfMemoryError::alloc();
	}

	SLAKE_RETURN_IF_EXCEPT(loadIdRefEntries(runtime, reader, member, idRefOut->entries));

	uint32_t nParamTypes;

	SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(nParamTypes)));

	if (idRefOut->entries.back().name == "getX") {
		puts("");
	}

	if (nParamTypes != UINT32_MAX) {
		peff::DynArray<Type> paramTypes(&runtime->globalHeapPoolAlloc);

		if (!paramTypes.resize(nParamTypes)) {
			return OutOfMemoryError::alloc();
		}
		for (size_t i = 0; i < nParamTypes; ++i) {
			SLAKE_RETURN_IF_EXCEPT(loadType(runtime, reader, member, paramTypes.at(i)));
		}

		idRefOut->paramTypes = std::move(paramTypes);
	}

	SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readBool(idRefOut->hasVarArgs)));

	return {};
}

SLAKE_API InternalExceptionPointer loader::loadModuleMembers(Runtime *runtime, Reader *reader, ModuleObject *moduleObject) noexcept {
	{
		uint32_t nClasses;

		SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(nClasses)));
		for (size_t i = 0; i < nClasses; ++i) {
			slake::slxfmt::ClassTypeDesc ctd;

			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read((char *)&ctd, sizeof(ctd))));

			assert(!ctd.nImpls);

			HostObjectRef<ClassObject> clsObject;

			if (!(clsObject = ClassObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			AccessModifier access = 0;

			if (ctd.flags & slxfmt::CTD_PUB) {
				access |= ACCESS_PUB;
			}
			if (ctd.flags & slxfmt::CTD_FINAL) {
				access |= ACCESS_FINAL;
			}

			if (!clsObject->name.resize(ctd.lenName)) {
				return OutOfMemoryError::alloc();
			}
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read(clsObject->name.data(), ctd.lenName)));

			for (size_t j = 0; j < ctd.nGenericParams; ++j) {
				GenericParam gp(&runtime->globalHeapPoolAlloc);

				SLAKE_RETURN_IF_EXCEPT(loadGenericParam(runtime, reader, moduleObject, gp));

				if (!clsObject->genericParams.pushBack(std::move(gp))) {
					return OutOfMemoryError::alloc();
				}
			}

			if (ctd.flags & slxfmt::CTD_DERIVED) {
				SLAKE_RETURN_IF_EXCEPT(loadType(runtime, reader, moduleObject, clsObject->baseType));
			}

			if (!clsObject->implTypes.resize(ctd.nImpls)) {
				return OutOfMemoryError::alloc();
			}
			for (size_t j = 0; j < ctd.nImpls; ++j) {
				SLAKE_RETURN_IF_EXCEPT(loadType(runtime, reader, moduleObject, clsObject->implTypes.at(j)));
			}

			SLAKE_RETURN_IF_EXCEPT(loadModuleMembers(runtime, reader, clsObject.get()));

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
			slake::slxfmt::InterfaceTypeDesc itd;

			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read((char *)&itd, sizeof(itd))));

			HostObjectRef<InterfaceObject> interfaceObject;

			if (!(interfaceObject = InterfaceObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			AccessModifier access = 0;

			if (itd.flags & slxfmt::ITD_PUB) {
				access |= ACCESS_PUB;
			}

			if (!interfaceObject->name.resize(itd.lenName)) {
				return OutOfMemoryError::alloc();
			}
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read(interfaceObject->name.data(), itd.lenName)));

			for (size_t j = 0; j < itd.nGenericParams; ++j) {
				GenericParam gp(&runtime->globalHeapPoolAlloc);

				SLAKE_RETURN_IF_EXCEPT(loadGenericParam(runtime, reader, moduleObject, gp));

				if (!interfaceObject->genericParams.pushBack(std::move(gp))) {
					return OutOfMemoryError::alloc();
				}
			}

			if (!interfaceObject->implTypes.resize(itd.nParents)) {
				return OutOfMemoryError::alloc();
			}
			for (size_t j = 0; j < itd.nParents; ++j) {
				SLAKE_RETURN_IF_EXCEPT(loadType(runtime, reader, moduleObject, interfaceObject->implTypes.at(j)));
			}

			SLAKE_RETURN_IF_EXCEPT(loadModuleMembers(runtime, reader, interfaceObject.get()));

			interfaceObject->setParent(moduleObject);

			if (!moduleObject->addMember(interfaceObject.get())) {
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
			if (!fnObject->name.resize(lenName)) {
				return OutOfMemoryError::alloc();
			}
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read(fnObject->name.data(), lenName)));

			uint32_t nOverloadings;

			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(nOverloadings)));
			for (size_t j = 0; j < nOverloadings; ++j) {
				HostObjectRef<RegularFnOverloadingObject> fnOverloadingObject;

				if (!(fnOverloadingObject = RegularFnOverloadingObject::alloc(fnObject.get()))) {
					return OutOfMemoryError::alloc();
				}

				slake::slxfmt::FnDesc fnd;

				SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read((char *)&fnd, sizeof(fnd))));

				assert(fnd.lenBody);

				if (fnd.flags & slxfmt::FND_PUB) {
					fnOverloadingObject->access |= ACCESS_PUB;
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
					GenericParam gp(&runtime->globalHeapPoolAlloc);

					SLAKE_RETURN_IF_EXCEPT(loadGenericParam(runtime, reader, fnOverloadingObject.get(), gp));

					if (!fnOverloadingObject->genericParams.pushBack(std::move(gp))) {
						return OutOfMemoryError::alloc();
					}
				}

				if (!fnOverloadingObject->paramTypes.resize(fnd.nParams)) {
					return OutOfMemoryError::alloc();
				}
				for (size_t k = 0; k < fnd.nParams; ++k) {
					SLAKE_RETURN_IF_EXCEPT(loadType(runtime, reader, fnOverloadingObject.get(), fnOverloadingObject->paramTypes.at(k)));
				}

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
					if (!ins.reserveOperands(&runtime->globalHeapPoolAlloc, nOperands)) {
						return OutOfMemoryError::alloc();
					}

					for (size_t l = 0; l < nOperands; ++l) {
						SLAKE_RETURN_IF_EXCEPT(loadValue(runtime, reader, fnOverloadingObject.get(), ins.operands[l]));
					}

					if (!fnOverloadingObject->instructions.pushBack(std::move(ins))) {
						return OutOfMemoryError::alloc();
					}
				}

				if (!fnObject->overloadings.insert(fnOverloadingObject.get())) {
					return OutOfMemoryError::alloc();
				}
			}

			fnObject->setParent(moduleObject);

			if (!moduleObject->addMember(fnObject.get())) {
				return OutOfMemoryError::alloc();
			}
		}
	}

	{
		uint32_t nFields;

		SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(nFields)));

		for (size_t i = 0; i < nFields; ++i) {
			slake::slxfmt::VarDesc vad;

			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read((char *)&vad, sizeof(vad))));

			FieldRecord fr(&runtime->globalHeapPoolAlloc);

			AccessModifier access = 0;

			if (vad.flags & slxfmt::VAD_PUB) {
				access |= ACCESS_PUB;
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

			if (!fr.name.resize(vad.lenName)) {
				return OutOfMemoryError::alloc();
			}
			SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read(fr.name.data(), vad.lenName)));
			SLAKE_RETURN_IF_EXCEPT(loadType(runtime, reader, moduleObject, fr.type));

			if (!moduleObject->appendFieldRecord(std::move(fr))) {
				return OutOfMemoryError::alloc();
			}

			Value initialValue;
			SLAKE_RETURN_IF_EXCEPT(loadValue(runtime, reader, moduleObject, initialValue));

			SLAKE_RETURN_IF_EXCEPT(runtime->writeVar(EntityRef::makeFieldRef(moduleObject, i), initialValue));
		}
	}

	return {};
}

SLAKE_API InternalExceptionPointer loader::loadModule(Runtime *runtime, Reader *reader, HostObjectRef<ModuleObject> &moduleObjectOut) noexcept {
	slxfmt::ImgHeader imh = { 0 };

	SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->read((char *)&imh, sizeof(imh))));

	if (memcmp(imh.magic, slxfmt::IMH_MAGIC, sizeof(imh.magic))) {
		return allocOutOfMemoryErrorIfAllocFailed(BadMagicError::alloc(&runtime->globalHeapPoolAlloc));
	}

	if (!(moduleObjectOut = ModuleObject::alloc(runtime))) {
		return OutOfMemoryError::alloc();
	}

	for (size_t i = 0; i < imh.nImports; ++i) {
		HostObjectRef<IdRefObject> path;

		SLAKE_RETURN_IF_EXCEPT(loadIdRef(runtime, reader, moduleObjectOut.get(), path));

		if (path->paramTypes) {
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

	SLAKE_RETURN_IF_EXCEPT(loadModuleMembers(runtime, reader, moduleObjectOut.get()));

	return {};
}
