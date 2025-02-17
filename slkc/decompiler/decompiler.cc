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

static std::unique_ptr<std::istream> fsModuleLocator(slake::Runtime *rt, const peff::DynArray<slake::IdRefEntry> &ref) {
	std::string path;
	for (size_t i = 0; i < ref.size(); ++i) {
		path += ref.at(i).name;
		if (i + 1 < ref.size())
			path += "/";
	}

	std::unique_ptr<std::ifstream> fs = std::make_unique<std::ifstream>();
	fs->exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
	for (auto i : modulePaths) {
		try {
			fs->open(i + "/" + path + ".slx", std::ios_base::binary);
			return fs;
		} catch (std::ios::failure) {
			fs->clear();
		}
	}

	return nullptr;
}

void slake::decompiler::decompile(std::istream &fs, std::ostream &os) {
	auto rt = std::make_unique<slake::Runtime>(peff::getDefaultAlloc());
	rt->setModuleLocator(fsModuleLocator);

	HostObjectRef<ModuleObject> mod;
	try {
		mod = rt->loadModule(fs, 0);
	} catch (slake::LoaderError e) {
		try {
			mod = rt->loadModule(fs, LMOD_NOIMPORT);
		} catch (slake::LoaderError e) {
			os << "Error loading the module: " << e.what() << std::endl;
			return;
		}
	}

	auto modName = rt->getFullName(mod.get());

	for (auto it = mod->scope->members.begin(); it != mod->scope->members.end(); ++it)
		decompileObject(rt.get(), it.value(), os);
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
		auto &scope = ref->entries.at(i);

		if (i)
			s += ".";

		s += scope.name.data();

		if (auto nGenericParams = scope.genericArgs.size(); nGenericParams) {
			s += "<";
			for (size_t j = 0; j < nGenericParams; ++j) {
				if (j)
					s += ",";
				s += decompileTypeName(scope.genericArgs.at(j), ref->getRuntime());
			}
			s += ">";
		}
		if (scope.hasParamTypes) {
			s += "(";

			for (size_t j = 0; j < scope.paramTypes.size(); ++j) {
				if (j)
					s += ",";
				s += decompileTypeName(scope.paramTypes.at(j), ref->getRuntime());
			}
			if (scope.hasVarArg)
				s += "...";

			s += ")";
		}
	}
	return s;
}

void slake::decompiler::decompileObject(Runtime *rt, Object *object, std::ostream &os, int indentLevel) {
	switch (object->getKind()) {
	case slake::ObjectKind::String: {
		os << '"';

		for (auto i : ((StringObject *)object)->data) {
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

		for (size_t i = 0; i < v->length; ++i) {
			if (i)
				os << ", ";

			Value value;
			value = rt->readVarUnsafe(ObjectRef::makeArrayElementRef(v, i));
			decompileValue(rt, value, os, indentLevel);
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
				switch (i->overloadingKind) {
				case FnOverloadingKind::Regular: {
					RegularFnOverloadingObject *fn = (RegularFnOverloadingObject *)i;

					HostRefHolder hostRefHolder(&rt->globalHeapPoolAlloc);

					/*
					opti::ProgramAnalyzedInfo analyzedInfo(rt);
					if (auto e = opti::analyzeProgramInfo(rt, fn, analyzedInfo, hostRefHolder);
						e) {
						e.reset();
					}*/

					// trimFnInstructions(fn->instructions);

					for (size_t i = 0; i < fn->instructions.size(); ++i) {
						auto &ins = fn->instructions.at(i);

						/*
						if (analyzedInfo.codeBlockBoundaries.contains(i)) {
							os << std::string(indentLevel + 1, '\t')
							   << "//" << std::string(80, '=') << std::endl;
						}*/

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

						for (size_t j = 0; j < ins.nOperands; ++j) {
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

		for (auto it = v->scope->members.begin(); it != v->scope->members.end(); ++it)
			decompileObject(rt, it.value(), os, indentLevel + 1);

		os << std::string(indentLevel, '\t')
		   << ".end"
		   << "\n\n";
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

		for (auto it = v->scope->members.begin(); it != v->scope->members.end(); ++it)
			decompileObject(rt, it.value(), os, indentLevel + 1);

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
		os << std::to_string(value.getI8()) << "i8";
		break;
	case ValueType::I16:
		os << std::to_string(value.getI16()) << "i16";
		break;
	case ValueType::I32:
		os << std::to_string(value.getI32());
		break;
	case ValueType::I64:
		os << std::to_string(value.getI64()) << "i64";
		break;
	case ValueType::U8:
		os << std::to_string(value.getU8()) << "u8";
		break;
	case ValueType::U16:
		os << std::to_string(value.getU16()) << "u16";
		break;
	case ValueType::U32:
		os << std::to_string(value.getU32()) << "u";
		break;
	case ValueType::U64:
		os << std::to_string(value.getU64()) << "u64";
		break;
	case ValueType::F32:
		os << std::to_string(value.getF32()) << "f";
		break;
	case ValueType::F64:
		os << std::to_string(value.getF64());
		break;
	case ValueType::Bool:
		os << (value.getBool() ? "true" : "false");
		break;
	case ValueType::RegRef:
		os << "%" << std::to_string(value.getRegIndex());
		break;
	case ValueType::ObjectRef: {
		auto ptr = value.getObjectRef();

		if (!ptr)
			os << "null";
		else
			decompileObject(rt, ptr.asInstance.instanceObject, os, indentLevel);
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
