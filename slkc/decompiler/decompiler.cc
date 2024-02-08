#include "decompiler.h"

#include <string>

using namespace slake;

static const char *_ctrlCharNames[] = {
	"0", "x01", "x02", "x03", "x04", "x05", "x06",
	"a", "b", "v", "n", "v", "f", "r",
	"x0e", "x0f", "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "x19", "x1a",
	"x1b", "x1c", "x1d", "x1e", "x1f"
};

decompiler::DecompilerFlags decompiler::decompilerFlags = 0;

void slake::decompiler::decompile(std::istream &fs, std::ostream &os) {
	auto rt = std::make_unique<slake::Runtime>();
	auto mod = rt->loadModule(fs, LMOD_NOIMPORT);

	auto modName = rt->getFullName(mod.get());

	for (auto &i : *mod.get())
		decompileValue(rt.get(), i.second, os);
}

void slake::decompiler::decompileValue(Runtime *rt, Value *value, std::ostream &os, int indentLevel) {
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
		case TypeId::STRING: {
			os << '"';

			for (auto i : ((StringValue *)value)->getData()) {
				if (isprint(i))
					os << i;
				else
					os << "\\" << _ctrlCharNames[i];
			}
			os << '"';
			break;
		}
		case TypeId::REF:
			os << to_string((RefValue *)value);
			break;
		case TypeId::TYPENAME:
			os << to_string(((TypeNameValue *)value)->getData(), rt);
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

			std::set<slxfmt::SourceLocDesc *> dumpedSourceLocationDescs;

			// Dump instructions.
			if (v->getInsCount()) {
				for (size_t i = 0; i < v->getInsCount(); ++i) {
					auto ins = &(v->getBody()[i]);

					if (!(decompilerFlags & DECOMP_SRCLOCINFO)) {
						for (auto &j : v->sourceLocDescs) {
							if (dumpedSourceLocationDescs.count(&j))
								continue;

							if ((i > j.offIns) &
								(i < j.offIns + j.nIns)) {
								os << std::string(indentLevel + 1, '\t')
								   << "// Source location=" << j.line << ":" << j.column << ", " << j.nIns << " instructions\n";

								dumpedSourceLocationDescs.insert(&j);
							}
						}
					}

					os << std::string(indentLevel + 1, '\t');

					if (slake::OPCODE_MNEMONIC_MAP.count(ins->opcode))
						os << slake::OPCODE_MNEMONIC_MAP.at(ins->opcode);
					else
						os << (uint16_t)ins->opcode;

					for (size_t j = 0; j < ins->operands.size(); ++j) {
						os << (j ? ", " : " ");
						decompileValue(rt, ins->operands[j].get(), os, indentLevel);
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
		case TypeId::REG_REF: {
			RegRefValue *v = (RegRefValue *)value;
			if (v->unwrapValue)
				os << "*";
			os << "%" << std::to_string(v->index);
			break;
		}
		case TypeId::LVAR_REF: {
			LocalVarRefValue *v = (LocalVarRefValue *)value;
			if (v->unwrapValue)
				os << "*";
			os << "$" << std::to_string(v->index);
			break;
		}
		case TypeId::ARG_REF: {
			ArgRefValue *v = (ArgRefValue *)value;
			if (v->unwrapValue)
				os << "*";
			os << "[" << std::to_string(v->index) << "]";
			break;
		}
		default:
			throw std::logic_error("Unhandled value type");
	}
}

std::string slake::decompiler::accessToString(AccessModifier access) {
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
