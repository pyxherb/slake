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
		decompileObject(rt.get(), i.second, os);
}

std::string slake::decompiler::decompileTypeName(const Type &type, Runtime *rt) {
	std::string s;
	switch (type.typeId) {
		case TypeId::GenericArg:
		case TypeId::Instance:
			s += "@";
			break;
		default:
			break;
	}
	s += std::to_string(type, rt);
	return s;
}

std::string slake::decompiler::decompileIdRef(const IdRefObject *ref) {
	std::string s;

	for (size_t i = 0; i < ref->entries.size(); ++i) {
		auto scope = ref->entries[i];

		if (i)
			s += ".";

		if ((scope.name.find('.') != std::string::npos) ||
			(scope.name.find('<') != std::string::npos) ||
			(scope.name.find('>') != std::string::npos)) {
			s += "{";
			s += scope.name;
			s += "}";
		} else {
			s += scope.name;
		}

		if (auto nGenericParams = scope.genericArgs.size(); nGenericParams) {
			s += "<";
			for (size_t j = 0; j < nGenericParams; ++j) {
				if (j)
					s += ",";
				s += decompileTypeName(scope.genericArgs[j], ref->getRuntime());
			}
			s += ">";
		}
	}
	return s;
}

void slake::decompiler::decompileObject(Runtime *rt, Object *object, std::ostream &os, int indentLevel) {
	switch (object->getKind()) {
		case slake::ObjectKind::String: {
			os << '"';

			for (auto i : ((StringObject*)object)->data) {
				if (isprint(i))
					os << i;
				else
					os << "\\" << _ctrlCharNames[i];
			}
			os << '"';
			break;
		}
		case slake::ObjectKind::Array: {
			os << "[";

			auto v = ((ArrayObject *)object);

			for (size_t i = 0; i < v->values.size(); ++i) {
				if (i)
					os << ", ";

				decompileValue(rt, v->values[i]->getData(), os, indentLevel);
			}

			os << "]";
			break;
		}
		case slake::ObjectKind::IdRef:
			os << decompileIdRef((IdRefObject *)object);
			break;
		case slake::ObjectKind::Fn: {
			auto v = (FnObject *)object;

			for (auto &i : v->overloadings) {
				// Dump access of the function.
				if (i->access)
					os << std::string(indentLevel, '\t')
					   << ".access " << accessToString(v->accessModifier) << "\n";

				os << std::string(indentLevel, '\t')
				   << ".fn ";

				os << decompileTypeName(i->returnType, rt) << " "
				   << v->getName();

				// Dump parameter types.
				for (auto &i : i->paramTypes)
					os << " " << decompileTypeName(i, rt);
				os << "\n";

				std::set<slxfmt::SourceLocDesc *> dumpedSourceLocationDescs;

				// Dump instructions.
				if (v->accessModifier & ACCESS_NATIVE) {
					os << std::string(indentLevel, '\t')
					   << "\n";
				} else {
					switch (i->getOverloadingKind()) {
						case FnOverloadingKind::Regular: {
							RegularFnOverloadingObject *fn = (RegularFnOverloadingObject *)i;
							for (size_t i = 0; i < fn->instructions.size(); ++i) {
								auto &ins = fn->instructions[i];

								if (!(decompilerFlags & DECOMP_SRCLOC)) {
									for (auto &j : fn->sourceLocDescs) {
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

								if (ins.output.valueType != ValueType::Undefined) {
									decompileValue(rt, ins.output, os, indentLevel);
									os << " = ";
								}

								if (slake::OPCODE_MNEMONIC_MAP.count(ins.opcode))
									os << slake::OPCODE_MNEMONIC_MAP.at(ins.opcode);
								else
									os << (uint16_t)ins.opcode;

								for (size_t j = 0; j < ins.operands.size(); ++j) {
									os << (j ? ", " : " ");
									decompileValue(rt, ins.operands[j], os, indentLevel);
								}

								os << ";\n";
							}
							os << std::string(indentLevel, '\t')
							   << ".end\n\n";
							break;
						}
						case FnOverloadingKind::Native: {
						}
					}
				}
			}
			break;
		}
		case slake::ObjectKind::Module: {
			auto v = (ModuleObject *)object;
			os << std::string(indentLevel, '\t')
			   << ".module " << v->getName() << "\n";

			for (auto &i : v->scope->members)
				decompileObject(rt, i.second, os, indentLevel + 1);

			os << std::string(indentLevel, '\t')
			   << ".end"
			   << "\n\n";
			break;
		}
		case slake::ObjectKind::Var: {
			VarObject *v = (VarObject *)object;
			os << std::string(indentLevel, '\t')
			   << ".var " << decompileTypeName(v->getVarType(), rt) << " " << v->getName() << "\n";
			break;
		}
		case slake::ObjectKind::Class: {
			ClassObject *v = (ClassObject *)object;

			// Dump access of the class.
			if (v->accessModifier)
				os << std::string(indentLevel, '\t')
				   << ".access " << accessToString(v->accessModifier) << "\n";

			os << std::string(indentLevel, '\t')
			   << ".class " << v->getName() << "\n";

			for (auto &i : v->scope->members)
				decompileObject(rt, i.second, os, indentLevel + 1);

			os << std::string(indentLevel, '\t')
			   << ".end"
			   << "\n\n";
			break;
		}
		case slake::ObjectKind::Interface: {
			break;
		}
	}
}

void slake::decompiler::decompileValue(Runtime *rt, Value value, std::ostream &os, int indentLevel) {
	switch (value.valueType) {
		case ValueType::I8:
			os << std::to_string(value.getI8());
			break;
		case ValueType::I16:
			os << std::to_string(value.getI16());
			break;
		case ValueType::I32:
			os << std::to_string(value.getI32());
			break;
		case ValueType::I64:
			os << std::to_string(value.getI64());
			break;
		case ValueType::U8:
			os << std::to_string(value.getU8());
			break;
		case ValueType::U16:
			os << std::to_string(value.getU16());
			break;
		case ValueType::U32:
			os << std::to_string(value.getU32());
			break;
		case ValueType::U64:
			os << std::to_string(value.getU64());
			break;
		case ValueType::F32:
			os << std::to_string(value.getF32());
			break;
		case ValueType::F64:
			os << std::to_string(value.getF64());
			break;
		case ValueType::Bool:
			os << value.getBool() ? "true" : "false";
			break;
		case ValueType::RegRef:
			os << "%" << std::to_string(value.getRegIndex());
			break;
		case ValueType::ObjectRef: {
			auto ptr = value.getObjectRef().objectPtr;

			if (!ptr)
				os << "null";
			else
				decompileObject(rt, ptr, os, indentLevel);
			break;
		}
		case ValueType::TypeName:
			os << decompileTypeName(value.getTypeName(), rt);
			break;
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
