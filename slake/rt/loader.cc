#include <slake/runtime.h>
#include <slake/slxfmt.h>

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
static RefValue *_loadRef(Runtime *rt, std::istream &fs) {
	auto ref = std::make_unique<RefValue>(rt);

	slxfmt::RefScopeDesc i = { 0 };
	while (true) {
		i = _read<slxfmt::RefScopeDesc>(fs);
		std::string name(i.lenName, '\0');
		fs.read(&(name[0]), i.lenName);

		ref->scopes.push_back(name);
		if (!(i.flags & slxfmt::RSD_NEXT))
			break;
	};

	return ref.release();
}

/// @brief Load a single value from a stream.
/// @param rt Runtime for the new value.
/// @param fs Stream to be read.
/// @return Value loaded from the stream.
static Value *_loadValue(Runtime *rt, std::istream &fs) {
	slxfmt::ValueDesc i = {};
	fs.read((char *)&i, sizeof(i));
	switch (i.type) {
		case slxfmt::ValueType::NONE:
			return nullptr;
		case slxfmt::ValueType::I8:
			return new I8Value(rt, _read<std::int8_t>(fs));
		case slxfmt::ValueType::I16:
			return new I16Value(rt, _read<std::int16_t>(fs));
		case slxfmt::ValueType::I32:
			return new I32Value(rt, _read<std::int32_t>(fs));
		case slxfmt::ValueType::I64:
			return new I64Value(rt, _read<std::int64_t>(fs));
		case slxfmt::ValueType::U8:
			return new U8Value(rt, _read<uint8_t>(fs));
		case slxfmt::ValueType::U16:
			return new U16Value(rt, _read<uint16_t>(fs));
		case slxfmt::ValueType::U32:
			return new U32Value(rt, _read<uint32_t>(fs));
		case slxfmt::ValueType::U64:
			return new U64Value(rt, _read<uint64_t>(fs));
		case slxfmt::ValueType::BOOL:
			return new BoolValue(rt, _read<bool>(fs));
		case slxfmt::ValueType::F32:
			return new F32Value(rt, _read<float>(fs));
		case slxfmt::ValueType::F64:
			return new F64Value(rt, _read<double>(fs));
		case slxfmt::ValueType::STRING: {
			auto len = _read<uint32_t>(fs);
			std::string s(len, '\0');
			fs.read(&(s[0]), len);
			return new StringValue(rt, s);
		}
		case slxfmt::ValueType::REF: {
			return _loadRef(rt, fs);
		}
		default:
			throw new LoaderError("Invalid value type detected");
	}
}

/// @brief Load a single type name from a stream.
/// @param rt Runtime for the new type.
/// @param fs Stream to be read.
/// @param vt Previous read value type.
/// @return Loaded complete type name.
static Type _loadTypeName(Runtime *rt, std::istream &fs, slxfmt::ValueType vt) {
	switch (vt) {
		case slxfmt::ValueType::I8:
			return ValueType::I8;
		case slxfmt::ValueType::I16:
			return ValueType::I16;
		case slxfmt::ValueType::I32:
			return ValueType::I32;
		case slxfmt::ValueType::I64:
			return ValueType::I64;
		case slxfmt::ValueType::U8:
			return ValueType::U8;
		case slxfmt::ValueType::U16:
			return ValueType::U16;
		case slxfmt::ValueType::U32:
			return ValueType::U32;
		case slxfmt::ValueType::U64:
			return ValueType::U64;
		case slxfmt::ValueType::F32:
			return ValueType::F32;
		case slxfmt::ValueType::F64:
			return ValueType::F64;
		case slxfmt::ValueType::STRING:
			return ValueType::STRING;
		case slxfmt::ValueType::OBJECT:
			return _loadRef(rt, fs);
		case slxfmt::ValueType::ANY:
			return ValueType::ANY;
		case slxfmt::ValueType::BOOL:
			return ValueType::BOOL;
		case slxfmt::ValueType::NONE:
			return ValueType::NONE;
		case slxfmt::ValueType::ARRAY:
			return Type(_loadTypeName(rt, fs, _read<slxfmt::ValueType>(fs)));
		case slxfmt::ValueType::MAP:
			return Type(
				_loadTypeName(rt, fs, _read<slxfmt::ValueType>(fs)),
				_loadTypeName(rt, fs, _read<slxfmt::ValueType>(fs)));
		default:
			throw new LoaderError("Invalid type name detected");
	}
}

/// @brief Load a single scope.
/// @param mod Module value which is treated as a scope.
/// @param fs The input stream.
static void _loadScope(ModuleValue *mod, std::istream &fs) {
	Runtime *const rt = mod->getRuntime();

	uint32_t nItemsToRead;

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

		std::unique_ptr<VarValue> var =
			std::make_unique<VarValue>(
				rt,
				access,
				_loadTypeName(rt, fs, _read<slxfmt::ValueType>(fs)));

		// Load initial value.
		if (i.flags & slxfmt::VAD_INIT)
			var->setData(_loadValue(rt, fs));

		mod->addMember(name, var.release());
	}

	//
	// Load functions.
	//
	nItemsToRead = _read<uint32_t>(fs);
	for (slxfmt::FnDesc i = { 0 }; nItemsToRead--;) {
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

		/*
		for (auto j = 0; j < i.nGenericParams; j++) {
			uint32_t lenGenericParamName = _read<uint32_t>(fs);
			std::string name(lenGenericParamName, '\0');
			fs.read(&(name[0]), lenGenericParamName);
		}*/

		std::unique_ptr<FnValue> fn = std::make_unique<FnValue>(rt, (uint32_t)i.lenBody, access, _loadTypeName(rt, fs, _read<slxfmt::ValueType>(fs)));

		for (auto j = 0; j < i.nParams; j++) {
			_loadTypeName(rt, fs, _read<slxfmt::ValueType>(fs));
		}

		if (i.flags & slxfmt::FND_VARG)
			/* stub */;

		if (i.lenBody) {
			auto body = fn->getBody();

			for (uint32_t j = 0; j < i.lenBody; j++) {
				slxfmt::InsHeader ih = _read<slxfmt::InsHeader>(fs);
				body[j].opcode = ih.opcode;
				body[j].nOperands = ih.nOperands;
				for (uint8_t k = 0; k < ih.nOperands; k++)
					body[j].operands[k] = _loadValue(rt, fs);
			}
		}
		mod->addMember(name, fn.release());
	}

	//
	// Load classes.
	//
	nItemsToRead = _read<uint32_t>(fs);
	for (slxfmt::ClassTypeDesc i = { 0 }; nItemsToRead--;) {
		i = _read<slxfmt::ClassTypeDesc>(fs);

		std::string name(i.lenName, '\0');
		fs.read(name.data(), i.lenName);

		AccessModifier access = 0;
		if (i.flags & slxfmt::CTD_PUB)
			access |= ACCESS_PUB;
		if (i.flags & slxfmt::CTD_FINAL)
			access |= ACCESS_FINAL;

		std::unique_ptr<ClassValue> value = std::make_unique<ClassValue>(rt, access);

		// Load reference to the parent class.
		if (i.flags & slxfmt::CTD_DERIVED)
			value->parentClass = Type(ValueType::CLASS, _loadRef(rt, fs));

		// Load references to implemented interfaces.
		for (auto j = i.nImpls; j; j--)
			value->implInterfaces.push_back(_loadRef(rt, fs));

		_loadScope(value.get(), fs);

		mod->addMember(name, value.release());
	}

	//
	// Load interfaces.
	//
	nItemsToRead = _read<uint32_t>(fs);
	for (slxfmt::InterfaceTypeDesc i = { 0 }; nItemsToRead--;) {
		i = _read<slxfmt::InterfaceTypeDesc>(fs);

		std::string name(i.lenName, '\0');
		fs.read(name.data(), i.lenName);

		AccessModifier access = 0;
		if (i.flags & slxfmt::ITD_PUB)
			access |= ACCESS_PUB;

		std::unique_ptr<InterfaceValue> value = std::make_unique<InterfaceValue>(rt, access);

		for (auto j = i.nParents; j; j--)
			value->parents.push_back(_loadRef(rt, fs));

		_loadScope(value.get(), fs);

		mod->addMember(name, value.release());
	}

	//
	// Load traits.
	//
	nItemsToRead = _read<uint32_t>(fs);
	for (slxfmt::TraitTypeDesc i = { 0 }; nItemsToRead--;) {
		i = _read<slxfmt::TraitTypeDesc>(fs);

		std::string name(i.lenName, '\0');
		fs.read(name.data(), i.lenName);

		AccessModifier access = 0;
		if (i.flags & slxfmt::TTD_PUB)
			access |= ACCESS_PUB;

		std::unique_ptr<TraitValue> value = std::make_unique<TraitValue>(rt, access);

		for (auto j = i.nParents; j; j--)
			value->parents.push_back(_loadRef(rt, fs));

		_loadScope(value.get(), fs);

		mod->addMember(name, value.release());
	}

	//
	// Load structures.
	//
	nItemsToRead = _read<uint32_t>(fs);
	for (slxfmt::StructTypeDesc i = { 0 }; nItemsToRead--;) {
		fs.read((char *)&i, sizeof(i));
		std::string name(i.lenName, '\0');
		fs.read(&(name[0]), i.lenName);

		AccessModifier access = 0;
		access |= ACCESS_PUB;

		while (i.nMembers--) {
			slxfmt::StructMemberDesc smd;
			fs.read((char *)&smd, sizeof(smd));
			std::string memberName(smd.lenName, '\0');
			fs.read(&(memberName[0]), smd.lenName);

			{
				auto t = smd.type;
				_loadTypeName(rt, fs, t);
			}
		}
	}
}

ValueRef<ModuleValue> slake::Runtime::loadModule(std::istream &fs, std::string name) {
	std::unique_ptr<ModuleValue> mod = std::make_unique<ModuleValue>(this, ACCESS_PUB);

	slxfmt::ImgHeader ih;
	fs.read((char *)&ih, sizeof(ih));
	if (memcmp(ih.magic, slxfmt::IMH_MAGIC, sizeof(slxfmt::IMH_MAGIC)))
		throw LoaderError("Bad SLX magic");
	if (ih.fmtVer != 0)
		throw LoaderError("Bad SLX format version");

	for (uint8_t i = 0; i < ih.nImports; i++) {
		auto len = _read<uint32_t>(fs);
		std::string name(len, '\0');
		fs.read(&(name[0]), len);

		std::unique_ptr<Value> ref(_loadValue(this, fs));
	}

	_loadScope(mod.get(), fs);
	return mod.release();
}

ValueRef<ModuleValue> slake::Runtime::loadModule(const void *buf, std::size_t size, std::string name) {
	util::InputMemStream fs(buf, size);
	return loadModule(fs, name);
}
