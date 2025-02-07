#include <slake/runtime.h>

#include <memory>

using namespace slake;

/// @brief Read a single element from a stream.
/// @tparam T Type of element to read.
/// @param fs Stream to be read.
/// @return Element read from the stream.
template <typename T>
static T _read(std::istream &fs) {
	T value;
	fs.read((char *)&value, sizeof(value));
	return value;
}

/// @brief Load a single reference value from a stream.
/// @param rt Runtime for the new value.
/// @param fs Stream to be read.
/// @return Reference value loaded from the stream.
SLAKE_API HostObjectRef<IdRefObject> Runtime::_loadIdRef(LoaderContext &context, HostRefHolder &holder) {
	auto ref = IdRefObject::alloc(this);

	slxfmt::IdRefEntryDesc i = { 0 };
	while (true) {
		i = _read<slxfmt::IdRefEntryDesc>(context.fs);

		peff::String name(&globalHeapPoolAlloc);
		name.resize(i.lenName);
		context.fs.read(name.data(), i.lenName);

		GenericArgList genericArgs(&globalHeapPoolAlloc);
		genericArgs.resize(i.nGenericArgs);
		// genericArgs.shrink_to_fit();
		for (size_t j = 0; j < i.nGenericArgs; ++j)
			genericArgs.at(j) = _loadType(context, holder);

		bool hasArgs = i.flags & slxfmt::RSD_HASARG;
		bool hasVarArg = false;
		peff::DynArray<Type> paramTypes(&globalHeapPoolAlloc);

		if (hasArgs) {
			hasVarArg = i.flags & slxfmt::RSD_VARARG;

			paramTypes.resize(i.nParams);
			// paramTypes.shrink_to_fit();
			for (size_t j = 0; j < i.nParams; ++j)
				paramTypes.at(j) = _loadType(context, holder);
		}

		if (!ref->entries.pushBack(IdRefEntry(std::move(name), std::move(genericArgs), hasArgs, std::move(paramTypes), hasVarArg)))
			std::terminate();

		if (!(i.flags & slxfmt::RSD_NEXT))
			break;
	};

	return ref;
}

/// @brief Load a single value from a stream.
/// @param rt Runtime for the new value.
/// @param fs Stream to be read.
/// @return Object loaded from the stream.
SLAKE_API Value Runtime::_loadValue(LoaderContext &context, HostRefHolder &holder) {
	slxfmt::TypeId typeId = _read<slxfmt::TypeId>(context.fs);

	switch (typeId) {
		case slxfmt::TypeId::None:
			return Value(ObjectRef::makeInstanceRef(nullptr));
		case slxfmt::TypeId::I8:
			return Value(_read<std::int8_t>(context.fs));
		case slxfmt::TypeId::I16:
			return Value(_read<std::int16_t>(context.fs));
		case slxfmt::TypeId::I32:
			return Value(_read<std::int32_t>(context.fs));
		case slxfmt::TypeId::I64:
			return Value(_read<std::int64_t>(context.fs));
		case slxfmt::TypeId::U8:
			return Value(_read<uint8_t>(context.fs));
		case slxfmt::TypeId::U16:
			return Value(_read<uint16_t>(context.fs));
		case slxfmt::TypeId::U32:
			return Value(_read<uint32_t>(context.fs));
		case slxfmt::TypeId::U64:
			return Value(_read<uint64_t>(context.fs));
		case slxfmt::TypeId::Bool:
			return Value(_read<bool>(context.fs));
		case slxfmt::TypeId::F32:
			return Value(_read<float>(context.fs));
		case slxfmt::TypeId::F64:
			return Value(_read<double>(context.fs));
		case slxfmt::TypeId::String: {
			auto len = _read<uint32_t>(context.fs);
			peff::String s(&globalHeapPoolAlloc);
			s.resize(len);
			context.fs.read(s.data(), len);

			auto object = StringObject::alloc(this, std::move(s));

			holder.addObject(object.get());

			return Value(ObjectRef::makeInstanceRef(object.get()));
		}
		case slxfmt::TypeId::Array: {
			auto elementType = _loadType(context, holder);

			// stub for debugging.
			// elementType = Type(TypeId::Any);

			auto len = _read<uint32_t>(context.fs);

			HostObjectRef<ArrayObject> value = newArrayInstance(this, elementType, len);
			InternalExceptionPointer e;

			for (uint32_t i = 0; i < len; ++i) {
				if ((e = writeVar(ObjectRef::makeArrayElementRef(value.get(), i), _loadValue(context, holder)))) {
					throw LoaderError("Error setting value of element #" + std::to_string(i));
				}
			}

			holder.addObject(value.get());

			return ObjectRef::makeInstanceRef(value.release());
		}
		case slxfmt::TypeId::IdRef:
			return ObjectRef::makeInstanceRef(_loadIdRef(context, holder).release());
		case slxfmt::TypeId::TypeName:
			return Value(_loadType(context, holder));
		case slxfmt::TypeId::Reg:
			return Value(ValueType::RegRef, _read<uint32_t>(context.fs));
		default:
			throw LoaderError("Invalid object type detected");
	}
}

/// @brief Load a single type name from a stream.
/// @param rt Runtime for the new type.
/// @param fs Stream to be read.
/// @return Loaded complete type name.
SLAKE_API Type Runtime::_loadType(LoaderContext &context, HostRefHolder &holder) {
	slxfmt::TypeId vt = _read<slxfmt::TypeId>(context.fs);

	switch (vt) {
		case slxfmt::TypeId::I8:
			return Type(ValueType::I8);
		case slxfmt::TypeId::I16:
			return Type(ValueType::I16);
		case slxfmt::TypeId::I32:
			return Type(ValueType::I32);
		case slxfmt::TypeId::I64:
			return Type(ValueType::I64);
		case slxfmt::TypeId::U8:
			return Type(ValueType::U8);
		case slxfmt::TypeId::U16:
			return Type(ValueType::U16);
		case slxfmt::TypeId::U32:
			return Type(ValueType::U32);
		case slxfmt::TypeId::U64:
			return Type(ValueType::U64);
		case slxfmt::TypeId::F32:
			return Type(ValueType::F32);
		case slxfmt::TypeId::F64:
			return Type(ValueType::F64);
		case slxfmt::TypeId::String:
			return TypeId::String;
		case slxfmt::TypeId::Object: {
			auto idRef = _loadIdRef(context, holder);

			holder.addObject(idRef.get());

			return idRef.release();
		}
		case slxfmt::TypeId::Any:
			return TypeId::Any;
		case slxfmt::TypeId::Bool:
			return Type(ValueType::Bool);
		case slxfmt::TypeId::None:
			return TypeId::None;
		case slxfmt::TypeId::Array: {
			Type type = _loadType(context, holder);

			if (type.typeId == TypeId::Array)
				throw LoaderError("Nested array type detected");

			return Type::makeArrayTypeName(this, type);
		}
		case slxfmt::TypeId::Ref:
			return Type::makeRefTypeName(this, _loadType(context, holder));
		case slxfmt::TypeId::TypeName:
			return Type(ValueType::TypeName);
		case slxfmt::TypeId::GenericArg: {
			uint8_t length = _read<uint8_t>(context.fs);

			peff::String name(&globalHeapPoolAlloc);
			name.resize(length);
			context.fs.read(name.data(), length);

			auto nameObject = StringObject::alloc(this, std::move(name));

			return Type(nameObject.get(), context.ownerObject);
		}
		default:
			throw LoaderError("Invalid type ID");
	}
}

SLAKE_API GenericParam Runtime::_loadGenericParam(LoaderContext &context, HostRefHolder &holder) {
	auto gpd = _read<slxfmt::GenericParamDesc>(context.fs);

	peff::String name(&globalHeapPoolAlloc);
	name.resize(gpd.lenName);
	context.fs.read(name.data(), gpd.lenName);

	GenericParam param(&this->globalHeapPoolAlloc);
	param.name = std::move(name);

	if (gpd.hasBaseType)
		param.baseType = _loadType(context, holder);

	param.interfaces.resize(gpd.nInterfaces);
	// param.interfaces.shrink_to_fit();
	for (size_t i = 0; i < gpd.nInterfaces; ++i) {
		param.interfaces.at(i) = _loadType(context, holder);
	}

	return param;
}

/// @brief Load a single scope.
/// @param mod Module value which is treated as a scope.
/// @param fs The input stream.
SLAKE_API void Runtime::_loadScope(LoaderContext &context,
	HostObjectRef<ModuleObject> mod,
	LoadModuleFlags loadModuleFlags,
	HostRefHolder &holder) {
	uint32_t nItemsToRead;

	//
	// Load classes.
	//
	nItemsToRead = _read<uint32_t>(context.fs);
	for (slxfmt::ClassTypeDesc i = {}; nItemsToRead--;) {
		i = _read<slxfmt::ClassTypeDesc>(context.fs);

		peff::String name(&globalHeapPoolAlloc);
		name.resize(i.lenName);
		context.fs.read(name.data(), i.lenName);

		AccessModifier access = 0;
		if (i.flags & slxfmt::CTD_PUB)
			access |= ACCESS_PUB;
		if (i.flags & slxfmt::CTD_FINAL)
			access |= ACCESS_FINAL;

		ScopeUniquePtr scope(Scope::alloc(&globalHeapPoolAlloc, nullptr));

		HostObjectRef<ClassObject> value = ClassObject::alloc(this, std::move(scope), access);

		value->name = std::move(name);

		LoaderContext newContext = context;
		newContext.ownerObject = value.get();

		// value->genericParams.resizeWith(i.nGenericParams, GenericParam(&globalHeapPoolAlloc));
		//  value->genericParams.shrink_to_fit();
		if (i.nGenericParams) {
			newContext.isInGenericScope = true;
			for (size_t j = 0; j < i.nGenericParams; ++j)
				value->genericParams.pushBack(_loadGenericParam(context, holder));
		}

		// Load reference to the parent class.
		if (i.flags & slxfmt::CTD_DERIVED)
			value->parentClass = Type(TypeId::Instance, _loadIdRef(context, holder).release());

		// Load references to implemented interfaces.
		// value->implInterfaces.resize(i.nImpls);
		// value->implInterfaces.shrink_to_fit();
		for (auto j = 0; j < i.nImpls; ++j)
			value->implInterfaces.pushBack(_loadIdRef(context, holder).release());

		_loadScope(newContext, value.get(), loadModuleFlags, holder);

		mod->scope->putMember(value.release());
	}

	//
	// Load interfaces.
	//
	nItemsToRead = _read<uint32_t>(context.fs);
	for (slxfmt::InterfaceTypeDesc i = {}; nItemsToRead--;) {
		i = _read<slxfmt::InterfaceTypeDesc>(context.fs);

		peff::String name(&globalHeapPoolAlloc);
		name.resize(i.lenName);
		context.fs.read(name.data(), i.lenName);

		AccessModifier access = 0;
		if (i.flags & slxfmt::ITD_PUB)
			access |= ACCESS_PUB;

		ScopeUniquePtr scope(Scope::alloc(&globalHeapPoolAlloc, nullptr));
		peff::DynArray<Type> parents(&globalHeapPoolAlloc);
		HostObjectRef<InterfaceObject> value = InterfaceObject::alloc(this, std::move(scope), access, std::move(parents));
		value->name = std::move(name);

		LoaderContext newContext = context;
		newContext.ownerObject = value.get();

		value->genericParams.resize(i.nGenericParams);
		// value->genericParams.shrink_to_fit();
		if (i.nGenericParams) {
			newContext.isInGenericScope = true;
			for (size_t j = 0; j < i.nGenericParams; ++j)
				value->genericParams.pushBack(_loadGenericParam(context, holder));
		}

		value->parents.resize(i.nParents);
		// value->parents.shrink_to_fit();
		for (auto j = 0; j < i.nParents; ++j)
			value->parents.at(j) = (_loadIdRef(context, holder).release());

		_loadScope(newContext, value.get(), loadModuleFlags, holder);

		mod->scope->putMember(value.release());
	}

	//
	// Load variables.
	//
	size_t szLocalFieldStorage = 0;
	peff::DynArray<FieldRecord> fieldRecords(&globalHeapPoolAlloc);
	nItemsToRead = _read<uint32_t>(context.fs);
	for (slxfmt::VarDesc i = { 0 }; nItemsToRead--;) {
		i = _read<slxfmt::VarDesc>(context.fs);

		peff::String name(&globalHeapPoolAlloc);
		name.resize(i.lenName);
		context.fs.read(name.data(), i.lenName);

		AccessModifier access = 0;
		if (i.flags & slxfmt::VAD_PUB)
			access |= ACCESS_PUB;
		if (i.flags & slxfmt::VAD_STATIC)
			access |= ACCESS_STATIC;
		if (i.flags & slxfmt::VAD_FINAL)
			access |= ACCESS_FINAL;
		if (i.flags & slxfmt::VAD_NATIVE)
			access |= ACCESS_NATIVE;

		auto varType = _loadType(context, holder);

		FieldRecord curFieldRecord(&globalHeapPoolAlloc);
		curFieldRecord.name = std::move(name);
		curFieldRecord.type = varType;
		curFieldRecord.accessModifier = access;

		switch (varType.typeId) {
			case TypeId::Value:
				switch (varType.getValueTypeExData()) {
					case ValueType::I8:
						curFieldRecord.offset = szLocalFieldStorage;
						szLocalFieldStorage += sizeof(int8_t);
						break;
					case ValueType::I16:
						if (szLocalFieldStorage & 1) {
							szLocalFieldStorage += (2 - (szLocalFieldStorage & 1));
						}
						curFieldRecord.offset = szLocalFieldStorage;
						szLocalFieldStorage += sizeof(int16_t);
						break;
					case ValueType::I32:
						if (szLocalFieldStorage & 3) {
							szLocalFieldStorage += (4 - (szLocalFieldStorage & 3));
						}
						curFieldRecord.offset = szLocalFieldStorage;
						szLocalFieldStorage += sizeof(int32_t);
						break;
					case ValueType::I64:
						if (szLocalFieldStorage & 7) {
							szLocalFieldStorage += (8 - (szLocalFieldStorage & 7));
						}
						curFieldRecord.offset = szLocalFieldStorage;
						szLocalFieldStorage += sizeof(int64_t);
						break;
					case ValueType::U8:
						curFieldRecord.offset = szLocalFieldStorage;
						szLocalFieldStorage += sizeof(uint8_t);
						break;
					case ValueType::U16:
						if (szLocalFieldStorage & 1) {
							szLocalFieldStorage += (2 - (szLocalFieldStorage & 1));
						}
						curFieldRecord.offset = szLocalFieldStorage;
						szLocalFieldStorage += sizeof(uint16_t);
						break;
					case ValueType::U32:
						if (szLocalFieldStorage & 3) {
							szLocalFieldStorage += (4 - (szLocalFieldStorage & 3));
						}
						curFieldRecord.offset = szLocalFieldStorage;
						szLocalFieldStorage += sizeof(uint32_t);
						break;
					case ValueType::U64:
						if (szLocalFieldStorage & 7) {
							szLocalFieldStorage += (8 - (szLocalFieldStorage & 7));
						}
						curFieldRecord.offset = szLocalFieldStorage;
						szLocalFieldStorage += sizeof(uint64_t);
						break;
					case ValueType::Bool:
						curFieldRecord.offset = szLocalFieldStorage;
						szLocalFieldStorage += sizeof(bool);
						break;
					default:
						// Unenumerated value types should never occur.
						throw std::logic_error("Invalid value type");
				}
				break;
			case TypeId::String:
			case TypeId::Instance:
				if (szLocalFieldStorage & (sizeof(void *) - 1)) {
					szLocalFieldStorage += (sizeof(void *) - (szLocalFieldStorage & (sizeof(void *) - 1)));
				}
				curFieldRecord.offset = szLocalFieldStorage;
				szLocalFieldStorage += sizeof(Object *);
				break;
			case TypeId::Any:
				if (szLocalFieldStorage % sizeof(Value)) {
					szLocalFieldStorage += (sizeof(Value) - (szLocalFieldStorage % sizeof(Value)));
				}
				curFieldRecord.offset = szLocalFieldStorage;
				szLocalFieldStorage += sizeof(Value);
				break;
			case TypeId::GenericArg:
				curFieldRecord.offset = SIZE_MAX;
				break;
			default:
				throw LoaderError("Invalid variable type");
		}

		if (!fieldRecords.pushBack(std::move(curFieldRecord)))
			throw std::bad_alloc();
	}

	auto accessor = FieldAccessorVarObject::alloc(this, mod.get());
	if (!accessor)
		throw std::bad_alloc();
	mod->fieldAccessor = accessor.get();

	char *fieldStorage = (char *)globalHeapPoolAlloc.alloc(szLocalFieldStorage, sizeof(std::max_align_t));
	if (!fieldStorage)
		throw std::bad_alloc();
	mod->localFieldStorage = fieldStorage;
	mod->szLocalFieldStorage = szLocalFieldStorage;

	for (size_t i = 0; i < fieldRecords.size(); ++i) {
		FieldRecord &curFieldRecord = fieldRecords.at(i);

		if (!mod->fieldRecordIndices.insert(curFieldRecord.name, +i))
			throw std::bad_alloc();

		char *rawDataPtr = fieldStorage + curFieldRecord.offset;
		switch (curFieldRecord.type.typeId) {
			case TypeId::Value:
				switch (curFieldRecord.type.getValueTypeExData()) {
					case ValueType::I8:
						*((int8_t *)rawDataPtr) = 0;
						break;
					case ValueType::I16:
						*((int16_t *)rawDataPtr) = 0;
						break;
					case ValueType::I32:
						*((int32_t *)rawDataPtr) = 0;
						break;
					case ValueType::I64:
						*((int64_t *)rawDataPtr) = 0;
						break;
					case ValueType::U8:
						*((uint8_t *)rawDataPtr) = 0;
						break;
					case ValueType::U16:
						*((uint16_t *)rawDataPtr) = 0;
						break;
					case ValueType::U32:
						*((int32_t *)rawDataPtr) = 0;
						break;
					case ValueType::U64:
						*((int64_t *)rawDataPtr) = 0;
						break;
					case ValueType::Bool:
						*((bool *)rawDataPtr) = false;
						break;
					default:
						// Unenumerated value types should never occur.
						throw std::logic_error("Invalid value type");
				}
				break;
			case TypeId::String:
			case TypeId::Instance:
				*((Object **)rawDataPtr) = nullptr;
				break;
			case TypeId::Any:
				*((Value *)rawDataPtr) = Value(ObjectRef::makeInstanceRef(nullptr));
				break;
			case TypeId::GenericArg:
				break;
			default:
				throw LoaderError("Invalid variable type");
		}
	}
	mod->fieldRecords = std::move(fieldRecords);

	//
	// Load functions.
	//
	nItemsToRead = _read<uint32_t>(context.fs);
	while (nItemsToRead--) {
		uint32_t nOverloadings = _read<uint32_t>(context.fs);

		for (slxfmt::FnDesc i = { 0 }; nOverloadings--;) {
			i = _read<slxfmt::FnDesc>(context.fs);

			peff::String name(&globalHeapPoolAlloc);
			name.resize(i.lenName);
			context.fs.read(name.data(), i.lenName);

			AccessModifier access = 0;
			if (i.flags & slxfmt::FND_PUB)
				access |= ACCESS_PUB;
			if (i.flags & slxfmt::FND_STATIC)
				access |= ACCESS_STATIC;
			if (i.flags & slxfmt::FND_FINAL)
				access |= ACCESS_FINAL;
			if (i.flags & slxfmt::FND_OVERRIDE)
				access |= ACCESS_OVERRIDE;
			// if (i.flags & slxfmt::FND_NATIVE)
			//	access |= ACCESS_NATIVE;

			HostObjectRef<FnObject> fn = (FnObject *)mod->scope->getMember(name);

			if (!fn) {
				fn = FnObject::alloc(this);
				fn->name = std::move(name);
				mod->scope->putMember(fn.get());
			}

			HostObjectRef<RegularFnOverloadingObject> overloading = RegularFnOverloadingObject::alloc(fn.get(), access, peff::DynArray<Type>(&globalHeapPoolAlloc), Type(), i.nRegisters, 0);

			if (!(overloading->access & ACCESS_STATIC)) {
				switch (mod->getKind()) {
					case ObjectKind::Class:
					case ObjectKind::Interface:
						overloading->thisObjectType = Type(TypeId::Instance, mod.get());
						break;
					default:
						throw LoaderError("Non-static function in non-class type detected");
				}
			}

			LoaderContext newContext = context;
			newContext.ownerObject = overloading.get();

			if (i.flags & slxfmt::FND_ASYNC)
				overloading->overloadingFlags |= OL_ASYNC;
			if (i.flags & slxfmt::FND_VIRTUAL)
				overloading->overloadingFlags |= OL_VIRTUAL;

			overloading->returnType = _loadType(newContext, holder);

			// overloading->genericParams.resize(i.nGenericParams);
			//  overloading->genericParams.shrink_to_fit();
			if (i.nGenericParams) {
				newContext.isInGenericScope = true;
				for (size_t j = 0; j < i.nGenericParams; ++j) {
					overloading->genericParams.pushBack(_loadGenericParam(newContext, holder));
				}
			}

			// overloading->paramTypes.resize(i.nParams);
			//  overloading->paramTypes.shrink_to_fit();
			for (uint8_t j = 0; j < i.nParams; ++j) {
				overloading->paramTypes.pushBack(_loadType(newContext, holder));
			}

			if (i.flags & slxfmt::FND_VARG)
				overloading->overloadingFlags |= OL_VARG;

			if (i.lenBody) {
				if (!overloading->instructions.resize(i.lenBody))
					std::terminate();
				// overloading->instructions.shrink_to_fit();

				for (uint32_t j = 0; j < i.lenBody; j++) {
					auto &ins = overloading->instructions.at(j);

					slxfmt::InsHeader ih = _read<slxfmt::InsHeader>(newContext.fs);

					ins.opcode = ih.opcode;
					ins.operands.resize(ih.nOperands, Value(ValueType::Undefined));

					if (ih.hasOutputOperand)
						ins.output = _loadValue(newContext, holder);

					for (uint8_t k = 0; k < ih.nOperands; k++)
						ins.operands[k] = _loadValue(newContext, holder);
				}
			}

			for (uint32_t j = 0; j < i.nSourceLocDescs; ++j) {
				slxfmt::SourceLocDesc sld = _read<slxfmt::SourceLocDesc>(context.fs);
				overloading->sourceLocDescs.pushBack(std::move(sld));
			}

			fn->overloadings.insert(overloading.release());
		}
	}
}

SLAKE_API HostObjectRef<ModuleObject> slake::Runtime::loadModule(std::istream &fs, LoadModuleFlags flags) {
	ScopeUniquePtr scope(Scope::alloc(&globalHeapPoolAlloc, nullptr));
	if (!scope)
		return nullptr;
	HostObjectRef<ModuleObject> mod = ModuleObject::alloc(this, std::move(scope), ACCESS_PUB);

	mod->loadStatus = ModuleLoadStatus::Loading;

	HostRefHolder holder(&globalHeapPoolAlloc);

	LoaderContext context{ fs, mod.get(), false };

	slxfmt::ImgHeader ih;
	fs.read((char *)&ih, sizeof(ih));
	if (memcmp(ih.magic, slxfmt::IMH_MAGIC, sizeof(slxfmt::IMH_MAGIC)))
		throw LoaderError("Bad SLX magic");
	if (ih.fmtVer != 0)
		throw LoaderError("Bad SLX format version");

	if (ih.flags & slxfmt::IMH_MODNAME) {
		auto modName = _loadIdRef(context, holder);
		if (!modName->entries.size())
			throw LoaderError("Empty module name with module name flag set");

		Object *curObject = (Object *)_rootObject;

		// Create parent modules.
		for (size_t i = 0; i < modName->entries.size() - 1; ++i) {
			ObjectRef objectRef = curObject->getMember(modName->entries.at(i).name);

			if ((objectRef.kind != ObjectRefKind::InstanceRef) ||
				(!objectRef)) {
				ScopeUniquePtr subscope(Scope::alloc(&globalHeapPoolAlloc, nullptr));

				peff::String name(&globalHeapPoolAlloc);
				if (!peff::copyAssign(name, modName->entries.at(i).name)) {
					std::terminate();
				}

				// Create a new one if corresponding module does not present.
				auto mod = ModuleObject::alloc(this, std::move(subscope), ACCESS_PUB);
				mod->name = std::move(name);

				if (curObject->getKind() == ObjectKind::RootObject)
					((RootObject *)curObject)->scope->putMember(mod.get());
				else
					((ModuleObject *)curObject)->scope->putMember(mod.get());

				curObject = (Object *)mod.get();
			} else {
				// Continue if the module presents.
				curObject = objectRef.asInstance.instanceObject;
			}
		}

		peff::String lastName(&globalHeapPoolAlloc);
		if (!peff::copyAssign(lastName, modName->entries.back().name)) {
			std::terminate();
		}

		mod->name = std::move(lastName);

		// Add current module.
		if (curObject->getKind() == ObjectKind::RootObject) {
			((RootObject *)curObject)->scope->putMember(mod.get());
		} else {
			auto moduleObject = (ModuleObject *)curObject;

			if (auto member = moduleObject->getMember(lastName); member) {
				if ((member.kind != ObjectRefKind::InstanceRef) ||
					(member.asInstance.instanceObject->getKind() != ObjectKind::Module)) {
					throw LoaderError(
						"Object which corresponds to module name \"" + std::to_string(modName.get(), this) + "\" was found, but is not a module");
				}
				ModuleObject *modMember = (ModuleObject *)member.asInstance.instanceObject;
				if (modMember->loadStatus == ModuleLoadStatus::Loading) {
					throw LoaderError("Cyclic dependency detected");
				}
				if (flags & LMOD_NORELOAD) {
					++modMember->depCount;
					return modMember;
				}
				if (flags & LMOD_NOCONFLICT)
					throw LoaderError("Module \"" + std::to_string(modName.get(), this) + "\" conflicted with existing value which is on the same path");
			}

			if (!moduleObject->scope->putMember(mod.get()))
				throw std::bad_alloc();
		}
	}

	for (uint8_t i = 0; i < ih.nImports; i++) {
		uint32_t len = _read<uint32_t>(fs);
		peff::String name(&globalHeapPoolAlloc);
		name.resize(len);
		fs.read(name.data(), len);

		HostObjectRef<IdRefObject> moduleName = _loadIdRef(context, holder);

		if (!(flags & LMOD_NOIMPORT)) {
			std::unique_ptr<std::istream> moduleStream(_moduleLocator(this, moduleName));
			if (!moduleStream)
				throw LoaderError("Error finding module `" + std::to_string(moduleName.get()) + "' for dependencies");

			auto mod = loadModule(*moduleStream.get(), LMOD_NORELOAD);
		}

		mod->imports.insert(std::move(name), moduleName.get());
	}

	_loadScope(context, mod.get(), flags, holder);

	if (flags & LMOD_IMPLICIT) {
		mod->loadStatus = ModuleLoadStatus::ImplicitlyLoaded;
	} else {
		mod->loadStatus = ModuleLoadStatus::ManuallyLoaded;
	}

	return mod;
}

SLAKE_API HostObjectRef<ModuleObject> slake::Runtime::loadModule(const void *buf, size_t size, LoadModuleFlags flags) {
	util::InputMemStream fs(buf, size);
	return loadModule(fs, flags);
}
