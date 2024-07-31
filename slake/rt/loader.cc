#include <slake/runtime.h>

#include <memory>
#include <slake/util/stream.hh>

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
IdRefObject *Runtime::_loadIdRef(std::istream &fs) {
	auto ref = IdRefObject::alloc(this);

	slxfmt::IdRefEntryDesc i = { 0 };
	while (true) {
		i = _read<slxfmt::IdRefEntryDesc>(fs);

		std::string name(i.lenName, '\0');
		fs.read(name.data(), i.lenName);

		GenericArgList genericArgs;
		for (size_t j = i.nGenericArgs; j; --j)
			genericArgs.push_back(_loadType(fs));

		ref->entries.push_back(IdRefEntry(name, genericArgs));
		if (!(i.flags & slxfmt::RSD_NEXT))
			break;
	};

	return ref.release();
}

/// @brief Load a single value from a stream.
/// @param rt Runtime for the new value.
/// @param fs Stream to be read.
/// @return Object loaded from the stream.
Value Runtime::_loadValue(std::istream &fs) {
	slxfmt::TypeId typeId = _read<slxfmt::TypeId>(fs);

	switch (typeId) {
		case slxfmt::TypeId::None:
			return Value(nullptr);
		case slxfmt::TypeId::I8:
			return Value(_read<std::int8_t>(fs));
		case slxfmt::TypeId::I16:
			return Value(_read<std::int16_t>(fs));
		case slxfmt::TypeId::I32:
			return Value(_read<std::int32_t>(fs));
		case slxfmt::TypeId::I64:
			return Value(_read<std::int64_t>(fs));
		case slxfmt::TypeId::U8:
			return Value(_read<uint8_t>(fs));
		case slxfmt::TypeId::U16:
			return Value(_read<uint16_t>(fs));
		case slxfmt::TypeId::U32:
			return Value(_read<uint32_t>(fs));
		case slxfmt::TypeId::U64:
			return Value(_read<uint64_t>(fs));
		case slxfmt::TypeId::Bool:
			return Value(_read<bool>(fs));
		case slxfmt::TypeId::F32:
			return Value(_read<float>(fs));
		case slxfmt::TypeId::F64:
			return Value(_read<double>(fs));
		case slxfmt::TypeId::String: {
			auto len = _read<uint32_t>(fs);
			std::string s(len, '\0');
			fs.read(&(s[0]), len);
			return Value(StringObject::alloc(this, std::move(s)).release(), true);
		}
		case slxfmt::TypeId::Array: {
			auto elementType = _loadType(fs);

			// stub for debugging.
			// elementType = Type(TypeId::Any);

			HostObjectRef<ArrayObject> value = ArrayObject::alloc(this, elementType);

			auto len = _read<uint32_t>(fs);

			value->values.resize(len);
			for (uint32_t i = 0; i < len; ++i) {
				(value->values[i] = VarObject::alloc(this, ACCESS_PUB, elementType).release())->setData(_loadValue(fs));
			}

			return value.release();
		}
		case slxfmt::TypeId::IdRef:
			return _loadIdRef(fs);
		case slxfmt::TypeId::TypeName:
			return Value(_loadType(fs));
		case slxfmt::TypeId::Reg:
			return Value(ValueType::RegRef, _read<uint32_t>(fs), false);
		case slxfmt::TypeId::RegValue:
			return Value(ValueType::RegRef, _read<uint32_t>(fs), true);
		case slxfmt::TypeId::LocalVar:
			return Value(ValueType::LocalVarRef, _read<uint32_t>(fs), false);
		case slxfmt::TypeId::LocalVarValue:
			return Value(ValueType::LocalVarRef, _read<uint32_t>(fs), true);
		case slxfmt::TypeId::Arg:
			return Value(ValueType::ArgRef, _read<uint32_t>(fs), false);
		case slxfmt::TypeId::ArgValue:
			return Value(ValueType::ArgRef, _read<uint32_t>(fs), true);
		default:
			throw LoaderError("Invalid object type detected");
	}
}

/// @brief Load a single type name from a stream.
/// @param rt Runtime for the new type.
/// @param fs Stream to be read.
/// @return Loaded complete type name.
Type Runtime::_loadType(std::istream &fs) {
	slxfmt::TypeId vt = _read<slxfmt::TypeId>(fs);

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
		case slxfmt::TypeId::Object:
			return _loadIdRef(fs);
		case slxfmt::TypeId::Any:
			return TypeId::Any;
		case slxfmt::TypeId::Bool:
			return Type(ValueType::Bool);
		case slxfmt::TypeId::None:
			return TypeId::None;
		case slxfmt::TypeId::Array:
			return Type(TypeId::Array, _loadType(fs));
		case slxfmt::TypeId::Ref:
			return Type(TypeId::Ref, _loadType(fs));
		case slxfmt::TypeId::TypeName:
			return Type(ValueType::TypeName);
		case slxfmt::TypeId::GenericArg: {
			uint8_t length = _read<uint8_t>(fs);
			std::string name(length, '\0');
			fs.read(name.data(), length);
			return Type(name);
		}
		default:
			throw LoaderError("Invalid type ID");
	}
}

GenericParam Runtime::_loadGenericParam(std::istream &fs) {
	auto gpd = _read<slxfmt::GenericParamDesc>(fs);

	std::string name(gpd.lenName, '\0');
	fs.read(&(name[0]), gpd.lenName);

	GenericParam param;
	param.name = name;

	if (gpd.hasBaseType)
		param.baseType = _loadType(fs);

	for (size_t i = 0; i < gpd.nInterfaces; ++i) {
		param.interfaces.push_back(_loadType(fs));
	}

	return param;
}

/// @brief Load a single scope.
/// @param mod Module value which is treated as a scope.
/// @param fs The input stream.
void Runtime::_loadScope(ModuleObject *mod, std::istream &fs, LoadModuleFlags loadModuleFlags) {
	uint32_t nItemsToRead;

	//
	// Load classes.
	//
	nItemsToRead = _read<uint32_t>(fs);
	for (slxfmt::ClassTypeDesc i = {}; nItemsToRead--;) {
		i = _read<slxfmt::ClassTypeDesc>(fs);

		std::string name(i.lenName, '\0');
		fs.read(name.data(), i.lenName);

		AccessModifier access = 0;
		if (i.flags & slxfmt::CTD_PUB)
			access |= ACCESS_PUB;
		if (i.flags & slxfmt::CTD_FINAL)
			access |= ACCESS_FINAL;

		HostObjectRef<ClassObject> value = ClassObject::alloc(this, access);

		for (size_t j = 0; j < i.nGenericParams; ++j)
			value->genericParams.push_back(_loadGenericParam(fs));

		// Load reference to the parent class.
		if (i.flags & slxfmt::CTD_DERIVED)
			value->parentClass = Type(TypeId::Class, _loadIdRef(fs));

		// Load references to implemented interfaces.
		for (auto j = i.nImpls; j; j--)
			value->implInterfaces.push_back(_loadIdRef(fs));

		_loadScope(value.get(), fs, loadModuleFlags);

		mod->scope->putMember(name, value.release());
	}

	//
	// Load interfaces.
	//
	nItemsToRead = _read<uint32_t>(fs);
	for (slxfmt::InterfaceTypeDesc i = {}; nItemsToRead--;) {
		i = _read<slxfmt::InterfaceTypeDesc>(fs);

		std::string name(i.lenName, '\0');
		fs.read(name.data(), i.lenName);

		AccessModifier access = 0;
		if (i.flags & slxfmt::ITD_PUB)
			access |= ACCESS_PUB;

		HostObjectRef<InterfaceObject> value = InterfaceObject::alloc(this, access);

		for (size_t j = 0; j < i.nGenericParams; ++j)
			value->genericParams.push_back(_loadGenericParam(fs));

		for (auto j = i.nParents; j; j--)
			value->parents.push_back(_loadIdRef(fs));

		_loadScope(value.get(), fs, loadModuleFlags);

		mod->scope->putMember(name, value.release());
	}

	//
	// Load variables.
	//
	nItemsToRead = _read<uint32_t>(fs);
	for (slxfmt::VarDesc i = { 0 }; nItemsToRead--;) {
		i = _read<slxfmt::VarDesc>(fs);

		std::string name(i.lenName, '\0');
		fs.read(name.data(), i.lenName);

		AccessModifier access = 0;
		if (i.flags & slxfmt::VAD_PUB)
			access |= ACCESS_PUB;
		if (i.flags & slxfmt::VAD_STATIC)
			access |= ACCESS_STATIC;
		if (i.flags & slxfmt::VAD_FINAL)
			access |= ACCESS_FINAL;
		if (i.flags & slxfmt::VAD_NATIVE)
			access |= ACCESS_NATIVE;

		auto varType = _loadType(fs);
		HostObjectRef<VarObject> var =
			VarObject::alloc(
				this,
				access,
				varType);

		// Load initial value.
		if (i.flags & slxfmt::VAD_INIT)
			var->setData(_loadValue(fs));
		else {
			switch (varType.typeId) {
				case TypeId::Value:
					switch (varType.getValueTypeExData()) {
						case ValueType::I8:
							var->setData(Value((int8_t)0));
							break;
						case ValueType::I16:
							var->setData(Value((int16_t)0));
							break;
						case ValueType::I32:
							var->setData(Value((int32_t)0));
							break;
						case ValueType::I64:
							var->setData(Value((int64_t)0));
							break;
						case ValueType::U8:
							var->setData(Value((uint8_t)0));
							break;
						case ValueType::U16:
							var->setData(Value((uint16_t)0));
							break;
						case ValueType::U32:
							var->setData(Value((uint32_t)0));
							break;
						case ValueType::U64:
							var->setData(Value((uint64_t)0));
							break;
						case ValueType::Bool:
							var->setData(Value((bool)false));
							break;
						default:
							// Unenumerated value types should never occur.
							throw std::logic_error("Invalid value type");
					}
					break;
				case TypeId::String:
					var->setData(Value(nullptr));
					break;
				case TypeId::Instance:
					var->setData(Value(nullptr));
					break;
				case TypeId::Any:
					var->setData(Value(nullptr));
					break;
				case TypeId::GenericArg:
					break;
				default:
					throw LoaderError("Invalid variable type");
			}
		}

		mod->scope->putMember(name, var.release());
	}

	//
	// Load functions.
	//
	nItemsToRead = _read<uint32_t>(fs);
	while (nItemsToRead--) {
		uint32_t nOverloadings = _read<uint32_t>(fs);

		for (slxfmt::FnDesc i = { 0 }; nOverloadings--;) {
			i = _read<slxfmt::FnDesc>(fs);

			std::string name(i.lenName, '\0');
			fs.read(name.data(), i.lenName);

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
				mod->scope->putMember(name, fn.get());
			}

			HostObjectRef<RegularFnOverloadingObject> overloading = RegularFnOverloadingObject::alloc(fn.get(), access, std::deque<Type>{}, Type());

			if (i.flags & slxfmt::FND_ASYNC)
				overloading->overloadingFlags |= OL_ASYNC;
			if (i.flags & slxfmt::FND_VIRTUAL)
				overloading->overloadingFlags |= OL_VIRTUAL;

			overloading->returnType = _loadType(fs);

			for (size_t j = 0; j < i.nGenericParams; ++j) {
				overloading->genericParams.push_back(_loadGenericParam(fs));
			}

			for (uint8_t j = 0; j < i.nParams; j++) {
				overloading->paramTypes.push_back(_loadType(fs));
			}

			if (i.flags & slxfmt::FND_VARG)
				overloading->overloadingFlags |= OL_VARG;

			if (i.lenBody) {
				overloading->instructions.resize(i.lenBody);

				for (uint32_t j = 0; j < i.lenBody; j++) {
					auto &ins = overloading->instructions[j];

					slxfmt::InsHeader ih = _read<slxfmt::InsHeader>(fs);

					ins.opcode = ih.opcode;
					ins.operands.resize(ih.nOperands);

					for (uint8_t k = 0; k < ih.nOperands; k++)
						ins.operands[k] = _loadValue(fs);
				}
			}

			for (uint32_t j = 0; j < i.nSourceLocDescs; ++j) {
				slxfmt::SourceLocDesc sld = _read<slxfmt::SourceLocDesc>(fs);
				overloading->sourceLocDescs.push_back(sld);
			}

			fn->overloadings.push_back(overloading.release());
		}
	}
}

HostObjectRef<ModuleObject> slake::Runtime::loadModule(std::istream &fs, LoadModuleFlags flags) {
	HostObjectRef<ModuleObject> mod = ModuleObject::alloc(this, ACCESS_PUB);

	slxfmt::ImgHeader ih;
	fs.read((char *)&ih, sizeof(ih));
	if (memcmp(ih.magic, slxfmt::IMH_MAGIC, sizeof(slxfmt::IMH_MAGIC)))
		throw LoaderError("Bad SLX magic");
	if (ih.fmtVer != 0)
		throw LoaderError("Bad SLX format version");

	if (ih.flags & slxfmt::IMH_MODNAME) {
		auto modName = _loadIdRef(fs);
		if (!modName->entries.size())
			throw LoaderError("Empty module name with module name flag set");

		Object *curObject = (Object *)_rootObject;

		// Create parent modules.
		for (size_t i = 0; i < modName->entries.size() - 1; ++i) {
			auto &name = modName->entries[i].name;

			if (!curObject->getMember(name)) {
				// Create a new one if corresponding module does not present.
				auto mod = ModuleObject::alloc(this, ACCESS_PUB);

				if (curObject->getType() == TypeId::RootObject)
					((RootObject *)curObject)->scope->putMember(name, mod.get());
				else
					((ModuleObject *)curObject)->scope->putMember(name, mod.get());

				curObject = (Object *)mod.get();
			} else {
				// Continue if the module presents.
				curObject = curObject->getMember(name);
			}
		}

		auto lastName = modName->entries.back().name;
		// Add current module.
		if (curObject->getType() == TypeId::RootObject)
			((RootObject *)curObject)->scope->putMember(lastName, mod.get());
		else {
			auto moduleObject = (ModuleObject *)curObject;

			if (auto member = moduleObject->getMember(lastName); member) {
				if (flags & LMOD_NORELOAD) {
					if (member->getType() != TypeId::Module)
						throw LoaderError(
							"Object which corresponds to module name \"" + std::to_string(modName, this) + "\" was found, but is not a module");
				}
				if (flags & LMOD_NOCONFLICT)
					throw LoaderError("Module \"" + std::to_string(modName, this) + "\" conflicted with existing value which is on the same path");
			}

			moduleObject->scope->putMember(modName->entries.back().name, mod.get());
		}
	}

	for (uint8_t i = 0; i < ih.nImports; i++) {
		auto len = _read<uint32_t>(fs);
		std::string name(len, '\0');
		fs.read(name.data(), len);

		HostObjectRef<IdRefObject> moduleName = _loadIdRef(fs);

		if (!(flags & LMOD_NOIMPORT)) {
			std::unique_ptr<std::istream> moduleStream(_moduleLocator(this, moduleName));
			if (!moduleStream)
				throw LoaderError("Error finding module `" + std::to_string(moduleName) + "' for dependencies");

			auto mod = loadModule(*moduleStream.get(), LMOD_NORELOAD);

			if (name.size())
				mod->scope->putMember(name, (MemberObject *)AliasObject::alloc(this, mod.get()).release());
		}

		mod->imports[name] = moduleName.get();
	}

	_loadScope(mod.get(), fs, flags);
	return mod.release();
}

HostObjectRef<ModuleObject> slake::Runtime::loadModule(const void *buf, size_t size, LoadModuleFlags flags) {
	util::InputMemStream fs(buf, size);
	return loadModule(fs, flags);
}
