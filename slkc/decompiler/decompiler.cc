#include "decompiler.h"

#include <string>

#include "mnemonic.h"

using namespace slake;

static const char *_ctrlCharNames[] = {
	"0", "x01", "x02", "x03", "x04", "x05", "x06",
	"a", "b", "v", "n", "v", "f", "r",
	"x0e", "x0f", "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "x19", "x1a",
	"x1b", "x1c", "x1d", "x1e", "x1f"
};

void slake::Decompiler::decompile(std::istream &fs, std::ostream &os) {
	auto rt = std::make_unique<slake::Runtime>();
	rt->getRootValue()->addMember("main", *(rt->loadModule(fs, "main")));
	for (auto &i : *rt->getRootValue())
		decompileValue(rt.get(), *i.second, os);
}

void slake::Decompiler::decompileValue(Runtime *rt, Value *value, std::ostream &os, int indentLevel) {
	switch (value->getType().valueType) {
		case ValueType::I8:
			os << to_string(((I8Value *)value)->getData());
			break;
		case ValueType::I16:
			os << to_string(((I16Value *)value)->getData());
			break;
		case ValueType::I32:
			os << to_string(((I32Value *)value)->getData());
			break;
		case ValueType::I64:
			os << to_string(((I64Value *)value)->getData());
			break;
		case ValueType::U8:
			os << to_string(((U8Value *)value)->getData());
			break;
		case ValueType::U16:
			os << to_string(((U16Value *)value)->getData());
			break;
		case ValueType::U32:
			os << to_string(((U32Value *)value)->getData());
			break;
		case ValueType::U64:
			os << to_string(((U64Value *)value)->getData());
			break;
		case ValueType::F32:
			os << to_string(((F32Value *)value)->getData());
			break;
		case ValueType::F64:
			os << to_string(((F64Value *)value)->getData());
			break;
		case ValueType::BOOL:
			os << ((BoolValue *)value)->getData() ? "true" : "false";
			break;
		case ValueType::FN: {
			auto v = (FnValue *)value;
			if (v->getAccess())
				os << std::string(indentLevel, '\t') << ".access " << accessToString(v->getAccess()) << "\n";
			os << std::string(indentLevel, '\t');
			os << ".fn "
			   << getTypeName(rt, v->getReturnType()) << " "
			   << v->getName();
			for (auto &i : v->getParamTypes())
				os << " " << getTypeName(rt, i);
			os << "\n";

			for (size_t i = 0; i < v->getInsCount(); ++i) {
				auto ins = &(v->getBody()[i]);
				os << std::string(indentLevel + 1, '\t');

				if (mnemonics.count(ins->opcode))
					os << mnemonics.at(ins->opcode);
				else
					os << (uint16_t)ins->opcode;

				for (size_t j = 0; j < ins->nOperands; j++) {
					os << (j ? ", " : " ");
					decompileValue(rt, *ins->operands[j], os, indentLevel);
				}

				os << ";\n";
			}
			os << std::string(indentLevel, '\t') << ".end\n\n";
			break;
		}
		case ValueType::MOD: {
			auto v = (ModuleValue *)value;
			os << std::string(indentLevel, '\t') << ".module " << v->getName() << "\n";

			for (auto &i : *v)
				decompileValue(rt, i.second, os, indentLevel + 1);

			os << std::string(indentLevel, '\t') << ".end"
			   << "\n\n";
			break;
		}
		case ValueType::VAR: {
			VarValue *v = (VarValue *)value;
			os << std::string(indentLevel, '\t') << ".var " << getTypeName(rt, v->getVarType()) << "\n\n";
			break;
		}
		case ValueType::CLASS: {
			ClassValue *v = (ClassValue *)value;
			os << std::string(indentLevel, '\t') << ".class " << v->getName() << "\n";

			for (auto &i : *v)
				decompileValue(rt, i.second, os, indentLevel + 1);

			os << std::string(indentLevel, '\t') << ".end"
			   << "\n\n";
			break;
		}
		case ValueType::INTERFACE: {
			break;
		}
		case ValueType::TRAIT: {
			break;
		}
		case ValueType::STRUCT: {
			break;
		}
	}
}

std::string slake::Decompiler::getTypeName(Runtime *rt, Type type) {
	switch (type.valueType) {
		case ValueType::I8:
			return "i8";
		case ValueType::I16:
			return "i16";
		case ValueType::I32:
			return "i32";
		case ValueType::I64:
			return "i64";
		case ValueType::U8:
			return "u8";
		case ValueType::U16:
			return "u16";
		case ValueType::U32:
			return "u32";
		case ValueType::U64:
			return "u64";
		case ValueType::F32:
			return "f32";
		case ValueType::F64:
			return "f64";
		case ValueType::STRING:
			return "string";
		case ValueType::BOOL:
			return "bool";
		case ValueType::ARRAY:
			return getTypeName(rt, type.getArrayExData()) + "[]";
		case ValueType::OBJECT:
			return rt->resolveName((MemberValue *)*type.getCustomTypeExData()) + "[]";
		case ValueType::ANY:
			return "any";
		case ValueType::NONE:
			return "void";
		default:
			throw DecompileError("Unrecognized type");
	}
}

std::string slake::Decompiler::refToString(shared_ptr<RefValue> ref) {
	string s;
	for (size_t i = 0; i < ref->scopes.size(); i++)
		s += (i ? "." : "") + ref->scopes[i];
	return s;
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
