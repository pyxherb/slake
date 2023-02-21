#include "state.hh"

#include <string>

static const char* _ctrlCharNames[] = {
	"0",
	"x01",
	"x02",
	"x03",
	"x04",
	"x05",
	"x06",
	"a",
	"b",
	"v",
	"n",
	"v",
	"f",
	"r",
	"x0e",
	"x0f",
	"x10",
	"x11",
	"x12",
	"x13",
	"x14",
	"x15",
	"x16",
	"x17",
	"x18",
	"x19",
	"x1a",
	"x1b",
	"x1c",
	"x1d",
	"x1e",
	"x1f"
};

const char* mnemonics[] = {
	"NOP",
	"PUSH",
	"POP",
	"LOAD",
	"STORE",
	"LLOAD",
	"LSTORE",
	"EXPAND",
	"SHRINK",
	"ENTER",
	"LEAVE",
	"LBASE",
	"ADD",
	"SUB",
	"MUL",
	"DIV",
	"MOD",
	"AND",
	"OR",
	"XOR",
	"LAND",
	"LOR",
	"EQ",
	"NEQ",
	"LT",
	"GT",
	"LTEQ",
	"GTEQ",
	"REV",
	"NOT",
	"INC",
	"DEC",
	"NEG",
	"LSH",
	"RSH",
	"JMP",
	"JT",
	"JF",
	"CALL",
	"ACALL",
	"RET",
	"SYSCALL",
	"ASYSCALL",
	"THROW",
	"PUSHXH",
	"ABORT"
};

template <typename T>
static T _readValue(std::fstream& fs) {
	T value;
	fs.read((char*)&value, sizeof(value));
	return value;
}

std::string Slake::Decompiler::readValue(std::fstream& fs) {
	SlxFmt::ValueDesc i = {};
	fs.read((char*)&i, sizeof(i));
	switch (i.type) {
		case SlxFmt::ValueType::ANY:
			return "null";
		case SlxFmt::ValueType::I8:
			return std::to_string(_readValue<std::int8_t>(fs));
		case SlxFmt::ValueType::I16:
			return std::to_string(_readValue<std::int16_t>(fs));
		case SlxFmt::ValueType::I32:
			return std::to_string(_readValue<std::int32_t>(fs));
		case SlxFmt::ValueType::I64:
			return std::to_string(_readValue<std::int64_t>(fs));
		case SlxFmt::ValueType::U8:
			return std::to_string(_readValue<std::uint8_t>(fs));
		case SlxFmt::ValueType::U16:
			return std::to_string(_readValue<std::uint16_t>(fs));
		case SlxFmt::ValueType::U32:
			return std::to_string(_readValue<std::uint32_t>(fs));
		case SlxFmt::ValueType::U64:
			return std::to_string(_readValue<std::uint64_t>(fs));
		case SlxFmt::ValueType::BOOL:
			return _readValue<bool>(fs) ? "true" : "false";
		case SlxFmt::ValueType::FLOAT:
			return std::to_string(_readValue<float>(fs));
		case SlxFmt::ValueType::DOUBLE:
			return std::to_string(_readValue<double>(fs));
		case SlxFmt::ValueType::STRING: {
			auto len = _readValue<std::uint32_t>(fs);
			std::string rs(len + 1, '\0'), s;
			fs.read(&(rs[0]), len);
			while (rs.size()) {
				auto c = rs[0];
				if (c == '\\')
					s += c;
				if (c == '\"')
					s += '\\';
				if (std::isprint(c))
					s += c;
				else if (std::iscntrl(c) && c != '\xff')
					s += _ctrlCharNames[c];
				else {
					char esc[5];
					std::sprintf(esc, "\\x%02x", (int)c);
					s += esc;
				}
				rs.erase(rs.begin());
				rs.shrink_to_fit();
			}
			return s;
		}
		case SlxFmt::ValueType::REF: {
			std::string s;
			SlxFmt::ScopeRefDesc i = { 0 };
			do {
				i = _readValue<SlxFmt::ScopeRefDesc>(fs);
				std::string name(i.lenName + 1, '\0');
				fs.read(&(name[0]), i.lenName);
				s += name + (i.hasNext ? "." : "");
			} while (i.hasNext);
			return s;
		}
		default:
			throw DecompileError("Invalid value type: " + std::to_string((std::uint8_t)i.type));
	}
}

void Slake::Decompiler::decompileScope(std::fstream& fs) {
	for (SlxFmt::VarDesc i = { 0 };;) {
		fs.read((char*)&i, sizeof(i));
		if (!(i.lenName))
			break;
		std::string name(i.lenName, '\0');
		fs.read(&(name[0]), i.lenName);
		printf("VAR %s = %s;\n", name.c_str(), readValue(fs).c_str());
	}

	for (SlxFmt::FnDesc i = { 0 };;) {
		fs.read((char*)&i, sizeof(i));
		if (!(i.lenName))
			break;
		std::string name(i.lenName, '\0');
		fs.read(&(name[0]), i.lenName);
		printf("FN %s:\n", name.c_str());

		std::shared_ptr<State> s = std::make_shared<State>();
		for (std::uint32_t j = 0; j < i.lenBody; j++) {
			SlxFmt::InsHeader ih = _readValue<SlxFmt::InsHeader>(fs);
			if ((std::uint8_t)ih.opcode < sizeof(mnemonics) / sizeof(mnemonics[0]))
				printf("%s ", mnemonics[(std::uint8_t)ih.opcode]);
			else
				printf("0x%02x ", (std::uint32_t)ih.opcode);
			for (std::uint8_t k = 0; k < ih.nOperands; k++)
				printf("%s%s", readValue(fs).c_str(), k && (k < ih.nOperands) ? "," : "");
			putchar('\n');
		}
		/*
		for (auto i : readValue(fs))
			if (i == '\\')
				putchar(i);
			if (i == '\"')
				putchar('\\');
			if (std::isprint(i))
				putchar(i);
			else if (std::iscntrl(i) && i != '\xff')
				printf("%s", _ctrlCharNames[i]);
			else
				printf("\\x%02x", (int)i);
		}
		*/
	}
}


void Slake::Decompiler::decompile(std::fstream& fs) {
	SlxFmt::ImgHeader ih;
	fs.read((char*)&ih, sizeof(ih));
	if ((ih.magic[0] != SlxFmt::IMH_MAGIC[0]) ||
		(ih.magic[1] != SlxFmt::IMH_MAGIC[1]) ||
		(ih.magic[2] != SlxFmt::IMH_MAGIC[2]) ||
		(ih.magic[3] != SlxFmt::IMH_MAGIC[3]))
		throw DecompileError("Bad SLX magic");
	if (ih.fmtVer != 0)
		throw DecompileError("Bad SLX format version");
	decompileScope(fs);
}
