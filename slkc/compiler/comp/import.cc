#include "../compiler.h"

using namespace slake::slkc;

void Compiler::importDefinitions(ModuleValue *mod) {
	if (importedModules.count(mod))
		return;

	importedModules.insert(mod);

	for (auto i : mod->_members) {
		switch (i.second->getType().typeId) {
			case TypeId::FN: {
				BasicFnValue *value = (BasicFnValue *)i.second;

				string fnName = i.first;
				size_t j = 0;

				while (j < i.first.size())
					if (i.first[j] == '$') {
						break;
					} else
						++j;

				fnName = fnName.substr(0, j);

				auto returnType = toTypeName(value->getReturnType());
				GenericParamList genericParams;

				deque<Param> params;

				for (auto i : value->getParamTypes()) {
					Param param(Location(), toTypeName(i), "");

					params.push_back(param);
				}

				FnOverloadingRegistry registry(Location(), returnType, genericParams, params);
			}
			case TypeId::CLASS: {
			}
		}
	}
}

void Compiler::importDefinitions(Value *value) {
	switch (value->getType().typeId) {
		case TypeId::ROOT: {
			RootValue *v = (RootValue *)value;

			for(auto i : v->_members) {
			}
		}
		case TypeId::FN: {
			FnValue *v = (FnValue *)value;
		}
		case TypeId::MOD: {
			ModuleValue *v = (ModuleValue *)value;

			for (auto i : v->_members) {

			}
		}
		case TypeId::VAR: {
			VarValue *v = (VarValue *)value;
		}
		case TypeId::CLASS: {
			ClassValue *v = (ClassValue *)value;

			for (auto i : v->_members) {
			}
		}
		case TypeId::INTERFACE: {
			InterfaceValue *v = (InterfaceValue *)value;

			for (auto i : v->_members) {
			}
		}
		case TypeId::TRAIT: {
			TraitValue *v = (TraitValue *)value;

			for (auto i : v->_members) {
			}
		}
		case TypeId::ALIAS: {
			AliasValue *v = (AliasValue *)value;
		}
		default:
			// Ignored.
			;
	}
}

shared_ptr<TypeNameNode> Compiler::toTypeName(slake::Type runtimeType) {
	bool isConst = runtimeType.flags & TYPE_CONST;

	switch (runtimeType.typeId) {
		case TypeId::I8:
			return make_shared<I8TypeNameNode>(Location{}, isConst);
		case TypeId::I16:
			return make_shared<I16TypeNameNode>(Location{}, isConst);
		case TypeId::I32:
			return make_shared<I32TypeNameNode>(Location{}, isConst);
		case TypeId::I64:
			return make_shared<I64TypeNameNode>(Location{}, isConst);
		case TypeId::U8:
			return make_shared<U8TypeNameNode>(Location{}, isConst);
		case TypeId::U16:
			return make_shared<U16TypeNameNode>(Location{}, isConst);
		case TypeId::U32:
			return make_shared<U32TypeNameNode>(Location{}, isConst);
		case TypeId::U64:
			return make_shared<U64TypeNameNode>(Location{}, isConst);
		case TypeId::F32:
			return make_shared<F32TypeNameNode>(Location{}, isConst);
		case TypeId::F64:
			return make_shared<F64TypeNameNode>(Location{}, isConst);
		case TypeId::STRING:
			return make_shared<StringTypeNameNode>(Location{}, isConst);
		case TypeId::BOOL:
			return make_shared<BoolTypeNameNode>(Location{}, isConst);
		case TypeId::NONE:
			return make_shared<VoidTypeNameNode>(Location{}, isConst);
		case TypeId::ANY:
			return make_shared<AnyTypeNameNode>(Location{}, isConst);
		case TypeId::TYPENAME: {
			auto refs = _rt->getFullRef((MemberValue *)runtimeType.getCustomTypeExData().get());
			Ref ref;

			for (auto &i : refs) {
				deque<shared_ptr<TypeNameNode>> genericArgs;
				for (auto j : i.genericArgs) {
					genericArgs.push_back(toTypeName(j));
				}

				ref.push_back(RefEntry(Location{}, i.name, genericArgs));
			}

			return make_shared<CustomTypeNameNode>(Location{}, ref, isConst);
		}
		case TypeId::ARRAY:
			return make_shared<ArrayTypeNameNode>(toTypeName(runtimeType.getArrayExData()), isConst);
		case TypeId::MAP: {
			auto exData = runtimeType.getMapExData();
			return make_shared<MapTypeNameNode>(toTypeName(*exData.first), toTypeName(*exData.second), isConst);
		}
		default:
			// Inconvertible/unrecognized type
			assert(false);
	}
}
