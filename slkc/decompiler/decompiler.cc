#include "decompiler.hh"

#include <string>

#include "mnemonic.hh"

using namespace Slake;

static const char* _ctrlCharNames[] = {
	"0", "x01", "x02", "x03", "x04", "x05", "x06",
	"a", "b", "v", "n", "v", "f", "r",
	"x0e", "x0f", "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "x19", "x1a",
	"x1b", "x1c", "x1d", "x1e", "x1f"
};

template <typename T>
static T _readValue(std::fstream& fs) {
	T value;
	fs.read((char*)&value, sizeof(value));
	return value;
}

std::shared_ptr<Compiler::Expr> Slake::Decompiler::readValue(std::fstream& fs) {
	SlxFmt::ValueDesc i = {};
	fs.read((char*)&i, sizeof(i));
	switch (i.type) {
		case SlxFmt::ValueType::NONE:
			return std::make_shared<Compiler::NullLiteralExpr>(Compiler::location());
		case SlxFmt::ValueType::I8:
			return std::make_shared<Compiler::IntLiteralExpr>(Compiler::location(), _readValue<std::int8_t>(fs));
		case SlxFmt::ValueType::I16:
			return std::make_shared<Compiler::IntLiteralExpr>(Compiler::location(), _readValue<std::int16_t>(fs));
		case SlxFmt::ValueType::I32:
			return std::make_shared<Compiler::IntLiteralExpr>(Compiler::location(), _readValue<std::int32_t>(fs));
		case SlxFmt::ValueType::I64:
			return std::make_shared<Compiler::LongLiteralExpr>(Compiler::location(), _readValue<std::int64_t>(fs));
		case SlxFmt::ValueType::U8:
			return std::make_shared<Compiler::UIntLiteralExpr>(Compiler::location(), _readValue<std::uint8_t>(fs));
		case SlxFmt::ValueType::U16:
			return std::make_shared<Compiler::UIntLiteralExpr>(Compiler::location(), _readValue<std::uint16_t>(fs));
		case SlxFmt::ValueType::U32:
			return std::make_shared<Compiler::UIntLiteralExpr>(Compiler::location(), _readValue<std::uint32_t>(fs));
		case SlxFmt::ValueType::U64:
			return std::make_shared<Compiler::ULongLiteralExpr>(Compiler::location(), _readValue<std::uint64_t>(fs));
		case SlxFmt::ValueType::BOOL:
			return std::make_shared<Compiler::BoolLiteralExpr>(Compiler::location(), _readValue<bool>(fs));
		case SlxFmt::ValueType::FLOAT:
			return std::make_shared<Compiler::FloatLiteralExpr>(Compiler::location(), _readValue<float>(fs));
		case SlxFmt::ValueType::DOUBLE:
			return std::make_shared<Compiler::DoubleLiteralExpr>(Compiler::location(), _readValue<double>(fs));
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
			return std::make_shared<Compiler::StringLiteralExpr>(Compiler::location(), s);
		}
		case SlxFmt::ValueType::REF: {
			auto ref = std::make_shared<Compiler::RefExpr>(Compiler::location(), "");
			SlxFmt::ScopeRefDesc i = { 0 };
			auto& j = ref;
			do {
				i = _readValue<SlxFmt::ScopeRefDesc>(fs);
				std::string name(i.lenName + 1, '\0');
				fs.read(&(name[0]), i.lenName);
				j->next = std::make_shared<Compiler::RefExpr>(Compiler::location(), name + (i.hasNext ? "." : ""));
				j = j->next;
			} while (i.hasNext);
			return ref;
		}
		default:
			throw Decompiler::DecompileError("Invalid value type: " + std::to_string((std::uint8_t)i.type));
	}
}

void toTypeName(std::fstream& fs, SlxFmt::ValueType& vt) {
	switch (vt) {
		case SlxFmt::ValueType::I8:
			printf("i8");
			break;
		case SlxFmt::ValueType::I16:
			printf("i16");
			break;
		case SlxFmt::ValueType::I32:
			printf("i32");
			break;
		case SlxFmt::ValueType::I64:
			printf("i64");
			break;
		case SlxFmt::ValueType::U8:
			printf("u8");
			break;
		case SlxFmt::ValueType::U16:
			printf("u16");
			break;
		case SlxFmt::ValueType::U32:
			printf("u32");
			break;
		case SlxFmt::ValueType::U64:
			printf("u64");
			break;
		case SlxFmt::ValueType::FLOAT:
			printf("float");
			break;
		case SlxFmt::ValueType::DOUBLE:
			printf("double");
			break;
		case SlxFmt::ValueType::STRING:
			printf("string");
			break;
		case SlxFmt::ValueType::OBJECT: {
			SlxFmt::ValueDesc vd;
			fs.read((char*)&vd, sizeof(vd));

			auto ref = std::make_shared<Compiler::RefExpr>(Compiler::location(), "");
			SlxFmt::ScopeRefDesc i = { 0 };
			auto& j = ref;
			do {
				i = _readValue<SlxFmt::ScopeRefDesc>(fs);
				std::string name(i.lenName + 1, '\0');
				fs.read(&(name[0]), i.lenName);
				j->next = std::make_shared<Compiler::RefExpr>(Compiler::location(), name + (i.hasNext ? "." : ""));
				j = j->next;
			} while (i.hasNext);
			printf("@%s", std::to_string(*ref).c_str());
			break;
		}
		case SlxFmt::ValueType::NONE:
			printf("any");
			break;
	}
}

void Slake::Decompiler::decompileScope(std::fstream& fs, std::uint8_t indentLevel) {
	for (SlxFmt::VarDesc i = { 0 };;) {
		fs.read((char*)&i, sizeof(i));
		if (!(i.lenName))
			break;
		std::string name(i.lenName, '\0');
		fs.read(&(name[0]), i.lenName);

		printf("%s", std::string(indentLevel, '\t').c_str());

		if (i.flags & SlxFmt::VAD_INIT)
			printf("any %s = %s;\n", name.c_str(), std::to_string(*readValue(fs)).c_str());
		else
			printf("any %s;\n", name.c_str());
	}

	for (SlxFmt::FnDesc i = { 0 };;) {
		fs.read((char*)&i, sizeof(i));
		if (!(i.lenName))
			break;
		std::string name(i.lenName, '\0');
		fs.read(&(name[0]), i.lenName);

		printf("%sany %s(", std::string(indentLevel, '\t').c_str(), name.c_str());

		for (auto j = 0; j < i.nParams; j++) {
			SlxFmt::ValueType vt = SlxFmt::ValueType::NONE;
			fs.read((char*)&vt, sizeof(vt));
			toTypeName(fs, vt);
			if (j + 1 < i.nParams)
				printf(", ");
		}
		puts(") {");

		std::shared_ptr<State> s = std::make_shared<State>();
		std::list<Compiler::Ins> insList;
		for (std::uint32_t j = 0; j < i.lenBody; j++) {
			SlxFmt::InsHeader ih = _readValue<SlxFmt::InsHeader>(fs);
			Compiler::Ins ins(ih.opcode, {});
			for (std::uint8_t k = 0; k < ih.nOperands; k++)
				ins.operands.push_back(readValue(fs));
			insList.push_back(ins);
			switch (ih.opcode) {
				case Opcode::JMP:
				case Opcode::JT:
				case Opcode::JF:
					s->labelNames[j + 1] = name + "_" + std::to_string(j + 1);
					break;
				case Opcode::ENTER:
					s->labelNames[j + 1] = name + "_blkend_" + std::to_string(j + 1);
			}
		}

		std::size_t k = 0;
		for (auto j : insList) {
			if (s->labelNames.count(k))
				printf("%s%s:\n", std::string(indentLevel, '\t').c_str(), s->labelNames[k].c_str());

			if (j.opcode == Opcode::LEAVE)
				indentLevel--;
			printf("%s", std::string(indentLevel, '\t').c_str());
			if (j.opcode == Opcode::ENTER)
				indentLevel++;

			if (mnemonics.count(j.opcode))
				printf("\tasm %s ", mnemonics[j.opcode]);
			else
				printf("\tasm 0x%02x ", (std::uint32_t)j.opcode);
			for (std::uint8_t l = 0; l < j.operands.size(); l++) {
				switch (j.opcode) {
					case Opcode::ENTER:
					case Opcode::JMP:
					case Opcode::JT:
					case Opcode::JF:
						if (j.operands[l]->getType() == Compiler::ExprType::LITERAL &&
							std::static_pointer_cast<Compiler::LiteralExpr>(j.operands[l])->getLiteralType() == Compiler::LiteralType::LT_UINT) {
							auto addr = std::static_pointer_cast<Compiler::UIntLiteralExpr>(j.operands[l])->data;
							if (s->labelNames.count(addr)) {
								printf("%s%s", s->labelNames[addr].c_str(), l && (l < j.operands.size()) ? "," : "");
								continue;
							}
						}
						break;
				}
				printf("%s%s", std::to_string(*j.operands[l]).c_str(), l && (l < j.operands.size()) ? "," : "");
			}
			putchar('\n');
			k++;
		}

		printf("%s}\n", std::string(indentLevel, '\t').c_str());
	}

	for (SlxFmt::ClassTypeDesc i = { 0 };;) {
		fs.read((char*)&i, sizeof(i));
		if (!(i.lenName))
			break;
		std::string name(i.lenName, '\0');
		fs.read(&(name[0]), i.lenName);

		printf("%s", std::string(indentLevel, '\t').c_str());

		if (i.flags & SlxFmt::CTD_PUB)
			printf("pub ");
		if (i.flags & SlxFmt::CTD_FINAL)
			printf("final ");
		printf("%s %s", i.flags & SlxFmt::CTD_TRAIT ? "trait" : "class", name.c_str());
		if (i.flags & SlxFmt::CTD_DERIVED) {
			printf(": ");

			SlxFmt::ValueDesc vd;
			fs.read((char*)&vd, sizeof(vd));

			auto ref = std::make_shared<Compiler::RefExpr>(Compiler::location(), "");
			SlxFmt::ScopeRefDesc i = { 0 };
			auto& j = ref;
			do {
				i = _readValue<SlxFmt::ScopeRefDesc>(fs);
				std::string name(i.lenName + 1, '\0');
				fs.read(&(name[0]), i.lenName);
				j->next = std::make_shared<Compiler::RefExpr>(Compiler::location(), name + (i.hasNext ? "." : ""));
				j = j->next;
			} while (i.hasNext);
			printf("@%s", std::to_string(*ref).c_str());
		}
		puts(" {");

		decompileScope(fs, indentLevel + 1);

		printf("%s}\n", std::string(indentLevel, '\t').c_str());
	}

	for (SlxFmt::StructTypeDesc i = { 0 };;) {
		fs.read((char*)&i, sizeof(i));
		if (!(i.lenName))
			break;
		std::string name(i.lenName, '\0');
		fs.read(&(name[0]), i.lenName);

		printf("%s", std::string(indentLevel, '\t').c_str());

		if (i.flags & SlxFmt::STD_PUB)
			printf("pub ");
		printf("struct %s", name.c_str());
		puts(" {");

		printf("%s}\n", std::string(indentLevel, '\t').c_str());
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
