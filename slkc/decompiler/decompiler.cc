#include "decompiler.h"
#include "mnemonic.h"

#include <string>

using namespace slake;

static const char *_ctrlCharNames[] = {
	"0", "x01", "x02", "x03", "x04", "x05", "x06",
	"a", "b", "v", "n", "v", "f", "r",
	"x0e", "x0f", "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "x19", "x1a",
	"x1b", "x1c", "x1d", "x1e", "x1f"
};

void slake::Decompiler::decompile(std::istream &fs, std::ostream &os) {
	auto rt = std::make_unique<slake::Runtime>();
	auto mod = rt->loadModule(fs, 0);

	auto modName = rt->resolveName(*mod);

	for (auto &i : **mod)
		decompileValue(rt.get(), i.second, os);
}

void slake::Decompiler::decompileValue(Runtime *rt, Value *value, std::ostream &os, int indentLevel) {
	if (!value) {
		os << "null";
		return;
	}
	switch (value->getType().typeId) {
		case TypeId::I8:
			os << to_string(((I8Value *)value)->getData());
			break;
		case TypeId::I16:
			os << to_string(((I16Value *)value)->getData());
			break;
		case TypeId::I32:
			os << to_string(((I32Value *)value)->getData());
			break;
		case TypeId::I64:
			os << to_string(((I64Value *)value)->getData());
			break;
		case TypeId::U8:
			os << to_string(((U8Value *)value)->getData());
			break;
		case TypeId::U16:
			os << to_string(((U16Value *)value)->getData());
			break;
		case TypeId::U32:
			os << to_string(((U32Value *)value)->getData());
			break;
		case TypeId::U64:
			os << to_string(((U64Value *)value)->getData());
			break;
		case TypeId::F32:
			os << to_string(((F32Value *)value)->getData());
			break;
		case TypeId::F64:
			os << to_string(((F64Value *)value)->getData());
			break;
		case TypeId::BOOL:
			os << ((BoolValue *)value)->getData() ? "true" : "false";
			break;
		case TypeId::STRING:
			os << ((StringValue *)value)->getData();
			break;
		case TypeId::REF:
			os << to_string((RefValue *)value);
			break;
		case TypeId::TYPENAME:
			os << to_string(((TypeNameValue *)value)->getData());
			break;
		case TypeId::FN: {
			auto v = (FnValue *)value;

			// Dump access of the function.
			if (v->getAccess())
				os << std::string(indentLevel, '\t')
				   << ".access " << accessToString(v->getAccess()) << "\n";

			os << std::string(indentLevel, '\t')
			   << (v->getInsCount() ? ".fn " : ".fndecl ")
			   << std::to_string(v->getReturnType(), rt) << " "
			   << v->getName();

			// Dump parameter types.
			for (auto &i : v->getParamTypes())
				os << " " << std::to_string(i, rt);
			os << "\n";

			// Dump instructions.
			if (v->getInsCount()) {
				for (size_t i = 0; i < v->getInsCount(); ++i) {
					auto ins = &(v->getBody()[i]);

					os << std::string(indentLevel + 1, '\t');

					if (mnemonics.count(ins->opcode))
						os << mnemonics.at(ins->opcode);
					else
						os << (uint16_t)ins->opcode;

					for (size_t j = 0; j < ins->nOperands; ++j) {
						os << (j ? ", " : " ");
						decompileValue(rt, *ins->operands[j], os, indentLevel);
					}

					os << ";\n";
				}
				os << std::string(indentLevel, '\t')
				   << ".end\n\n";
			} else
				os << std::string(indentLevel, '\t')
				   << "\n";
			break;
		}
		case TypeId::MOD: {
			auto v = (ModuleValue *)value;
			os << std::string(indentLevel, '\t')
			   << ".module " << v->getName() << "\n";

			for (auto &i : *v)
				decompileValue(rt, i.second, os, indentLevel + 1);

			os << std::string(indentLevel, '\t')
			   << ".end"
			   << "\n\n";
			break;
		}
		case TypeId::VAR: {
			VarValue *v = (VarValue *)value;
			os << std::string(indentLevel, '\t')
			   << ".var " << std::to_string(v->getVarType(), rt) << " " << v->getName() << "\n";
			break;
		}
		case TypeId::CLASS: {
			ClassValue *v = (ClassValue *)value;
			os << std::string(indentLevel, '\t')
			   << ".class " << v->getName() << "\n";

			for (auto &i : *v)
				decompileValue(rt, i.second, os, indentLevel + 1);

			os << std::string(indentLevel, '\t')
			   << ".end"
			   << "\n\n";
			break;
		}
		case TypeId::INTERFACE: {
			break;
		}
		case TypeId::TRAIT: {
			break;
		}
		case TypeId::STRUCT: {
			break;
		}
		default:
			throw std::logic_error("Unhandled value type");
	}
}

std::string slake::Decompiler::accessToString(AccessModifier access) {
	std::string s;

	if (access & ACCESS_PUB)
		s += "pub ";
	if (access & ACCESS_NATIVE)
		s += "native ";
	if (access & ACCESS_STATIC)
		s += "static ";
	if (access & ACCESS_FINAL)
		s += "final ";
	if (access & ACCESS_OVERRIDE)
		s += "override ";

	return s;
}
