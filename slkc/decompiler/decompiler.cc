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

	for (auto &i : mod->scope->members)
		decompileValue(rt.get(), i.second, os);
}

void slake::decompiler::decompileValue(Runtime *rt, Value *value, std::ostream &os, int indentLevel) {
	if (!value) {
		os << "null";
		return;
	}
	switch (value->getType().typeId) {
		case slake::TypeId::I8:
			os << std::to_string(((I8Value *)value)->getData());
			break;
		case slake::TypeId::I16:
			os << std::to_string(((I16Value *)value)->getData());
			break;
		case slake::TypeId::I32:
			os << std::to_string(((I32Value *)value)->getData());
			break;
		case slake::TypeId::I64:
			os << std::to_string(((I64Value *)value)->getData());
			break;
		case slake::TypeId::U8:
			os << std::to_string(((U8Value *)value)->getData());
			break;
		case slake::TypeId::U16:
			os << std::to_string(((U16Value *)value)->getData());
			break;
		case slake::TypeId::U32:
			os << std::to_string(((U32Value *)value)->getData());
			break;
		case slake::TypeId::U64:
			os << std::to_string(((U64Value *)value)->getData());
			break;
		case slake::TypeId::F32:
			os << std::to_string(((F32Value *)value)->getData());
			break;
		case slake::TypeId::F64:
			os << std::to_string(((F64Value *)value)->getData());
			break;
		case slake::TypeId::Bool:
			os << ((BoolValue *)value)->getData() ? "true" : "false";
			break;
		case slake::TypeId::String: {
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
		case slake::TypeId::IdRef:
			os << std::to_string((IdRefValue *)value);
			break;
		case slake::TypeId::TypeName:
			os << std::to_string(((TypeNameValue *)value)->getData(), rt);
			break;
		case slake::TypeId::Fn: {
			auto v = (FnValue *)value;

			// Dump access of the function.
			if (v->getAccess())
				os << std::string(indentLevel, '\t')
				   << ".access " << accessToString(v->getAccess()) << "\n";

			if (v->fnFlags & FN_ASYNC)
				puts(".async");

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

					if (!(decompilerFlags & DECOMP_SRCLOC)) {
						for (auto &j : v->sourceLocDescs) {
							if (dumpedSourceLocationDescs.count(&j))
								continue;

							if ((i > j.offIns) &&
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
						decompileValue(rt, ins->operands[j], os, indentLevel);
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
		case slake::TypeId::Module: {
			auto v = (ModuleValue *)value;
			os << std::string(indentLevel, '\t')
			   << ".module " << v->getName() << "\n";

			for (auto &i : v->scope->members)
				decompileValue(rt, i.second, os, indentLevel + 1);

			os << std::string(indentLevel, '\t')
			   << ".end"
			   << "\n\n";
			break;
		}
		case slake::TypeId::Var: {
			VarValue *v = (VarValue *)value;
			os << std::string(indentLevel, '\t')
			   << ".var " << std::to_string(v->getVarType(), rt) << " " << v->getName() << "\n";
			break;
		}
		case slake::TypeId::Class: {
			ClassValue *v = (ClassValue *)value;
			os << std::string(indentLevel, '\t')
			   << ".class " << v->getName() << "\n";

			for (auto &i : v->scope->members)
				decompileValue(rt, i.second, os, indentLevel + 1);

			os << std::string(indentLevel, '\t')
			   << ".end"
			   << "\n\n";
			break;
		}
		case slake::TypeId::Interface: {
			break;
		}
		case slake::TypeId::Trait: {
			break;
		}
		case slake::TypeId::RegRef: {
			RegRefValue *v = (RegRefValue *)value;
			if (v->unwrapValue)
				os << "*";
			os << "%" << std::to_string(v->index);
			break;
		}
		case slake::TypeId::LocalVarRef: {
			LocalVarRefValue *v = (LocalVarRefValue *)value;
			if (v->unwrapValue)
				os << "*";
			os << "$" << std::to_string(v->index);
			break;
		}
		case slake::TypeId::ArgRef: {
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
