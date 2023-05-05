#include <slake/runtime.h>
#include <slake/slxfmt.h>

#include <memory>
#include <slake/util/stream.hh>

using namespace Slake;

template <typename T>
static T _read(std::istream &fs) {
	T value;
	fs.read((char *)&value, sizeof(value));
	return value;
}

RefValue *readRef(Runtime *rt, std::istream &fs) {
	auto ref = std::make_unique<RefValue>(rt);

	SlxFmt::ScopeRefDesc i = { 0 };
	while (true) {
		i = _read<SlxFmt::ScopeRefDesc>(fs);
		std::string name(i.lenName, '\0');
		fs.read(&(name[0]), i.lenName);

		ref->scopes.push_back(name);
		if (!(i.flags & SlxFmt::SRD_NEXT))
			break;
	};

	return ref.release();
}

Value *readValue(Runtime *rt, std::istream &fs) {
	SlxFmt::ValueDesc i = {};
	fs.read((char *)&i, sizeof(i));
	switch (i.type) {
		case SlxFmt::ValueType::NONE:
			return nullptr;
		case SlxFmt::ValueType::I8:
			return new I8Value(rt, _read<std::int8_t>(fs));
		case SlxFmt::ValueType::I16:
			return new I16Value(rt, _read<std::int16_t>(fs));
		case SlxFmt::ValueType::I32:
			return new I32Value(rt, _read<std::int32_t>(fs));
		case SlxFmt::ValueType::I64:
			return new I64Value(rt, _read<std::int64_t>(fs));
		case SlxFmt::ValueType::U8:
			return new U8Value(rt, _read<std::uint8_t>(fs));
		case SlxFmt::ValueType::U16:
			return new U16Value(rt, _read<std::uint16_t>(fs));
		case SlxFmt::ValueType::U32:
			return new U32Value(rt, _read<std::uint32_t>(fs));
		case SlxFmt::ValueType::U64:
			return new U64Value(rt, _read<std::uint64_t>(fs));
		case SlxFmt::ValueType::BOOL:
			return new BoolValue(rt, _read<bool>(fs));
		case SlxFmt::ValueType::FLOAT:
			return new FloatValue(rt, _read<float>(fs));
		case SlxFmt::ValueType::DOUBLE:
			return new DoubleValue(rt, _read<double>(fs));
		case SlxFmt::ValueType::STRING: {
			auto len = _read<std::uint32_t>(fs);
			std::string s(len + 1, '\0');
			fs.read(&(s[0]), len);
			return new StringValue(rt, s);
		}
		case SlxFmt::ValueType::REF: {
			return readRef(rt, fs);
		}
		default:
			throw new LoaderError("Invalid value type detected");
	}
}

Type readTypeName(Runtime *rt, std::istream &fs, SlxFmt::ValueType vt) {
	switch (vt) {
		case SlxFmt::ValueType::I8:
			return ValueType::I8;
		case SlxFmt::ValueType::I16:
			return ValueType::I16;
		case SlxFmt::ValueType::I32:
			return ValueType::I32;
		case SlxFmt::ValueType::I64:
			return ValueType::I64;
		case SlxFmt::ValueType::U8:
			return ValueType::U8;
		case SlxFmt::ValueType::U16:
			return ValueType::U16;
		case SlxFmt::ValueType::U32:
			return ValueType::U32;
		case SlxFmt::ValueType::U64:
			return ValueType::U64;
		case SlxFmt::ValueType::FLOAT:
			return ValueType::FLOAT;
		case SlxFmt::ValueType::DOUBLE:
			return ValueType::DOUBLE;
		case SlxFmt::ValueType::STRING:
			return ValueType::STRING;
		case SlxFmt::ValueType::OBJECT: {
			SlxFmt::ValueDesc vd;
			fs.read((char *)&vd, sizeof(vd));
			return readRef(rt, fs);
		}
		case SlxFmt::ValueType::ANY:
			return ValueType::ANY;
		case SlxFmt::ValueType::BOOL:
			return ValueType::BOOL;
		case SlxFmt::ValueType::NONE:
			return ValueType::NONE;
		case SlxFmt::ValueType::ARRAY:
			return Type(readTypeName(rt, fs, _read<SlxFmt::ValueType>(fs)));
		case SlxFmt::ValueType::MAP:
			return Type(
				readTypeName(rt, fs, _read<SlxFmt::ValueType>(fs)),
				readTypeName(rt, fs, _read<SlxFmt::ValueType>(fs)));
		default:
			throw new LoaderError("Invalid type name detected");
	}
}

void Slake::Runtime::_loadScope(ModuleValue *mod, std::istream &fs) {
	Runtime *const rt = mod->getRuntime();

	for (SlxFmt::VarDesc i = { 0 };;) {
		std::unique_ptr<VarValue> var;

		fs.read((char *)&i, sizeof(i));
		if (!(i.lenName))
			break;
		std::string name(i.lenName, '\0');
		fs.read(&(name[0]), i.lenName);

		AccessModifier access = 0;
		if (i.flags & SlxFmt::VAD_PUB)
			access |= ACCESS_PUB;
		if (i.flags & SlxFmt::VAD_STATIC)
			access |= ACCESS_STATIC;
		if (i.flags & SlxFmt::VAD_FINAL)
			access |= ACCESS_FINAL;
		if (i.flags & SlxFmt::VAD_NATIVE)
			access |= ACCESS_NATIVE;

		auto tn = readTypeName(rt, fs, _read<SlxFmt::ValueType>(fs));
		var = std::make_unique<VarValue>(rt, access, tn, mod);
		if (i.flags & SlxFmt::VAD_INIT) {
			std::unique_ptr<Value> v(readValue(rt, fs));

			var->setValue(v.release());
		}
		mod->addMember(name, var.release());
	}

	for (SlxFmt::FnDesc i = { 0 };;) {
		fs.read((char *)&i, sizeof(i));
		if (!(i.lenName))
			break;
		std::string name(i.lenName, '\0');
		fs.read(&(name[0]), i.lenName);

		AccessModifier access = 0;
		if (i.flags & SlxFmt::FND_PUB)
			access |= ACCESS_PUB;
		if (i.flags & SlxFmt::FND_STATIC)
			access |= ACCESS_STATIC;
		if (i.flags & SlxFmt::FND_FINAL)
			access |= ACCESS_FINAL;
		if (i.flags & SlxFmt::FND_OVERRIDE)
			access |= ACCESS_OVERRIDE;
		if (i.flags & SlxFmt::FND_NATIVE)
			access |= ACCESS_NATIVE;

		std::unique_ptr<FnValue> fn = std::make_unique<FnValue>(
			rt,
			(std::uint32_t)i.lenBody,  // Code trick for avoiding referencing to the bit-field.
			access,
			mod);

		auto resultType = _read<SlxFmt::ValueType>(fs);

		if (i.nGenericParams) {
			for (auto j = 0; j < i.nGenericParams; j++) {
				std::uint32_t lenGenericParamName = _read<std::uint32_t>(fs);
				std::string name(lenGenericParamName, '\0');
				fs.read(&(name[0]), lenGenericParamName);
			}
		}

		for (auto j = 0; j < i.nParams; j++) {
			SlxFmt::ValueType vt = SlxFmt::ValueType::NONE;
			fs.read((char *)&vt, sizeof(vt));
			auto tn = readTypeName(rt, fs, vt);
		}
		if (i.flags & SlxFmt::FND_VARG)
			/* stub */;

		for (std::uint32_t j = 0; j < i.lenBody; j++) {
			SlxFmt::InsHeader ih = _read<SlxFmt::InsHeader>(fs);
			fn->_body[j].opcode = ih.opcode;
			fn->_body[j].nOperands = ih.nOperands;
			for (std::uint8_t k = 0; k < ih.nOperands; k++)
				fn->_body[j].operands[k] = readValue(rt, fs);
		}

		mod->addMember(name, fn.release());
	}

	for (SlxFmt::ClassTypeDesc i = { 0 };;) {
		fs.read((char *)&i, sizeof(i));
		if (!(i.lenName))
			break;
		std::string name(i.lenName, '\0');
		fs.read(&(name[0]), i.lenName);

		AccessModifier access = 0;
		if (i.flags & SlxFmt::CTD_PUB)
			access |= ACCESS_PUB;
		if (i.flags & SlxFmt::CTD_FINAL)
			access |= ACCESS_FINAL;

		RefValue *parent;

		if (i.flags & SlxFmt::CTD_DERIVED) {
			SlxFmt::ValueDesc vd;
			fs.read((char *)&vd, sizeof(vd));

			SlxFmt::ScopeRefDesc i = { 0 };
			parent = (RefValue *)readValue(rt, fs);
		}

		std::unique_ptr<ClassValue> value = std::make_unique<ClassValue>(rt, access, parent);

		if (i.nImpls) {
			for (auto j = i.nImpls; j; j--) {
				auto tn = readTypeName(rt, fs, _read<SlxFmt::ValueType>(fs));
				if (tn.valueType != ValueType::CLASS)
					throw LoaderError("Incompatible value type for traits");
				value->_traits.push_back(tn);
			}
		}

		_loadScope(value.get(), fs);
		mod->addMember(name, value.release());
	}

	for (SlxFmt::StructTypeDesc i = { 0 };;) {
		fs.read((char *)&i, sizeof(i));
		if (!(i.lenName))
			break;
		std::string name(i.lenName, '\0');
		fs.read(&(name[0]), i.lenName);

		AccessModifier access = 0;
		access |= ACCESS_PUB;

		while (i.nMembers--) {
			SlxFmt::StructMemberDesc smd;
			fs.read((char *)&smd, sizeof(smd));
			std::string memberName(smd.lenName, '\0');
			fs.read(&(memberName[0]), smd.lenName);

			{
				auto t = smd.type;
				readTypeName(rt, fs, t);
			}
		}
	}
}


void Slake::Runtime::loadModule(std::string name, std::istream &fs) {
	std::unique_ptr<ModuleValue> mod = std::make_unique<ModuleValue>(this, ACCESS_PUB, _rootValue);

	SlxFmt::ImgHeader ih;
	fs.read((char *)&ih, sizeof(ih));
	if ((ih.magic[0] != SlxFmt::IMH_MAGIC[0]) ||
		(ih.magic[1] != SlxFmt::IMH_MAGIC[1]) ||
		(ih.magic[2] != SlxFmt::IMH_MAGIC[2]) ||
		(ih.magic[3] != SlxFmt::IMH_MAGIC[3]))
		throw LoaderError("Bad SLX magic");
	if (ih.fmtVer != 0)
		throw LoaderError("Bad SLX format version");
	if (ih.nImports) {
		for (std::uint8_t i = 0; i < ih.nImports; i++) {
			auto len = _read<std::uint32_t>(fs);
			std::string name(len, '\0');
			fs.read(&(name[0]), len);

			std::unique_ptr<Value> ref(readValue(this, fs));
		}
	}

	_loadScope(mod.get(), fs);
	_rootValue->addMember(name, mod.release());
}

void Slake::Runtime::loadModule(std::string name, const void *buf, std::size_t size) {
	Util::InputMemStream fs(buf, size);
	loadModule(name, fs);
}
