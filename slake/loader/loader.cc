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

SLAKE_API InternalExceptionPointer loader::loadValue(Runtime *runtime, Reader *reader, Object *member, Value &valueOut) noexcept {
	slxfmt::ValueType vt;

	SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU8((uint8_t &)vt)));

	switch (vt) {
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
		case slake::slxfmt::ValueType::Array:
			std::terminate();
			break;
	}
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

	if (!idRefOut->paramTypes.resize(nParamTypes)) {
		return OutOfMemoryError::alloc();
	}
	for (size_t i = 0; i < nParamTypes; ++i) {
		SLAKE_RETURN_IF_EXCEPT(loadType(runtime, reader, member, idRefOut->paramTypes.at(i)));
	}

	return {};
}

SLAKE_API InternalExceptionPointer loader::loadModuleMembers(Runtime *runtime, Reader *reader, ModuleObject *moduleObject) noexcept {
	uint32_t nClasses;

	SLAKE_RETURN_IF_EXCEPT(_normalizeReadResult(runtime, reader->readU32(nClasses)));
	for (size_t i = 0; i < nClasses; ++i) {

	}
}

SLAKE_API InternalExceptionPointer loader::loadModule(Runtime *runtime, Reader *reader, HostObjectRef<ModuleObject> &moduleObjectOut) noexcept {
	slxfmt::ImgHeader imh = { 0 };

	if (memcmp(imh.magic, slxfmt::IMH_MAGIC, sizeof(imh.magic))) {
		return allocOutOfMemoryErrorIfAllocFailed(BadMagicError::alloc(&runtime->globalHeapPoolAlloc));
	}

	if (!(moduleObjectOut = ModuleObject::alloc(runtime))) {
		return OutOfMemoryError::alloc();
	}

	for (size_t i = 0; i < imh.nImports; ++i) {
		HostObjectRef<IdRefObject> path;

		SLAKE_RETURN_IF_EXCEPT(loadIdRef(runtime, reader, moduleObjectOut.get(), path));

		if (path->paramTypes.size()) {
			std::terminate();
		}

		for (size_t i = 0; i < path; ++i) {
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
