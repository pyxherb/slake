#include "../bc2cxx.h"

using namespace slake;
using namespace slake::slkaot;
using namespace slake::slkaot::bc2cxx;

std::string BC2CXX::mangleGeneratorStateClassName(Object *object) {
	char s[sizeof("_SLKAOTGeneratorState_") - 1 + sizeof(void *) * 2 + 1];

	memcpy(s, "_SLKAOTGeneratorState_", sizeof("_SLKAOTGeneratorState_") - 1);

	sprintf(s + (sizeof("_SLKAOTGeneratorState_") - 1), "%0*zx", (int)sizeof(void *) * 2, (uintptr_t)object);

	return std::string(s);
}

std::string BC2CXX::mangleJumpDestLabelName(uint32_t offIns) {
	return "ins_" + std::to_string(offIns);
}

std::string BC2CXX::mangleConstantObjectName(Object *object) {
	char s[sizeof("constobj_") - 1 + sizeof(void *) * 2 + 1];

	memcpy(s, "constobj_", sizeof("constobj_") - 1);

	sprintf(s + (sizeof("constobj_") - 1), "%0*zx", (int)sizeof(void *) * 2, (uintptr_t)object);
	return s;
}

std::string BC2CXX::mangleRegLocalVarName(uint32_t idxReg) {
	return "local_reg_" + std::to_string(idxReg);
}

std::string BC2CXX::mangleArgListLocalVarName(uint32_t idxReg) {
	return "args_" + std::to_string(idxReg);
}

std::string BC2CXX::mangleLocalVarName(uint32_t idxReg) {
	return "local_var_" + std::to_string(idxReg);
}

std::string BC2CXX::mangleParamName(uint32_t idxArg) {
	return "param_" + std::to_string(idxArg);
}

std::string BC2CXX::mangleRef(const peff::DynArray<IdRefEntry> &entries) {
	std::string name;

	for (size_t i = 0; i < entries.size(); ++i) {
		const IdRefEntry &idRefEntry = entries.at(i);

		if (i)
			name += "_";

		for (size_t j = 0; j < idRefEntry.name.size(); ++j) {
			char c[3];

			c[0] = (idRefEntry.name.at(j) & 0xf) + 'A';
			c[1] = (idRefEntry.name.at(j) >> 4) + 'A';
			c[2] = '\0';

			name += c;
		}

		name += "0";

		for (size_t j = 0; j < idRefEntry.genericArgs.size(); ++j) {
			if (j)
				name += "2";
			name += mangleTypeName(idRefEntry.genericArgs.at(j));
		}

		name += "1";
	}

	return name;
}

std::string BC2CXX::mangleTypeName(const Type &type) {
	switch (type.typeId) {
		case TypeId::None:
			return "void";
		case TypeId::I8:
			return "i8";
		case TypeId::I16:
			return "i16";
		case TypeId::I32:
			return "i32";
		case TypeId::I64:
			return "i64";
		case TypeId::U8:
			return "u8";
		case TypeId::U16:
			return "u16";
		case TypeId::U32:
			return "u32";
		case TypeId::U64:
			return "u64";
		case TypeId::F32:
			return "f32";
		case TypeId::F64:
			return "f64";
		case TypeId::Bool:
			return "bool";
		case TypeId::String:
			return "string";
		case TypeId::Instance: {
			if (type.isLoadingDeferred()) {
				HostObjectRef<IdRefObject> id = (IdRefObject *)type.getCustomTypeExData();

				return "obj" + mangleRef(id->entries);
			} else {
				HostObjectRef<MemberObject> id = (MemberObject *)type.getCustomTypeExData();

				peff::DynArray<IdRefEntry> entries;
				if (!id->associatedRuntime->getFullRef(peff::getDefaultAlloc(), id.get(), entries))
					throw std::bad_alloc();

				return "obj" + mangleRef(entries);
			}
		}
		case TypeId::Array: {
			return "arr" + mangleTypeName(type.getArrayExData());
		}
		case TypeId::FnDelegate: {
			std::string name = "fn";

			FnTypeDefObject *typeDef = (FnTypeDefObject *)type.exData.typeDef;

			name += mangleTypeName(typeDef->returnType);

			name += "0";

			for (size_t i = 0; i < typeDef->paramTypes.size(); ++i) {
				if (i)
					name += "2";
				name += mangleTypeName(typeDef->paramTypes.at(i));
			}

			name += "1";

			return name;
		}
		case TypeId::Ref:
			return "ref" + mangleTypeName(type.getRefExData());
		case TypeId::Any:
			return "any";
		default:
			std::terminate();
	}
}

std::string BC2CXX::mangleClassName(const std::string &className) {
	return "_SLKAOT_" + className;
}

std::string BC2CXX::mangleFnName(const std::string_view &fnName) {
	std::string mangledName = "_slkaot_";
	bool mangleFnName = false;

	for (auto i : fnName) {
		if ((!isalnum(i)) && (i != '_')) {
			mangledName = "_slkaotmg_";
			mangleFnName = true;
			break;
		}
	}

	if (mangleFnName) {
		for (size_t i = 0; i < fnName.size(); ++i) {
			char c[3];

			c[0] = (fnName[i] & 0xf) + 'A';
			c[1] = (fnName[i] >> 4) + 'A';
			c[2] = '\0';

			mangledName += c;
		}
	} else {
		mangledName += fnName;
	}

	return mangledName;
}

std::string BC2CXX::mangleFieldName(const std::string &fieldName) {
	return "_slkaot_" + fieldName;
}
